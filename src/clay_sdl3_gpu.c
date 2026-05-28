/* clay_sdl3_gpu.c — Clay + SDL3 GPU rendering backend implementation. */

#define CLAY_IMPLEMENTATION
#include "clay_sdl3_gpu.h"
#include <string.h>
#include <math.h>

/* ---- Font discovery ---- */
const char *ClayGPUCtx_find_font(const char **candidates) {
    for (const char **p = candidates; *p; p++) {
        SDL_PathInfo info;
        if (SDL_GetPathInfo(*p, &info)) return *p;
    }
    return NULL;
}


/* ---- Column-major 4x4 matrix ---- */
typedef struct { float m[4][4]; } Mat4;

static Mat4 ortho_proj(float w, float h) {
    Mat4 m = {0};
    m.m[0][0] =  2.0f / w;
    m.m[1][1] = -2.0f / h;
    m.m[2][2] = -1.0f;
    m.m[3][0] = -1.0f;
    m.m[3][1] =  1.0f;
    m.m[3][3] =  1.0f;
    return m;
}

/* ---- Shader loading ---- */
static SDL_GPUShader *load_spv(SDL_GPUDevice *dev, const char *path,
                               SDL_GPUShaderStage stage,
                               Uint32 num_ubo, Uint32 num_samplers) {
    size_t sz = 0;
    void *code = SDL_LoadFile(path, &sz);
    if (!code) { SDL_Log("load_spv: %s: %s", path, SDL_GetError()); return NULL; }
    SDL_GPUShaderCreateInfo ci = {
        .code = code, .code_size = sz,
        .entrypoint = "main",
        .format = SDL_GPU_SHADERFORMAT_SPIRV,
        .stage = stage,
        .num_uniform_buffers = num_ubo,
        .num_samplers = num_samplers,
    };
    SDL_GPUShader *s = SDL_CreateGPUShader(dev, &ci);
    SDL_free(code);
    if (!s) SDL_Log("CreateGPUShader(%s): %s", path, SDL_GetError());
    return s;
}

/* ---- Blend state (shared) ---- */
static SDL_GPUColorTargetDescription blend_desc(SDL_GPUDevice *dev, SDL_Window *win) {
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
static SDL_GPUGraphicsPipeline *create_rect_pipeline(SDL_GPUDevice *dev, SDL_Window *win) {
    SDL_GPUShader *vert = load_spv(dev, "shaders/rect.vert.spv", SDL_GPU_SHADERSTAGE_VERTEX, 1, 0);
    SDL_GPUShader *frag = load_spv(dev, "shaders/rect.frag.spv", SDL_GPU_SHADERSTAGE_FRAGMENT, 0, 0);
    if (!vert || !frag) { if(vert) SDL_ReleaseGPUShader(dev,vert); if(frag) SDL_ReleaseGPUShader(dev,frag); return NULL; }
    SDL_GPUColorTargetDescription ctd = blend_desc(dev, win);
    SDL_GPUGraphicsPipelineCreateInfo ci = {
        .target_info = { .num_color_targets = 1, .color_target_descriptions = &ctd },
        .vertex_input_state = {
            .num_vertex_buffers = 1,
            .vertex_buffer_descriptions = (SDL_GPUVertexBufferDescription[]){{
                .slot = 0, .pitch = sizeof(VertexRect), .input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX,
            }},
            .num_vertex_attributes = 5,
            .vertex_attributes = (SDL_GPUVertexAttribute[]){
                {.location=0, .buffer_slot=0, .format=SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2, .offset=offsetof(VertexRect, pos_x)},
                {.location=1, .buffer_slot=0, .format=SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2, .offset=offsetof(VertexRect, local_x)},
                {.location=2, .buffer_slot=0, .format=SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2, .offset=offsetof(VertexRect, size_w)},
                {.location=3, .buffer_slot=0, .format=SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4, .offset=offsetof(VertexRect, radius_tl)},
                {.location=4, .buffer_slot=0, .format=SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4, .offset=offsetof(VertexRect, r)},
            },
        },
        .primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST,
        .vertex_shader = vert, .fragment_shader = frag,
    };
    SDL_GPUGraphicsPipeline *p = SDL_CreateGPUGraphicsPipeline(dev, &ci);
    SDL_ReleaseGPUShader(dev, vert); SDL_ReleaseGPUShader(dev, frag);
    return p;
}

static SDL_GPUGraphicsPipeline *create_text_pipeline(SDL_GPUDevice *dev, SDL_Window *win) {
    SDL_GPUShader *vert = load_spv(dev, "shaders/tex.vert.spv", SDL_GPU_SHADERSTAGE_VERTEX, 1, 0);
    SDL_GPUShader *frag = load_spv(dev, "shaders/tex.frag.spv", SDL_GPU_SHADERSTAGE_FRAGMENT, 0, 1);
    if (!vert || !frag) { if(vert) SDL_ReleaseGPUShader(dev,vert); if(frag) SDL_ReleaseGPUShader(dev,frag); return NULL; }
    SDL_GPUColorTargetDescription ctd = blend_desc(dev, win);
    SDL_GPUGraphicsPipelineCreateInfo ci = {
        .target_info = { .num_color_targets = 1, .color_target_descriptions = &ctd },
        .vertex_input_state = {
            .num_vertex_buffers = 1,
            .vertex_buffer_descriptions = (SDL_GPUVertexBufferDescription[]){{
                .slot = 0, .pitch = sizeof(VertexTex), .input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX,
            }},
            .num_vertex_attributes = 3,
            .vertex_attributes = (SDL_GPUVertexAttribute[]){
                {.location=0, .buffer_slot=0, .format=SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2, .offset=offsetof(VertexTex, pos_x)},
                {.location=1, .buffer_slot=0, .format=SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2, .offset=offsetof(VertexTex, u)},
                {.location=2, .buffer_slot=0, .format=SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4, .offset=offsetof(VertexTex, r)},
            },
        },
        .primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST,
        .vertex_shader = vert, .fragment_shader = frag,
    };
    SDL_GPUGraphicsPipeline *p = SDL_CreateGPUGraphicsPipeline(dev, &ci);
    SDL_ReleaseGPUShader(dev, vert); SDL_ReleaseGPUShader(dev, frag);
    return p;
}

/* ---- MeasureText callback ---- */
static ClayGPUCtx *g_measure_ctx = NULL;

static Clay_Dimensions measure_text_cb(Clay_StringSlice text,
                                       Clay_TextElementConfig *config,
                                       void *userData) {
    (void)userData;
    ClayGPUCtx *ctx = g_measure_ctx;
    if (!ctx || !ctx->font || !text.chars || text.length <= 0)
        return (Clay_Dimensions){0, 0};

    int w = 0, h = 0;
    TTF_GetStringSize(ctx->font, text.chars, (size_t)text.length, &w, &h);
    if (config->letterSpacing && text.length > 1)
        w += config->letterSpacing * ((int)text.length - 1);
    if (config->lineHeight > 0 && config->lineHeight > (uint16_t)h)
        h = config->lineHeight;
    return (Clay_Dimensions){(float)w, (float)h};
}

/* ---- Clay error handler ---- */
static void clay_error(Clay_ErrorData e) { SDL_Log("Clay: %s", e.errorText.chars); }

/* ---- Init ---- */
bool ClayGPUCtx_init(ClayGPUCtx *ctx, const char *title,
                     int w, int h, const char *font_path, int font_size) {
    memset(ctx, 0, sizeof(*ctx));

    if (!SDL_Init(SDL_INIT_VIDEO)) { SDL_Log("SDL_Init: %s", SDL_GetError()); return false; }
    /* Prevent KWin from disabling the compositor (avoids tearing / bypass mode) */
    SDL_SetHint(SDL_HINT_VIDEO_X11_NET_WM_BYPASS_COMPOSITOR, "0");
    if (!TTF_Init()) { SDL_Log("TTF_Init: %s", SDL_GetError()); return false; }

    ctx->window = SDL_CreateWindow(title, w, h, SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY);
    if (!ctx->window) { SDL_Log("CreateWindow: %s", SDL_GetError()); return false; }

    ctx->gpu = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV, true, NULL);
    if (!ctx->gpu) { SDL_Log("CreateGPUDevice: %s", SDL_GetError()); return false; }
    if (!SDL_ClaimWindowForGPUDevice(ctx->gpu, ctx->window)) { SDL_Log("ClaimWindow: %s", SDL_GetError()); return false; }

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
    if (!ctx->font) { SDL_Log("TTF_OpenFont(%s): %s", font_path, SDL_GetError()); return false; }
    ctx->font_size = font_size;
    TTF_SetFontKerning(ctx->font, true);

    /* Glyph cache */
    GlyphCache_init(&ctx->glyph_cache, 512, ctx->gpu);

    /* Pipelines */
    ctx->pipeline_rect = create_rect_pipeline(ctx->gpu, ctx->window);
    ctx->pipeline_text = create_text_pipeline(ctx->gpu, ctx->window);
    if (!ctx->pipeline_rect || !ctx->pipeline_text) { SDL_Log("Pipeline creation failed"); return false; }

    /* Sampler */
    ctx->sampler_linear = SDL_CreateGPUSampler(ctx->gpu, &(SDL_GPUSamplerCreateInfo){
        .min_filter = SDL_GPU_FILTER_LINEAR, .mag_filter = SDL_GPU_FILTER_LINEAR,
        .address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
        .address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
    });

    /* Clay init */
    uint32_t mem_size = Clay_MinMemorySize();
    ctx->clay_mem = SDL_malloc(mem_size);
    Clay_Arena arena = Clay_CreateArenaWithCapacityAndMemory(mem_size, ctx->clay_mem);
    ctx->clay_ctx = Clay_Initialize(arena, (Clay_Dimensions){(float)w, (float)h},
                                    (Clay_ErrorHandler){clay_error, NULL});
    g_measure_ctx = ctx;
    Clay_SetMeasureTextFunction(measure_text_cb, NULL);

    /* Enable vsync to prevent the render loop from spinning at unlimited rate */
    SDL_SetGPUSwapchainParameters(ctx->gpu, ctx->window,
        SDL_GPU_SWAPCHAINCOMPOSITION_SDR, SDL_GPU_PRESENTMODE_VSYNC);

    return true;
}

/* ---- Event handling ---- */
void ClayGPUCtx_handle_event(ClayGPUCtx *ctx, const SDL_Event *e) {
    switch (e->type) {
    case SDL_EVENT_MOUSE_MOTION:
        Clay_SetPointerState((Clay_Vector2){e->motion.x, e->motion.y}, (e->motion.state & SDL_BUTTON_LMASK) != 0);
        break;
    case SDL_EVENT_MOUSE_BUTTON_DOWN:
    case SDL_EVENT_MOUSE_BUTTON_UP:
        Clay_SetPointerState((Clay_Vector2){e->button.x, e->button.y}, e->type == SDL_EVENT_MOUSE_BUTTON_DOWN);
        break;
    case SDL_EVENT_MOUSE_WHEEL:
        Clay_UpdateScrollContainers(true, (Clay_Vector2){e->wheel.x * 30, e->wheel.y * 30}, 0.016f);
        break;
    case SDL_EVENT_WINDOW_RESIZED:
        /* SDL_EVENT_WINDOW_RESIZED gives logical dimensions */
        Clay_SetLayoutDimensions((Clay_Dimensions){(float)e->window.data1, (float)e->window.data2});
        /* Fall through: refresh dpi_scale in case the window moved to a
         * different monitor or the pixel density changed. */
        /* FALLTHROUGH */
    case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED: {
        /* Recompute actual physical/logical ratio after any resize or DPI change */
        int pw, ph, lw, lh;
        SDL_GetWindowSizeInPixels(ctx->window, &pw, &ph);
        SDL_GetWindowSize(ctx->window, &lw, &lh);
        if (lw > 0) ctx->dpi_scale = (float)pw / (float)lw;
        break;
    }
    }
}

/* ---- Rect batch state ---- */
static VertexRect s_rect_verts[MAX_RECTS * 4];
static Uint16     s_rect_idx[MAX_RECTS * 6];
static int        s_rect_count;

static void push_rect(float x, float y, float w, float h,
                      Clay_Color c, Clay_CornerRadius cr, float scale) {
    if (s_rect_count >= MAX_RECTS) return;
    x *= scale; y *= scale; w *= scale; h *= scale;
    float nr = (c.r/255.f)*c.a/255.f, ng = (c.g/255.f)*c.a/255.f;
    float nb = (c.b/255.f)*c.a/255.f, na = c.a/255.f;
    float max_r = fminf(w, h) * 0.5f;
    float rtl = fminf(cr.topLeft * scale, max_r);
    float rtr = fminf(cr.topRight * scale, max_r);
    float rbl = fminf(cr.bottomLeft * scale, max_r);
    float rbr = fminf(cr.bottomRight * scale, max_r);
    int vi = s_rect_count * 4, ii = s_rect_count * 6;
    s_rect_verts[vi+0] = (VertexRect){x,   y,   0,0, w,h, rtl,rtr,rbl,rbr, nr,ng,nb,na};
    s_rect_verts[vi+1] = (VertexRect){x+w, y,   w,0, w,h, rtl,rtr,rbl,rbr, nr,ng,nb,na};
    s_rect_verts[vi+2] = (VertexRect){x+w, y+h, w,h, w,h, rtl,rtr,rbl,rbr, nr,ng,nb,na};
    s_rect_verts[vi+3] = (VertexRect){x,   y+h, 0,h, w,h, rtl,rtr,rbl,rbr, nr,ng,nb,na};
    s_rect_idx[ii+0]=vi; s_rect_idx[ii+1]=vi+1; s_rect_idx[ii+2]=vi+2;
    s_rect_idx[ii+3]=vi; s_rect_idx[ii+4]=vi+2; s_rect_idx[ii+5]=vi+3;
    s_rect_count++;
}

/* ---- Batched GPU upload: copies ALL vertex/index data in ONE copy pass ---- *
 * Callers fill an upload_batch, then call upload_batch_commit() which:
 *   1. Maps a single transfer buffer
 *   2. Copies all regions into it
 *   3. Runs ONE copy pass in ONE command buffer
 *   4. Submits that command buffer once
 * This eliminates N separate AcquireGPUCommandBuffer/Submit calls per frame.
 */

#define UPLOAD_MAX_REGIONS 128
typedef struct {
    /* interleaved vertex+index layout in the transfer buffer:
     * [vert0 | idx0 | vert1 | idx1 | ...] */
    struct {
        Uint32 vert_offset, vert_size;
        Uint32 idx_offset,  idx_size;
        SDL_GPUBuffer *vbuf, *ibuf;
    } regions[UPLOAD_MAX_REGIONS];
    int count;
    Uint32 total_bytes;
} UploadBatch;




/* Simpler: just use a flat staging buffer, copy on add, commit once */
#define STAGING_SIZE (8 * 1024 * 1024)  /* 8 MB staging area */
static Uint8 s_staging[STAGING_SIZE];
static Uint32 s_staging_used;

static void upload_batch_reset(void) { s_staging_used = 0; }

static bool batch_add(UploadBatch *b, SDL_GPUDevice *gpu,
                      const void *vdata, int vbytes,
                      const void *idata, int ibytes,
                      SDL_GPUBuffer **out_vbuf, SDL_GPUBuffer **out_ibuf) {
    Uint32 needed = (Uint32)(vbytes + ibytes);
    if (b->count >= UPLOAD_MAX_REGIONS) return false;
    if (s_staging_used + needed > STAGING_SIZE) return false;

    int i = b->count++;

    *out_vbuf = SDL_CreateGPUBuffer(gpu, &(SDL_GPUBufferCreateInfo){
        .usage = SDL_GPU_BUFFERUSAGE_VERTEX, .size = (Uint32)vbytes});
    *out_ibuf = SDL_CreateGPUBuffer(gpu, &(SDL_GPUBufferCreateInfo){
        .usage = SDL_GPU_BUFFERUSAGE_INDEX, .size = (Uint32)ibytes});

    b->regions[i].vbuf        = *out_vbuf;
    b->regions[i].ibuf        = *out_ibuf;
    b->regions[i].vert_offset = s_staging_used;
    b->regions[i].vert_size   = (Uint32)vbytes;
    memcpy(s_staging + s_staging_used, vdata, vbytes);
    s_staging_used           += (Uint32)vbytes;
    b->regions[i].idx_offset  = s_staging_used;
    b->regions[i].idx_size    = (Uint32)ibytes;
    memcpy(s_staging + s_staging_used, idata, ibytes);
    s_staging_used           += (Uint32)ibytes;
    b->total_bytes            = s_staging_used;
    return true;
}

/* Commit: ONE transfer buffer, ONE copy pass, ONE command buffer, ONE submit */
static void batch_commit(UploadBatch *b, SDL_GPUDevice *gpu) {
    if (b->count == 0 || b->total_bytes == 0) return;

    SDL_GPUTransferBuffer *tbuf = SDL_CreateGPUTransferBuffer(gpu,
        &(SDL_GPUTransferBufferCreateInfo){
            .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
            .size  = b->total_bytes,
        });
    void *mapped = SDL_MapGPUTransferBuffer(gpu, tbuf, false);
    memcpy(mapped, s_staging, b->total_bytes);
    SDL_UnmapGPUTransferBuffer(gpu, tbuf);

    SDL_GPUCommandBuffer *copy_cmd = SDL_AcquireGPUCommandBuffer(gpu);
    SDL_GPUCopyPass *cp = SDL_BeginGPUCopyPass(copy_cmd);

    for (int i = 0; i < b->count; i++) {
        SDL_UploadToGPUBuffer(cp,
            &(SDL_GPUTransferBufferLocation){
                .transfer_buffer = tbuf,
                .offset = b->regions[i].vert_offset,
            },
            &(SDL_GPUBufferRegion){
                .buffer = b->regions[i].vbuf,
                .size   = b->regions[i].vert_size,
            }, false);
        SDL_UploadToGPUBuffer(cp,
            &(SDL_GPUTransferBufferLocation){
                .transfer_buffer = tbuf,
                .offset = b->regions[i].idx_offset,
            },
            &(SDL_GPUBufferRegion){
                .buffer = b->regions[i].ibuf,
                .size   = b->regions[i].idx_size,
            }, false);
    }

    SDL_EndGPUCopyPass(cp);
    SDL_SubmitGPUCommandBuffer(copy_cmd);
    SDL_ReleaseGPUTransferBuffer(gpu, tbuf);
}

/* ---- Pre-build text vertex data for one TEXT command ---- */
typedef struct {
    VertexTex       verts[256 * 4];
    Uint16          idx[256 * 6];
    int             quad_count;
    /* Per-quad glyph texture reference */
    SDL_GPUTexture *glyph_textures[256];
} TextBatch;

static void build_text_batch(ClayGPUCtx *ctx, TextBatch *tb,
                             Clay_BoundingBox bb, Clay_TextRenderData *td, float scale) {
    const char *str = td->stringContents.chars;
    int len = td->stringContents.length;
    tb->quad_count = 0;
    if (!str || len <= 0) return;

    int font_ascent = TTF_GetFontAscent(ctx->font);
    float cx = bb.x * scale;
    float cy = bb.y * scale;

    float nr = (td->textColor.r / 255.f) * (td->textColor.a / 255.f);
    float ng = (td->textColor.g / 255.f) * (td->textColor.a / 255.f);
    float nb = (td->textColor.b / 255.f) * (td->textColor.a / 255.f);
    float na = td->textColor.a / 255.f;

    uint32_t prev_cp = 0;
    for (int i = 0; i < len && tb->quad_count < 256; ) {
        uint32_t cp;
        uint8_t c = (uint8_t)str[i];
        if (c < 0x80) { cp = c; i += 1; }
        else if (c < 0xE0) { cp = (c & 0x1F) << 6 | (str[i+1] & 0x3F); i += 2; }
        else if (c < 0xF0) { cp = (c & 0x0F) << 12 | (str[i+1] & 0x3F) << 6 | (str[i+2] & 0x3F); i += 3; }
        else { cp = (c & 0x07) << 18 | (str[i+1] & 0x3F) << 12 | (str[i+2] & 0x3F) << 6 | (str[i+3] & 0x3F); i += 4; }

        if (prev_cp) {
            int kern = 0;
            TTF_GetGlyphKerning(ctx->font, prev_cp, cp, &kern);
            cx += kern * scale;
        }
        prev_cp = cp;

        const GlyphEntry *ge = GlyphCache_get(&ctx->glyph_cache, ctx->font, cp, (uint16_t)ctx->font_size);
        if (!ge) { cx += 8 * scale; continue; }

        float gx = cx + ge->bearing_x * scale;
        float gy = cy + (font_ascent - ge->bearing_y) * scale;
        float gw = ge->w * scale;
        float gh = ge->h * scale;

        int q = tb->quad_count;
        int vi = q * 4, ii = q * 6;
        tb->verts[vi+0] = (VertexTex){gx,    gy,    0,0, nr,ng,nb,na};
        tb->verts[vi+1] = (VertexTex){gx+gw, gy,    1,0, nr,ng,nb,na};
        tb->verts[vi+2] = (VertexTex){gx+gw, gy+gh, 1,1, nr,ng,nb,na};
        tb->verts[vi+3] = (VertexTex){gx,    gy+gh, 0,1, nr,ng,nb,na};
        tb->idx[ii+0]=vi; tb->idx[ii+1]=vi+1; tb->idx[ii+2]=vi+2;
        tb->idx[ii+3]=vi; tb->idx[ii+4]=vi+2; tb->idx[ii+5]=vi+3;
        tb->glyph_textures[q] = ge->tex;

        cx += ge->advance * scale;
        if (td->letterSpacing) cx += td->letterSpacing * scale;
        tb->quad_count++;
    }
}

/* ---- Main render function ---- */
void ClayGPUCtx_render(ClayGPUCtx *ctx, Clay_RenderCommandArray cmds) {
    int pw, ph;
    SDL_GetWindowSizeInPixels(ctx->window, &pw, &ph);
    float scale = ctx->dpi_scale;
    Mat4 proj = ortho_proj((float)pw, (float)ph);

    /* ====== Phase 1: Collect geometry ====== */

    /* Rectangles */
    s_rect_count = 0;
    for (int i = 0; i < cmds.length; i++) {
        Clay_RenderCommand *cmd = Clay_RenderCommandArray_Get(&cmds, i);
        if (cmd->commandType == CLAY_RENDER_COMMAND_TYPE_RECTANGLE) {
            Clay_BoundingBox bb = cmd->boundingBox;
            push_rect(bb.x, bb.y, bb.width, bb.height,
                      cmd->renderData.rectangle.backgroundColor,
                      cmd->renderData.rectangle.cornerRadius, scale);
        }
    }

    /* Text batches — count how many TEXT commands exist */
    int text_cmd_count = 0;
    for (int i = 0; i < cmds.length; i++) {
        Clay_RenderCommand *cmd = Clay_RenderCommandArray_Get(&cmds, i);
        if (cmd->commandType == CLAY_RENDER_COMMAND_TYPE_TEXT) text_cmd_count++;
    }

    /* Build text batches (stack allocated, max 64 text commands per frame) */
    #define MAX_TEXT_CMDS 64
    TextBatch text_batches[MAX_TEXT_CMDS];
    int text_batch_indices[MAX_TEXT_CMDS]; /* which cmd index each batch corresponds to */
    int tb_count = 0;
    for (int i = 0; i < cmds.length && tb_count < MAX_TEXT_CMDS; i++) {
        Clay_RenderCommand *cmd = Clay_RenderCommandArray_Get(&cmds, i);
        if (cmd->commandType == CLAY_RENDER_COMMAND_TYPE_TEXT) {
            build_text_batch(ctx, &text_batches[tb_count], cmd->boundingBox, &cmd->renderData.text, scale);
            text_batch_indices[tb_count] = i;
            tb_count++;
        }
    }

    /* ====== Phase 2: Upload all GPU buffers — ONE copy pass, ONE submit ====== */
    UploadBatch batch;
    batch.count = 0;
    batch.total_bytes = 0;
    upload_batch_reset();

    SDL_GPUBuffer *rect_vbuf = NULL, *rect_ibuf = NULL;
    if (s_rect_count > 0) {
        batch_add(&batch, ctx->gpu,
            s_rect_verts, s_rect_count * 4 * (int)sizeof(VertexRect),
            s_rect_idx,   s_rect_count * 6 * (int)sizeof(Uint16),
            &rect_vbuf, &rect_ibuf);
    }

    /* Upload text buffers */
    SDL_GPUBuffer *text_vbufs[MAX_TEXT_CMDS];
    SDL_GPUBuffer *text_ibufs[MAX_TEXT_CMDS];
    for (int t = 0; t < tb_count; t++) {
        TextBatch *tb = &text_batches[t];
        if (tb->quad_count > 0) {
            batch_add(&batch, ctx->gpu,
                tb->verts, tb->quad_count * 4 * (int)sizeof(VertexTex),
                tb->idx,   tb->quad_count * 6 * (int)sizeof(Uint16),
                &text_vbufs[t], &text_ibufs[t]);
        } else {
            text_vbufs[t] = NULL;
            text_ibufs[t] = NULL;
        }
    }

    /* Single commit: one transfer buffer, one copy pass, one command buffer */
    batch_commit(&batch, ctx->gpu);

    /* ====== Phase 3: Render pass ====== */

    SDL_GPUCommandBuffer *cmdbuf = SDL_AcquireGPUCommandBuffer(ctx->gpu);
    if (!cmdbuf) return;

    SDL_GPUTexture *swapchain = NULL;
    SDL_WaitAndAcquireGPUSwapchainTexture(cmdbuf, ctx->window, &swapchain, NULL, NULL);
    if (!swapchain) { SDL_SubmitGPUCommandBuffer(cmdbuf); return; }

    SDL_GPUColorTargetInfo ct = {
        .texture = swapchain,
        .clear_color = {0.08f, 0.08f, 0.12f, 1.0f},
        .load_op = SDL_GPU_LOADOP_CLEAR,
        .store_op = SDL_GPU_STOREOP_STORE,
    };
    SDL_GPURenderPass *rp = SDL_BeginGPURenderPass(cmdbuf, &ct, 1, NULL);

    /* Draw rectangles */
    if (s_rect_count > 0 && rect_vbuf && rect_ibuf) {
        SDL_BindGPUGraphicsPipeline(rp, ctx->pipeline_rect);
        SDL_PushGPUVertexUniformData(cmdbuf, 0, &proj, sizeof(proj));
        SDL_BindGPUVertexBuffers(rp, 0, &(SDL_GPUBufferBinding){.buffer=rect_vbuf}, 1);
        SDL_BindGPUIndexBuffer(rp, &(SDL_GPUBufferBinding){.buffer=rect_ibuf}, SDL_GPU_INDEXELEMENTSIZE_16BIT);
        SDL_DrawGPUIndexedPrimitives(rp, (Uint32)(s_rect_count * 6), 1, 0, 0, 0);
    }

    /* Draw text and handle scissor — iterate commands in order */
    SDL_BindGPUGraphicsPipeline(rp, ctx->pipeline_text);
    SDL_PushGPUVertexUniformData(cmdbuf, 0, &proj, sizeof(proj));
    int tb_idx = 0;

    for (int i = 0; i < cmds.length; i++) {
        Clay_RenderCommand *cmd = Clay_RenderCommandArray_Get(&cmds, i);
        switch (cmd->commandType) {
        case CLAY_RENDER_COMMAND_TYPE_TEXT:
            if (tb_idx < tb_count && text_batch_indices[tb_idx] == i) {
                TextBatch *tb = &text_batches[tb_idx];
                if (tb->quad_count > 0 && text_vbufs[tb_idx]) {
                    SDL_BindGPUVertexBuffers(rp, 0, &(SDL_GPUBufferBinding){.buffer=text_vbufs[tb_idx]}, 1);
                    SDL_BindGPUIndexBuffer(rp, &(SDL_GPUBufferBinding){.buffer=text_ibufs[tb_idx]}, SDL_GPU_INDEXELEMENTSIZE_16BIT);
                    /* Draw glyph by glyph (each has its own texture) */
                    for (int q = 0; q < tb->quad_count; q++) {
                        SDL_BindGPUFragmentSamplers(rp, 0,
                            &(SDL_GPUTextureSamplerBinding){.texture = tb->glyph_textures[q], .sampler = ctx->sampler_linear}, 1);
                        SDL_DrawGPUIndexedPrimitives(rp, 6, 1, (Uint32)(q * 6), 0, 0);
                    }
                }
                tb_idx++;
            }
            break;
        case CLAY_RENDER_COMMAND_TYPE_SCISSOR_START: {
            Clay_BoundingBox sb = cmd->boundingBox;
            SDL_Rect sc = {(int)(sb.x*scale), (int)(sb.y*scale), (int)(sb.width*scale), (int)(sb.height*scale)};
            SDL_SetGPUScissor(rp, &sc);
            break;
        }
        case CLAY_RENDER_COMMAND_TYPE_SCISSOR_END:
            SDL_SetGPUScissor(rp, &(SDL_Rect){0, 0, pw, ph});
            break;
        default: break;
        }
    }

    SDL_EndGPURenderPass(rp);
    SDL_SubmitGPUCommandBuffer(cmdbuf);

    /* ====== Phase 4: Release GPU buffers ====== */
    if (rect_vbuf) SDL_ReleaseGPUBuffer(ctx->gpu, rect_vbuf);
    if (rect_ibuf) SDL_ReleaseGPUBuffer(ctx->gpu, rect_ibuf);
    for (int t = 0; t < tb_count; t++) {
        if (text_vbufs[t]) SDL_ReleaseGPUBuffer(ctx->gpu, text_vbufs[t]);
        if (text_ibufs[t]) SDL_ReleaseGPUBuffer(ctx->gpu, text_ibufs[t]);
    }
}

/* ---- Destroy ---- */
void ClayGPUCtx_destroy(ClayGPUCtx *ctx) {
    GlyphCache_destroy(&ctx->glyph_cache);
    if (ctx->pipeline_rect) SDL_ReleaseGPUGraphicsPipeline(ctx->gpu, ctx->pipeline_rect);
    if (ctx->pipeline_text) SDL_ReleaseGPUGraphicsPipeline(ctx->gpu, ctx->pipeline_text);
    if (ctx->sampler_linear) SDL_ReleaseGPUSampler(ctx->gpu, ctx->sampler_linear);
    if (ctx->font) TTF_CloseFont(ctx->font);
    TTF_Quit();
    if (ctx->gpu && ctx->window) SDL_ReleaseWindowFromGPUDevice(ctx->gpu, ctx->window);
    if (ctx->gpu) SDL_DestroyGPUDevice(ctx->gpu);
    if (ctx->window) SDL_DestroyWindow(ctx->window);
    if (ctx->clay_mem) SDL_free(ctx->clay_mem);
    SDL_Quit();
}
