/* kilnui.c — KilnUI context lifecycle: init, event handling, destroy.
 *
 * This file owns everything that runs once (or rarely):
 *   - SDL / TTF / GPU device setup
 *   - Shader loading and pipeline creation
 *   - Clay initialisation and the MeasureText callback
 *   - Window event routing
 *   - Resource teardown
 *
 * Hot-path rendering lives in kilnui_render.c.
 */

#define CLAY_IMPLEMENTATION
#include "kilnui.h"
#include <math.h>
#include <string.h>

/* ---- Font discovery ---- */
const char *KilnUI_find_font(const char **candidates)
{
    for (const char **p = candidates; *p; p++) {
        SDL_PathInfo info;
        if (SDL_GetPathInfo(*p, &info))
            return *p;
    }
    return NULL;
}

/* ---- Shader loading ---- */
static SDL_GPUShader *load_spv(SDL_GPUDevice *dev, const char *path,
                               SDL_GPUShaderStage stage,
                               Uint32 num_ubo, Uint32 num_samplers)
{
    /* Resolve path relative to the executable directory so shaders are found
     * regardless of the current working directory. */
    const char *base = SDL_GetBasePath();
    char *full_path = NULL;
    SDL_asprintf(&full_path, "%s%s", base ? base : "", path);
    if (!full_path) {
        SDL_Log("load_spv: path allocation failed");
        return NULL;
    }

    size_t sz   = 0;
    void  *code = SDL_LoadFile(full_path, &sz);
    if (!code) {
        SDL_Log("load_spv: %s: %s", full_path, SDL_GetError());
        SDL_free(full_path);
        return NULL;
    }
    SDL_free(full_path);
    SDL_GPUShaderCreateInfo ci = {
        .code               = code,
        .code_size          = sz,
        .entrypoint         = "main",
        .format             = SDL_GPU_SHADERFORMAT_SPIRV,
        .stage              = stage,
        .num_uniform_buffers = num_ubo,
        .num_samplers       = num_samplers,
    };
    SDL_GPUShader *s = SDL_CreateGPUShader(dev, &ci);
    SDL_free(code);
    if (!s)
        SDL_Log("CreateGPUShader(%s): %s", path, SDL_GetError());
    return s;
}

/* ---- Shared blend state ---- */
static SDL_GPUColorTargetDescription blend_desc(SDL_GPUDevice *dev, SDL_Window *win)
{
    return (SDL_GPUColorTargetDescription){
        .format = SDL_GetGPUSwapchainTextureFormat(dev, win),
        .blend_state = {
            .enable_blend          = true,
            .color_blend_op        = SDL_GPU_BLENDOP_ADD,
            .alpha_blend_op        = SDL_GPU_BLENDOP_ADD,
            .src_color_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA,
            .dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
            .src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE,
            .dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
        },
    };
}

/* ---- Pipeline creation ---- */
static SDL_GPUGraphicsPipeline *create_rect_pipeline(SDL_GPUDevice *dev, SDL_Window *win)
{
    SDL_GPUShader *vert = load_spv(dev, "shaders/rect.vert.spv", SDL_GPU_SHADERSTAGE_VERTEX,   1, 0);
    SDL_GPUShader *frag = load_spv(dev, "shaders/rect.frag.spv", SDL_GPU_SHADERSTAGE_FRAGMENT, 0, 0);
    if (!vert || !frag) {
        if (vert) SDL_ReleaseGPUShader(dev, vert);
        if (frag) SDL_ReleaseGPUShader(dev, frag);
        return NULL;
    }
    SDL_GPUColorTargetDescription ctd = blend_desc(dev, win);
    SDL_GPUGraphicsPipelineCreateInfo ci = {
        .target_info        = { .num_color_targets = 1, .color_target_descriptions = &ctd },
        .primitive_type     = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST,
        .vertex_shader      = vert,
        .fragment_shader    = frag,
        .vertex_input_state = {
            .num_vertex_buffers          = 1,
            .vertex_buffer_descriptions  = (SDL_GPUVertexBufferDescription[]){ {
                .slot       = 0,
                .pitch      = sizeof(VertexRect),
                .input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX,
            } },
            .num_vertex_attributes = 5,
            .vertex_attributes     = (SDL_GPUVertexAttribute[]){
                { .location = 0, .buffer_slot = 0, .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2, .offset = offsetof(VertexRect, pos_x)     },
                { .location = 1, .buffer_slot = 0, .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2, .offset = offsetof(VertexRect, local_x)   },
                { .location = 2, .buffer_slot = 0, .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2, .offset = offsetof(VertexRect, size_w)    },
                { .location = 3, .buffer_slot = 0, .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4, .offset = offsetof(VertexRect, radius_tl) },
                { .location = 4, .buffer_slot = 0, .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4, .offset = offsetof(VertexRect, r)         },
            },
        },
    };
    SDL_GPUGraphicsPipeline *p = SDL_CreateGPUGraphicsPipeline(dev, &ci);
    SDL_ReleaseGPUShader(dev, vert);
    SDL_ReleaseGPUShader(dev, frag);
    return p;
}

static SDL_GPUGraphicsPipeline *create_text_pipeline(SDL_GPUDevice *dev, SDL_Window *win)
{
    SDL_GPUShader *vert = load_spv(dev, "shaders/tex.vert.spv", SDL_GPU_SHADERSTAGE_VERTEX,   1, 0);
    SDL_GPUShader *frag = load_spv(dev, "shaders/tex.frag.spv", SDL_GPU_SHADERSTAGE_FRAGMENT, 0, 1);
    if (!vert || !frag) {
        if (vert) SDL_ReleaseGPUShader(dev, vert);
        if (frag) SDL_ReleaseGPUShader(dev, frag);
        return NULL;
    }
    SDL_GPUColorTargetDescription ctd = blend_desc(dev, win);
    SDL_GPUGraphicsPipelineCreateInfo ci = {
        .target_info        = { .num_color_targets = 1, .color_target_descriptions = &ctd },
        .primitive_type     = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST,
        .vertex_shader      = vert,
        .fragment_shader    = frag,
        .vertex_input_state = {
            .num_vertex_buffers         = 1,
            .vertex_buffer_descriptions = (SDL_GPUVertexBufferDescription[]){ {
                .slot       = 0,
                .pitch      = sizeof(VertexTex),
                .input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX,
            } },
            .num_vertex_attributes = 3,
            .vertex_attributes     = (SDL_GPUVertexAttribute[]){
                { .location = 0, .buffer_slot = 0, .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2, .offset = offsetof(VertexTex, pos_x) },
                { .location = 1, .buffer_slot = 0, .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2, .offset = offsetof(VertexTex, u)     },
                { .location = 2, .buffer_slot = 0, .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4, .offset = offsetof(VertexTex, r)     },
            },
        },
    };
    SDL_GPUGraphicsPipeline *p = SDL_CreateGPUGraphicsPipeline(dev, &ci);
    SDL_ReleaseGPUShader(dev, vert);
    SDL_ReleaseGPUShader(dev, frag);
    return p;
}

/* ---- Clay MeasureText callback ---- */
/* Single global pointer: Clay's measure callback has no user-data path
 * that survives a context switch, so we store it here. Multi-window
 * support would require a thread-local or a Clay API extension. */
static KilnUI *g_measure_ctx = NULL;

static Clay_Dimensions measure_text_cb(Clay_StringSlice text,
                                       Clay_TextElementConfig *config,
                                       void *userData)
{
    (void)userData;
    KilnUI *ctx = g_measure_ctx;
    if (!ctx || !ctx->font || !text.chars || text.length <= 0)
        return (Clay_Dimensions){ 0, 0 };

    int req_size = (config && config->fontSize > 0) ? config->fontSize : ctx->font_size;
    TTF_SetFontSize(ctx->font, (float)req_size);

    int w = 0, h = 0;
    TTF_GetStringSize(ctx->font, text.chars, (size_t)text.length, &w, &h);
    if (config->letterSpacing && text.length > 1)
        w += config->letterSpacing * ((int)text.length - 1);
    if (config->lineHeight > 0 && config->lineHeight > (uint16_t)h)
        h = config->lineHeight;
    return (Clay_Dimensions){ (float)w, (float)h };
}

static void clay_error_cb(Clay_ErrorData e)
{
    SDL_Log("Clay error: %s", e.errorText.chars);
}

/* ---- Init ---- */
bool KilnUI_init(KilnUI *ctx, const char *title,
                 int w, int h, const char *font_path, int font_size)
{
    SDL_memset(ctx, 0, sizeof(*ctx));

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("SDL_Init: %s", SDL_GetError());
        return false;
    }
    /* Prevent KWin from disabling the compositor (avoids tearing / bypass mode) */
    SDL_SetHint(SDL_HINT_VIDEO_X11_NET_WM_BYPASS_COMPOSITOR, "0");

    if (!TTF_Init()) {
        SDL_Log("TTF_Init: %s", SDL_GetError());
        return false;
    }

    ctx->window = SDL_CreateWindow(title, w, h,
                                   SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY);
    if (!ctx->window) {
        SDL_Log("CreateWindow: %s", SDL_GetError());
        return false;
    }

    ctx->gpu = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV, true, NULL);
    if (!ctx->gpu) {
        SDL_Log("CreateGPUDevice: %s", SDL_GetError());
        return false;
    }
    if (!SDL_ClaimWindowForGPUDevice(ctx->gpu, ctx->window)) {
        SDL_Log("ClaimWindow: %s", SDL_GetError());
        return false;
    }

    /* dpi_scale = max(pixel_ratio, display_scale).
     * pixel_ratio covers Wayland HiDPI (e.g. 2.0 on a 4K display).
     * display_scale covers X11 where pixel_ratio == 1.0 but the OS
     * reports a fractional content scale (e.g. 1.5 at 150% in KDE/GNOME). */
    {
        int pw, ph, lw, lh;
        SDL_GetWindowSizeInPixels(ctx->window, &pw, &ph);
        SDL_GetWindowSize(ctx->window, &lw, &lh);
        float pixel_ratio   = (lw > 0) ? (float)pw / (float)lw : 1.0f;
        float display_scale = SDL_GetWindowDisplayScale(ctx->window);
        if (display_scale < 1.0f) display_scale = 1.0f;
        ctx->dpi_scale   = (pixel_ratio >= display_scale) ? pixel_ratio : display_scale;
        /* SDL mouse events are in logical px (range 0..lw).
         * Clay layout is in range 0..(pw/dpi_scale).
         * mouse_scale converts between the two. */
        ctx->mouse_scale = (lw > 0) ? (float)pw / ((float)lw * ctx->dpi_scale) : 1.0f;
    }

    ctx->font = TTF_OpenFont(font_path, (float)font_size);
    if (!ctx->font) {
        SDL_Log("TTF_OpenFont(%s): %s", font_path, SDL_GetError());
        return false;
    }
    ctx->font_size = font_size;
    TTF_SetFontKerning(ctx->font, true);

    GlyphCache_init(&ctx->glyph_cache, 512, ctx->gpu);

    ctx->pipeline_rect = create_rect_pipeline(ctx->gpu, ctx->window);
    ctx->pipeline_text = create_text_pipeline(ctx->gpu, ctx->window);
    if (!ctx->pipeline_rect || !ctx->pipeline_text) {
        SDL_Log("Pipeline creation failed");
        return false;
    }

    ctx->sampler_linear = SDL_CreateGPUSampler(ctx->gpu, &(SDL_GPUSamplerCreateInfo){
        .min_filter    = SDL_GPU_FILTER_LINEAR,
        .mag_filter    = SDL_GPU_FILTER_LINEAR,
        .address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
        .address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
    });

    uint32_t mem_size = Clay_MinMemorySize();
    ctx->clay_mem = SDL_malloc(mem_size);
    {
        int pw, ph;
        SDL_GetWindowSizeInPixels(ctx->window, &pw, &ph);
        float clay_w = (float)pw / ctx->dpi_scale;
        float clay_h = (float)ph / ctx->dpi_scale;
        Clay_Arena arena = Clay_CreateArenaWithCapacityAndMemory(mem_size, ctx->clay_mem);
        ctx->clay_ctx = Clay_Initialize(arena,
                                        (Clay_Dimensions){ clay_w, clay_h },
                                        (Clay_ErrorHandler){ clay_error_cb, NULL });
    }
    g_measure_ctx = ctx;
    Clay_SetMeasureTextFunction(measure_text_cb, NULL);

    SDL_SetGPUSwapchainParameters(ctx->gpu, ctx->window,
                                  SDL_GPU_SWAPCHAINCOMPOSITION_SDR,
                                  SDL_GPU_PRESENTMODE_VSYNC);
    ctx->dirty = true; /* always render the first frame */
    return true;
}

/* ---- Event handling ---- */
void KilnUI_handle_event(KilnUI *ctx, const SDL_Event *e)
{
    switch (e->type) {
    case SDL_EVENT_MOUSE_MOTION:
        Clay_SetPointerState(
            (Clay_Vector2){ e->motion.x * ctx->mouse_scale,
                            e->motion.y * ctx->mouse_scale },
            (e->motion.state & SDL_BUTTON_LMASK) != 0);
        ctx->dirty = true;
        break;
    case SDL_EVENT_MOUSE_BUTTON_DOWN:
    case SDL_EVENT_MOUSE_BUTTON_UP:
        Clay_SetPointerState(
            (Clay_Vector2){ e->button.x * ctx->mouse_scale,
                            e->button.y * ctx->mouse_scale },
            e->type == SDL_EVENT_MOUSE_BUTTON_DOWN);
        ctx->dirty = true;
        break;
    case SDL_EVENT_MOUSE_WHEEL:
        Clay_UpdateScrollContainers(true,
                                    (Clay_Vector2){ e->wheel.x * 30, e->wheel.y * 30 },
                                    0.016f);
        ctx->dirty = true;
        break;
    case SDL_EVENT_WINDOW_RESIZED:
    case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED: {
        int pw, ph, lw, lh;
        SDL_GetWindowSizeInPixels(ctx->window, &pw, &ph);
        SDL_GetWindowSize(ctx->window, &lw, &lh);
        float pixel_ratio   = (lw > 0) ? (float)pw / (float)lw : 1.0f;
        float display_scale = SDL_GetWindowDisplayScale(ctx->window);
        if (display_scale < 1.0f) display_scale = 1.0f;
        ctx->dpi_scale   = (pixel_ratio >= display_scale) ? pixel_ratio : display_scale;
        ctx->mouse_scale = (lw > 0) ? (float)pw / ((float)lw * ctx->dpi_scale) : 1.0f;
        Clay_SetLayoutDimensions((Clay_Dimensions){
            (float)pw / ctx->dpi_scale, (float)ph / ctx->dpi_scale });
        ctx->dirty = true;
        break;
    }
    }
}

/* ---- Destroy ---- */
void KilnUI_destroy(KilnUI *ctx)
{
    GlyphCache_destroy(&ctx->glyph_cache);
    if (ctx->rect_vbuf)    SDL_ReleaseGPUBuffer(ctx->gpu, ctx->rect_vbuf);
    if (ctx->rect_ibuf)    SDL_ReleaseGPUBuffer(ctx->gpu, ctx->rect_ibuf);
    if (ctx->text_vbuf)    SDL_ReleaseGPUBuffer(ctx->gpu, ctx->text_vbuf);
    if (ctx->text_ibuf)    SDL_ReleaseGPUBuffer(ctx->gpu, ctx->text_ibuf);
    if (ctx->staging_tbuf) SDL_ReleaseGPUTransferBuffer(ctx->gpu, ctx->staging_tbuf);
    if (ctx->pipeline_rect)  SDL_ReleaseGPUGraphicsPipeline(ctx->gpu, ctx->pipeline_rect);
    if (ctx->pipeline_text)  SDL_ReleaseGPUGraphicsPipeline(ctx->gpu, ctx->pipeline_text);
    if (ctx->sampler_linear) SDL_ReleaseGPUSampler(ctx->gpu, ctx->sampler_linear);
    if (ctx->font)           TTF_CloseFont(ctx->font);
    TTF_Quit();
    if (ctx->gpu && ctx->window)
        SDL_ReleaseWindowFromGPUDevice(ctx->gpu, ctx->window);
    if (ctx->gpu)      SDL_DestroyGPUDevice(ctx->gpu);
    if (ctx->window)   SDL_DestroyWindow(ctx->window);
    if (ctx->clay_mem) SDL_free(ctx->clay_mem);
    SDL_Quit();
}
