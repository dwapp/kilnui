/* SPDX-License-Identifier: MIT */
/* glyph_cache.h — Open-addressing hash table caching per-glyph GPU textures.
 *
 * Key:   { codepoint, font_size } packed into uint64_t
 * Value: { SDL_GPUTexture*, w, h, bearing_x, bearing_y, advance }
 *
 * Automatically resizes when load factor exceeds 0.75.
 *
 * Upload flow (avoids per-glyph command buffer submits):
 *   GlyphCache_get()          — on miss: rasterizes + stages SDL_Surface in pending[]
 *   GlyphCache_flush_uploads() — call once per frame BEFORE the render pass;
 *                                uploads all pending surfaces in ONE copy pass.
 */

#ifndef GLYPH_CACHE_H
#define GLYPH_CACHE_H

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <stdbool.h>
#include <stdint.h>

/* ---- Cached glyph entry ---- */
typedef struct
{
    uint64_t key; /* packed { codepoint | font_size << 32 } */
    bool occupied;
    SDL_GPUTexture *tex;
    int w, h;
    int bearing_x; /* minx from TTF_GetGlyphMetrics */
    int bearing_y; /* maxy (top bearing) */
    int advance;
} GlyphEntry;

/* ---- Pending glyph upload (surface staged, not yet on GPU) ---- */
#define MAX_PENDING_GLYPH_UPLOADS 512

typedef struct
{
    SDL_Surface    *surf; /* ABGR8888 surface ready to copy */
    SDL_GPUTexture *tex;  /* destination texture (already created) */
} PendingGlyphUpload;

/* ---- Hash table ---- */
typedef struct
{
    GlyphEntry *slots;
    uint32_t    capacity;
    uint32_t    count;
    SDL_GPUDevice *gpu; /* needed for releasing textures on destroy */

    /* Pending uploads — filled by GlyphCache_get on cache miss,
     * drained by GlyphCache_flush_uploads() before the render pass. */
    PendingGlyphUpload pending[MAX_PENDING_GLYPH_UPLOADS];
    int                pending_count;

    /* Persistent transfer buffer (grow-only) to avoid per-frame allocation */
    SDL_GPUTransferBuffer *staging_tbuf;
    uint32_t               staging_tbuf_cap;
} GlyphCache;

/* Pack a codepoint + font_size into a single 64-bit key */
static inline uint64_t GlyphCache_make_key(uint32_t codepoint, uint16_t font_size)
{
    return ((uint64_t)font_size << 32) | (uint64_t)codepoint;
}

/* Initialize the cache with the given initial capacity (must be power of 2). */
bool GlyphCache_init(GlyphCache *gc, uint32_t initial_cap, SDL_GPUDevice *gpu);

/* Look up or rasterize a glyph.
 * On cache miss: rasterizes with TTF, creates the GPU texture, and STAGES the
 * upload in gc->pending[].  The texture is not valid until after
 * GlyphCache_flush_uploads() is called.
 * Returns pointer to cached entry, or NULL on failure.
 *
 * IMPORTANT: The caller MUST call TTF_SetFontSize(font, font_size) before
 * calling this function. GlyphCache_get does NOT set the font size itself
 * to avoid interfering with the caller's cached font-size state. */
const GlyphEntry *GlyphCache_get(GlyphCache *gc, TTF_Font *font,
                                 uint32_t codepoint, uint16_t font_size);

/* Upload all pending glyph surfaces to GPU in a single copy pass.
 * Call this ONCE per frame, after building geometry but BEFORE the render pass.
 * If cmdbuf is NULL, creates and submits its own command buffer (legacy path).
 * If cmdbuf is provided, uses it for the copy pass (merged path - preferred). */
void GlyphCache_flush_uploads_ex(GlyphCache *gc, SDL_GPUCommandBuffer *cmdbuf);
static inline void GlyphCache_flush_uploads(GlyphCache *gc) { GlyphCache_flush_uploads_ex(gc, NULL); }

/* Release all GPU textures and free the table. */
void GlyphCache_destroy(GlyphCache *gc);

#endif /* GLYPH_CACHE_H */
