/* SPDX-License-Identifier: MIT */
/* demo/input_demo.c — Minimal text input demo with debug logging.
 *
 * Build: cmake --build build --target input_demo
 * Run:   ./build/kilnui/demo/input_demo
 */

#include "../src/kilnui.h"
#include "../src/ui/ui.h"
#include "../src/ui/input.h"
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <stdio.h>
#include <string.h>

/* ---- State ---- */
static char g_text_buf[256] = "";
static bool g_focused = false;
static int  g_frame = 0;

/* ---- Helper: log with prefix ---- */
#define LOG(fmt, ...) fprintf(stderr, "[frame %d] " fmt "\n", g_frame, ##__VA_ARGS__)

/* ---- Layout ---- */
static void ui_build(void)
{
    g_frame++;

    CLAY(CLAY_ID("Root"), {
        .layout = {
            .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0) },
            .padding = { 40, 40, 40, 40 },
            .childGap = 20,
            .layoutDirection = CLAY_TOP_TO_BOTTOM,
        },
        .backgroundColor = ds_theme->base,
    }) {
        /* Title */
        CLAY_TEXT(CLAY_STRING("Input Demo — Debug"), &(Clay_TextElementConfig){
            .textColor = ds_theme->text,
            .fontSize = 24,
        });

        /* Instructions */
        CLAY_TEXT(CLAY_STRING("Click the input to focus. Type text. ESC to unfocus. Click outside to unfocus."),
                  &(Clay_TextElementConfig){ .textColor = ds_theme->muted, .fontSize = 14 });

        /* The input field */
        UIInputResult ir = UI_Input(1, "Name", g_text_buf,
                                    "Click to type here...", g_focused, false);

        if (ir.clicked) {
            LOG("UI_Input: clicked! Setting focused=true");
            g_focused = true;
        }
        if (ir.focused != g_focused) {
            LOG("UI_Input: focused changed %d -> %d", g_focused, ir.focused);
        }

        /* Click-outside detection */
        if (UI__mouse_released && !ir.clicked && g_focused) {
            LOG("Click outside input: unfocusing");
            g_focused = false;
            UI_Input_ResetFocus();
        }

        /* Debug info */
        char dbg[512];
        snprintf(dbg, sizeof(dbg),
                 "focused=%d | buf=\"%s\" | len=%zu | mouse_released=%d",
                 g_focused, g_text_buf, strlen(g_text_buf), UI__mouse_released);
        CLAY_TEXT(UI__str(dbg), &(Clay_TextElementConfig){
            .textColor = ds_theme->overlay0,
            .fontSize = 12,
        });

        /* SDL text input status */
        SDL_Window *win = SDL_GetKeyboardFocus();
        bool text_active = win ? SDL_TextInputActive(win) : false;
        snprintf(dbg, sizeof(dbg),
                 "SDL_TextInputActive=%d | keyboard_focus=%p | text_input_window=%p",
                 text_active, (void*)win, (void*)UI__text_input_window);
        CLAY_TEXT(UI__str(dbg), &(Clay_TextElementConfig){
            .textColor = ds_theme->overlay0,
            .fontSize = 12,
        });
    }
}

/* ---- Main ---- */
int main(int argc, char *argv[])
{
    (void)argc;
    (void)argv;

    KilnUI ctx;
    static const char *fonts[] = {
        "assets/Inter-Regular.ttf",
        "/usr/share/fonts/truetype/noto/NotoSans-Regular.ttf",
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
        NULL
    };
    const char *font = KilnUI_find_font(fonts);
    if (!font) {
        LOG("ERROR: No font found");
        return 1;
    }
    LOG("Font: %s", font);

    if (!KilnUI_init(&ctx, "Input Demo", 600, 400, font, 16)) {
        LOG("ERROR: KilnUI_init failed");
        return 1;
    }
    LOG("KilnUI_init OK, window=%p", (void*)ctx.window);

    /* Set the window for text input */
    UI_SetTextInputWindow(ctx.window);
    LOG("UI_SetTextInputWindow(%p) called", (void*)ctx.window);

    bool running = true;
    bool dirty = true;
    bool mouse_down = false;
    bool mouse_released = false;
    float mx = 0, my = 0;

    while (running) {
        SDL_Event e;
        mouse_released = false;

        if (!dirty) {
            SDL_WaitEvent(NULL);
        }

        while (SDL_PollEvent(&e)) {
            dirty = true;

            /* Log every event type */
            switch (e.type) {
            case SDL_EVENT_MOUSE_MOTION:
                mx = e.motion.x;
                my = e.motion.y;
                break;

            case SDL_EVENT_MOUSE_BUTTON_DOWN:
                LOG("EVENT: MOUSE_BUTTON_DOWN at (%.0f, %.0f)", e.button.x, e.button.y);
                mouse_down = true;
                mx = e.button.x;
                my = e.button.y;
                break;

            case SDL_EVENT_MOUSE_BUTTON_UP:
                LOG("EVENT: MOUSE_BUTTON_UP at (%.0f, %.0f)", e.button.x, e.button.y);
                mouse_down = false;
                mouse_released = true;
                mx = e.button.x;
                my = e.button.y;
                break;

            case SDL_EVENT_TEXT_INPUT:
                LOG("EVENT: TEXT_INPUT text=\"%s\"", e.text.text);
                if (g_focused) {
                    size_t len = strlen(g_text_buf);
                    size_t add = strlen(e.text.text);
                    if (len + add < sizeof(g_text_buf) - 1) {
                        memcpy(g_text_buf + len, e.text.text, add + 1);
                        LOG("  -> Appended to buf, now=\"%s\"", g_text_buf);
                    } else {
                        LOG("  -> Buffer full, ignored");
                    }
                } else {
                    LOG("  -> Not focused, ignored");
                }
                break;

            case SDL_EVENT_KEY_DOWN:
                LOG("EVENT: KEY_DOWN key=%d (0x%x) name=\"%s\"",
                    e.key.key, e.key.key, SDL_GetKeyName(e.key.key));
                if (e.key.key == SDLK_ESCAPE) {
                    if (g_focused) {
                        LOG("  -> ESCAPE: unfocusing");
                        g_focused = false;
                        UI_Input_ResetFocus();
                    } else {
                        LOG("  -> ESCAPE: quitting");
                        running = false;
                    }
                } else if (e.key.key == SDLK_BACKSPACE) {
                    if (g_focused) {
                        size_t len = strlen(g_text_buf);
                        if (len > 0) {
                            g_text_buf[len - 1] = '\0';
                            LOG("  -> Backspace: buf now=\"%s\"", g_text_buf);
                        }
                    }
                }
                break;

            case SDL_EVENT_WINDOW_FOCUS_GAINED:
                LOG("EVENT: WINDOW_FOCUS_GAINED");
                break;

            case SDL_EVENT_WINDOW_FOCUS_LOST:
                LOG("EVENT: WINDOW_FOCUS_LOST");
                break;

            default:
                /* skip noisy mouse motion events */
                if (e.type != SDL_EVENT_MOUSE_MOTION) {
                    LOG("EVENT: type=%u", e.type);
                }
                break;
            }

            KilnUI_handle_event(&ctx, &e);
        }

        if (!running) break;

        if (dirty) {
            UI_SetPointerState(mouse_down, mouse_released, mx, my);

            Clay_BeginLayout();
            ui_build();
            Clay_RenderCommandArray cmds = Clay_EndLayout(0.016f);
            KilnUI_render(&ctx, cmds);

            dirty = false;
        }
    }

    LOG("Shutting down");
    KilnUI_destroy(&ctx);
    return 0;
}
