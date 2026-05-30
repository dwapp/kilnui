/* src/ui/label.h - Text labels for Clay layouts. */

#ifndef UI_LABEL_H
#define UI_LABEL_H

#include "clay.h"

typedef enum
{
    UI_LABEL_BODY = 0,
    UI_LABEL_MUTED,
    UI_LABEL_HEADING,
    UI_LABEL_CAPTION,
} UILabelStyle;

void UI_Label(int uid, const char *text, UILabelStyle style);

#endif /* UI_LABEL_H */
