/* glyph_cache.c — Open-addressing hash table for per-glyph GPU textures.
 *
 * On cache miss, rasterizes the glyph with TTF_RenderGlyph_Blended,
 * creates the GPU texture, and STAGES the pixel data in gc->pending[].
 * The actual GPU upload is deferred to GlyphCache_flush_uploads(), which
 * uploads all pending glyphs in ONE transfer buffer + ONE copy pass + ONE
 * command buffer submit — eliminating the previous per-glyph submit cost.
 */

#include "glyph_cache.h"
#include <stdlib.h>
#include <string.h>

/* ---- FNV-1a 64-bit hash ---- */
static uint64_t hash64(uint64_t key)
{
    uint64_t h = 14695981039346656037ULL;
    for (int i = 0; i < 8; i++) {
        h ^= (key & 0xFF);
        h *= 1099511628211ULL;
        key >>= 8;
    }
    return h;
}

/* Probe for a key; returns index of existing slot or first empty slot. */
static uint32_t probe(const GlyphCache *gc, uint64_t key)
{
    uint32_t mask = gc->capacity - 1;
    uint32_t idx = (uint32_t)(hash64(key) & mask);
    while (gc->slots[idx].occupied && gc->slots[idx].key != key)
        idx = (idx + 1) & mask;
    return idx;
}

/* Grow the table to double capacity and re-insert all entries. */
static bool grow(GlyphCache *gc)
{
    uint32_t old_cap = gc->capacity;
    GlyphEntry *old_slots = gc->slots;
    uint32_t new_cap = old_cap * 2;

    GlyphEntry *new_slots = (GlyphEntry *)SDL_calloc(new_cap, sizeof(GlyphEntry));
    if (!new_slots)
        return false;

    gc->slots    = new_slots;
    gc->capacity = new_cap;
    gc->count    = 0;

    for (uint32_t i = 0; i < old_cap; i++) {
        if (old_slots[i].occupied) {
            uint32_t idx = probe(gc, old_slots[i].key);
            new_slots[idx] = old_slots[i];
            gc->count++;
        }
    }
    SDL_free(old_slots);
    return true;
}

/* Initialize the cache. */
bool GlyphCache_init(GlyphCache *gc, uint32_t initial_cap, SDL_GPUDevice *gpu)
{
    uint32_t cap = 1;
    while (cap < initial_cap)
        cap <<= 1;

    gc->slots         = (GlyphEntry *)SDL_calloc(cap, sizeof(GlyphEntry));
    gc->capacity      = cap;
    gc->count         = 0;
    gc->gpu           = gpu;
    gc->pending_count = 0;
    return gc->slots != NULL;
}

/* Look up or rasterize a glyph, staging the upload for later flush. */
const GlyphEntry *GlyphCache_get(GlyphCache *gc, TTF_Font *font,
                                 uint32_t codepoint, uint16_t font_size)
{
    /* U+0000 and C0 controls have no renderable glyph; bail out early
     * to avoid TTF_RenderGlyph_Blended failing with "Text has zero width". */
    if (codepoint == 0 || codepoint < 0x20)
        return NULL;

    uint64_t key = GlyphCache_make_key(codepoint, font_size);
    uint32_t idx = probe(gc, key);

    if (gc->slots[idx].occupied)
        return &gc->slots[idx];

    /* Cache miss — rasterize at the requested size.
     * The caller MUST have already called TTF_SetFontSize(font, font_size)
     * before calling GlyphCache_get, so we skip the redundant call here.
     * This avoids stomping on the caller's cached font-size state. */
    int minx = 0, maxx = 0, miny = 0, maxy = 0, advance = 0;
    if (!TTF_GetGlyphMetrics(font, codepoint, &minx, &maxx, &miny, &maxy, &advance))
        return NULL;

    SDL_Surface *surf = TTF_RenderGlyph_Blended(font, codepoint,
                                                (SDL_Color){ 255, 255, 255, 255 });
    if (!surf) {
        SDL_Log("GlyphCache: TTF_RenderGlyph_Blended(U+%04X): %s",
                codepoint, SDL_GetError());
        return NULL;
    }

    /* Convert to ABGR8888 (bytes in memory: R,G,B,A → matches R8G8B8A8_UNORM) */
    SDL_Surface *rgba = SDL_ConvertSurface(surf, SDL_PIXELFORMAT_ABGR8888);
    SDL_DestroySurface(surf);
    if (!rgba) {
        SDL_Log("GlyphCache: ConvertSurface: %s", SDL_GetError());
        return NULL;
    }

    int tex_w = rgba->w;
    int tex_h = rgba->h;

    /* Create the GPU texture now (so we can return the entry immediately),
     * but defer pixel upload to GlyphCache_flush_uploads(). */
    SDL_GPUTexture *tex = SDL_CreateGPUTexture(gc->gpu, &(SDL_GPUTextureCreateInfo){
        .type                = SDL_GPU_TEXTURETYPE_2D,
        .format              = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM,
        .width               = (Uint32)tex_w,
        .height              = (Uint32)tex_h,
        .layer_count_or_depth = 1,
        .num_levels          = 1,
        .usage               = SDL_GPU_TEXTUREUSAGE_SAMPLER,
    });
    if (!tex) {
        SDL_Log("GlyphCache: CreateGPUTexture: %s", SDL_GetError());
        SDL_DestroySurface(rgba);
        return NULL;
    }

    /* Stage upload — will be committed by GlyphCache_flush_uploads() */
    if (gc->pending_count < MAX_PENDING_GLYPH_UPLOADS) {
        gc->pending[gc->pending_count++] = (PendingGlyphUpload){ .surf = rgba, .tex = tex };
    } else {
        /* Pending list full: fall back to immediate upload (rare) */
        SDL_Log("GlyphCache: pending list full, uploading immediately");
        Uint32 byte_size = (Uint32)(rgba->pitch * rgba->h);
        SDL_GPUTransferBuffer *tbuf = SDL_CreateGPUTransferBuffer(gc->gpu,
            &(SDL_GPUTransferBufferCreateInfo){
                .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD, .size = byte_size });
        if (tbuf) {
            void *mapped = SDL_MapGPUTransferBuffer(gc->gpu, tbuf, false);
            SDL_memcpy(mapped, rgba->pixels, byte_size);
            SDL_UnmapGPUTransferBuffer(gc->gpu, tbuf);
            SDL_GPUCommandBuffer *cmd = SDL_AcquireGPUCommandBuffer(gc->gpu);
            SDL_GPUCopyPass *cp = SDL_BeginGPUCopyPass(cmd);
            SDL_UploadToGPUTexture(cp,
                &(SDL_GPUTextureTransferInfo){
                    .transfer_buffer = tbuf, .offset = 0,
                    .pixels_per_row  = (Uint32)rgba->w,
                    .rows_per_layer  = (Uint32)rgba->h,
                },
                &(SDL_GPUTextureRegion){
                    .texture = tex,
                    .w = (Uint32)rgba->w, .h = (Uint32)rgba->h, .d = 1,
                }, false);
            SDL_EndGPUCopyPass(cp);
            SDL_SubmitGPUCommandBuffer(cmd);
            SDL_ReleaseGPUTransferBuffer(gc->gpu, tbuf);
        }
        SDL_DestroySurface(rgba);
    }

    /* Grow table if load factor > 0.75 */
    if ((gc->count + 1) * 4 > gc->capacity * 3) {
        if (!grow(gc)) {
            SDL_ReleaseGPUTexture(gc->gpu, tex);
            return NULL;
        }
        idx = probe(gc, key);
    }

    int bearing_y_px = (maxy > 0) ? maxy : tex_h;
    gc->slots[idx] = (GlyphEntry){
        .key      = key,
        .occupied = true,
        .tex      = tex,
        .w        = tex_w,
        .h        = tex_h,
        .bearing_x = minx,
        .bearing_y = bearing_y_px,
        .advance   = advance,
    };
    gc->count++;
    return &gc->slots[idx];
}

/* Upload all pending glyph surfaces in ONE transfer buffer + ONE copy pass. */
void GlyphCache_flush_uploads(GlyphCache *gc)
{
    if (gc->pending_count == 0)
        return;

    /* Calculate total bytes needed */
    Uint32 total = 0;
    for (int i = 0; i < gc->pending_count; i++) {
        SDL_Surface *s = gc->pending[i].surf;
        total += (Uint32)(s->pitch * s->h);
    }

    SDL_GPUTransferBuffer *tbuf = SDL_CreateGPUTransferBuffer(gc->gpu,
        &(SDL_GPUTransferBufferCreateInfo){
            .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD, .size = total });
    if (!tbuf) {
        SDL_Log("GlyphCache_flush_uploads: CreateGPUTransferBuffer: %s", SDL_GetError());
        for (int i = 0; i < gc->pending_count; i++)
            SDL_DestroySurface(gc->pending[i].surf);
        gc->pending_count = 0;
        return;
    }

    /* Copy all glyph pixels into the staging buffer */
    uint8_t *mapped = (uint8_t *)SDL_MapGPUTransferBuffer(gc->gpu, tbuf, false);
    Uint32 offset = 0;
    for (int i = 0; i < gc->pending_count; i++) {
        SDL_Surface *s = gc->pending[i].surf;
        Uint32 sz = (Uint32)(s->pitch * s->h);
        SDL_memcpy(mapped + offset, s->pixels, sz);
        offset += sz;
    }
    SDL_UnmapGPUTransferBuffer(gc->gpu, tbuf);

    /* ONE command buffer, ONE copy pass for all pending glyphs */
    SDL_GPUCommandBuffer *cmdbuf = SDL_AcquireGPUCommandBuffer(gc->gpu);
    SDL_GPUCopyPass *cp = SDL_BeginGPUCopyPass(cmdbuf);

    offset = 0;
    for (int i = 0; i < gc->pending_count; i++) {
        SDL_Surface *s   = gc->pending[i].surf;
        SDL_GPUTexture *t = gc->pending[i].tex;
        SDL_UploadToGPUTexture(cp,
            &(SDL_GPUTextureTransferInfo){
                .transfer_buffer = tbuf,
                .offset          = offset,
                .pixels_per_row  = (Uint32)s->w,
                .rows_per_layer  = (Uint32)s->h,
            },
            &(SDL_GPUTextureRegion){
                .texture = t,
                .w = (Uint32)s->w, .h = (Uint32)s->h, .d = 1,
            }, false);
        offset += (Uint32)(s->pitch * s->h);
        SDL_DestroySurface(s);
    }

    SDL_EndGPUCopyPass(cp);
    SDL_SubmitGPUCommandBuffer(cmdbuf);
    SDL_ReleaseGPUTransferBuffer(gc->gpu, tbuf);
    gc->pending_count = 0;
}

/* Release all cached textures and free the hash table. */
void GlyphCache_destroy(GlyphCache *gc)
{
    /* Free any surfaces still waiting for upload */
    for (int i = 0; i < gc->pending_count; i++)
        SDL_DestroySurface(gc->pending[i].surf);
    gc->pending_count = 0;

    if (!gc->slots)
        return;
    for (uint32_t i = 0; i < gc->capacity; i++) {
        if (gc->slots[i].occupied && gc->slots[i].tex)
            SDL_ReleaseGPUTexture(gc->gpu, gc->slots[i].tex);
    }
    SDL_free(gc->slots);
    gc->slots    = NULL;
    gc->capacity = 0;
    gc->count    = 0;
}
