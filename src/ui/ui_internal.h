/* SPDX-License-Identifier: MIT */
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

/* Window pointer for SDL text input management (set once by the app). */
extern struct SDL_Window *UI__text_input_window;
void UI_SetTextInputWindow(struct SDL_Window *win);

static inline Clay_String UI__str(const char *s)
{
    return (Clay_String){ .chars = s ? s : "", .length = s ? (int32_t)SDL_strlen(s) : 0 };
}

static inline float UI__clampf(float v, float min, float max)
{
    return v < min ? min : (v > max ? max : v);
}

#endif /* UI_INTERNAL_H */
