#include "dropdown.h"
#include "ui_internal.h"

UIDropdownResult UI_Dropdown(int uid, const char *label,
                             const char *const *items, int item_count,
                             int selected_index, bool open, bool disabled)
{
    Clay_ElementId header_id = Clay_GetElementIdWithIndex(CLAY_STRING("UIDropdownHeader"), uid);
    bool header_hovered = !disabled && Clay_PointerOver(header_id);
    bool header_clicked = header_hovered && UI__mouse_released;
    int chosen = -1;

    CLAY(CLAY_SIDI(CLAY_STRING("UIDropdown"), uid), {
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

        const char *selected = (items && selected_index >= 0 && selected_index < item_count)
            ? items[selected_index] : "";

        CLAY(header_id, {
            .layout = {
                .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIXED(38) },
                .padding = { 12, 12, 8, 8 },
                .childGap = 8,
                .childAlignment = { CLAY_ALIGN_X_LEFT, CLAY_ALIGN_Y_CENTER },
            },
            .backgroundColor = disabled ? UI_C(49, 50, 68, 90) :
                (header_hovered ? UI_COL_SURFACE2 : UI_COL_SURFACE),
            .cornerRadius = CLAY_CORNER_RADIUS(7),
        }) {
            CLAY_TEXT(UI__str(selected), {
                .textColor = disabled ? UI_COL_DISABLED : UI_COL_TEXT,
                .fontSize = 14,
            });
        }

        if (open && !disabled && items) {
            CLAY(CLAY_SIDI(CLAY_STRING("UIDropdownMenu"), uid), {
                .layout = {
                    .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIT(0) },
                    .padding = { 6, 6, 6, 6 },
                    .childGap = 2,
                    .layoutDirection = CLAY_TOP_TO_BOTTOM,
                },
                .backgroundColor = UI_C(30, 30, 46, 245),
                .cornerRadius = CLAY_CORNER_RADIUS(7),
            }) {
                for (int i = 0; i < item_count; i++) {
                    Clay_ElementId item_id = Clay_GetElementIdWithIndex(CLAY_STRING("UIDropdownItem"),
                                                                        (uint32_t)(uid * 1024 + i));
                    bool item_hovered = Clay_PointerOver(item_id);
                    if (item_hovered && UI__mouse_released)
                        chosen = i;

                    CLAY(item_id, {
                        .layout = {
                            .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIXED(30) },
                            .padding = { 8, 8, 5, 5 },
                            .childAlignment = { CLAY_ALIGN_X_LEFT, CLAY_ALIGN_Y_CENTER },
                        },
                        .backgroundColor = item_hovered ? UI_COL_SURFACE2 :
                            (i == selected_index ? UI_C(137, 112, 194, 80) : UI_C(0, 0, 0, 0)),
                        .cornerRadius = CLAY_CORNER_RADIUS(5),
                    }) {
                        CLAY_TEXT(UI__str(items[i]), {
                            .textColor = i == selected_index ? UI_COL_ACCENT2 : UI_COL_TEXT,
                            .fontSize = 14,
                        });
                    }
                }
            }
        }
    }

    return (UIDropdownResult){ header_clicked, chosen };
}
