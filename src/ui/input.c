#include "input.h"
#include "ui_internal.h"

UIInputResult UI_Input(int uid, const char *label, const char *value,
                       const char *placeholder, bool focused, bool disabled)
{
    Clay_ElementId id = Clay_GetElementIdWithIndex(CLAY_STRING("UIInput"), uid);
    bool hovered = !disabled && Clay_PointerOver(id);
    bool clicked = hovered && UI__mouse_released;
    const char *shown = (value && value[0]) ? value : placeholder;
    bool showing_placeholder = !(value && value[0]);

    CLAY(CLAY_SIDI(CLAY_STRING("UIInputWrap"), uid), {
        .layout = {
            .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIT(0) },
            .childGap = 6,
            .layoutDirection = CLAY_TOP_TO_BOTTOM,
        },
    }) {
        if (label && label[0]) {
            CLAY_TEXT(UI__str(label), {
                .textColor = disabled ? UI_COL_DISABLED : UI_COL_MUTED,
                .fontSize = 12,
            });
        }
        CLAY(id, {
            .layout = {
                .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIXED(38) },
                .padding = { 12, 12, 8, 8 },
                .childAlignment = { CLAY_ALIGN_X_LEFT, CLAY_ALIGN_Y_CENTER },
            },
            .backgroundColor = disabled ? UI_C(49, 50, 68, 90) :
                (focused ? UI_COL_SURFACE2 : (hovered ? UI_C(69, 71, 90, 190) : UI_COL_SURFACE)),
            .cornerRadius = CLAY_CORNER_RADIUS(7),
        }) {
            CLAY_TEXT(UI__str(shown), {
                .textColor = disabled ? UI_COL_DISABLED :
                    (showing_placeholder ? UI_COL_MUTED : UI_COL_TEXT),
                .fontSize = 14,
            });
        }
    }

    return (UIInputResult){ clicked, clicked ? true : focused };
}
