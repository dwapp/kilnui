#include "container.h"
#include "ui_internal.h"
#include "design_system.h"

Clay_ElementDeclaration UI_ContainerConfig(UIContainerVariant variant)
{
    Clay_ElementDeclaration cfg = {
        .layout = {
            .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIT(0) },
            .padding = { 16, 16, 16, 16 },
            .childGap = 12,
            .layoutDirection = CLAY_TOP_TO_BOTTOM,
        },
        .backgroundColor = ds_theme->surface0,
        .cornerRadius = CLAY_CORNER_RADIUS(10),
    };

    if (variant == UI_CONTAINER_ROW) {
        cfg.layout.padding = (Clay_Padding){ 0, 0, 0, 0 };
        cfg.layout.layoutDirection = CLAY_LEFT_TO_RIGHT;
        cfg.layout.childAlignment = (Clay_ChildAlignment){ CLAY_ALIGN_X_LEFT, CLAY_ALIGN_Y_CENTER };
        cfg.backgroundColor = (Clay_Color){ 0, 0, 0, 0 };
        cfg.cornerRadius = CLAY_CORNER_RADIUS(0);
    } else if (variant == UI_CONTAINER_COLUMN) {
        cfg.layout.padding = (Clay_Padding){ 0, 0, 0, 0 };
        cfg.backgroundColor = (Clay_Color){ 0, 0, 0, 0 };
        cfg.cornerRadius = CLAY_CORNER_RADIUS(0);
    } else if (variant == UI_CONTAINER_CARD) {
        cfg.layout.padding = (Clay_Padding){ 18, 18, 16, 16 };
        cfg.backgroundColor = ds_theme->surface0;
        cfg.cornerRadius = CLAY_CORNER_RADIUS(8);
    }

    return cfg;
}
