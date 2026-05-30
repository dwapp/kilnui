/* demo/button_demo.c — Button component gallery + click counter demo.
 *
 * Shows all button variants across three sizes and tracks click counts.
 * Build: cmake .. && make button_demo
 * Run:   ./button_demo
 */

#include "../src/kilnui.h"
#include "../src/clay_colors.h"
#include "../src/ui/button.h"
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <stdio.h>
#include <string.h>

/* ---- Button unique IDs ---- */
enum
{
    /* Row: Primary */
    BTN_PRI_SM = 0,
    BTN_PRI_MD,
    BTN_PRI_LG,
    /* Row: Secondary */
    BTN_SEC_SM,
    BTN_SEC_MD,
    BTN_SEC_LG,
    /* Row: Ghost */
    BTN_GHO_SM,
    BTN_GHO_MD,
    BTN_GHO_LG,
    /* Row: Danger */
    BTN_DAN_SM,
    BTN_DAN_MD,
    BTN_DAN_LG,
    /* Row: Disabled */
    BTN_DIS_PRI,
    BTN_DIS_SEC,
    BTN_DIS_GHO,
    BTN_DIS_DAN,
    BTN_ID_COUNT
};

/* ---- App state ---- */
static int g_clicks[BTN_ID_COUNT] = { 0 };
static int g_total_clicks = 0;
static char g_count_str[64] = "Click any button";

/* ---- Section label ---- */
static void section_label(const char *text, int idx)
{
    Clay_String s = { .chars = text, .length = (int32_t)SDL_strlen(text) };
    CLAY(CLAY_SIDI(CLAY_STRING("SecLabel"), idx), {
                                                      .layout = {
                                                          .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIXED(24) },
                                                          .childAlignment = { CLAY_ALIGN_X_LEFT, CLAY_ALIGN_Y_CENTER },
                                                      },
                                                  })
    {
        CLAY_TEXT(s, { .textColor = COL_SUBTEXT, .fontSize = 12 });
    }
}

/* ---- Horizontal button row ---- */
static void btn_row(int id_sm, int id_md, int id_lg, const char *label,
                    UIBtnVariant var, bool disabled)
{
    (void)label;
    CLAY(CLAY_SIDI(CLAY_STRING("BtnRow"), id_sm), {
                                                      .layout = {
                                                          .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIT(0) },
                                                          .childGap = 12,
                                                          .childAlignment = { CLAY_ALIGN_X_LEFT, CLAY_ALIGN_Y_CENTER },
                                                      },
                                                  })
    {
        if (UI_Button(id_sm, "Small", var, UI_BTN_SM, disabled)) {
            g_clicks[id_sm]++;
            g_total_clicks++;
        }
        if (UI_Button(id_md, "Medium", var, UI_BTN_MD, disabled)) {
            g_clicks[id_md]++;
            g_total_clicks++;
        }
        if (UI_Button(id_lg, "Large", var, UI_BTN_LG, disabled)) {
            g_clicks[id_lg]++;
            g_total_clicks++;
        }
    }
}

/* ---- Disabled row ---- */
static void disabled_row(void)
{
    CLAY(CLAY_ID("DisabledRow"), {
                                     .layout = {
                                         .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIT(0) },
                                         .childGap = 12,
                                         .childAlignment = { CLAY_ALIGN_X_LEFT, CLAY_ALIGN_Y_CENTER },
                                     },
                                 })
    {
        UI_Button(BTN_DIS_PRI, "Primary", UI_BTN_PRIMARY, UI_BTN_MD, true);
        UI_Button(BTN_DIS_SEC, "Secondary", UI_BTN_SECONDARY, UI_BTN_MD, true);
        UI_Button(BTN_DIS_GHO, "Ghost", UI_BTN_GHOST, UI_BTN_MD, true);
        UI_Button(BTN_DIS_DAN, "Danger", UI_BTN_DANGER, UI_BTN_MD, true);
    }
}

/* ---- Counter card ---- */
static void counter_card(void)
{
    Clay_String total_str = {
        .chars = g_count_str,
        .length = (int32_t)SDL_strlen(g_count_str),
    };
    CLAY(CLAY_ID("CounterCard"), {
                                     .layout = {
                                         .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIXED(72) },
                                         .padding = { 20, 20, 16, 16 },
                                         .childGap = 8,
                                         .childAlignment = { CLAY_ALIGN_X_LEFT, CLAY_ALIGN_Y_CENTER },
                                     },
                                     .backgroundColor = COL_MANTLE,
                                     .cornerRadius = CLAY_CORNER_RADIUS(10),
                                 })
    {
        CLAY_TEXT(CLAY_STRING("Click counter"), {
                                                    .textColor = COL_SUBTEXT,
                                                    .fontSize = 12,
                                                });
        CLAY_TEXT(total_str, {
                                 .textColor = COL_MAUVE,
                                 .fontSize = 22,
                             });
    }
}

/* ---- Root UI ---- */
static void ui_build(void)
{
    CLAY(CLAY_ID("Root"), {
                              .layout = {
                                  .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0) },
                                  .padding = { 40, 40, 32, 32 },
                                  .childGap = 0,
                                  .layoutDirection = CLAY_TOP_TO_BOTTOM,
                              },
                              .backgroundColor = COL_BASE,
                          })
    {
        /* Title */
        CLAY(CLAY_ID("Title"), {
                                   .layout = {
                                       .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIXED(56) },
                                       .childAlignment = { CLAY_ALIGN_X_LEFT, CLAY_ALIGN_Y_CENTER },
                                   },
                               })
        {
            CLAY_TEXT(CLAY_STRING("Button Gallery"), {
                                                         .textColor = COL_TEXT,
                                                         .fontSize = 26,
                                                     });
        }

        /* Panel */
        CLAY(CLAY_ID("Panel"), {
                                   .layout = {
                                       .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIT(0) },
                                       .padding = { 24, 24, 24, 24 },
                                       .childGap = 14,
                                       .layoutDirection = CLAY_TOP_TO_BOTTOM,
                                   },
                                   .backgroundColor = COL_SURFACE0,
                                   .cornerRadius = CLAY_CORNER_RADIUS(12),
                               })
        {
            section_label("PRIMARY", 0);
            btn_row(BTN_PRI_SM, BTN_PRI_MD, BTN_PRI_LG, "Primary",
                    UI_BTN_PRIMARY, false);

            section_label("SECONDARY", 1);
            btn_row(BTN_SEC_SM, BTN_SEC_MD, BTN_SEC_LG, "Secondary",
                    UI_BTN_SECONDARY, false);

            section_label("GHOST", 2);
            btn_row(BTN_GHO_SM, BTN_GHO_MD, BTN_GHO_LG, "Ghost",
                    UI_BTN_GHOST, false);

            section_label("DANGER", 3);
            btn_row(BTN_DAN_SM, BTN_DAN_MD, BTN_DAN_LG, "Danger",
                    UI_BTN_DANGER, false);

            /* Thin separator */
            CLAY(CLAY_ID("Sep"), {
                                     .layout = { .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIXED(1) } },
                                     .backgroundColor = COL_OVERLAY,
                                 })
            {
            }

            section_label("DISABLED", 4);
            disabled_row();
        }

        /* Spacer */
        CLAY(CLAY_ID("Spacer"), {
                                    .layout = { .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIXED(16) } },
                                })
        {
        }

        /* Click counter */
        counter_card();
    }
}

/* ---- Entry point ---- */
int main(int argc, char *argv[])
{
    (void)argc;
    (void)argv;

    ClayGPUCtx ctx;
    static const char *font_candidates[] = {
        "assets/Inter-Regular.ttf",
        "/usr/share/fonts/truetype/noto/NotoSans-Regular.ttf",
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
        "/usr/share/fonts/opentype/noto/NotoSans-Regular.ttf",
        NULL
    };
    const char *font = ClayGPUCtx_find_font(font_candidates);
    if (!font) {
        SDL_Log("No usable font found");
        return 1;
    }
    SDL_Log("Using font: %s", font);
    if (!ClayGPUCtx_init(&ctx, "Button Gallery", 640, 600, font, 16)) {
        SDL_Log("ClayGPUCtx_init failed");
        return 1;
    }

    bool running = true;
    bool dirty = true;
    bool mouse_down = false;
    bool mouse_released = false;
    Uint64 last = SDL_GetPerformanceCounter();

    while (running) {
        SDL_Event e;
        mouse_released = false;

        if (dirty) {
            while (SDL_PollEvent(&e)) {
                if (e.type == SDL_EVENT_QUIT) {
                    running = false;
                    break;
                }
                if (e.type == SDL_EVENT_KEY_DOWN && e.key.key == SDLK_ESCAPE) {
                    running = false;
                    break;
                }
                if (e.type == SDL_EVENT_MOUSE_BUTTON_DOWN)
                    mouse_down = true;
                if (e.type == SDL_EVENT_MOUSE_BUTTON_UP) {
                    mouse_down = false;
                    mouse_released = true;
                }
                ClayGPUCtx_handle_event(&ctx, &e);
                dirty = true;
            }
        } else {
            if (!SDL_WaitEvent(&e))
                break;
            dirty = true;
            if (e.type == SDL_EVENT_QUIT) {
                running = false;
                break;
            }
            if (e.type == SDL_EVENT_KEY_DOWN && e.key.key == SDLK_ESCAPE) {
                running = false;
                break;
            }
            if (e.type == SDL_EVENT_MOUSE_BUTTON_DOWN)
                mouse_down = true;
            if (e.type == SDL_EVENT_MOUSE_BUTTON_UP) {
                mouse_down = false;
                mouse_released = true;
            }
            ClayGPUCtx_handle_event(&ctx, &e);
            while (SDL_PollEvent(&e)) {
                if (e.type == SDL_EVENT_QUIT) {
                    running = false;
                    break;
                }
                if (e.type == SDL_EVENT_KEY_DOWN && e.key.key == SDLK_ESCAPE) {
                    running = false;
                    break;
                }
                if (e.type == SDL_EVENT_MOUSE_BUTTON_DOWN)
                    mouse_down = true;
                if (e.type == SDL_EVENT_MOUSE_BUTTON_UP) {
                    mouse_down = false;
                    mouse_released = true;
                }
                ClayGPUCtx_handle_event(&ctx, &e);
            }
        }

        if (!running)
            break;

        if (dirty) {
            /* Update counter string */
            if (g_total_clicks == 0)
                snprintf(g_count_str, sizeof(g_count_str), "Click any button");
            else
                snprintf(g_count_str, sizeof(g_count_str),
                         "Total clicks: %d", g_total_clicks);

            Uint64 now = SDL_GetPerformanceCounter();
            float dt = (float)(now - last) / (float)SDL_GetPerformanceFrequency();
            last = now;

            UI_SetMouseState(mouse_down, mouse_released);

            Clay_BeginLayout();
            ui_build();
            Clay_RenderCommandArray cmds = Clay_EndLayout(dt);
            ClayGPUCtx_render(&ctx, cmds);

            /* Only go idle if mouse is not held (pressed state needs redraw) */
            dirty = mouse_down;
        }
    }

    ClayGPUCtx_destroy(&ctx);
    return 0;
}
