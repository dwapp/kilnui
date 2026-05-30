/* clay_sdl3_gpu.c — Clay + SDL3 GPU rendering backend implementation. */

#define CLAY_IMPLEMENTATION
#include "clay_sdl3_gpu.h"
#include <math.h>
#include <string.h>

/* ---- Font discovery ---- */
const char *ClayGPUCtx_find_font(const char **candidates)
{
    for (const char **p = candidates; *p; p++) {
        SDL_PathInfo info;
        if (SDL_GetPathInfo(*p, &info))
            return *p;
    }
    return NULL;
}

/* ---- Column-major 4x4 matrix ---- */
typedef struct
{
    float m[4][4];
} Mat4;

static Mat4 ortho_proj(float w, float h)
{
    Mat4 m = { 0 };
    m.m[0][0] = 2.0f / w;
    m.m[1][1] = -2.0f / h;
    m.m[2][2] = -1.0f;
    m.m[3][0] = -1.0f;
    m.m[3][1] = 1.0f;
    m.m[3][3] = 1.0f;
    return m;
}

/* ---- Shader loading ---- */
static SDL_GPUShader *load_spv(SDL_GPUDevice *dev, const char *path,
                               SDL_GPUShaderStage stage,
                               Uint32 num_ubo, Uint32 num_samplers)
{
    size_t sz = 0;
    void *code = SDL_LoadFile(path, &sz);
    if (!code) {
        SDL_Log("load_spv: %s: %s", path, SDL_GetError());
        return NULL;
    }
    SDL_GPUShaderCreateInfo ci = {
        .code = code,
        .code_size = sz,
        .entrypoint = "main",
        .format = SDL_GPU_SHADERFORMAT_SPIRV,
        .stage = stage,
        .num_uniform_buffers = num_ubo,
        .num_samplers = num_samplers,
    };
    SDL_GPUShader *s = SDL_CreateGPUShader(dev, &ci);
    SDL_free(code);
    if (!s)
        SDL_Log("CreateGPUShader(%s): %s", path, SDL_GetError());
    return s;
}

/* ---- Blend state (shared) ---- */
static SDL_GPUColorTargetDescription blend_desc(SDL_GPUDevice *dev, SDL_Window *win)
{
    return (SDL_GPUColorTargetDescription){
        .format = SDL_GetGPUSwapchainTextureFormat(dev, win),
        .blend_state = {
            .enable_blend = true,
            .color_blend_op = SDL_GPU_BLENDOP_ADD,
            .alpha_blend_op = SDL_GPU_BLENDOP_ADD,
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
    SDL_GPUShader *vert = load_spv(dev, "shaders/rect.vert.spv", SDL_GPU_SHADERSTAGE_VERTEX, 1, 0);
    SDL_GPUShader *frag = load_spv(dev, "shaders/rect.frag.spv", SDL_GPU_SHADERSTAGE_FRAGMENT, 0, 0);
    if (!vert || !frag) {
        if (vert)
            SDL_ReleaseGPUShader(dev, vert);
        if (frag)
            SDL_ReleaseGPUShader(dev, frag);
        return NULL;
    }
    SDL_GPUColorTargetDescription ctd = blend_desc(dev, win);
    SDL_GPUGraphicsPipelineCreateInfo ci = {
        .target_info = { .num_color_targets = 1, .color_target_descriptions = &ctd },
        .vertex_input_state = {
            .num_vertex_buffers = 1,
            .vertex_buffer_descriptions = (SDL_GPUVertexBufferDescription[]){ {
                .slot = 0,
                .pitch = sizeof(VertexRect),
                .input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX,
            } },
            .num_vertex_attributes = 5,
            .vertex_attributes = (SDL_GPUVertexAttribute[]){
                { .location = 0, .buffer_slot = 0, .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2, .offset = offsetof(VertexRect, pos_x) },
                { .location = 1, .buffer_slot = 0, .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2, .offset = offsetof(VertexRect, local_x) },
                { .location = 2, .buffer_slot = 0, .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2, .offset = offsetof(VertexRect, size_w) },
                { .location = 3, .buffer_slot = 0, .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4, .offset = offsetof(VertexRect, radius_tl) },
                { .location = 4, .buffer_slot = 0, .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4, .offset = offsetof(VertexRect, r) },
            },
        },
        .primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST,
        .vertex_shader = vert,
        .fragment_shader = frag,
    };
    SDL_GPUGraphicsPipeline *p = SDL_CreateGPUGraphicsPipeline(dev, &ci);
    SDL_ReleaseGPUShader(dev, vert);
    SDL_ReleaseGPUShader(dev, frag);
    return p;
}

static SDL_GPUGraphicsPipeline *create_text_pipeline(SDL_GPUDevice *dev, SDL_Window *win)
{
    SDL_GPUShader *vert = load_spv(dev, "shaders/tex.vert.spv", SDL_GPU_SHADERSTAGE_VERTEX, 1, 0);
    SDL_GPUShader *frag = load_spv(dev, "shaders/tex.frag.spv", SDL_GPU_SHADERSTAGE_FRAGMENT, 0, 1);
    if (!vert || !frag) {
        if (vert)
            SDL_ReleaseGPUShader(dev, vert);
        if (frag)
            SDL_ReleaseGPUShader(dev, frag);
        return NULL;
    }
    SDL_GPUColorTargetDescription ctd = blend_desc(dev, win);
    SDL_GPUGraphicsPipelineCreateInfo ci = {
        .target_info = { .num_color_targets = 1, .color_target_descriptions = &ctd },
        .vertex_input_state = {
            .num_vertex_buffers = 1,
            .vertex_buffer_descriptions = (SDL_GPUVertexBufferDescription[]){ {
                .slot = 0,
                .pitch = sizeof(VertexTex),
                .input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX,
            } },
            .num_vertex_attributes = 3,
            .vertex_attributes = (SDL_GPUVertexAttribute[]){
                { .location = 0, .buffer_slot = 0, .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2, .offset = offsetof(VertexTex, pos_x) },
                { .location = 1, .buffer_slot = 0, .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2, .offset = offsetof(VertexTex, u) },
                { .location = 2, .buffer_slot = 0, .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4, .offset = offsetof(VertexTex, r) },
            },
        },
        .primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST,
        .vertex_shader = vert,
        .fragment_shader = frag,
    };
    SDL_GPUGraphicsPipeline *p = SDL_CreateGPUGraphicsPipeline(dev, &ci);
    SDL_ReleaseGPUShader(dev, vert);
    SDL_ReleaseGPUShader(dev, frag);
    return p;
}

/* ---- MeasureText callback ---- */
static ClayGPUCtx *g_measure_ctx = NULL;

static Clay_Dimensions measure_text_cb(Clay_StringSlice text,
                                       Clay_TextElementConfig *config,
                                       void *userData)
{
    (void)userData;
    ClayGPUCtx *ctx = g_measure_ctx;
    if (!ctx || !ctx->font || !text.chars || text.length <= 0)
        return (Clay_Dimensions){ 0, 0 };

    /* Apply the font size requested by Clay's text config */
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

/* ---- Clay error handler ---- */
static void clay_error(Clay_ErrorData e) { SDL_Log("Clay: %s", e.errorText.chars); }

/* ---- Init ---- */
bool ClayGPUCtx_init(ClayGPUCtx *ctx, const char *title,
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

    ctx->window = SDL_CreateWindow(title, w, h, SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY);
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

    /* Compute dpi_scale as the ACTUAL physical/logical pixel ratio.
     * SDL_GetWindowDisplayScale() returns the display content scale (e.g. 1.25
     * for 125% DPI), which may differ from the real pixel ratio and cause mouse
     * coordinate offsets.  GetWindowSizeInPixels / GetWindowSize gives the true
     * conversion factor used by the vertex coordinate system. */
    {
        int pw, ph, lw, lh;
        SDL_GetWindowSizeInPixels(ctx->window, &pw, &ph);
        SDL_GetWindowSize(ctx->window, &lw, &lh);
        ctx->dpi_scale = (lw > 0) ? (float)pw / (float)lw : 1.0f;
    }

    /* Font */
    ctx->font = TTF_OpenFont(font_path, (float)font_size);
    if (!ctx->font) {
        SDL_Log("TTF_OpenFont(%s): %s", font_path, SDL_GetError());
        return false;
    }
    ctx->font_size = font_size;
    TTF_SetFontKerning(ctx->font, true);

    /* Glyph cache */
    GlyphCache_init(&ctx->glyph_cache, 512, ctx->gpu);

    /* Pipelines */
    ctx->pipeline_rect = create_rect_pipeline(ctx->gpu, ctx->window);
    ctx->pipeline_text = create_text_pipeline(ctx->gpu, ctx->window);
    if (!ctx->pipeline_rect || !ctx->pipeline_text) {
        SDL_Log("Pipeline creation failed");
        return false;
    }

    /* Sampler */
    ctx->sampler_linear = SDL_CreateGPUSampler(ctx->gpu, &(SDL_GPUSamplerCreateInfo){
                                                             .min_filter = SDL_GPU_FILTER_LINEAR,
                                                             .mag_filter = SDL_GPU_FILTER_LINEAR,
                                                             .address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
                                                             .address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
                                                         });

    /* Clay init */
    uint32_t mem_size = Clay_MinMemorySize();
    ctx->clay_mem = SDL_malloc(mem_size);
    Clay_Arena arena = Clay_CreateArenaWithCapacityAndMemory(mem_size, ctx->clay_mem);
    ctx->clay_ctx = Clay_Initialize(arena, (Clay_Dimensions){ (float)w, (float)h },
                                    (Clay_ErrorHandler){ clay_error, NULL });
    g_measure_ctx = ctx;
    Clay_SetMeasureTextFunction(measure_text_cb, NULL);

    /* Enable vsync to prevent the render loop from spinning at unlimited rate */
    SDL_SetGPUSwapchainParameters(ctx->gpu, ctx->window,
                                  SDL_GPU_SWAPCHAINCOMPOSITION_SDR, SDL_GPU_PRESENTMODE_VSYNC);

    return true;
}

/* ---- Event handling ---- */
void ClayGPUCtx_handle_event(ClayGPUCtx *ctx, const SDL_Event *e)
{
    switch (e->type) {
    case SDL_EVENT_MOUSE_MOTION:
        Clay_SetPointerState((Clay_Vector2){ e->motion.x, e->motion.y }, (e->motion.state & SDL_BUTTON_LMASK) != 0);
        break;
    case SDL_EVENT_MOUSE_BUTTON_DOWN:
    case SDL_EVENT_MOUSE_BUTTON_UP:
        Clay_SetPointerState((Clay_Vector2){ e->button.x, e->button.y }, e->type == SDL_EVENT_MOUSE_BUTTON_DOWN);
        break;
    case SDL_EVENT_MOUSE_WHEEL:
        Clay_UpdateScrollContainers(true, (Clay_Vector2){ e->wheel.x * 30, e->wheel.y * 30 }, 0.016f);
        break;
    case SDL_EVENT_WINDOW_RESIZED:
        /* SDL_EVENT_WINDOW_RESIZED gives logical dimensions */
        Clay_SetLayoutDimensions((Clay_Dimensions){ (float)e->window.data1, (float)e->window.data2 });
        /* Fall through: refresh dpi_scale in case the window moved to a
         * different monitor or the pixel density changed. */
        /* FALLTHROUGH */
    case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
    {
        /* Recompute actual physical/logical ratio after any resize or DPI change */
        int pw, ph, lw, lh;
        SDL_GetWindowSizeInPixels(ctx->window, &pw, &ph);
        SDL_GetWindowSize(ctx->window, &lw, &lh);
        if (lw > 0)
            ctx->dpi_scale = (float)pw / (float)lw;
        break;
    }
    }
}

/* ---- Rect batch state ---- */
static VertexRect s_rect_verts[MAX_RECTS * 4];
static Uint16 s_rect_idx[MAX_RECTS * 6];
static int s_rect_count;

static void push_rect(float x, float y, float w, float h,
                      Clay_Color c, Clay_CornerRadius cr, float scale)
{
    if (s_rect_count >= MAX_RECTS)
        return;
    x *= scale;
    y *= scale;
    w *= scale;
    h *= scale;
    float nr = (c.r / 255.f) * c.a / 255.f, ng = (c.g / 255.f) * c.a / 255.f;
    float nb = (c.b / 255.f) * c.a / 255.f, na = c.a / 255.f;
    float max_r = fminf(w, h) * 0.5f;
    float rtl = fminf(cr.topLeft * scale, max_r);
    float rtr = fminf(cr.topRight * scale, max_r);
    float rbl = fminf(cr.bottomLeft * scale, max_r);
    float rbr = fminf(cr.bottomRight * scale, max_r);
    int vi = s_rect_count * 4, ii = s_rect_count * 6;
    s_rect_verts[vi + 0] = (VertexRect){ x, y, 0, 0, w, h, rtl, rtr, rbl, rbr, nr, ng, nb, na };
    s_rect_verts[vi + 1] = (VertexRect){ x + w, y, w, 0, w, h, rtl, rtr, rbl, rbr, nr, ng, nb, na };
    s_rect_verts[vi + 2] = (VertexRect){ x + w, y + h, w, h, w, h, rtl, rtr, rbl, rbr, nr, ng, nb, na };
    s_rect_verts[vi + 3] = (VertexRect){ x, y + h, 0, h, w, h, rtl, rtr, rbl, rbr, nr, ng, nb, na };
    s_rect_idx[ii + 0] = vi;
    s_rect_idx[ii + 1] = vi + 1;
    s_rect_idx[ii + 2] = vi + 2;
    s_rect_idx[ii + 3] = vi;
    s_rect_idx[ii + 4] = vi + 2;
    s_rect_idx[ii + 5] = vi + 3;
    s_rect_count++;
}

/* ---- Persistent GPU buffer helpers ---- */
/* Grows *buf to at least `needed` bytes; no-op if already large enough.
 * SDL3 defers deletion, so no GPU idle wait is needed before release. */
static void ensure_gpu_buffer(SDL_GPUDevice *gpu, SDL_GPUBuffer **buf, uint32_t *cap,
                              uint32_t needed, SDL_GPUBufferUsageFlags usage)
{
    if (*cap >= needed) return;
    if (*buf) SDL_ReleaseGPUBuffer(gpu, *buf);
    uint32_t nc = needed + needed / 2;  /* 1.5x over-alloc to amortise growth */
    *buf = SDL_CreateGPUBuffer(gpu, &(SDL_GPUBufferCreateInfo){ .usage = usage, .size = nc });
    if (!*buf) { SDL_Log("ensure_gpu_buffer: %s", SDL_GetError()); *cap = 0; return; }
    *cap = nc;
}

static void ensure_transfer_buffer(SDL_GPUDevice *gpu, SDL_GPUTransferBuffer **tbuf,
                                   uint32_t *cap, uint32_t needed)
{
    if (*cap >= needed) return;
    if (*tbuf) SDL_ReleaseGPUTransferBuffer(gpu, *tbuf);
    uint32_t nc = needed + needed / 2;
    *tbuf = SDL_CreateGPUTransferBuffer(gpu,
        &(SDL_GPUTransferBufferCreateInfo){ .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD, .size = nc });
    if (!*tbuf) { SDL_Log("ensure_transfer_buffer: %s", SDL_GetError()); *cap = 0; return; }
    *cap = nc;
}

/* ---- Pre-build text vertex data for one TEXT command ---- */
typedef struct
{
    VertexTex verts[256 * 4];
    Uint16 idx[256 * 6];
    int quad_count;
    SDL_GPUTexture *glyph_textures[256];
} TextBatch;

/* Static storage to avoid ~2 MB stack allocation in the render function */
static TextBatch s_text_batches[MAX_TEXT_CMDS];

static void build_text_batch(ClayGPUCtx *ctx, TextBatch *tb,
                             Clay_BoundingBox bb, Clay_TextRenderData *td, float scale)
{
    const char *str = td->stringContents.chars;
    int len = td->stringContents.length;
    tb->quad_count = 0;
    if (!str || len <= 0)
        return;

    /* Switch font to the size requested by this text element */
    int req_size = (td->fontSize > 0) ? td->fontSize : ctx->font_size;
    TTF_SetFontSize(ctx->font, (float)req_size);

    int font_ascent = TTF_GetFontAscent(ctx->font);
    float cx = bb.x * scale;
    float cy = bb.y * scale;

    float nr = (td->textColor.r / 255.f) * (td->textColor.a / 255.f);
    float ng = (td->textColor.g / 255.f) * (td->textColor.a / 255.f);
    float nb = (td->textColor.b / 255.f) * (td->textColor.a / 255.f);
    float na = td->textColor.a / 255.f;

    uint32_t prev_cp = 0;
    for (int i = 0; i < len && tb->quad_count < 256;) {
        uint32_t cp;
        uint8_t c = (uint8_t)str[i];
        if (c < 0x80) {
            cp = c;
            i += 1;
        } else if (c < 0xE0) {
            cp = (c & 0x1F) << 6 | (str[i + 1] & 0x3F);
            i += 2;
        } else if (c < 0xF0) {
            cp = (c & 0x0F) << 12 | (str[i + 1] & 0x3F) << 6 | (str[i + 2] & 0x3F);
            i += 3;
        } else {
            cp = (c & 0x07) << 18 | (str[i + 1] & 0x3F) << 12 | (str[i + 2] & 0x3F) << 6 | (str[i + 3] & 0x3F);
            i += 4;
        }

        if (prev_cp) {
            int kern = 0;
            TTF_GetGlyphKerning(ctx->font, prev_cp, cp, &kern);
            cx += kern * scale;
        }
        prev_cp = cp;

        /* Rasterize at the requested size (GlyphCache key includes font_size) */
        const GlyphEntry *ge = GlyphCache_get(&ctx->glyph_cache, ctx->font,
                                              cp, (uint16_t)req_size);
        if (!ge) {
            cx += 8 * scale;
            continue;
        }

        float gx = cx + ge->bearing_x * scale;
        float gy = cy + (font_ascent - ge->bearing_y) * scale;
        float gw = ge->w * scale;
        float gh = ge->h * scale;

        int q = tb->quad_count;
        int vi = q * 4, ii = q * 6;
        tb->verts[vi + 0] = (VertexTex){ gx, gy, 0, 0, nr, ng, nb, na };
        tb->verts[vi + 1] = (VertexTex){ gx + gw, gy, 1, 0, nr, ng, nb, na };
        tb->verts[vi + 2] = (VertexTex){ gx + gw, gy + gh, 1, 1, nr, ng, nb, na };
        tb->verts[vi + 3] = (VertexTex){ gx, gy + gh, 0, 1, nr, ng, nb, na };
        tb->idx[ii + 0] = vi;
        tb->idx[ii + 1] = vi + 1;
        tb->idx[ii + 2] = vi + 2;
        tb->idx[ii + 3] = vi;
        tb->idx[ii + 4] = vi + 2;
        tb->idx[ii + 5] = vi + 3;
        tb->glyph_textures[q] = ge->tex;

        cx += ge->advance * scale;
        if (td->letterSpacing)
            cx += td->letterSpacing * scale;
        tb->quad_count++;
    }
}

/* ---- Main render function ---- */
void ClayGPUCtx_render(ClayGPUCtx *ctx, Clay_RenderCommandArray cmds)
{
    int pw, ph;
    SDL_GetWindowSizeInPixels(ctx->window, &pw, &ph);
    float scale = ctx->dpi_scale;
    Mat4 proj   = ortho_proj((float)pw, (float)ph);

    /* ===== Phase 1: Collect geometry, map each cmd to its slot ===== */
#define MAX_CMDS 2048
    static int cmd_to_rect[MAX_CMDS]; /* rect slot index, -1 if not a RECT */
    static int cmd_to_text[MAX_CMDS]; /* text batch index, -1 if not TEXT  */

    int n = cmds.length < MAX_CMDS ? cmds.length : MAX_CMDS;
    for (int i = 0; i < n; i++) { cmd_to_rect[i] = -1; cmd_to_text[i] = -1; }

    s_rect_count = 0;
    for (int i = 0; i < n; i++) {
        Clay_RenderCommand *cmd = Clay_RenderCommandArray_Get(&cmds, i);
        if (cmd->commandType == CLAY_RENDER_COMMAND_TYPE_RECTANGLE) {
            cmd_to_rect[i] = s_rect_count;
            Clay_BoundingBox bb = cmd->boundingBox;
            push_rect(bb.x, bb.y, bb.width, bb.height,
                      cmd->renderData.rectangle.backgroundColor,
                      cmd->renderData.rectangle.cornerRadius, scale);
        }
    }

    /* Per-batch vertex/index element offsets inside the combined text buffers */
    static uint32_t text_vtx_off[MAX_TEXT_CMDS];
    static uint32_t text_idx_off[MAX_TEXT_CMDS];
    uint32_t text_vtx_total = 0, text_idx_total = 0;
    int tb_count = 0;
    for (int i = 0; i < n && tb_count < MAX_TEXT_CMDS; i++) {
        Clay_RenderCommand *cmd = Clay_RenderCommandArray_Get(&cmds, i);
        if (cmd->commandType == CLAY_RENDER_COMMAND_TYPE_TEXT) {
            cmd_to_text[i] = tb_count;
            build_text_batch(ctx, &s_text_batches[tb_count],
                             cmd->boundingBox, &cmd->renderData.text, scale);
            text_vtx_off[tb_count] = text_vtx_total;
            text_idx_off[tb_count] = text_idx_total;
            text_vtx_total += (uint32_t)s_text_batches[tb_count].quad_count * 4;
            text_idx_total += (uint32_t)s_text_batches[tb_count].quad_count * 6;
            tb_count++;
        }
    }

    /* ===== Phase 2: Flush glyph uploads + upload all geometry ===== */
    /* Batch all newly rasterized glyphs into ONE copy pass before render pass */
    GlyphCache_flush_uploads(&ctx->glyph_cache);

    uint32_t rv_sz = (uint32_t)s_rect_count * 4 * sizeof(VertexRect);
    uint32_t ri_sz = (uint32_t)s_rect_count * 6 * sizeof(Uint16);
    uint32_t tv_sz = text_vtx_total * sizeof(VertexTex);
    uint32_t ti_sz = text_idx_total * sizeof(Uint16);
    uint32_t total = rv_sz + ri_sz + tv_sz + ti_sz;

    if (total > 0) {
        /* Grow persistent GPU buffers as needed — never shrink */
        ensure_gpu_buffer(ctx->gpu, &ctx->rect_vbuf, &ctx->rect_vbuf_cap,
                          rv_sz ? rv_sz : 4, SDL_GPU_BUFFERUSAGE_VERTEX);
        ensure_gpu_buffer(ctx->gpu, &ctx->rect_ibuf, &ctx->rect_ibuf_cap,
                          ri_sz ? ri_sz : 4, SDL_GPU_BUFFERUSAGE_INDEX);
        ensure_gpu_buffer(ctx->gpu, &ctx->text_vbuf, &ctx->text_vbuf_cap,
                          tv_sz ? tv_sz : 4, SDL_GPU_BUFFERUSAGE_VERTEX);
        ensure_gpu_buffer(ctx->gpu, &ctx->text_ibuf, &ctx->text_ibuf_cap,
                          ti_sz ? ti_sz : 4, SDL_GPU_BUFFERUSAGE_INDEX);
        ensure_transfer_buffer(ctx->gpu, &ctx->staging_tbuf,
                               &ctx->staging_tbuf_cap, total);

        /* Pack layout: [rect_verts | rect_idx | text_verts | text_idx] */
        uint8_t *m   = SDL_MapGPUTransferBuffer(ctx->gpu, ctx->staging_tbuf, true);
        uint32_t off = 0;
        uint32_t off_rv = off;
        if (rv_sz) { SDL_memcpy(m + off, s_rect_verts, rv_sz); off += rv_sz; }
        uint32_t off_ri = off;
        if (ri_sz) { SDL_memcpy(m + off, s_rect_idx,   ri_sz); off += ri_sz; }
        uint32_t off_tv = off;
        for (int t = 0; t < tb_count; t++) {
            int qc = s_text_batches[t].quad_count;
            if (qc > 0) {
                SDL_memcpy(m + off, s_text_batches[t].verts,
                           (size_t)qc * 4 * sizeof(VertexTex));
                off += (uint32_t)qc * 4 * sizeof(VertexTex);
            }
        }
        uint32_t off_ti = off;
        for (int t = 0; t < tb_count; t++) {
            int qc = s_text_batches[t].quad_count;
            if (qc > 0) {
                SDL_memcpy(m + off, s_text_batches[t].idx,
                           (size_t)qc * 6 * sizeof(Uint16));
                off += (uint32_t)qc * 6 * sizeof(Uint16);
            }
        }
        SDL_UnmapGPUTransferBuffer(ctx->gpu, ctx->staging_tbuf);

        /* ONE copy pass, ONE submit for all geometry */
        SDL_GPUCommandBuffer *cc = SDL_AcquireGPUCommandBuffer(ctx->gpu);
        SDL_GPUCopyPass    *cp   = SDL_BeginGPUCopyPass(cc);
        if (rv_sz) SDL_UploadToGPUBuffer(cp,
            &(SDL_GPUTransferBufferLocation){ .transfer_buffer = ctx->staging_tbuf, .offset = off_rv },
            &(SDL_GPUBufferRegion){ .buffer = ctx->rect_vbuf, .size = rv_sz }, true);
        if (ri_sz) SDL_UploadToGPUBuffer(cp,
            &(SDL_GPUTransferBufferLocation){ .transfer_buffer = ctx->staging_tbuf, .offset = off_ri },
            &(SDL_GPUBufferRegion){ .buffer = ctx->rect_ibuf, .size = ri_sz }, true);
        if (tv_sz) SDL_UploadToGPUBuffer(cp,
            &(SDL_GPUTransferBufferLocation){ .transfer_buffer = ctx->staging_tbuf, .offset = off_tv },
            &(SDL_GPUBufferRegion){ .buffer = ctx->text_vbuf, .size = tv_sz }, true);
        if (ti_sz) SDL_UploadToGPUBuffer(cp,
            &(SDL_GPUTransferBufferLocation){ .transfer_buffer = ctx->staging_tbuf, .offset = off_ti },
            &(SDL_GPUBufferRegion){ .buffer = ctx->text_ibuf, .size = ti_sz }, true);
        SDL_EndGPUCopyPass(cp);
        SDL_SubmitGPUCommandBuffer(cc);
    }

    /* ===== Phase 3: Render pass — iterate Clay commands IN ORDER ===== */
    SDL_GPUCommandBuffer *cmdbuf = SDL_AcquireGPUCommandBuffer(ctx->gpu);
    if (!cmdbuf) return;

    SDL_GPUTexture *swapchain = NULL;
    SDL_WaitAndAcquireGPUSwapchainTexture(cmdbuf, ctx->window, &swapchain, NULL, NULL);
    if (!swapchain) { SDL_SubmitGPUCommandBuffer(cmdbuf); return; }

    SDL_GPUColorTargetInfo ct = {
        .texture     = swapchain,
        .clear_color = { 0.08f, 0.08f, 0.12f, 1.0f },
        .load_op     = SDL_GPU_LOADOP_CLEAR,
        .store_op    = SDL_GPU_STOREOP_STORE,
    };
    SDL_GPURenderPass *rp = SDL_BeginGPURenderPass(cmdbuf, &ct, 1, NULL);

    /* Render in Clay's command order to honour Z-layering.
     * We switch the active pipeline only when the command type changes.
     * SDL_GPU's vertex_offset parameter rebases per-batch text indices. */
    SDL_GPUGraphicsPipeline *active_pipe = NULL;

    for (int i = 0; i < n; i++) {
        Clay_RenderCommand *cmd = Clay_RenderCommandArray_Get(&cmds, i);
        switch (cmd->commandType) {

        case CLAY_RENDER_COMMAND_TYPE_RECTANGLE: {
            int ri = cmd_to_rect[i];
            if (ri < 0 || !ctx->rect_vbuf) break;
            if (active_pipe != ctx->pipeline_rect) {
                SDL_BindGPUGraphicsPipeline(rp, ctx->pipeline_rect);
                SDL_PushGPUVertexUniformData(cmdbuf, 0, &proj, sizeof(proj));
                SDL_BindGPUVertexBuffers(rp, 0,
                    &(SDL_GPUBufferBinding){ .buffer = ctx->rect_vbuf }, 1);
                SDL_BindGPUIndexBuffer(rp,
                    &(SDL_GPUBufferBinding){ .buffer = ctx->rect_ibuf },
                    SDL_GPU_INDEXELEMENTSIZE_16BIT);
                active_pipe = ctx->pipeline_rect;
            }
            SDL_DrawGPUIndexedPrimitives(rp, 6, 1, (Uint32)(ri * 6), 0, 0);
            break;
        }

        case CLAY_RENDER_COMMAND_TYPE_TEXT: {
            int ti = cmd_to_text[i];
            if (ti < 0 || !ctx->text_vbuf) break;
            TextBatch *tb = &s_text_batches[ti];
            if (tb->quad_count == 0) break;
            if (active_pipe != ctx->pipeline_text) {
                SDL_BindGPUGraphicsPipeline(rp, ctx->pipeline_text);
                SDL_PushGPUVertexUniformData(cmdbuf, 0, &proj, sizeof(proj));
                SDL_BindGPUVertexBuffers(rp, 0,
                    &(SDL_GPUBufferBinding){ .buffer = ctx->text_vbuf }, 1);
                SDL_BindGPUIndexBuffer(rp,
                    &(SDL_GPUBufferBinding){ .buffer = ctx->text_ibuf },
                    SDL_GPU_INDEXELEMENTSIZE_16BIT);
                active_pipe = ctx->pipeline_text;
            }
            /* vertex_offset shifts the per-batch (0-based) indices into the
             * combined text VBO — no index rebasing needed in CPU code.    */
            Uint32  idx_base = text_idx_off[ti];
            Sint32  vtx_base = (Sint32)text_vtx_off[ti];
            for (int q = 0; q < tb->quad_count; q++) {
                SDL_BindGPUFragmentSamplers(rp, 0,
                    &(SDL_GPUTextureSamplerBinding){
                        .texture = tb->glyph_textures[q],
                        .sampler = ctx->sampler_linear }, 1);
                SDL_DrawGPUIndexedPrimitives(rp, 6, 1,
                    idx_base + (Uint32)(q * 6), vtx_base, 0);
            }
            break;
        }

        case CLAY_RENDER_COMMAND_TYPE_SCISSOR_START: {
            Clay_BoundingBox sb = cmd->boundingBox;
            SDL_Rect sc = { (int)(sb.x * scale), (int)(sb.y * scale),
                            (int)(sb.width * scale), (int)(sb.height * scale) };
            SDL_SetGPUScissor(rp, &sc);
            break;
        }
        case CLAY_RENDER_COMMAND_TYPE_SCISSOR_END:
            SDL_SetGPUScissor(rp, &(SDL_Rect){ 0, 0, pw, ph });
            break;
        default:
            break;
        }
    }

    SDL_EndGPURenderPass(rp);
    SDL_SubmitGPUCommandBuffer(cmdbuf);
}

/* ---- Destroy ---- */
void ClayGPUCtx_destroy(ClayGPUCtx *ctx)
{
    GlyphCache_destroy(&ctx->glyph_cache);
    /* Persistent geometry buffers */
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
