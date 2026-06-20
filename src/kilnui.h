/* SPDX-License-Identifier: MIT */
/* kilnui.h — Public API for the kilnui rendering backend.
 *
 * kilnui integrates the Clay layout library with SDL3's GPU API,
 * providing a lightweight, high-performance UI rendering layer.
 */

#ifndef KILNUI_H
#define KILNUI_H

#include "clay.h"
#include "glyph_cache.h"
#include "glyph_cache_atlas.h"
#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <stdbool.h>

/* ---- Vertex formats ---- */
typedef struct
{
    float pos_x, pos_y;
    float local_x, local_y;
    float size_w, size_h;
    float radius_tl, radius_tr, radius_bl, radius_br;
    float r, g, b, a;
} VertexRect;

typedef struct
{
    float pos_x, pos_y;
    float u, v;
    float r, g, b, a;
} VertexTex;

/* ---- Custom Shader Data ---- */
typedef enum {
    KILNUI_CUSTOM_SHADOW = 1,
    KILNUI_CUSTOM_BORDER = 2,
} KilnUICustomType;

typedef struct {
    KilnUICustomType type;
} KilnUICustomHeader;

typedef struct {
    KilnUICustomType type;
    float offset_x;
    float offset_y;
    float blur_radius;
    float spread;
} KilnUICustomShadow;

typedef struct {
    KilnUICustomType type;
    float width_top;
    float width_right;
    float width_bottom;
    float width_left;
    float dash_length;
    float dash_gap;
} KilnUICustomBorder;

/* ---- Capacity constants ---- */
#define MAX_RECTS          8192
#define MAX_TEXT_CMDS      2048  /* max TEXT render commands per frame */
#define MAX_FALLBACK_FONTS 8

/* ---- Main context ---- */
typedef struct
{
    SDL_Window    *window;
    SDL_GPUDevice *gpu;
    TTF_Font      *font;
    TTF_Font      *fallback_fonts[MAX_FALLBACK_FONTS];
    int            fallback_font_count;
    float          dpi_scale;   /* physical_px / logical_px (render scale) */
    float          mouse_scale; /* SDL logical mouse coords → Clay layout coords */
    bool           dirty;       /* true = re-render needed this frame */
    int            font_size;
    float          cached_font_size;  /* cached font size to avoid redundant TTF_SetFontSize calls */

    GlyphCache glyph_cache;
    GlyphAtlas  glyph_atlas;  /* texture atlas for batched text rendering */

    SDL_GPUGraphicsPipeline *pipeline_rect;
    SDL_GPUGraphicsPipeline *pipeline_text;
    SDL_GPUGraphicsPipeline *pipeline_shadow;
    SDL_GPUGraphicsPipeline *pipeline_border;
    SDL_GPUSampler          *sampler_linear;

    /* Persistent GPU buffers — grown on demand, never shrunk mid-session.
     * Reusing them avoids the per-frame Create/Release overhead. */
    SDL_GPUBuffer          *rect_vbuf;
    SDL_GPUBuffer          *rect_ibuf;
    uint32_t                rect_vbuf_cap;
    uint32_t                rect_ibuf_cap;

    SDL_GPUBuffer          *text_vbuf;
    SDL_GPUBuffer          *text_ibuf;
    uint32_t                text_vbuf_cap;
    uint32_t                text_ibuf_cap;

    SDL_GPUTransferBuffer  *staging_tbuf;
    uint32_t                staging_tbuf_cap;

    /* Clay memory */
    void          *clay_mem;
    Clay_Context  *clay_ctx;
} KilnUI;

/* ---- Public API ---- */

/* Initialise the context: creates window, GPU device, font, pipelines, Clay. */
bool KilnUI_init(KilnUI *ctx, const char *title,
                 int w, int h, const char *font_path, int font_size);

/* Feed a single SDL event into the context (mouse, resize, DPI change). */
void KilnUI_handle_event(KilnUI *ctx, const SDL_Event *e);

/* Render a completed Clay layout to the swapchain. */
void KilnUI_render(KilnUI *ctx, Clay_RenderCommandArray cmds);

/* Release all GPU and CPU resources. */
void KilnUI_destroy(KilnUI *ctx);

/* Helper to set the font size of both main and fallback fonts. */
void KilnUI_set_font_size(KilnUI *ctx, float ptsize);

/* Font discovery helper.
 * Tries each path in `candidates` (NULL-terminated) in order.
 * Returns the first path that exists, or NULL. */
const char *KilnUI_find_font(const char **candidates);

/* Mark the context dirty so KilnUI_render redraws on the next call.
 * Call this whenever application state changes (e.g. animation tick,
 * data update) that is not triggered by an input event. */
static inline void KilnUI_mark_dirty(KilnUI *ctx) { ctx->dirty = true; }

#endif /* KILNUI_H */
