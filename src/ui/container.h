/* src/ui/container.h - Container macro for nested Clay content. */

#ifndef UI_CONTAINER_H
#define UI_CONTAINER_H

#include "clay.h"

typedef enum
{
    UI_CONTAINER_PANEL = 0,
    UI_CONTAINER_ROW,
    UI_CONTAINER_COLUMN,
    UI_CONTAINER_CARD,
} UIContainerVariant;

Clay_ElementDeclaration UI_ContainerConfig(UIContainerVariant variant);

#define UI_Container(uid, variant) \
    CLAY(CLAY_SIDI(CLAY_STRING("UIContainer"), (uid)), UI_ContainerConfig((variant)))

#endif /* UI_CONTAINER_H */
