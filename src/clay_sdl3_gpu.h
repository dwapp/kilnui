/* clay_sdl3_gpu.h — Public API for Clay + SDL3 GPU rendering backend. */

#ifndef CLAY_SDL3_GPU_H
#define CLAY_SDL3_GPU_H

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <stdbool.h>
#include "../../clay/clay.h"
#include "glyph_cache.h"

/* ---- Vertex formats ---- */
typedef struct {
    float pos_x, pos_y;
    float local_x, local_y;
    float size_w, size_h;
    float radius_tl, radius_tr, radius_bl, radius_br;
    float r, g, b, a;
} VertexRect;

typedef struct {
    float pos_x, pos_y;
    float u, v;
    float r, g, b, a;
} VertexTex;

/* ---- Ring buffer parameters ---- */
#define RING_FRAMES   3
#define MAX_RECTS     8192
#define MAX_TEX_QUADS 8192

/* ---- Main context ---- */
typedef struct {
    SDL_Window      *window;
    SDL_GPUDevice   *gpu;
    SDL_GPUTexture  *depth_tex;
    TTF_Font        *font;
    float            dpi_scale;
    int              font_size;

    GlyphCache       glyph_cache;

    SDL_GPUGraphicsPipeline *pipeline_rect;
    SDL_GPUGraphicsPipeline *pipeline_text;

    SDL_GPUSampler  *sampler_linear;

    /* Clay memory */
    void            *clay_mem;
    Clay_Context    *clay_ctx;
} ClayGPUCtx;

/* ---- Public functions ---- */
bool ClayGPUCtx_init(ClayGPUCtx *ctx, const char *title,
                     int w, int h, const char *font_path, int font_size);
void ClayGPUCtx_handle_event(ClayGPUCtx *ctx, const SDL_Event *e);
void ClayGPUCtx_render(ClayGPUCtx *ctx, Clay_RenderCommandArray cmds);
void ClayGPUCtx_destroy(ClayGPUCtx *ctx);

#endif /* CLAY_SDL3_GPU_H */
