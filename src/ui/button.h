/* src/ui/button.h — Reusable Clay button component.
 *
 * Usage:
 *   1. Call UI_SetMouseState() once per frame before building layout.
 *   2. Call UI_Button() inside a Clay layout block.
 *      Returns true the frame the button is clicked (released over it).
 */

#ifndef UI_BUTTON_H
#define UI_BUTTON_H

#include <stdbool.h>
#include "../../clay/clay.h"

/* ---- Variant ---- */
typedef enum {
    UI_BTN_PRIMARY   = 0,   /* filled accent colour */
    UI_BTN_SECONDARY = 1,   /* subtle surface, accent text */
    UI_BTN_GHOST     = 2,   /* fully transparent, muted text */
    UI_BTN_DANGER    = 3,   /* red / destructive action */
} UIBtnVariant;

/* ---- Size ---- */
typedef enum {
    UI_BTN_SM = 0,   /* 28px height */
    UI_BTN_MD = 1,   /* 36px height (default) */
    UI_BTN_LG = 2,   /* 48px height */
} UIBtnSize;

/* --- Per-frame mouse state ---- */
/* Call once before Clay_BeginLayout() with the frame's accumulated state. */
void UI_SetMouseState(bool mouse_down, bool mouse_released);

/* ---- Main widget ---- */
/* uid   : caller-assigned unique integer (e.g. __LINE__ or an enum constant)
 * Returns true the frame the button is clicked (mouse released over it). */
bool UI_Button(int uid, const char *label,
               UIBtnVariant variant, UIBtnSize size, bool disabled);

#endif /* UI_BUTTON_H */
