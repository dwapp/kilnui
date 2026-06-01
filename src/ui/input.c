#include "input.h"
#include "ui_internal.h"
#include "design_system.h"

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
                .textColor = disabled ? ds_theme->surface2 : ds_theme->muted,
                .fontSize = 12,
            });
        }
        CLAY(id, {
            .layout = {
                .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIXED(38) },
                .padding = { 12, 12, 8, 8 },
                .childAlignment = { CLAY_ALIGN_X_LEFT, CLAY_ALIGN_Y_CENTER },
            },
            .backgroundColor = disabled ? ds_theme->surface0 :
                (focused ? ds_theme->surface1 : (hovered ? ds_theme->surface1 : ds_theme->surface0)),
            .cornerRadius = CLAY_CORNER_RADIUS(7),
        }) {
            CLAY_TEXT(UI__str(shown), {
                .textColor = disabled ? ds_theme->muted :
                    (showing_placeholder ? ds_theme->muted : ds_theme->text),
                .fontSize = 14,
            });
        }
    }

    return (UIInputResult){ clicked, clicked ? true : focused };
}
