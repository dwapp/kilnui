/* clay_sdl3_gpu.h — Public API for Clay + SDL3 GPU rendering backend. */

#ifndef CLAY_SDL3_GPU_H
#define CLAY_SDL3_GPU_H

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
    uint32_t                rect_vbuf_cap;   /* bytes currently allocated */
    uint32_t                rect_ibuf_cap;

    SDL_GPUBuffer          *text_vbuf;       /* all text quads packed together */
    SDL_GPUBuffer          *text_ibuf;
    uint32_t                text_vbuf_cap;
    uint32_t                text_ibuf_cap;

    SDL_GPUTransferBuffer  *staging_tbuf;    /* shared staging for vert+idx uploads */
    uint32_t                staging_tbuf_cap;

    /* Clay memory */
    void          *clay_mem;
    Clay_Context  *clay_ctx;
} ClayGPUCtx;

/* ---- Public functions ---- */
bool ClayGPUCtx_init(ClayGPUCtx *ctx, const char *title,
                     int w, int h, const char *font_path, int font_size);
void ClayGPUCtx_handle_event(ClayGPUCtx *ctx, const SDL_Event *e);
void ClayGPUCtx_render(ClayGPUCtx *ctx, Clay_RenderCommandArray cmds);
void ClayGPUCtx_destroy(ClayGPUCtx *ctx);

/* ---- Font discovery helper ----
 * Tries each path in `candidates` (NULL-terminated array) in order.
 * Returns the first path whose file exists, or NULL if none found.
 */
const char *ClayGPUCtx_find_font(const char **candidates);

#endif /* CLAY_SDL3_GPU_H */
