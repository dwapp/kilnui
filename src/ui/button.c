/* src/ui/button.c — Reusable Clay button component implementation. */

#include "button.h"
#include "ui_internal.h"
#include "../kilnui.h"

#include "design_system.h"

/* ---- Per-frame mouse state ---- */
bool  UI__mouse_down = false;
bool  UI__mouse_released = false;
float UI__mouse_x = 0.0f;
float UI__mouse_y = 0.0f;

void UI_SetPointerState(bool mouse_down, bool mouse_released, float mouse_x, float mouse_y)
{
    UI__mouse_down = mouse_down;
    UI__mouse_released = mouse_released;
    UI__mouse_x = mouse_x;
    UI__mouse_y = mouse_y;
}

static Clay_Color get_btn_bg(UIBtnVariant variant, int state) {
    if (state == 3) { /* Disabled */
        if (variant == UI_BTN_PRIMARY)   return ds_theme->surface1;
        if (variant == UI_BTN_SECONDARY) return ds_theme->crust;
        if (variant == UI_BTN_GHOST)     return (Clay_Color){0,0,0,0};
        if (variant == UI_BTN_DANGER)    return ds_theme->surface0;
    }
    if (variant == UI_BTN_PRIMARY) {
        if (state == 0) return ds_theme->accent;
        if (state == 1) return ds_theme->accent_alt;
        return ds_theme->accent; /* pressed */
    }
    if (variant == UI_BTN_SECONDARY) {
        if (state == 0) return ds_theme->surface0;
        if (state == 1) return ds_theme->surface1;
        return ds_theme->base; /* pressed */
    }
    if (variant == UI_BTN_GHOST) {
        if (state == 0) return (Clay_Color){0,0,0,0};
        if (state == 1) return ds_theme->surface0;
        return ds_theme->surface1; /* pressed */
    }
    if (variant == UI_BTN_DANGER) {
        Clay_Color c = ds_theme->error;
        if (state == 1) { c.a = 200; return c; }
        if (state == 2) { c.a = 255; return c; }
        return c;
    }
    return (Clay_Color){0};
}

static Clay_Color get_btn_fg(UIBtnVariant variant, bool disabled) {
    if (disabled) return ds_theme->muted;
    if (variant == UI_BTN_PRIMARY || variant == UI_BTN_DANGER) {
        /* Contrast text for filled buttons (using base/crust usually) */
        return ds_theme->base; 
    }
    if (variant == UI_BTN_SECONDARY) return ds_theme->text;
    if (variant == UI_BTN_GHOST) return ds_theme->subtext;
    return ds_theme->text;
}

/* ---- Size metrics ---- */
typedef struct
{
    int h;
    int px;
    int py;
    int fs;
    int radius;
} SizeInfo;
static const SizeInfo SIZES[3] = {
    /*SM*/ { 28, 12, 4, 12, 4 },
    /*MD*/ { 36, 16, 6, 14, 6 },
    /*LG*/ { 48, 22, 8, 16, 8 },
};

/* ---- UI_Button ---- */
bool UI_Button(int uid, const char *label,
               UIBtnVariant variant, UIBtnSize size, bool disabled)
{
    Clay_ElementId id = Clay_GetElementIdWithIndex(CLAY_STRING("UIBtn"), uid);
    bool hovered = !disabled && Clay_PointerOver(id);
    bool pressed = hovered && UI__mouse_down;
    bool clicked = hovered && UI__mouse_released && !disabled;

    /* Pick state index */
    int state = 0;
    if (disabled)
        state = 3;
    else if (pressed)
        state = 2;
    else if (hovered)
        state = 1;

    const SizeInfo *sz = &SIZES[size];
    Clay_Color bg = get_btn_bg(variant, state);
    Clay_Color fg = get_btn_fg(variant, disabled);

    Clay_String lbl = UI__str(label);
    static KilnUICustomShadow shadow_pool[1024];
    static unsigned int shadow_pool_idx = 0;
    
    KilnUICustomShadow *shadow_data = &shadow_pool[(shadow_pool_idx++) % 1024];
    shadow_data->type = KILNUI_CUSTOM_SHADOW;
    shadow_data->offset_x = 0;
    shadow_data->offset_y = 2;
    shadow_data->blur_radius = 6;
    shadow_data->spread = 0;
    
    static KilnUICustomBorder border_pool[1024];
    static unsigned int border_pool_idx = 0;
    KilnUICustomBorder *border_data = &border_pool[(border_pool_idx++) % 1024];
    border_data->type = KILNUI_CUSTOM_BORDER;
    border_data->width_top = border_data->width_right = border_data->width_bottom = border_data->width_left = 1.0f;
    border_data->dash_length = 0;
    border_data->dash_gap = 0;
    
    // Animate shadow on press/hover
    if (pressed) {
        shadow_data->offset_y = 0;
        shadow_data->blur_radius = 2;
        border_data->width_top = border_data->width_right = border_data->width_bottom = border_data->width_left = 2.0f;
    } else if (hovered) {
        shadow_data->offset_y = 4;
        shadow_data->blur_radius = 12;
    }

    CLAY(id, {
                 .layout = {
                     .sizing = {
                         CLAY_SIZING_FIT(sz->px * 2 + 40), /* min-width */
                         CLAY_SIZING_FIXED((float)sz->h),
                     },
                     .padding = { (uint16_t)sz->px, (uint16_t)sz->px, (uint16_t)sz->py, (uint16_t)sz->py },
                     .childAlignment = { CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER },
                 },
                 .backgroundColor = bg,
                 .cornerRadius = CLAY_CORNER_RADIUS((float)sz->radius),
             })
    {
        if (!disabled && variant == UI_BTN_PRIMARY) {
            Clay_Color shadow_color = bg; // use background color as tint
            shadow_color.a = 100;         // make it semi-transparent
            CLAY(CLAY_ID_LOCAL("shadow"), {
                .floating = {
                    .attachPoints = { .element = CLAY_ATTACH_POINT_CENTER_CENTER, .parent = CLAY_ATTACH_POINT_CENTER_CENTER },
                    .zIndex = -1,
                    .attachTo = CLAY_ATTACH_TO_PARENT
                },
                .layout = { .sizing = { CLAY_SIZING_GROW(), CLAY_SIZING_GROW() } },
                .custom = { .customData = shadow_data },
                .backgroundColor = shadow_color,
                .cornerRadius = CLAY_CORNER_RADIUS((float)sz->radius)
            }) {}
        }
        
        if (variant == UI_BTN_SECONDARY) {
            Clay_Color border_color = ds_theme->surface2;
            CLAY(CLAY_ID_LOCAL("border"), {
                .floating = {
                    .attachPoints = { .element = CLAY_ATTACH_POINT_CENTER_CENTER, .parent = CLAY_ATTACH_POINT_CENTER_CENTER },
                    .zIndex = 0,
                    .attachTo = CLAY_ATTACH_TO_PARENT
                },
                .layout = { .sizing = { CLAY_SIZING_GROW(), CLAY_SIZING_GROW() } },
                .custom = { .customData = border_data },
                .backgroundColor = border_color,
                .cornerRadius = CLAY_CORNER_RADIUS((float)sz->radius)
            }) {}
        }

        CLAY_TEXT(lbl, {
                           .textColor = fg,
                           .fontSize = (uint16_t)sz->fs,
                       });
    }

    return clicked;
}

bool UI_IconButton(int uid, const char *icon, int size,
                   UIBtnVariant variant, bool disabled)
{
    Clay_ElementId id = Clay_GetElementIdWithIndex(CLAY_STRING("UIIconBtn"), uid);
    bool hovered = !disabled && Clay_PointerOver(id);
    bool pressed = hovered && UI__mouse_down;
    bool clicked = hovered && UI__mouse_released && !disabled;

    int state = 0;
    if (disabled) state = 3;
    else if (pressed) state = 2;
    else if (hovered) state = 1;

    Clay_Color bg = get_btn_bg(variant, state);
    Clay_Color fg = get_btn_fg(variant, disabled);

    CLAY(id, {
        .layout = {
            .sizing = { CLAY_SIZING_FIXED((float)(size + 12)),
                        CLAY_SIZING_FIXED((float)(size + 12)) },
            .childAlignment = { CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER },
        },
        .backgroundColor = bg,
        .cornerRadius = CLAY_CORNER_RADIUS(DS_RADIUS_MD),
    }) {
        CLAY_TEXT(UI__str(icon), { .textColor = fg, .fontSize = (uint16_t)size });
    }
    return clicked;
}
