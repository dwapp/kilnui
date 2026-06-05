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
#include <stdio.h>
#include <string.h>

#ifdef KILNUI_HAS_FONTCONFIG
#include <fontconfig/fontconfig.h>
#endif

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

#ifdef KILNUI_HAS_FONTCONFIG
/* Use fontconfig to find a font file by family name.
 * Returns a static string owned by fontconfig (valid until FcFini). */
static const char *fc_find_font(const char *family)
{
    FcPattern *pat = FcNameParse((const FcChar8 *)family);
    if (!pat) return NULL;
    FcConfigSubstitute(NULL, pat, FcMatchPattern);
    FcDefaultSubstitute(pat);
    FcResult result;
    FcPattern *match = FcFontMatch(NULL, pat, &result);
    FcPatternDestroy(pat);
    if (!match) return NULL;
    FcChar8 *file = NULL;
    if (FcPatternGetString(match, FC_FILE, 0, &file) != FcResultMatch || !file) {
        FcPatternDestroy(match);
        return NULL;
    }
    /* file points into match; we need to copy it before destroying match */
    static char s_fc_path[512];
    snprintf(s_fc_path, sizeof(s_fc_path), "%s", (const char *)file);
    FcPatternDestroy(match);
    return s_fc_path;
}
#endif

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

static SDL_GPUGraphicsPipeline *create_shadow_pipeline(SDL_GPUDevice *dev, SDL_Window *win)
{
    SDL_GPUShader *vert = load_spv(dev, "shaders/rect.vert.spv", SDL_GPU_SHADERSTAGE_VERTEX,   1, 0);
    SDL_GPUShader *frag = load_spv(dev, "shaders/shadow.frag.spv", SDL_GPU_SHADERSTAGE_FRAGMENT, 1, 0);
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

static SDL_GPUGraphicsPipeline *create_border_pipeline(SDL_GPUDevice *dev, SDL_Window *win)
{
    SDL_GPUShader *vert = load_spv(dev, "shaders/rect.vert.spv", SDL_GPU_SHADERSTAGE_VERTEX,   1, 0);
    SDL_GPUShader *frag = load_spv(dev, "shaders/border.frag.spv", SDL_GPU_SHADERSTAGE_FRAGMENT, 1, 0);
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

/* ---- Clay MeasureText callback ---- */
/* Single global pointer: Clay's measure callback has no user-data path
 * that survives a context switch, so we store it here. Multi-window
 * support would require a thread-local or a Clay API extension. */
static KilnUI *g_measure_ctx = NULL;

/* FNV-1a hash for string slices */
static uint32_t hash_string(Clay_StringSlice str, int font_size, int letter_spacing, int line_height) {
    uint32_t hash = 2166136261u;
    for (int i = 0; i < str.length; i++) {
        hash ^= (uint8_t)str.chars[i];
        hash *= 16777619u;
    }
    hash ^= (uint32_t)font_size;
    hash *= 16777619u;
    hash ^= (uint32_t)letter_spacing;
    hash *= 16777619u;
    hash ^= (uint32_t)line_height;
    hash *= 16777619u;
    return hash;
}

/* 2-way set-associative measure cache to avoid hash collisions.
 * Each slot has 2 entries; a collision in one way is evicted to the other.
 * Also stores a text prefix for exact matching on rare hash collisions. */
#define MEASURE_CACHE_SETS   512
#define MEASURE_CACHE_WAYS     2
#define MEASURE_CACHE_PREFIX  12

typedef struct {
    uint32_t hash;
    uint16_t w, h;
    uint16_t font_size;
    uint8_t  prefix_len;
    char     prefix[MEASURE_CACHE_PREFIX];
} MeasureCacheEntry;

typedef struct {
    MeasureCacheEntry entries[MEASURE_CACHE_WAYS];
} MeasureCacheSet;

static MeasureCacheSet s_measure_cache[MEASURE_CACHE_SETS];

static bool measure_entry_matches(const MeasureCacheEntry *e, uint32_t hash,
                                  Clay_StringSlice text, int font_size)
{
    if (e->hash != hash || e->font_size != (uint16_t)font_size)
        return false;
    int plen = e->prefix_len;
    if (plen > text.length) plen = text.length;
    if (plen > MEASURE_CACHE_PREFIX) plen = MEASURE_CACHE_PREFIX;
    return memcmp(e->prefix, text.chars, (size_t)plen) == 0;
}

static void measure_entry_store(MeasureCacheEntry *e, uint32_t hash,
                                Clay_StringSlice text, int font_size,
                                int w, int h)
{
    e->hash = hash;
    e->font_size = (uint16_t)font_size;
    e->w = (uint16_t)w;
    e->h = (uint16_t)h;
    int plen = text.length < MEASURE_CACHE_PREFIX ? text.length : MEASURE_CACHE_PREFIX;
    memcpy(e->prefix, text.chars, (size_t)plen);
    e->prefix_len = (uint8_t)plen;
}

static Clay_Dimensions measure_text_cb(Clay_StringSlice text,
                                       Clay_TextElementConfig *config,
                                       void *userData)
{
    (void)userData;
    KilnUI *ctx = g_measure_ctx;
    if (!ctx || !ctx->font || !text.chars || text.length <= 0)
        return (Clay_Dimensions){ 0, 0 };

    int req_size = (config && config->fontSize > 0) ? config->fontSize : ctx->font_size;
    int letter_spacing = config ? config->letterSpacing : 0;
    int line_height = config ? config->lineHeight : 0;

    uint32_t hash = hash_string(text, req_size, letter_spacing, line_height);
    int set_idx = hash % MEASURE_CACHE_SETS;
    MeasureCacheSet *set = &s_measure_cache[set_idx];

    /* Check both ways for a hit */
    for (int w = 0; w < MEASURE_CACHE_WAYS; w++) {
        if (measure_entry_matches(&set->entries[w], hash, text, req_size)) {
            int rw = set->entries[w].w, rh = set->entries[w].h;
            if (letter_spacing && text.length > 1)
                rw += letter_spacing * ((int)text.length - 1);
            if (line_height > 0 && line_height > rh)
                rh = line_height;
            return (Clay_Dimensions){ (float)rw, (float)rh };
        }
    }

    KilnUI_set_font_size(ctx, (float)req_size);

    int w = 0, h = 0;
    TTF_GetStringSize(ctx->font, text.chars, (size_t)text.length, &w, &h);
    if (letter_spacing && text.length > 1)
        w += letter_spacing * ((int)text.length - 1);
    if (line_height > 0 && line_height > (uint16_t)h)
        h = line_height;

    /* LRU: evict way 1, promote way 0 → way 1, insert new in way 0 */
    set->entries[1] = set->entries[0];
    measure_entry_store(&set->entries[0], hash, text, req_size, w, h);

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

    /* Load fallback fonts for icons/symbols if the primary font doesn't support them. */
    static const char *fallback_families[] = {
        "DejaVu Sans",
        "Noto Sans Symbols",
        "Noto Sans Symbols 2",
        NULL
    };
    static const char *fallback_hardcoded[] = {
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
        "/usr/share/fonts/truetype/noto/NotoSansSymbols-Regular.ttf",
        "/usr/share/fonts/truetype/noto/NotoSansSymbols2-Regular.ttf",
        NULL
    };
    ctx->fallback_font_count = 0;
#ifdef KILNUI_HAS_FONTCONFIG
    bool fc_ok = FcInit();
#endif
    for (int i = 0; fallback_families[i]; i++) {
        const char *path = NULL;
#ifdef KILNUI_HAS_FONTCONFIG
        if (fc_ok) path = fc_find_font(fallback_families[i]);
#endif
        if (!path) path = fallback_hardcoded[i];
        if (!path) continue;
        TTF_Font *fallback = TTF_OpenFont(path, (float)font_size);
        if (!fallback) continue;
        if (TTF_AddFallbackFont(ctx->font, fallback)) {
            if (ctx->fallback_font_count < MAX_FALLBACK_FONTS) {
                ctx->fallback_fonts[ctx->fallback_font_count++] = fallback;
            } else {
                TTF_CloseFont(fallback);
            }
        } else {
            SDL_Log("Failed to add fallback font %s: %s", path, SDL_GetError());
            TTF_CloseFont(fallback);
        }
    }

    GlyphCache_init(&ctx->glyph_cache, 512, ctx->gpu);

    ctx->pipeline_rect = create_rect_pipeline(ctx->gpu, ctx->window);
    ctx->pipeline_text = create_text_pipeline(ctx->gpu, ctx->window);
    ctx->pipeline_shadow = create_shadow_pipeline(ctx->gpu, ctx->window);
    ctx->pipeline_border = create_border_pipeline(ctx->gpu, ctx->window);
    if (!ctx->pipeline_rect || !ctx->pipeline_text || !ctx->pipeline_shadow || !ctx->pipeline_border) {
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
    if (ctx->pipeline_shadow) SDL_ReleaseGPUGraphicsPipeline(ctx->gpu, ctx->pipeline_shadow);
    if (ctx->pipeline_border) SDL_ReleaseGPUGraphicsPipeline(ctx->gpu, ctx->pipeline_border);
    if (ctx->sampler_linear) SDL_ReleaseGPUSampler(ctx->gpu, ctx->sampler_linear);
    if (ctx->font)           TTF_CloseFont(ctx->font);
    for (int i = 0; i < ctx->fallback_font_count; i++) {
        if (ctx->fallback_fonts[i]) {
            TTF_CloseFont(ctx->fallback_fonts[i]);
        }
    }
    ctx->fallback_font_count = 0;
    TTF_Quit();
    if (ctx->gpu && ctx->window)
        SDL_ReleaseWindowFromGPUDevice(ctx->gpu, ctx->window);
    if (ctx->gpu)      SDL_DestroyGPUDevice(ctx->gpu);
    if (ctx->window)   SDL_DestroyWindow(ctx->window);
    if (ctx->clay_mem) SDL_free(ctx->clay_mem);
    SDL_Quit();
}

void KilnUI_set_font_size(KilnUI *ctx, float ptsize)
{
    if (!ctx || !ctx->font) return;
    TTF_SetFontSize(ctx->font, ptsize);
    for (int i = 0; i < ctx->fallback_font_count; i++) {
        if (ctx->fallback_fonts[i]) {
            TTF_SetFontSize(ctx->fallback_fonts[i], ptsize);
        }
    }
}
