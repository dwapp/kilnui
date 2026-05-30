#include "radio.h"
#include "ui_internal.h"

bool UI_Radio(int uid, const char *label, bool selected, bool disabled)
{
    Clay_ElementId id = Clay_GetElementIdWithIndex(CLAY_STRING("UIRadio"), uid);
    bool hovered = !disabled && Clay_PointerOver(id);
    bool clicked = hovered && UI__mouse_released;
    Clay_Color outer = disabled ? UI_COL_SURFACE : (hovered ? UI_COL_SURFACE2 : UI_COL_SURFACE);
    Clay_Color dot = selected ? (disabled ? UI_COL_DISABLED : UI_COL_ACCENT) : UI_C(0, 0, 0, 0);

    CLAY(id, {
        .layout = {
            .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIXED(28) },
            .childGap = 10,
            .childAlignment = { CLAY_ALIGN_X_LEFT, CLAY_ALIGN_Y_CENTER },
        },
    }) {
        CLAY(CLAY_SIDI(CLAY_STRING("UIRadioOuter"), uid), {
            .layout = {
                .sizing = { CLAY_SIZING_FIXED(18), CLAY_SIZING_FIXED(18) },
                .padding = CLAY_PADDING_ALL(5),
                .childAlignment = { CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER },
            },
            .backgroundColor = outer,
            .cornerRadius = CLAY_CORNER_RADIUS(9),
        }) {
            CLAY(CLAY_SIDI(CLAY_STRING("UIRadioDot"), uid), {
                .layout = { .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0) } },
                .backgroundColor = dot,
                .cornerRadius = CLAY_CORNER_RADIUS(6),
            }) {}
        }
        CLAY_TEXT(UI__str(label), {
            .textColor = disabled ? UI_COL_DISABLED : UI_COL_TEXT,
            .fontSize = 14,
        });
    }

    return clicked;
}
