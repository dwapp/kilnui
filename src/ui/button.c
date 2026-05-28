/* src/ui/button.c — Reusable Clay button component implementation. */

#include "button.h"
#include <string.h>

/* ---- Per-frame mouse state (set via UI_SetMouseState) ---- */
static bool s_mouse_down     = false;
static bool s_mouse_released = false;

void UI_SetMouseState(bool mouse_down, bool mouse_released) {
    s_mouse_down     = mouse_down;
    s_mouse_released = mouse_released;
}

/* ---- Colour palette (Catppuccin Mocha) ---- */
#define C(r,g,b,a) ((Clay_Color){r,g,b,a})

/* Background colours: [variant][state: 0=normal, 1=hover, 2=pressed, 3=disabled] */
static const Clay_Color BG[4][4] = {
    /* PRIMARY   */ { C(137,112,194,255), C(155,130,210,255), C(118, 96,175,255), C( 69, 71, 90,120) },
    /* SECONDARY */ { C( 49, 50, 68,160), C( 69, 71, 90,200), C( 39, 40, 58,200), C( 49, 50, 68, 80) },
    /* GHOST     */ { C(  0,  0,  0,  0), C( 88, 91,112, 80), C( 88, 91,112,140), C(  0,  0,  0,  0) },
    /* DANGER    */ { C(210, 99,128,255), C(230,115,143,255), C(190, 82,112,255), C( 88, 60, 72,120) },
};

/* Foreground (text) colours: [variant][disabled] */
static const Clay_Color FG[4][2] = {
    /* PRIMARY   */ { C(255,255,255,255), C(166,173,200,100) },
    /* SECONDARY */ { C(180,190,254,255), C(166,173,200,100) },
    /* GHOST     */ { C(166,173,200,255), C(166,173,200, 80) },
    /* DANGER    */ { C(255,255,255,255), C(166,173,200,100) },
};

/* ---- Size metrics ---- */
typedef struct { int h; int px; int py; int fs; int radius; } SizeInfo;
static const SizeInfo SIZES[3] = {
    /*SM*/ { 28, 12, 4, 12, 4 },
    /*MD*/ { 36, 16, 6, 14, 6 },
    /*LG*/ { 48, 22, 8, 16, 8 },
};

/* ---- UI_Button ---- */
bool UI_Button(int uid, const char *label,
               UIBtnVariant variant, UIBtnSize size, bool disabled) {
    Clay_ElementId id = Clay_GetElementIdWithIndex(CLAY_STRING("UIBtn"), uid);
    bool hovered = !disabled && Clay_PointerOver(id);
    bool pressed = hovered && s_mouse_down;
    bool clicked = hovered && s_mouse_released && !disabled;

    /* Pick state index */
    int state = 0;
    if (disabled) state = 3;
    else if (pressed) state = 2;
    else if (hovered) state = 1;

    const SizeInfo *sz = &SIZES[size];
    Clay_Color bg = BG[variant][state];
    Clay_Color fg = FG[variant][disabled ? 1 : 0];

    Clay_String lbl = { .chars = label, .length = (int32_t)strlen(label) };

    CLAY(id, {
        .layout = {
            .sizing = {
                CLAY_SIZING_FIT(sz->px * 2 + 40),   /* min-width */
                CLAY_SIZING_FIXED((float)sz->h),
            },
            .padding = { (uint16_t)sz->px, (uint16_t)sz->px,
                         (uint16_t)sz->py, (uint16_t)sz->py },
            .childAlignment = { CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER },
        },
        .backgroundColor = bg,
        .cornerRadius = CLAY_CORNER_RADIUS((float)sz->radius),
    }) {
        CLAY_TEXT(lbl, {
            .textColor = fg,
            .fontSize  = (uint16_t)sz->fs,
        });
    }

    return clicked;
}
