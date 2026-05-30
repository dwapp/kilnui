#include "label.h"
#include "ui_internal.h"

void UI_Label(int uid, const char *text, UILabelStyle style)
{
    Clay_Color color = UI_COL_TEXT;
    uint16_t font_size = 14;

    if (style == UI_LABEL_MUTED) {
        color = UI_COL_MUTED;
    } else if (style == UI_LABEL_HEADING) {
        font_size = 22;
    } else if (style == UI_LABEL_CAPTION) {
        color = UI_COL_MUTED;
        font_size = 12;
    }

    CLAY(CLAY_SIDI(CLAY_STRING("UILabel"), uid), {
        .layout = {
            .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIT(0) },
            .childAlignment = { CLAY_ALIGN_X_LEFT, CLAY_ALIGN_Y_CENTER },
        },
    }) {
        CLAY_TEXT(UI__str(text), { .textColor = color, .fontSize = font_size });
    }
}
