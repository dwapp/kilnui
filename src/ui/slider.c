#include "slider.h"
#include "ui_internal.h"
#include "design_system.h"

UISliderResult UI_Slider(int uid, const char *label, float value,
                         float min_value, float max_value, bool disabled)
{
    Clay_ElementId id = Clay_GetElementIdWithIndex(CLAY_STRING("UISlider"), uid);
    bool hovered = !disabled && Clay_PointerOver(id);
    bool active = hovered && UI__mouse_down;
    bool changed = false;

    if (active && max_value > min_value) {
        Clay_ElementData data = Clay_GetElementData(id);
        if (data.boundingBox.width > 0.0f) {
            float pct = UI__clampf((UI__mouse_x - data.boundingBox.x) / data.boundingBox.width,
                                   0.0f, 1.0f);
            value = min_value + pct * (max_value - min_value);
            changed = true;
        }
    }

    float pct = max_value > min_value ? UI__clampf((value - min_value) / (max_value - min_value),
                                                    0.0f, 1.0f) : 0.0f;

    CLAY(CLAY_SIDI(CLAY_STRING("UISliderWrap"), uid), {
        .layout = {
            .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIT(0) },
            .childGap = 8,
            .layoutDirection = CLAY_TOP_TO_BOTTOM,
        },
    }) {
        if (label && label[0]) {
            CLAY_TEXT(UI__str(label), {
                .textColor = disabled ? ds_theme->muted : ds_theme->subtext,
                .fontSize = 12,
            });
        }
        CLAY(id, {
            .layout = {
                .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIXED(18) },
                .padding = { 0, 0, 4, 4 },
                .childAlignment = { CLAY_ALIGN_X_LEFT, CLAY_ALIGN_Y_CENTER },
            },
        }) {
            CLAY(CLAY_SIDI(CLAY_STRING("UISliderTrack"), uid), {
                .layout = { .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIXED(8) } },
                .backgroundColor = disabled ? ds_theme->surface0 : ds_theme->surface1,
                .cornerRadius = CLAY_CORNER_RADIUS(4),
            }) {
                CLAY(CLAY_SIDI(CLAY_STRING("UISliderFill"), uid), {
                    .layout = { .sizing = { CLAY_SIZING_PERCENT(pct), CLAY_SIZING_GROW(0) } },
                    .backgroundColor = disabled ? ds_theme->surface2 : (active ? ds_theme->accent_alt : ds_theme->accent),
                    .cornerRadius = CLAY_CORNER_RADIUS(4),
                }) {}
            }
        }
    }

    return (UISliderResult){ changed, value };
}
