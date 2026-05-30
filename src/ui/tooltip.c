#include "tooltip.h"
#include "ui_internal.h"

void UI_Tooltip(int uid, const char *text, bool visible)
{
    if (!visible || !text || !text[0])
        return;

    CLAY(CLAY_SIDI(CLAY_STRING("UITooltip"), uid), {
        .layout = {
            .sizing = { CLAY_SIZING_FIT(0), CLAY_SIZING_FIT(0) },
            .padding = { 10, 10, 6, 6 },
            .childAlignment = { CLAY_ALIGN_X_LEFT, CLAY_ALIGN_Y_CENTER },
        },
        .backgroundColor = UI_C(17, 17, 27, 245),
        .cornerRadius = CLAY_CORNER_RADIUS(6),
    }) {
        CLAY_TEXT(UI__str(text), {
            .textColor = UI_COL_TEXT,
            .fontSize = 12,
        });
    }
}
