/* glyph_cache_atlas.h — Texture atlas for glyph caching.
 *
 * Instead of one GPU texture per glyph, all glyphs are packed into a single
 * large texture atlas. This allows rendering an entire text command with
 * a single draw call instead of one per glyph.
 *
 * Packing algorithm: Shelf (Next Fit Decreasing Height)
 *   - Glyphs are sorted by height and placed in horizontal shelves
 *   - Each shelf has a fixed height (tallest glyph in that shelf)
 *   - Simple, O(n log n) for sorting, O(n) for packing
 */

#ifndef GLYPH_CACHE_ATLAS_H
#define GLYPH_CACHE_ATLAS_H

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <stdbool.h>
#include <stdint.h>

/* ---- Atlas configuration ---- */
#define ATLAS_WIDTH  2048
#define ATLAS_HEIGHT 2048
#define ATLAS_PADDING 1  /* 1px padding between glyphs to avoid bleeding */

/* ---- Shelf packing state ---- */
typedef struct {
    int x;        /* current x cursor in shelf */
    int y;        /* top edge of shelf */
    int height;   /* shelf height (tallest glyph in this shelf) */
} AtlasShelf;

#define MAX_SHELVES 256

/* ---- Glyph entry with atlas coordinates ---- */
typedef struct {
    uint64_t key;         /* packed { codepoint | font_size << 32 } */
    bool occupied;

    /* Atlas coordinates (in pixels) */
    int atlas_x, atlas_y; /* top-left corner in atlas texture */
    int w, h;             /* glyph size in atlas */

    /* Metrics (in physical pixels) */
    int bearing_x;        /* minx from TTF_GetGlyphMetrics */
    int bearing_y;        /* maxy (top bearing) */
    int advance;

    /* UV coordinates (normalized 0..1) */
    float u0, v0, u1, v1;
} GlyphAtlasEntry;

/* ---- Pending glyph upload ---- */
typedef struct {
    SDL_Surface *surf;
    int atlas_x, atlas_y;  /* destination in atlas */
} PendingAtlasUpload;

/* ---- Atlas state ---- */
#define MAX_PENDING_ATLAS_UPLOADS 512

typedef struct {
    GlyphAtlasEntry *slots;
    uint32_t capacity;
    uint32_t count;

    /* Atlas texture (single large texture for all glyphs) */
    SDL_GPUTexture *atlas_tex;
    bool atlas_initialized;

    /* Shelf packing state */
    AtlasShelf shelves[MAX_SHELVES];
    int shelf_count;
    int next_y;  /* next available y position for new shelf */

    /* Pending uploads */
    PendingAtlasUpload pending[MAX_PENDING_ATLAS_UPLOADS];
    int pending_count;

    /* Dirty flag: true if atlas was modified this frame */
    bool dirty;

    SDL_GPUDevice *gpu;
} GlyphAtlas;

/* ---- Public API ---- */

/* Initialize the atlas */
bool GlyphAtlas_init(GlyphAtlas *ga, SDL_GPUDevice *gpu);

/* Look up or rasterize a glyph, packing it into the atlas.
 * Returns pointer to cached entry, or NULL on failure.
 * The caller MUST have already called TTF_SetFontSize(font, font_size). */
const GlyphAtlasEntry *GlyphAtlas_get(GlyphAtlas *ga, TTF_Font *font,
                                      uint32_t codepoint, uint16_t font_size);

/* Upload all pending glyph surfaces to the atlas texture.
 * Call this ONCE per frame, before the render pass. */
void GlyphAtlas_flush_uploads(GlyphAtlas *ga);

/* Get the atlas texture (for binding in render pass) */
SDL_GPUTexture *GlyphAtlas_get_texture(GlyphAtlas *ga);

/* Release all GPU resources and free the table. */
void GlyphAtlas_destroy(GlyphAtlas *ga);

/* Pack a codepoint + font_size into a single 64-bit key */
static inline uint64_t GlyphAtlas_make_key(uint32_t codepoint, uint16_t font_size)
{
    return ((uint64_t)font_size << 32) | (uint64_t)codepoint;
}

#endif /* GLYPH_CACHE_ATLAS_H */
