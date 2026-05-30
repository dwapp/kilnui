#include "progress.h"
#include "ui_internal.h"

void UI_Progress(int uid, const char *label, float value, float max_value)
{
    float pct = max_value > 0.0f ? UI__clampf(value / max_value, 0.0f, 1.0f) : 0.0f;

    CLAY(CLAY_SIDI(CLAY_STRING("UIProgress"), uid), {
        .layout = {
            .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIT(0) },
            .childGap = 6,
            .layoutDirection = CLAY_TOP_TO_BOTTOM,
        },
    }) {
        if (label && label[0]) {
            CLAY_TEXT(UI__str(label), { .textColor = UI_COL_MUTED, .fontSize = 12 });
        }
        CLAY(CLAY_SIDI(CLAY_STRING("UIProgressTrack"), uid), {
            .layout = { .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIXED(10) } },
            .backgroundColor = UI_COL_SURFACE,
            .cornerRadius = CLAY_CORNER_RADIUS(5),
        }) {
            CLAY(CLAY_SIDI(CLAY_STRING("UIProgressFill"), uid), {
                .layout = { .sizing = { CLAY_SIZING_PERCENT(pct), CLAY_SIZING_GROW(0) } },
                .backgroundColor = UI_COL_ACCENT,
                .cornerRadius = CLAY_CORNER_RADIUS(5),
            }) {}
        }
    }
}
