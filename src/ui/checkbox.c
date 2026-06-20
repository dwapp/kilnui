/* SPDX-License-Identifier: MIT */
#include "checkbox.h"
#include "ui_internal.h"
#include "design_system.h"

bool UI_Checkbox(int uid, const char *label, bool checked, bool disabled)
{
    Clay_ElementId id = Clay_GetElementIdWithIndex(CLAY_STRING("UICheckbox"), uid);
    bool hovered = !disabled && Clay_PointerOver(id);
    bool clicked = hovered && UI__mouse_released;
    Clay_Color box = disabled ? ds_theme->surface0 : (hovered ? ds_theme->surface1 : ds_theme->surface0);
    Clay_Color mark = checked ? (disabled ? ds_theme->muted : ds_theme->accent) : (Clay_Color){0};

    CLAY(id, {
        .layout = {
            .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIXED(28) },
            .childGap = 10,
            .childAlignment = { CLAY_ALIGN_X_LEFT, CLAY_ALIGN_Y_CENTER },
        },
    }) {
        CLAY(CLAY_SIDI(CLAY_STRING("UICheckboxBox"), uid), {
            .layout = {
                .sizing = { CLAY_SIZING_FIXED(18), CLAY_SIZING_FIXED(18) },
                .padding = CLAY_PADDING_ALL(4),
                .childAlignment = { CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER },
            },
            .backgroundColor = box,
            .cornerRadius = CLAY_CORNER_RADIUS(4),
        }) {
            CLAY(CLAY_SIDI(CLAY_STRING("UICheckboxMark"), uid), {
                .layout = { .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0) } },
                .backgroundColor = mark,
                .cornerRadius = CLAY_CORNER_RADIUS(2),
            }) {}
        }
        CLAY_TEXT(UI__str(label), {
            .textColor = disabled ? ds_theme->muted : ds_theme->text,
            .fontSize = 14,
        });
    }

    return clicked;
}
