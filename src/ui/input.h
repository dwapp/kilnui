/* src/ui/input.h - Text input field shell.
 *
 * The widget draws focus/value/placeholder state and reports clicks. Text
 * editing remains owned by the caller's SDL_TEXT_INPUT / key handling.
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

#endif /* UI_INPUT_H */
