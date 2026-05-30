/* src/ui/ui_internal.h - Shared helpers for KilnUI widgets. */

#ifndef UI_INTERNAL_H
#define UI_INTERNAL_H

#include "clay.h"
#include <SDL3/SDL.h>
#include <stdbool.h>

#define UI_C(r, g, b, a) ((Clay_Color){ r, g, b, a })

extern bool  UI__mouse_down;
extern bool  UI__mouse_released;
extern float UI__mouse_x;
extern float UI__mouse_y;

static inline Clay_String UI__str(const char *s)
{
    return (Clay_String){ .chars = s ? s : "", .length = s ? (int32_t)SDL_strlen(s) : 0 };
}

static inline float UI__clampf(float v, float min, float max)
{
    return v < min ? min : (v > max ? max : v);
}

static const Clay_Color UI_COL_TEXT     = UI_C(205, 214, 244, 255);
static const Clay_Color UI_COL_MUTED    = UI_C(166, 173, 200, 255);
static const Clay_Color UI_COL_DISABLED = UI_C(166, 173, 200, 95);
static const Clay_Color UI_COL_SURFACE  = UI_C(49, 50, 68, 180);
static const Clay_Color UI_COL_SURFACE2 = UI_C(69, 71, 90, 220);
static const Clay_Color UI_COL_OVERLAY  = UI_C(88, 91, 112, 180);
static const Clay_Color UI_COL_ACCENT   = UI_C(137, 112, 194, 255);
static const Clay_Color UI_COL_ACCENT2  = UI_C(155, 130, 210, 255);
static const Clay_Color UI_COL_DANGER   = UI_C(210, 99, 128, 255);
static const Clay_Color UI_COL_GREEN    = UI_C(166, 227, 161, 255);

#endif /* UI_INTERNAL_H */
