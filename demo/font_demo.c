/* SPDX-License-Identifier: MIT */
/* demo/font_demo.c - Simple demo to reproduce font rendering issues. */

#include "../src/kilnui.h"
#include "../src/ui/ui.h"
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <stdio.h>
#include <string.h>

static void ui_build(void)
{
    CLAY(CLAY_ID("Root"), {
                              .layout = {
                                  .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0) },
                                  .padding = { 32, 32, 32, 32 },
                                  .childGap = 16,
                                  .layoutDirection = CLAY_TOP_TO_BOTTOM,
                              },
                              .backgroundColor = ds_theme->base,
                          })
    {
        CLAY(CLAY_ID("ScrollArea"), { .layout = {
                                          .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0) },
                                          .layoutDirection = CLAY_TOP_TO_BOTTOM,
                                      },
                                      .clip = { .vertical = true, .childOffset = Clay_GetScrollOffset() } })
        {
            static char bufs[2000][256];
            int base_codepoint = 0x4E00; // Start of CJK Unified Ideographs
            for (int i = 0; i < 2000; i++) {
                // Generate 10 unique Chinese characters per line
                char cjk_str[64] = { 0 };
                int idx = 0;
                for (int c = 0; c < 10; c++) {
                    int cp = base_codepoint + (i * 10) + c;
                    // simple UTF-8 encoding for 3-byte CJK chars (0x0800 - 0xFFFF)
                    cjk_str[idx++] = 0xE0 | (cp >> 12);
                    cjk_str[idx++] = 0x80 | ((cp >> 6) & 0x3F);
                    cjk_str[idx++] = 0x80 | (cp & 0x3F);
                }
                cjk_str[idx] = '\0';

                snprintf(bufs[i], sizeof(bufs[i]), "Unique characters %d: %s", i, cjk_str);
                CLAY_TEXT(((Clay_String){ .chars = bufs[i], .length = strlen(bufs[i]) }), {
                                                                                              .textColor = ds_theme->text,
                                                                                              .fontSize = 16,
                                                                                          });
            }
        }
    }
}

static void handle_demo_event(const SDL_Event *e, bool *mouse_down,
                              bool *mouse_released, float *mouse_x, float *mouse_y)
{
    if (e->type == SDL_EVENT_MOUSE_MOTION) {
        *mouse_x = e->motion.x;
        *mouse_y = e->motion.y;
    } else if (e->type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
        *mouse_down = true;
        *mouse_x = e->button.x;
        *mouse_y = e->button.y;
    } else if (e->type == SDL_EVENT_MOUSE_BUTTON_UP) {
        *mouse_down = false;
        *mouse_released = true;
        *mouse_x = e->button.x;
        *mouse_y = e->button.y;
    }
}

int main(int argc, char *argv[])
{
    (void)argc;
    (void)argv;

    KilnUI ctx;
    static const char *font_candidates[] = {
        "/usr/share/fonts/truetype/noto/NotoSansCJK-Regular.ttc",
        "/usr/share/fonts/opentype/noto/NotoSansCJK-Regular.ttc",
        "/usr/share/fonts/truetype/wqy/wqy-microhei.ttc",
        "assets/Inter-Regular.ttf",
        "/usr/share/fonts/truetype/noto/NotoSans-Regular.ttf",
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
        NULL
    };
    const char *font = KilnUI_find_font(font_candidates);
    if (!font) {
        SDL_Log("No usable font found");
        return 1;
    }
    SDL_Log("Using font: %s", font);
    if (!KilnUI_init(&ctx, "KilnUI Font Demo", 800, 600, font, 16)) {
        SDL_Log("KilnUI_init failed");
        return 1;
    }

    bool running = true;
    bool dirty = true;
    bool mouse_down = false;
    bool mouse_released = false;
    float mouse_x = 0.0f;
    float mouse_y = 0.0f;
    Uint64 last = SDL_GetPerformanceCounter();

    while (running) {
        SDL_Event e;
        mouse_released = false;

        if (!dirty) {
            SDL_WaitEvent(NULL);
        }

        while (SDL_PollEvent(&e)) {
            dirty = true;
            if (e.type == SDL_EVENT_QUIT) {
                running = false;
                break;
            }
            if (e.type == SDL_EVENT_KEY_DOWN && e.key.key == SDLK_ESCAPE) {
                running = false;
                break;
            }
            handle_demo_event(&e, &mouse_down, &mouse_released, &mouse_x, &mouse_y);
            KilnUI_handle_event(&ctx, &e);
        }

        if (!running)
            break;

        if (dirty) {
            Uint64 now = SDL_GetPerformanceCounter();
            float dt = (float)(now - last) / (float)SDL_GetPerformanceFrequency();
            last = now;

            UI_SetPointerState(mouse_down, mouse_released, mouse_x, mouse_y);

            Clay_BeginLayout();
            ui_build();
            Clay_RenderCommandArray cmds = Clay_EndLayout(dt);
            KilnUI_render(&ctx, cmds);

            dirty = false; /* SDL_WaitEvent drives the next frame */
        }
    }

    KilnUI_destroy(&ctx);
    return 0;
}
