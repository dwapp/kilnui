/* glyph_cache.c — Open-addressing hash table for per-glyph GPU textures.
 *
 * On cache miss, rasterizes the glyph with TTF_RenderGlyph_Blended,
 * uploads the resulting SDL_Surface to a GPU texture via staging buffer,
 * and stores the result for future lookups.
 */

#include "glyph_cache.h"
#include <string.h>
#include <stdlib.h>

/* ---- FNV-1a 64-bit hash ---- */
static uint64_t hash64(uint64_t key) {
    uint64_t h = 14695981039346656037ULL;
    for (int i = 0; i < 8; i++) {
        h ^= (key & 0xFF);
        h *= 1099511628211ULL;
        key >>= 8;
    }
    return h;
}

/* Probe for a key; returns index of existing slot or first empty slot. */
static uint32_t probe(const GlyphCache *gc, uint64_t key) {
    uint32_t mask = gc->capacity - 1;
    uint32_t idx  = (uint32_t)(hash64(key) & mask);
    while (gc->slots[idx].occupied && gc->slots[idx].key != key) {
        idx = (idx + 1) & mask;
    }
    return idx;
}

/* Grow the table to double capacity and re-insert all entries. */
static bool grow(GlyphCache *gc) {
    uint32_t    old_cap   = gc->capacity;
    GlyphEntry *old_slots = gc->slots;
    uint32_t    new_cap   = old_cap * 2;

    GlyphEntry *new_slots = (GlyphEntry *)SDL_calloc(new_cap, sizeof(GlyphEntry));
    if (!new_slots) return false;

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
bool GlyphCache_init(GlyphCache *gc, uint32_t initial_cap, SDL_GPUDevice *gpu) {
    /* Round up to power of 2 */
    uint32_t cap = 1;
    while (cap < initial_cap) cap <<= 1;

    gc->slots    = (GlyphEntry *)SDL_calloc(cap, sizeof(GlyphEntry));
    gc->capacity = cap;
    gc->count    = 0;
    gc->gpu      = gpu;
    return gc->slots != NULL;
}

/* Upload an SDL_Surface (assumed ARGB8888 or RGBA8888) to a new GPU texture. */
static SDL_GPUTexture *upload_surface(SDL_GPUDevice *gpu, SDL_Surface *surf) {
    /* Convert to ABGR8888: on little-endian systems this lays bytes out as
     * R,G,B,A in memory, matching SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM.
     * (SDL_PIXELFORMAT_RGBA8888 would give A,B,G,R — causing R/B swap) */
    SDL_Surface *rgba = SDL_ConvertSurface(surf, SDL_PIXELFORMAT_ABGR8888);
    if (!rgba) {
        SDL_Log("GlyphCache: ConvertSurface failed: %s", SDL_GetError());
        return NULL;
    }

    int w = rgba->w;
    int h = rgba->h;
    int pitch = rgba->pitch;

    /* Create GPU texture */
    SDL_GPUTexture *tex = SDL_CreateGPUTexture(gpu, &(SDL_GPUTextureCreateInfo){
        .type   = SDL_GPU_TEXTURETYPE_2D,
        .format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM,
        .width  = (Uint32)w,
        .height = (Uint32)h,
        .layer_count_or_depth = 1,
        .num_levels           = 1,
        .usage  = SDL_GPU_TEXTUREUSAGE_SAMPLER,
    });
    if (!tex) {
        SDL_Log("GlyphCache: CreateGPUTexture failed: %s", SDL_GetError());
        SDL_DestroySurface(rgba);
        return NULL;
    }

    /* Create staging transfer buffer */
    Uint32 byte_size = (Uint32)(pitch * h);
    SDL_GPUTransferBuffer *tbuf = SDL_CreateGPUTransferBuffer(gpu,
        &(SDL_GPUTransferBufferCreateInfo){
            .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
            .size  = byte_size,
        });
    if (!tbuf) {
        SDL_Log("GlyphCache: CreateGPUTransferBuffer failed: %s", SDL_GetError());
        SDL_ReleaseGPUTexture(gpu, tex);
        SDL_DestroySurface(rgba);
        return NULL;
    }

    /* Map, copy, unmap */
    void *mapped = SDL_MapGPUTransferBuffer(gpu, tbuf, false);
    SDL_memcpy(mapped, rgba->pixels, byte_size);
    SDL_UnmapGPUTransferBuffer(gpu, tbuf);

    /* Upload via copy pass */
    SDL_GPUCommandBuffer *cmdbuf = SDL_AcquireGPUCommandBuffer(gpu);
    SDL_GPUCopyPass *cp = SDL_BeginGPUCopyPass(cmdbuf);
    SDL_UploadToGPUTexture(cp,
        &(SDL_GPUTextureTransferInfo){
            .transfer_buffer = tbuf,
            .offset          = 0,
            .pixels_per_row  = (Uint32)w,
            .rows_per_layer  = (Uint32)h,
        },
        &(SDL_GPUTextureRegion){
            .texture = tex,
            .w = (Uint32)w,
            .h = (Uint32)h,
            .d = 1,
        },
        false);
    SDL_EndGPUCopyPass(cp);
    SDL_SubmitGPUCommandBuffer(cmdbuf);

    SDL_ReleaseGPUTransferBuffer(gpu, tbuf);
    SDL_DestroySurface(rgba);
    return tex;
}

/* Look up or rasterize a glyph. */
const GlyphEntry *GlyphCache_get(GlyphCache *gc, TTF_Font *font,
                                 uint32_t codepoint, uint16_t font_size) {
    uint64_t key = GlyphCache_make_key(codepoint, font_size);
    uint32_t idx = probe(gc, key);

    if (gc->slots[idx].occupied) {
        return &gc->slots[idx];
    }

    /* Cache miss — rasterize at the requested size */
    int minx = 0, maxx = 0, miny = 0, maxy = 0, advance = 0;
    /* Ensure font is set to the correct size before measuring/rendering.
     * TTF_SetFontSize is a no-op if the size hasn't changed, so it's cheap. */
    TTF_SetFontSize(font, (float)font_size);
    if (!TTF_GetGlyphMetrics(font, codepoint, &minx, &maxx, &miny, &maxy, &advance)) {
        /* Glyph not in font; return NULL */
        return NULL;
    }

    /* Render glyph to surface */
    SDL_Surface *surf = TTF_RenderGlyph_Blended(font, codepoint,
                                                 (SDL_Color){255, 255, 255, 255});
    if (!surf) {
        SDL_Log("GlyphCache: TTF_RenderGlyph_Blended(U+%04X) failed: %s",
                codepoint, SDL_GetError());
        return NULL;
    }

    /* Upload to GPU — use the surface dimensions as the authoritative size */
    SDL_GPUTexture *tex = upload_surface(gc->gpu, surf);
    int tex_w = surf->w;
    int tex_h = surf->h;
    SDL_DestroySurface(surf);
    if (!tex) return NULL;

    /* Grow if load factor > 0.75 */
    if ((gc->count + 1) * 4 > gc->capacity * 3) {
        if (!grow(gc)) {
            SDL_ReleaseGPUTexture(gc->gpu, tex);
            return NULL;
        }
        idx = probe(gc, key);
    }

    /* bearing_y is the distance from the baseline to the TOP of the glyph
     * in screen-down coordinates (positive = above baseline).
     * TTF_GetGlyphMetrics returns maxy in font's Y-up space.
     * When maxy == 0 (e.g. '.' in some fonts), the glyph top sits exactly
     * at the baseline, making small glyphs fall below the text box top.
     * Use max(maxy, tex_h) as a safe lower bound: if the rendered surface
     * is taller than maxy, the font engine included extra leading/padding
     * above the glyph and we must account for it. */
    int bearing_y_px = (maxy > 0) ? maxy : tex_h;

    gc->slots[idx] = (GlyphEntry){
        .key       = key,
        .occupied  = true,
        .tex       = tex,
        .w         = tex_w,
        .h         = tex_h,
        .bearing_x = minx,
        .bearing_y = bearing_y_px,
        .advance   = advance,
    };
    gc->count++;
    return &gc->slots[idx];
}

/* Release all cached textures and free the hash table. */
void GlyphCache_destroy(GlyphCache *gc) {
    if (!gc->slots) return;
    for (uint32_t i = 0; i < gc->capacity; i++) {
        if (gc->slots[i].occupied && gc->slots[i].tex) {
            SDL_ReleaseGPUTexture(gc->gpu, gc->slots[i].tex);
        }
    }
    SDL_free(gc->slots);
    gc->slots    = NULL;
    gc->capacity = 0;
    gc->count    = 0;
}
