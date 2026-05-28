/* glyph_cache.h — Open-addressing hash table caching per-glyph GPU textures.
 *
 * Key:   { codepoint, font_size } packed into uint64_t
 * Value: { SDL_GPUTexture*, w, h, bearing_x, bearing_y, advance }
 *
 * Automatically resizes when load factor exceeds 0.75.
 */

#ifndef GLYPH_CACHE_H
#define GLYPH_CACHE_H

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <stdint.h>
#include <stdbool.h>

/* ---- Cached glyph entry ---- */
typedef struct {
    uint64_t        key;          /* packed { codepoint | font_size << 32 } */
    bool            occupied;
    SDL_GPUTexture *tex;
    int             w, h;
    int             bearing_x;    /* minx from TTF_GetGlyphMetrics */
    int             bearing_y;    /* maxy (top bearing) */
    int             advance;
} GlyphEntry;

/* ---- Hash table ---- */
typedef struct {
    GlyphEntry    *slots;
    uint32_t       capacity;
    uint32_t       count;
    SDL_GPUDevice *gpu;          /* needed for releasing textures on destroy */
} GlyphCache;

/* Pack a codepoint + font_size into a single 64-bit key */
static inline uint64_t GlyphCache_make_key(uint32_t codepoint, uint16_t font_size) {
    return ((uint64_t)font_size << 32) | (uint64_t)codepoint;
}

/* Initialize the cache with the given initial capacity (must be power of 2). */
bool GlyphCache_init(GlyphCache *gc, uint32_t initial_cap, SDL_GPUDevice *gpu);

/* Look up or rasterize a glyph.  On cache miss, rasterizes via TTF_RenderGlyph_Blended
 * and uploads to GPU texture.  Returns pointer to cached entry, or NULL on failure. */
const GlyphEntry *GlyphCache_get(GlyphCache *gc, TTF_Font *font,
                                 uint32_t codepoint, uint16_t font_size);

/* Release all GPU textures and free the table. */
void GlyphCache_destroy(GlyphCache *gc);

#endif /* GLYPH_CACHE_H */
