/* SPDX-License-Identifier: MIT */
/* glyph_cache_atlas.c — Texture atlas for glyph caching.
 *
 * Uses shelf packing (Next Fit Decreasing Height) to pack all glyphs
 * into a single large texture atlas. This allows rendering an entire
 * text command with a single draw call.
 */

#include "glyph_cache_atlas.h"
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
static uint32_t probe(const GlyphAtlas *ga, uint64_t key)
{
    uint32_t mask = ga->capacity - 1;
    uint32_t idx = (uint32_t)(hash64(key) & mask);
    while (ga->slots[idx].occupied && ga->slots[idx].key != key)
        idx = (idx + 1) & mask;
    return idx;
}

/* Grow the table to double capacity and re-insert all entries. */
static bool grow(GlyphAtlas *ga)
{
    uint32_t old_cap = ga->capacity;
    GlyphAtlasEntry *old_slots = ga->slots;
    uint32_t new_cap = old_cap * 2;

    GlyphAtlasEntry *new_slots = (GlyphAtlasEntry *)SDL_calloc(new_cap, sizeof(GlyphAtlasEntry));
    if (!new_slots)
        return false;

    ga->slots    = new_slots;
    ga->capacity = new_cap;
    ga->count    = 0;

    for (uint32_t i = 0; i < old_cap; i++) {
        if (old_slots[i].occupied) {
            uint32_t idx = probe(ga, old_slots[i].key);
            new_slots[idx] = old_slots[i];
            ga->count++;
        }
    }
    SDL_free(old_slots);
    return true;
}

/* Initialize the atlas */
bool GlyphAtlas_init(GlyphAtlas *ga, SDL_GPUDevice *gpu)
{
    uint32_t cap = 1024;  /* initial capacity */
    while (cap < 1024)
        cap <<= 1;

    ga->slots = (GlyphAtlasEntry *)SDL_calloc(cap, sizeof(GlyphAtlasEntry));
    if (!ga->slots)
        return false;

    ga->capacity      = cap;
    ga->count         = 0;
    ga->gpu           = gpu;
    ga->atlas_tex     = NULL;
    ga->atlas_initialized = false;
    ga->shelf_count   = 0;
    ga->next_y        = 0;
    ga->pending_count = 0;
    ga->dirty         = false;

    /* Initialize persistent staging buffer (will be grown as needed) */
    ga->staging_tbuf     = NULL;
    ga->staging_tbuf_cap = 0;

    return true;
}

/* Initialize the atlas texture if not already done */
static bool ensure_atlas_texture(GlyphAtlas *ga)
{
    if (ga->atlas_initialized)
        return true;

    ga->atlas_tex = SDL_CreateGPUTexture(ga->gpu, &(SDL_GPUTextureCreateInfo){
        .type                = SDL_GPU_TEXTURETYPE_2D,
        .format              = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM,
        .width               = ATLAS_WIDTH,
        .height              = ATLAS_HEIGHT,
        .layer_count_or_depth = 1,
        .num_levels          = 1,
        .usage               = SDL_GPU_TEXTUREUSAGE_SAMPLER,
    });

    if (!ga->atlas_tex) {
        SDL_Log("GlyphAtlas: Failed to create atlas texture: %s", SDL_GetError());
        return false;
    }

    ga->atlas_initialized = true;
    return true;
}

/* Find space in the atlas using shelf packing */
static bool find_space(GlyphAtlas *ga, int gw, int gh, int *out_x, int *out_y)
{
    /* Try to fit in existing shelves */
    for (int i = 0; i < ga->shelf_count; i++) {
        AtlasShelf *shelf = &ga->shelves[i];
        if (gh <= shelf->height && shelf->x + gw + ATLAS_PADDING <= ATLAS_WIDTH) {
            *out_x = shelf->x;
            *out_y = shelf->y;
            shelf->x += gw + ATLAS_PADDING;
            return true;
        }
    }

    /* Create new shelf */
    if (ga->shelf_count >= MAX_SHELVES)
        return false;

    if (ga->next_y + gh + ATLAS_PADDING > ATLAS_HEIGHT)
        return false;

    AtlasShelf *shelf = &ga->shelves[ga->shelf_count++];
    shelf->x = gw + ATLAS_PADDING;
    shelf->y = ga->next_y;
    shelf->height = gh;

    *out_x = 0;
    *out_y = ga->next_y;
    ga->next_y += gh + ATLAS_PADDING;

    return true;
}

/* Look up or rasterize a glyph, packing it into the atlas */
const GlyphAtlasEntry *GlyphAtlas_get(GlyphAtlas *ga, TTF_Font *font,
                                      uint32_t codepoint, uint16_t font_size)
{
    /* Skip non-renderable codepoints */
    if (codepoint < 0xA0 &&
        (codepoint == 0 || codepoint < 0x20 || codepoint == 0x7F ||
         (codepoint >= 0x80 && codepoint <= 0x9F)))
        return NULL;

    uint64_t key = GlyphAtlas_make_key(codepoint, font_size);
    uint32_t idx = probe(ga, key);

    if (ga->slots[idx].occupied)
        return &ga->slots[idx];

    /* Cache miss — rasterize at the requested size */
    int minx = 0, maxx = 0, miny = 0, maxy = 0, advance = 0;
    if (!TTF_GetGlyphMetrics(font, codepoint, &minx, &maxx, &miny, &maxy, &advance))
        return NULL;

    SDL_Surface *surf = TTF_RenderGlyph_Blended(font, codepoint,
                                                (SDL_Color){ 255, 255, 255, 255 });
    if (!surf) {
        SDL_Log("GlyphAtlas: TTF_RenderGlyph_Blended(U+%04X): %s",
                codepoint, SDL_GetError());
        return NULL;
    }

    /* Convert to ABGR8888 */
    SDL_Surface *rgba = SDL_ConvertSurface(surf, SDL_PIXELFORMAT_ABGR8888);
    SDL_DestroySurface(surf);
    if (!rgba) {
        SDL_Log("GlyphAtlas: ConvertSurface: %s", SDL_GetError());
        return NULL;
    }

    int gw = rgba->w;
    int gh = rgba->h;

    /* Find space in atlas */
    int atlas_x, atlas_y;
    if (!find_space(ga, gw, gh, &atlas_x, &atlas_y)) {
        SDL_Log("GlyphAtlas: Atlas full, cannot fit glyph U+%04X (%dx%d)", codepoint, gw, gh);
        SDL_DestroySurface(rgba);
        return NULL;
    }

    /* Ensure atlas texture exists */
    if (!ensure_atlas_texture(ga)) {
        SDL_DestroySurface(rgba);
        return NULL;
    }

    /* Stage upload */
    if (ga->pending_count >= MAX_PENDING_ATLAS_UPLOADS) {
        SDL_Log("GlyphAtlas: pending list full (%d), flushing early", MAX_PENDING_ATLAS_UPLOADS);
        GlyphAtlas_flush_uploads(ga);
    }
    ga->pending[ga->pending_count++] = (PendingAtlasUpload){
        .surf = rgba,
        .atlas_x = atlas_x,
        .atlas_y = atlas_y,
    };

    /* Grow table if load factor > 0.75 */
    if ((ga->count + 1) * 4 > ga->capacity * 3) {
        if (!grow(ga)) {
            SDL_DestroySurface(rgba);
            return NULL;
        }
        idx = probe(ga, key);
    }

    /* Calculate UV coordinates */
    float u0 = (float)atlas_x / ATLAS_WIDTH;
    float v0 = (float)atlas_y / ATLAS_HEIGHT;
    float u1 = (float)(atlas_x + gw) / ATLAS_WIDTH;
    float v1 = (float)(atlas_y + gh) / ATLAS_HEIGHT;

    int bearing_y_px = (maxy > 0) ? maxy : gh;
    ga->slots[idx] = (GlyphAtlasEntry){
        .key       = key,
        .occupied  = true,
        .atlas_x   = atlas_x,
        .atlas_y   = atlas_y,
        .w         = gw,
        .h         = gh,
        .bearing_x = minx,
        .bearing_y = bearing_y_px,
        .advance   = advance,
        .u0        = u0,
        .v0        = v0,
        .u1        = u1,
        .v1        = v1,
    };
    ga->count++;
    ga->dirty = true;

    return &ga->slots[idx];
}

/* Ensure the staging transfer buffer is at least `needed` bytes. */
static void ensure_staging_buffer(GlyphAtlas *ga, uint32_t needed)
{
    if (ga->staging_tbuf_cap >= needed)
        return;

    if (ga->staging_tbuf)
        SDL_ReleaseGPUTransferBuffer(ga->gpu, ga->staging_tbuf);

    /* Grow with 1.5x headroom to amortize reallocation */
    uint32_t new_cap = needed + needed / 2;
    ga->staging_tbuf = SDL_CreateGPUTransferBuffer(ga->gpu,
        &(SDL_GPUTransferBufferCreateInfo){
            .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD, .size = new_cap });
    if (!ga->staging_tbuf) {
        SDL_Log("GlyphAtlas: staging buffer allocation failed: %s", SDL_GetError());
        ga->staging_tbuf_cap = 0;
        return;
    }
    ga->staging_tbuf_cap = new_cap;
}

/* Upload all pending glyph surfaces to the atlas texture.
 * If cmdbuf is provided, uses it (merged path); otherwise creates its own. */
void GlyphAtlas_flush_uploads_ex(GlyphAtlas *ga, SDL_GPUCommandBuffer *cmdbuf)
{
    if (ga->pending_count == 0)
        return;

    /* Calculate total bytes needed */
    Uint32 total = 0;
    for (int i = 0; i < ga->pending_count; i++) {
        SDL_Surface *s = ga->pending[i].surf;
        total += (Uint32)(s->pitch * s->h);
    }

    /* Reuse persistent staging buffer (grow-only) */
    ensure_staging_buffer(ga, total);
    if (!ga->staging_tbuf) {
        SDL_Log("GlyphAtlas_flush_uploads: no staging buffer");
        for (int i = 0; i < ga->pending_count; i++)
            SDL_DestroySurface(ga->pending[i].surf);
        ga->pending_count = 0;
        return;
    }

    /* Copy all glyph pixels into the staging buffer */
    uint8_t *mapped = (uint8_t *)SDL_MapGPUTransferBuffer(ga->gpu, ga->staging_tbuf, false);
    Uint32 offset = 0;
    for (int i = 0; i < ga->pending_count; i++) {
        SDL_Surface *s = ga->pending[i].surf;
        Uint32 sz = (Uint32)(s->pitch * s->h);
        SDL_memcpy(mapped + offset, s->pixels, sz);
        offset += sz;
    }
    SDL_UnmapGPUTransferBuffer(ga->gpu, ga->staging_tbuf);

    /* Use provided command buffer or create our own */
    bool own_cmdbuf = (cmdbuf == NULL);
    if (own_cmdbuf) {
        cmdbuf = SDL_AcquireGPUCommandBuffer(ga->gpu);
        if (!cmdbuf) {
            for (int i = 0; i < ga->pending_count; i++)
                SDL_DestroySurface(ga->pending[i].surf);
            ga->pending_count = 0;
            return;
        }
    }

    SDL_GPUCopyPass *cp = SDL_BeginGPUCopyPass(cmdbuf);

    offset = 0;
    for (int i = 0; i < ga->pending_count; i++) {
        SDL_Surface *s = ga->pending[i].surf;
        int atlas_x = ga->pending[i].atlas_x;
        int atlas_y = ga->pending[i].atlas_y;

        SDL_UploadToGPUTexture(cp,
            &(SDL_GPUTextureTransferInfo){
                .transfer_buffer = ga->staging_tbuf,
                .offset          = offset,
                .pixels_per_row  = (Uint32)(s->pitch / 4),
                .rows_per_layer  = (Uint32)s->h,
            },
            &(SDL_GPUTextureRegion){
                .texture = ga->atlas_tex,
                .x = (Uint32)atlas_x,
                .y = (Uint32)atlas_y,
                .w = (Uint32)s->w,
                .h = (Uint32)s->h,
                .d = 1,
            }, false);
        offset += (Uint32)(s->pitch * s->h);
        SDL_DestroySurface(s);
    }

    SDL_EndGPUCopyPass(cp);

    /* Only submit if we own the command buffer */
    if (own_cmdbuf) {
        SDL_SubmitGPUCommandBuffer(cmdbuf);
    }
    ga->pending_count = 0;
    ga->dirty = false;
}

/* Get the atlas texture */
SDL_GPUTexture *GlyphAtlas_get_texture(GlyphAtlas *ga)
{
    return ga->atlas_tex;
}

/* Release all GPU resources and free the table */
void GlyphAtlas_destroy(GlyphAtlas *ga)
{
    /* Free any surfaces still waiting for upload */
    for (int i = 0; i < ga->pending_count; i++)
        SDL_DestroySurface(ga->pending[i].surf);
    ga->pending_count = 0;

    /* Release persistent staging buffer */
    if (ga->staging_tbuf)
        SDL_ReleaseGPUTransferBuffer(ga->gpu, ga->staging_tbuf);
    ga->staging_tbuf     = NULL;
    ga->staging_tbuf_cap = 0;

    if (ga->atlas_tex)
        SDL_ReleaseGPUTexture(ga->gpu, ga->atlas_tex);
    ga->atlas_tex = NULL;

    if (ga->slots)
        SDL_free(ga->slots);
    ga->slots    = NULL;
    ga->capacity = 0;
    ga->count    = 0;
}
