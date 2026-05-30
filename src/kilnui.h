/* kilnui.h — Public API for the kilnui rendering backend.
 *
 * kilnui integrates the Clay layout library with SDL3's GPU API,
 * providing a lightweight, high-performance UI rendering layer.
 */

#ifndef KILNUI_H
#define KILNUI_H

#include "clay.h"
#include "glyph_cache.h"
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

/* ---- Capacity constants ---- */
#define MAX_RECTS     8192
#define MAX_TEXT_CMDS 64    /* max TEXT render commands per frame */

/* ---- Main context ---- */
typedef struct
{
    SDL_Window    *window;
    SDL_GPUDevice *gpu;
    TTF_Font      *font;
    float          dpi_scale;
    int            font_size;

    GlyphCache glyph_cache;

    SDL_GPUGraphicsPipeline *pipeline_rect;
    SDL_GPUGraphicsPipeline *pipeline_text;
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

/* Font discovery helper.
 * Tries each path in `candidates` (NULL-terminated) in order.
 * Returns the first path that exists, or NULL. */
const char *KilnUI_find_font(const char **candidates);

/* ---- Backward-compatibility aliases (old ClayGPUCtx API) ---- */
typedef KilnUI ClayGPUCtx;
#define ClayGPUCtx_init         KilnUI_init
#define ClayGPUCtx_handle_event KilnUI_handle_event
#define ClayGPUCtx_render       KilnUI_render
#define ClayGPUCtx_destroy      KilnUI_destroy
#define ClayGPUCtx_find_font    KilnUI_find_font

#endif /* KILNUI_H */
