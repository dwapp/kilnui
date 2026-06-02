/* kilnui_render.c — KilnUI hot-path rendering.
 *
 * Responsibilities:
 *   - Orthographic projection math
 *   - Persistent GPU buffer management (grow-only)
 *   - Rectangle geometry batching (SDF rounded-rect vertices)
 *   - Text geometry batching (per-glyph quads from the glyph cache)
 *   - Single-pass GPU upload (one transfer buffer, one copy pass)
 *   - Render pass: iterates Clay commands in order to preserve Z-layering
 *
 * Context lifecycle lives in kilnui.c.
 */

#include "kilnui.h"
#include <math.h>

/* ---- Orthographic projection (column-major, Y-down) ---- */
typedef struct { float m[4][4]; } Mat4;

static Mat4 ortho_proj(float w, float h)
{
    Mat4 m = { 0 };
    m.m[0][0] =  2.0f / w;
    m.m[1][1] = -2.0f / h;
    m.m[2][2] = -1.0f;
    m.m[3][0] = -1.0f;
    m.m[3][1] =  1.0f;
    m.m[3][3] =  1.0f;
    return m;
}

/* ---- Persistent GPU buffer helpers ---- */
/* Grows *buf to at least `needed` bytes; no-op if already large enough.
 * SDL3 defers GPU-buffer deletion, so no idle wait is required. */
static void ensure_gpu_buffer(SDL_GPUDevice *gpu, SDL_GPUBuffer **buf, uint32_t *cap,
                              uint32_t needed, SDL_GPUBufferUsageFlags usage)
{
    if (*cap >= needed) return;
    if (*buf) SDL_ReleaseGPUBuffer(gpu, *buf);
    uint32_t nc = needed + needed / 2;   /* 1.5x over-alloc to amortise growth */
    *buf = SDL_CreateGPUBuffer(gpu, &(SDL_GPUBufferCreateInfo){ .usage = usage, .size = nc });
    if (!*buf) { SDL_Log("ensure_gpu_buffer: %s", SDL_GetError()); *buf = NULL; *cap = 0; return; }
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
    if (!*tbuf) { SDL_Log("ensure_transfer_buffer: %s", SDL_GetError()); *tbuf = NULL; *cap = 0; return; }
    *cap = nc;
}

/* ---- Rectangle batch (static storage shared across frames) ---- */
static VertexRect s_rect_verts[MAX_RECTS * 4];
static Uint16     s_rect_idx  [MAX_RECTS * 6];
static int        s_rect_count;

static uint32_t hash_combine(uint32_t h, uint32_t v) {
    h ^= v;
    h *= 16777619u;
    return h;
}

static inline uint32_t float_bits(float f) {
    uint32_t u;
    memcpy(&u, &f, sizeof(u));
    return u;
}

static uint32_t hash_rect_cmd(Clay_BoundingBox bb, Clay_Color c, Clay_CornerRadius cr, float scale) {
    uint32_t h = 2166136261u;
    h = hash_combine(h, float_bits(bb.x));
    h = hash_combine(h, float_bits(bb.y));
    h = hash_combine(h, float_bits(bb.width));
    h = hash_combine(h, float_bits(bb.height));
    h = hash_combine(h, float_bits(c.r));
    h = hash_combine(h, float_bits(c.g));
    h = hash_combine(h, float_bits(c.b));
    h = hash_combine(h, float_bits(c.a));
    h = hash_combine(h, float_bits(cr.topLeft));
    h = hash_combine(h, float_bits(cr.topRight));
    h = hash_combine(h, float_bits(cr.bottomLeft));
    h = hash_combine(h, float_bits(cr.bottomRight));
    h = hash_combine(h, float_bits(scale));
    return h;
}

typedef struct {
    uint32_t hash;
    bool valid;
} VertexCacheEntry;

static VertexCacheEntry s_vcache[MAX_RECTS];
static bool s_idx_initialized = false;

static void push_rect_at(int ri, float x, float y, float w, float h,
                         Clay_Color c, Clay_CornerRadius cr, float scale)
{
    if (ri >= MAX_RECTS) return;
    x *= scale; y *= scale; w *= scale; h *= scale;

    /* Premultiplied alpha */
    float nr = (c.r / 255.f) * c.a / 255.f, ng = (c.g / 255.f) * c.a / 255.f;
    float nb = (c.b / 255.f) * c.a / 255.f, na = c.a / 255.f;

    float max_r = fminf(w, h) * 0.5f;
    float rtl = fminf(cr.topLeft    * scale, max_r);
    float rtr = fminf(cr.topRight   * scale, max_r);
    float rbl = fminf(cr.bottomLeft * scale, max_r);
    float rbr = fminf(cr.bottomRight* scale, max_r);

    int vi = ri * 4;
    s_rect_verts[vi+0] = (VertexRect){ x,   y,   0, 0, w, h, rtl, rtr, rbl, rbr, nr, ng, nb, na };
    s_rect_verts[vi+1] = (VertexRect){ x+w, y,   w, 0, w, h, rtl, rtr, rbl, rbr, nr, ng, nb, na };
    s_rect_verts[vi+2] = (VertexRect){ x+w, y+h, w, h, w, h, rtl, rtr, rbl, rbr, nr, ng, nb, na };
    s_rect_verts[vi+3] = (VertexRect){ x,   y+h, 0, h, w, h, rtl, rtr, rbl, rbr, nr, ng, nb, na };
}

/* Track last TTF font size to avoid redundant TTF_SetFontSize calls. */
static int s_last_phys_size = 0;

static void set_font_size(TTF_Font *font, int phys_size)
{
    if (phys_size == s_last_phys_size) return;
    TTF_SetFontSize(font, (float)phys_size);
    s_last_phys_size = phys_size;
}

/* ---- Text batch (one per TEXT command, static to avoid stack bloat) ---- */
typedef struct
{
    VertexTex      verts[256 * 4];
    Uint16         idx  [256 * 6];
    int            quad_count;
    SDL_GPUTexture *glyph_textures[256];
} TextBatch;

static TextBatch s_text_batches[MAX_TEXT_CMDS];

static void build_text_batch(KilnUI *ctx, TextBatch *tb,
                             Clay_BoundingBox bb, Clay_TextRenderData *td, float scale)
{
    const char *str = td->stringContents.chars;
    int len         = td->stringContents.length;
    tb->quad_count  = 0;
    if (!str || len <= 0) return;

    int req_size = (td->fontSize > 0) ? td->fontSize : ctx->font_size;

    /* Rasterize glyphs at PHYSICAL pixel size for crisp HiDPI rendering.
     * Glyph metrics (w, h, bearing, advance) are then already in physical px. */
    int phys_size = (int)((float)req_size * scale + 0.5f);
    if (phys_size < 1) phys_size = 1;
    set_font_size(ctx->font, phys_size);

    float cx = bb.x * scale;                          /* logical → physical */
    float cy = bb.y * scale;

    float nr = (td->textColor.r / 255.f) * (td->textColor.a / 255.f);
    float ng = (td->textColor.g / 255.f) * (td->textColor.a / 255.f);
    float nb = (td->textColor.b / 255.f) * (td->textColor.a / 255.f);
    float na =  td->textColor.a / 255.f;

    uint32_t prev_cp = 0;
    for (int i = 0; i < len && tb->quad_count < 256;) {
        uint32_t cp;
        uint8_t  c = (uint8_t)str[i];

        /* Decode one UTF-8 code point with explicit bounds checking.
         * Clay_StringSlice is NOT null-terminated, so reading past `len`
         * yields garbage bytes (often \0) which would produce U+0000. */
        if (c < 0x80) {
            cp = c;
            i += 1;
        } else if (c < 0xE0) {
            if (i + 1 >= len) break;                          /* truncated 2-byte seq */
            cp = (uint32_t)(c & 0x1F) << 6
               | (uint32_t)((uint8_t)str[i+1] & 0x3F);
            i += 2;
        } else if (c < 0xF0) {
            if (i + 2 >= len) break;                          /* truncated 3-byte seq */
            cp = (uint32_t)(c & 0x0F) << 12
               | (uint32_t)((uint8_t)str[i+1] & 0x3F) << 6
               | (uint32_t)((uint8_t)str[i+2] & 0x3F);
            i += 3;
        } else {
            if (i + 3 >= len) break;                          /* truncated 4-byte seq */
            cp = (uint32_t)(c & 0x07) << 18
               | (uint32_t)((uint8_t)str[i+1] & 0x3F) << 12
               | (uint32_t)((uint8_t)str[i+2] & 0x3F) <<  6
               | (uint32_t)((uint8_t)str[i+3] & 0x3F);
            i += 4;
        }

        /* Skip null bytes and non-printable control characters.
         * U+0000 is the most common source of "Text has zero width" errors. */
        if (cp == 0 || cp < 0x20) continue;

        if (prev_cp) {
            int kern = 0;
            TTF_GetGlyphKerning(ctx->font, prev_cp, cp, &kern);
            cx += kern; /* kern from physical-size font, already in physical px */
        }
        prev_cp = cp;

        const GlyphEntry *ge = GlyphCache_get(&ctx->glyph_cache, ctx->font,
                                              cp, (uint16_t)phys_size);
        if (!ge) { cx += phys_size * 0.5f; continue; } /* fallback advance */

        float gx = cx;
        float gy = cy;
        float gw = ge->w;              /* glyph size already in physical px */
        float gh = ge->h;

        int q  = tb->quad_count;
        int vi = q * 4, ii = q * 6;
        tb->verts[vi+0] = (VertexTex){ gx,    gy,    0, 0, nr, ng, nb, na };
        tb->verts[vi+1] = (VertexTex){ gx+gw, gy,    1, 0, nr, ng, nb, na };
        tb->verts[vi+2] = (VertexTex){ gx+gw, gy+gh, 1, 1, nr, ng, nb, na };
        tb->verts[vi+3] = (VertexTex){ gx,    gy+gh, 0, 1, nr, ng, nb, na };
        tb->idx[ii+0]=vi; tb->idx[ii+1]=vi+1; tb->idx[ii+2]=vi+2;
        tb->idx[ii+3]=vi; tb->idx[ii+4]=vi+2; tb->idx[ii+5]=vi+3;
        tb->glyph_textures[q] = ge->tex;

        cx += ge->advance; /* advance in physical px, no scale */
        if (td->letterSpacing) cx += td->letterSpacing * scale; /* logical px → physical */
        tb->quad_count++;
    }
}

/* ---- Main render function ---- */
void KilnUI_render(KilnUI *ctx, Clay_RenderCommandArray cmds)
{
    /* Reset font-size cache so the first batch correctly sets the size
     * (measure_text_cb may have changed it since the last frame). */
    s_last_phys_size = 0;

    int pw, ph;
    SDL_GetWindowSizeInPixels(ctx->window, &pw, &ph);
    float scale = ctx->dpi_scale;
    Mat4  proj  = ortho_proj((float)pw, (float)ph);

    /* ===== Phase 1: Collect geometry, map each cmd to its slot ===== */
#define MAX_CMDS 2048
    static int cmd_to_rect[MAX_CMDS]; /* rect slot index, -1 if not RECT */
    static int cmd_to_text[MAX_CMDS]; /* text batch index, -1 if not TEXT */

    int n = cmds.length < MAX_CMDS ? cmds.length : MAX_CMDS;
    for (int i = 0; i < n; i++) { cmd_to_rect[i] = -1; cmd_to_text[i] = -1; }

    if (!s_idx_initialized) {
        for (int i = 0; i < MAX_RECTS; i++) {
            int vi = i * 4, ii = i * 6;
            s_rect_idx[ii+0]=vi; s_rect_idx[ii+1]=vi+1; s_rect_idx[ii+2]=vi+2;
            s_rect_idx[ii+3]=vi; s_rect_idx[ii+4]=vi+2; s_rect_idx[ii+5]=vi+3;
        }
        s_idx_initialized = true;
    }

    s_rect_count = 0;
    for (int i = 0; i < n; i++) {
        Clay_RenderCommand *cmd = Clay_RenderCommandArray_Get(&cmds, i);
        if (cmd->commandType == CLAY_RENDER_COMMAND_TYPE_RECTANGLE) {
            int ri = s_rect_count++;
            cmd_to_rect[i] = ri;
            Clay_BoundingBox bb = cmd->boundingBox;
            Clay_Color c = cmd->renderData.rectangle.backgroundColor;
            Clay_CornerRadius cr = cmd->renderData.rectangle.cornerRadius;

            uint32_t hash = hash_rect_cmd(bb, c, cr, scale);
            if (!s_vcache[ri].valid || s_vcache[ri].hash != hash) {
                push_rect_at(ri, bb.x, bb.y, bb.width, bb.height, c, cr, scale);
                s_vcache[ri].hash = hash;
                s_vcache[ri].valid = true;
            }
        } else if (cmd->commandType == CLAY_RENDER_COMMAND_TYPE_CUSTOM) {
            KilnUICustomHeader *header = (KilnUICustomHeader *)cmd->renderData.custom.customData;
            if (header && (header->type == KILNUI_CUSTOM_SHADOW || header->type == KILNUI_CUSTOM_BORDER)) {
                int ri = s_rect_count++;
                cmd_to_rect[i] = ri;
                Clay_BoundingBox bb = cmd->boundingBox;
                Clay_Color c = cmd->renderData.custom.backgroundColor;
                Clay_CornerRadius cr = cmd->renderData.custom.cornerRadius;

                uint32_t hash = hash_rect_cmd(bb, c, cr, scale) ^ header->type;
                if (!s_vcache[ri].valid || s_vcache[ri].hash != hash) {
                    push_rect_at(ri, bb.x, bb.y, bb.width, bb.height, c, cr, scale);
                    s_vcache[ri].hash = hash;
                    s_vcache[ri].valid = true;
                }
            }
        }
    }

    for (int ri = s_rect_count; ri < MAX_RECTS; ri++) {
        if (!s_vcache[ri].valid) break;
        s_vcache[ri].valid = false;
    }

    static uint32_t text_vtx_off[MAX_TEXT_CMDS]; /* vertex element offset per batch */
    static uint32_t text_idx_off[MAX_TEXT_CMDS]; /* index element offset per batch  */
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

    /* ===== Phase 2 + 3: single CommandBuffer — copy then render ===== */
    /* Acquiring the swapchain texture first then doing copy + render in one
     * submission halves the number of GPU round-trips vs two separate submits. */
    GlyphCache_flush_uploads(&ctx->glyph_cache);

    uint32_t rv_sz = (uint32_t)s_rect_count * 4 * sizeof(VertexRect);
    uint32_t ri_sz = (uint32_t)s_rect_count * 6 * sizeof(Uint16);
    uint32_t tv_sz = text_vtx_total * sizeof(VertexTex);
    uint32_t ti_sz = text_idx_total * sizeof(Uint16);
    uint32_t total = rv_sz + ri_sz + tv_sz + ti_sz;

    SDL_GPUCommandBuffer *cmdbuf = SDL_AcquireGPUCommandBuffer(ctx->gpu);
    if (!cmdbuf) return;

    if (total > 0) {
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
                SDL_memcpy(m + off, s_text_batches[t].verts, (size_t)qc * 4 * sizeof(VertexTex));
                off += (uint32_t)qc * 4 * sizeof(VertexTex);
            }
        }
        uint32_t off_ti = off;
        for (int t = 0; t < tb_count; t++) {
            int qc = s_text_batches[t].quad_count;
            if (qc > 0) {
                SDL_memcpy(m + off, s_text_batches[t].idx, (size_t)qc * 6 * sizeof(Uint16));
                off += (uint32_t)qc * 6 * sizeof(Uint16);
            }
        }
        SDL_UnmapGPUTransferBuffer(ctx->gpu, ctx->staging_tbuf);

        SDL_GPUCopyPass *cp = SDL_BeginGPUCopyPass(cmdbuf);
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
    }


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

    /* Switch pipeline only on type change — avoids redundant state changes. */
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

            int batch_count = 1;
            while (i + 1 < n) {
                Clay_RenderCommand *next_cmd = Clay_RenderCommandArray_Get(&cmds, i + 1);
                if (next_cmd->commandType == CLAY_RENDER_COMMAND_TYPE_RECTANGLE) {
                    batch_count++;
                    i++;
                } else {
                    break;
                }
            }

            SDL_DrawGPUIndexedPrimitives(rp, 6 * batch_count, 1, (Uint32)(ri * 6), 0, 0);
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
            /* vertex_offset rebases 0-based per-batch indices into the combined VBO.
             * No CPU-side index patching needed. */
            Uint32 idx_base = text_idx_off[ti];
            Sint32 vtx_base = (Sint32)text_vtx_off[ti];
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

        case CLAY_RENDER_COMMAND_TYPE_CUSTOM: {
            KilnUICustomHeader *header = (KilnUICustomHeader *)cmd->renderData.custom.customData;
            if (!header) break;
            if (header->type == KILNUI_CUSTOM_SHADOW || header->type == KILNUI_CUSTOM_BORDER) {
                int ri = cmd_to_rect[i];
                if (ri < 0 || !ctx->rect_vbuf) break;
                
                SDL_GPUGraphicsPipeline *target_pipe = (header->type == KILNUI_CUSTOM_SHADOW) ? ctx->pipeline_shadow : ctx->pipeline_border;
                
                if (active_pipe != target_pipe) {
                    SDL_BindGPUGraphicsPipeline(rp, target_pipe);
                    SDL_PushGPUVertexUniformData(cmdbuf, 0, &proj, sizeof(proj));
                    SDL_BindGPUVertexBuffers(rp, 0, &(SDL_GPUBufferBinding){ .buffer = ctx->rect_vbuf }, 1);
                    SDL_BindGPUIndexBuffer(rp, &(SDL_GPUBufferBinding){ .buffer = ctx->rect_ibuf }, SDL_GPU_INDEXELEMENTSIZE_16BIT);
                    active_pipe = target_pipe;
                }
                
                if (header->type == KILNUI_CUSTOM_SHADOW) {
                    KilnUICustomShadow *sh = (KilnUICustomShadow *)header;
                    float udata[4] = { sh->offset_x * scale, sh->offset_y * scale, sh->blur_radius * scale, sh->spread * scale };
                    SDL_PushGPUFragmentUniformData(cmdbuf, 0, udata, sizeof(udata));
                } else if (header->type == KILNUI_CUSTOM_BORDER) {
                    KilnUICustomBorder *b = (KilnUICustomBorder *)header;
                    float udata[8] = { 
                        b->width_top * scale, b->width_right * scale, b->width_bottom * scale, b->width_left * scale,
                        b->dash_length * scale, b->dash_gap * scale, 0.0f, 0.0f 
                    };
                    SDL_PushGPUFragmentUniformData(cmdbuf, 0, udata, sizeof(udata));
                }
                
                SDL_DrawGPUIndexedPrimitives(rp, 6, 1, (Uint32)(ri * 6), 0, 0);
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
