/* src/ui/theme.c — Theme-aware component implementations */

#include "theme.h"

/* ================================================================
 * Badge
 * ================================================================ */
void UI_Badge(int uid, const char *text, DSVariant variant)
{
    Clay_Color bg = DS_VariantBg(variant);
    Clay_Color fg = DS_VariantFg(variant);

    CLAY(CLAY_SIDI(CLAY_STRING("UIBadge"), uid), {
        .layout = {
            .sizing  = { CLAY_SIZING_FIT(0), CLAY_SIZING_FIT(0) },
            .padding = { 6, 6, 2, 2 },
            .childAlignment = { CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER },
        },
        .backgroundColor = bg,
        .cornerRadius    = CLAY_CORNER_RADIUS(DS_RADIUS_FULL),
    }) {
        CLAY_TEXT(UI__str(text), { .textColor = fg, .fontSize = 11 });
    }
}

/* ================================================================
 * Chip
 * ================================================================ */
bool UI_Chip(int uid, const char *text, bool selected, bool dismissible)
{
    Clay_ElementId id      = Clay_GetElementIdWithIndex(CLAY_STRING("UIChip"), uid);
    Clay_ElementId id_dim  = Clay_GetElementIdWithIndex(CLAY_STRING("UIChipX"), uid);
    bool hovered   = Clay_PointerOver(id);
    bool dismissed = dismissible && Clay_PointerOver(id_dim) && UI__mouse_released;

    Clay_Color bg = selected
        ? ds_theme->accent
        : hovered ? ds_theme->surface1 : ds_theme->surface0;
    Clay_Color fg = selected ? ds_theme->base : ds_theme->text;
    Clay_Color border_col = selected ? ds_theme->accent : ds_theme->surface2;

    CLAY(id, {
        .layout = {
            .sizing  = { CLAY_SIZING_FIT(0), CLAY_SIZING_FIXED(DS_HEIGHT_XS) },
            .padding = { 10, dismissible ? 4 : 10, 0, 0 },
            .childGap = 4,
            .layoutDirection = CLAY_LEFT_TO_RIGHT,
            .childAlignment  = { CLAY_ALIGN_X_LEFT, CLAY_ALIGN_Y_CENTER },
        },
        .backgroundColor = bg,
        .cornerRadius    = CLAY_CORNER_RADIUS(DS_RADIUS_FULL),
        .border = {
            .color = border_col,
            .width = { .left = 1, .right = 1, .top = 1, .bottom = 1 },
        },
    }) {
        CLAY_TEXT(UI__str(text), { .textColor = fg, .fontSize = 12 });

        if (dismissible) {
            bool x_hov = Clay_PointerOver(id_dim);
            Clay_Color x_col = x_hov ? ds_theme->error : ds_theme->muted;
            CLAY(id_dim, {
                .layout = {
                    .sizing  = { CLAY_SIZING_FIXED(16), CLAY_SIZING_FIXED(16) },
                    .childAlignment = { CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER },
                },
            }) {
                CLAY_TEXT(CLAY_STRING("\xe2\x9c\x95"),   /* ✕ */
                          { .textColor = x_col, .fontSize = 10 });
            }
        }
    }
    return !dismissed; /* false = chip should be removed */
}

/* ================================================================
 * Divider
 * ================================================================ */
void UI_Divider(int uid)
{
    CLAY(CLAY_SIDI(CLAY_STRING("UIDivider"), uid), {
        .layout = {
            .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIXED(1) },
        },
        .backgroundColor = ds_theme->surface2,
    }) {}
}

void UI_DividerLabel(int uid, const char *label)
{
    CLAY(CLAY_SIDI(CLAY_STRING("UIDividerLbl"), uid), {
        .layout = {
            .sizing  = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIT(0) },
            .padding = { 0, 0, 8, 8 },
            .childGap = 8,
            .layoutDirection = CLAY_LEFT_TO_RIGHT,
            .childAlignment  = { CLAY_ALIGN_X_LEFT, CLAY_ALIGN_Y_CENTER },
        },
    }) {
        /* Left line */
        CLAY(CLAY_SIDI(CLAY_STRING("UIDivL"), uid), {
            .layout = { .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIXED(1) } },
            .backgroundColor = ds_theme->surface2,
        }) {}

        /* Label */
        CLAY(CLAY_SIDI(CLAY_STRING("UIDivTxt"), uid), {
            .layout = { .sizing = { CLAY_SIZING_FIT(0), CLAY_SIZING_FIT(0) } },
        }) {
            CLAY_TEXT(UI__str(label),
                      { .textColor = ds_theme->muted, .fontSize = 11 });
        }

        /* Right line */
        CLAY(CLAY_SIDI(CLAY_STRING("UIDivR"), uid), {
            .layout = { .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIXED(1) } },
            .backgroundColor = ds_theme->surface2,
        }) {}
    }
}

/* ================================================================
 * Backdrop
 * ================================================================ */
void UI_Backdrop(int uid, bool visible)
{
    if (!visible) return;
    CLAY(CLAY_SIDI(CLAY_STRING("UIBackdrop"), uid), {
        .layout = {
            .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0) },
        },
        .backgroundColor = ds_theme->crust,
        .floating = {
            .zIndex = 100,
            .attachTo = CLAY_ATTACH_TO_ROOT,
        },
    }) {}
}

/* ================================================================
 * Avatar
 * ================================================================ */
void UI_Avatar(int uid, const char *initials, DSVariant color_variant, int size)
{
    if (size <= 0) size = DS_HEIGHT_MD;
    Clay_Color bg = DS_VariantBg(color_variant);
    Clay_Color fg = DS_VariantFg(color_variant);

    CLAY(CLAY_SIDI(CLAY_STRING("UIAvatar"), uid), {
        .layout = {
            .sizing  = { CLAY_SIZING_FIXED((float)size), CLAY_SIZING_FIXED((float)size) },
            .childAlignment = { CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER },
        },
        .backgroundColor = bg,
        .cornerRadius    = CLAY_CORNER_RADIUS(DS_RADIUS_FULL),
    }) {
        int fs = size / 3;
        if (fs < 10) fs = 10;
        CLAY_TEXT(UI__str(initials), { .textColor = fg, .fontSize = (uint16_t)fs });
    }
}

/* ================================================================
 * Alert
 * ================================================================ */
void UI_Alert(int uid, const char *message, DSVariant variant)
{
    Clay_Color bg  = DS_VariantBg(variant);
    Clay_Color fg  = DS_VariantFg(variant);

    /* Soften the background for alerts */
    bg.a = 60;

    CLAY(CLAY_SIDI(CLAY_STRING("UIAlert"), uid), {
        .layout = {
            .sizing  = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIT(0) },
            .padding = { DS_SPACE_3, DS_SPACE_3, DS_SPACE_2, DS_SPACE_2 },
            .childGap = DS_SPACE_2,
            .layoutDirection = CLAY_LEFT_TO_RIGHT,
            .childAlignment  = { CLAY_ALIGN_X_LEFT, CLAY_ALIGN_Y_CENTER },
        },
        .backgroundColor = bg,
        .cornerRadius    = CLAY_CORNER_RADIUS(DS_RADIUS_MD),
        .border = {
            .color = DS_VariantBg(variant),
            .width = { .left = 3 },   /* left accent strip */
        },
    }) {
        CLAY_TEXT(UI__str(message), { .textColor = fg, .fontSize = 13 });
    }
}
