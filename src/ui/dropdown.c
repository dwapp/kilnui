/* SPDX-License-Identifier: MIT */
#include "dropdown.h"
#include "design_system.h"
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
                                                    })
    {
        if (label && label[0]) {
            CLAY_TEXT(UI__str(label), {
                                          .textColor = disabled ? ds_theme->surface2 : ds_theme->muted,
                                          .fontSize = 12,
                                      });
        }

        const char *selected = (items && selected_index >= 0 && selected_index < item_count)
                                   ? items[selected_index]
                                   : "";

        CLAY(header_id, {
                            .layout = {
                                .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIXED(38) },
                                .padding = { 12, 12, 8, 8 },
                                .childGap = 8,
                                .childAlignment = { CLAY_ALIGN_X_LEFT, CLAY_ALIGN_Y_CENTER },
                            },
                            .backgroundColor = disabled ? ds_theme->surface0 : (header_hovered ? ds_theme->surface1 : ds_theme->surface0),
                            .cornerRadius = CLAY_CORNER_RADIUS(DS_RADIUS_MD),
                        })
        {
            CLAY_TEXT(UI__str(selected), {
                                             .textColor = disabled ? ds_theme->muted : ds_theme->text,
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
                                                                    .backgroundColor = ds_theme->overlay,
                                                                    .cornerRadius = CLAY_CORNER_RADIUS(DS_RADIUS_MD),
                                                                })
            {
                for (int i = 0; i < item_count; i++) {
                    Clay_ElementId item_id = Clay__HashStringWithOffset(CLAY_STRING("UIDropdownItem"), i, uid);
                    bool item_hovered = Clay_PointerOver(item_id);
                    if (item_hovered && UI__mouse_released)
                        chosen = i;

                    Clay_Color sel_bg = ds_theme->accent;
                    sel_bg.a = 80;

                    CLAY(item_id, {
                                      .layout = {
                                          .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIXED(30) },
                                          .padding = { 8, 8, 5, 5 },
                                          .childAlignment = { CLAY_ALIGN_X_LEFT, CLAY_ALIGN_Y_CENTER },
                                      },
                                      .backgroundColor = item_hovered ? ds_theme->surface2 : (i == selected_index ? sel_bg : (Clay_Color){ 0 }),
                                      .cornerRadius = CLAY_CORNER_RADIUS(DS_RADIUS_SM),
                                  })
                    {
                        CLAY_TEXT(UI__str(items[i]), {
                                                         .textColor = i == selected_index ? ds_theme->accent : ds_theme->text,
                                                         .fontSize = 14,
                                                     });
                    }
                }
            }
        }
    }

    return (UIDropdownResult){ header_clicked, chosen };
}
