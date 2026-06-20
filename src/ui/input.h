/* SPDX-License-Identifier: MIT */
/* src/ui/input.h - Text input field shell.
 *
 * The widget draws focus/value/placeholder state and reports clicks.
 * It manages SDL_StartTextInput / SDL_StopTextInput automatically.
 * Text editing (buffer append, backspace) remains owned by the caller.
 *
 * IMPORTANT: When using multiple UI_Input on the same page, the caller
 * must queue TEXT_INPUT / KEY_DOWN events and process them AFTER
 * ui_build() so that focus state is up-to-date.
 * See docs/kilnui-pitfalls.md #11 for details.
 */

#ifndef UI_INPUT_H
#define UI_INPUT_H

#include <stdbool.h>

typedef struct
{
    bool clicked;
    bool focused;
} UIInputResult;

UIInputResult UI_Input(int uid, const char *label, const char *value,
                       const char *placeholder, bool focused, bool disabled);

/* Call when clicking outside any input to release keyboard focus.
 * Stops SDL text input so keystrokes no longer generate TEXT_INPUT events. */
void UI_Input_ResetFocus(void);

#endif /* UI_INPUT_H */
