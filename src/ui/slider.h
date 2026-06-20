/* SPDX-License-Identifier: MIT */
/* src/ui/slider.h - Horizontal slider widget. */

#ifndef UI_SLIDER_H
#define UI_SLIDER_H

#include <stdbool.h>

typedef struct
{
    bool changed;
    float value;
} UISliderResult;

UISliderResult UI_Slider(int uid, const char *label, float value,
                         float min_value, float max_value, bool disabled);

#endif /* UI_SLIDER_H */
