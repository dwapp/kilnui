#include "input.h"
#include "ui_internal.h"
#include "design_system.h"
#include <stdio.h>

/* Track which UI_Input uid currently holds focus so multiple inputs
 * don't fight over SDL_StartTextInput / SDL_StopTextInput. */
static int  s_focused_uid = -1;        /* -1 = no input focused */
static bool s_text_input_active = false;

UIInputResult UI_Input(int uid, const char *label, const char *value,
                       const char *placeholder, bool focused, bool disabled)
{
    Clay_ElementId id = Clay_GetElementIdWithIndex(CLAY_STRING("UIInput"), uid);
    bool hovered = !disabled && Clay_PointerOver(id);
    bool clicked = hovered && UI__mouse_released;
    const char *shown = (value && value[0]) ? value : placeholder;
    bool showing_placeholder = !(value && value[0]);

    /* Determine new focused state */
    bool new_focused = clicked ? true : focused;

    /* When this input gains focus, steal from whoever had it */
    if (new_focused && s_focused_uid != uid) {
        s_focused_uid = uid;
    }
    /* Only the focused uid actually controls SDL text input */
    bool is_owner = (s_focused_uid == uid);

    /* Enable / disable SDL text input based on ownership */
    if (UI__text_input_window) {
        if (new_focused && is_owner && !s_text_input_active) {
            SDL_StartTextInput(UI__text_input_window);
            s_text_input_active = true;
        } else if ((!new_focused || !is_owner) && s_text_input_active) {
            /* Only stop if NO input is focused */
            bool any_focused = (s_focused_uid >= 0);
            if (!any_focused) {
                SDL_StopTextInput(UI__text_input_window);
                s_text_input_active = false;
            }
        }
    }

    /* If this input lost focus and was the owner, clear ownership */
    if (!new_focused && s_focused_uid == uid) {
        s_focused_uid = -1;
    }

    CLAY(CLAY_SIDI(CLAY_STRING("UIInputWrap"), uid), {
        .layout = {
            .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIT(0) },
            .childGap = 6,
            .layoutDirection = CLAY_TOP_TO_BOTTOM,
        },
    }) {
        if (label && label[0]) {
            CLAY_TEXT(UI__str(label), {
                .textColor = disabled ? ds_theme->surface2 : ds_theme->muted,
                .fontSize = 12,
            });
        }
        CLAY(id, {
            .layout = {
                .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIXED(38) },
                .padding = { 12, 12, 8, 8 },
                .childAlignment = { CLAY_ALIGN_X_LEFT, CLAY_ALIGN_Y_CENTER },
            },
            .backgroundColor = disabled ? ds_theme->surface0 :
                (new_focused ? ds_theme->surface1 : (hovered ? ds_theme->surface1 : ds_theme->surface0)),
            .cornerRadius = CLAY_CORNER_RADIUS(7),
        }) {
            CLAY_TEXT(UI__str(shown), {
                .textColor = disabled ? ds_theme->muted :
                    (showing_placeholder ? ds_theme->muted : ds_theme->text),
                .fontSize = 14,
            });
        }
    }

    return (UIInputResult){ clicked, new_focused };
}

void UI_Input_ResetFocus(void)
{
    s_focused_uid = -1;
    if (s_text_input_active && UI__text_input_window) {
        SDL_StopTextInput(UI__text_input_window);
        s_text_input_active = false;
    }
}
