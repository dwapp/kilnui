/* demo/component_gallery.c - KilnUI component gallery.
 *
 * Shows the reusable UI components together in one Clay + SDL3 GPU demo.
 * Build: cmake --build build --target component_gallery
 * Run:   ./build/component_gallery
 */

#include "../src/kilnui.h"
#include "../src/clay_colors.h"
#include "../src/ui/button.h"
#include "../src/ui/checkbox.h"
#include "../src/ui/container.h"
#include "../src/ui/dropdown.h"
#include "../src/ui/input.h"
#include "../src/ui/label.h"
#include "../src/ui/progress.h"
#include "../src/ui/radio.h"
#include "../src/ui/slider.h"
#include "../src/ui/tooltip.h"
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <stdio.h>

enum
{
    ID_BTN_PRIMARY = 1,
    ID_BTN_SECONDARY,
    ID_BTN_GHOST,
    ID_BTN_DANGER,
    ID_CHECK_A,
    ID_CHECK_B,
    ID_RADIO_A,
    ID_RADIO_B,
    ID_RADIO_C,
    ID_INPUT_NAME,
    ID_INPUT_EMAIL,
    ID_SLIDER_VOLUME,
    ID_SLIDER_PROGRESS,
    ID_DROPDOWN,
    ID_PROGRESS,
    ID_TOOLTIP,
};

static bool  g_enabled = true;
static bool  g_notify = false;
static int   g_mode = 0;
static bool  g_input_name_focus = false;
static bool  g_input_email_focus = false;
static float g_volume = 0.62f;
static float g_progress = 0.38f;
static bool  g_dropdown_open = false;
static int   g_dropdown_choice = 1;
static int   g_clicks = 0;
static char  g_status[160] = "Interact with any control";

static const char *const g_options[] = {
    "Compact",
    "Comfortable",
    "Dense",
};

static void separator(int uid)
{
    CLAY(CLAY_SIDI(CLAY_STRING("Sep"), uid), {
        .layout = { .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIXED(1) } },
        .backgroundColor = COL_OVERLAY,
    }) {}
}

static void button_row(void)
{
    UI_Label(10, "Buttons", UI_LABEL_CAPTION);
    UI_Container(20, UI_CONTAINER_ROW) {
        if (UI_Button(ID_BTN_PRIMARY, "Primary", UI_BTN_PRIMARY, UI_BTN_MD, false)) {
            g_clicks++;
            snprintf(g_status, sizeof(g_status), "Primary button clicked");
        }
        if (UI_Button(ID_BTN_SECONDARY, "Secondary", UI_BTN_SECONDARY, UI_BTN_MD, false)) {
            g_clicks++;
            snprintf(g_status, sizeof(g_status), "Secondary button clicked");
        }
        if (UI_Button(ID_BTN_GHOST, "Ghost", UI_BTN_GHOST, UI_BTN_MD, false)) {
            g_clicks++;
            snprintf(g_status, sizeof(g_status), "Ghost button clicked");
        }
        if (UI_Button(ID_BTN_DANGER, "Danger", UI_BTN_DANGER, UI_BTN_MD, false)) {
            g_clicks++;
            snprintf(g_status, sizeof(g_status), "Danger button clicked");
        }
    }
}

static void choice_panel(void)
{
    UI_Container(30, UI_CONTAINER_CARD) {
        UI_Label(31, "Selection", UI_LABEL_HEADING);
        UI_Label(32, "Checkbox and radio components", UI_LABEL_MUTED);

        separator(33);

        if (UI_Checkbox(ID_CHECK_A, "Enable feature", g_enabled, false)) {
            g_enabled = !g_enabled;
            snprintf(g_status, sizeof(g_status), "Enable feature: %s", g_enabled ? "on" : "off");
        }
        if (UI_Checkbox(ID_CHECK_B, "Send notifications", g_notify, false)) {
            g_notify = !g_notify;
            snprintf(g_status, sizeof(g_status), "Notifications: %s", g_notify ? "on" : "off");
        }

        separator(34);

        if (UI_Radio(ID_RADIO_A, "Mode A", g_mode == 0, false)) {
            g_mode = 0;
            snprintf(g_status, sizeof(g_status), "Mode A selected");
        }
        if (UI_Radio(ID_RADIO_B, "Mode B", g_mode == 1, false)) {
            g_mode = 1;
            snprintf(g_status, sizeof(g_status), "Mode B selected");
        }
        if (UI_Radio(ID_RADIO_C, "Mode C", g_mode == 2, false)) {
            g_mode = 2;
            snprintf(g_status, sizeof(g_status), "Mode C selected");
        }
    }
}

static void form_panel(void)
{
    UI_Container(40, UI_CONTAINER_CARD) {
        UI_Label(41, "Form", UI_LABEL_HEADING);
        UI_Label(42, "Input, dropdown, slider, progress, and tooltip", UI_LABEL_MUTED);

        separator(43);

        UIInputResult name = UI_Input(ID_INPUT_NAME, "Name", "",
                                      "Click to focus name", g_input_name_focus, false);
        if (name.clicked) {
            g_input_name_focus = true;
            g_input_email_focus = false;
            snprintf(g_status, sizeof(g_status), "Name input focused");
        }

        UIInputResult email = UI_Input(ID_INPUT_EMAIL, "Email", "demo@kilnui.local",
                                       "Email address", g_input_email_focus, false);
        if (email.clicked) {
            g_input_email_focus = true;
            g_input_name_focus = false;
            snprintf(g_status, sizeof(g_status), "Email input focused");
        }

        UIDropdownResult dd = UI_Dropdown(ID_DROPDOWN, "Density",
                                          g_options, 3, g_dropdown_choice,
                                          g_dropdown_open, false);
        if (dd.header_clicked)
            g_dropdown_open = !g_dropdown_open;
        if (dd.selected_index >= 0) {
            g_dropdown_choice = dd.selected_index;
            g_dropdown_open = false;
            snprintf(g_status, sizeof(g_status), "Density: %s", g_options[g_dropdown_choice]);
        }

        UISliderResult volume = UI_Slider(ID_SLIDER_VOLUME, "Volume", g_volume,
                                          0.0f, 1.0f, false);
        if (volume.changed) {
            g_volume = volume.value;
            snprintf(g_status, sizeof(g_status), "Volume: %.0f%%", g_volume * 100.0f);
        }

        UISliderResult progress = UI_Slider(ID_SLIDER_PROGRESS, "Progress source",
                                            g_progress, 0.0f, 1.0f, false);
        if (progress.changed) {
            g_progress = progress.value;
            snprintf(g_status, sizeof(g_status), "Progress source: %.0f%%", g_progress * 100.0f);
        }

        UI_Progress(ID_PROGRESS, "Progress", g_progress, 1.0f);
        UI_Tooltip(ID_TOOLTIP, "Tooltip: inline bubble using only text and rectangles", true);
    }
}

static void status_bar(void)
{
    char buf[256];
    snprintf(buf, sizeof(buf), "%s | clicks=%d | mode=%c | volume=%.0f%%",
             g_status, g_clicks, (char)('A' + g_mode), g_volume * 100.0f);

    UI_Container(50, UI_CONTAINER_PANEL) {
        UI_Label(51, "Current state", UI_LABEL_CAPTION);
        UI_Label(52, buf, UI_LABEL_BODY);
    }
}

static void ui_build(void)
{
    CLAY(CLAY_ID("Root"), {
        .layout = {
            .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0) },
            .padding = { 32, 32, 26, 26 },
            .childGap = 16,
            .layoutDirection = CLAY_TOP_TO_BOTTOM,
        },
        .backgroundColor = COL_BASE,
    }) {
        CLAY(CLAY_ID("Title"), {
            .layout = {
                .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIXED(44) },
                .childAlignment = { CLAY_ALIGN_X_LEFT, CLAY_ALIGN_Y_CENTER },
            },
        }) {
            CLAY_TEXT(CLAY_STRING("KilnUI Component Gallery"), {
                .textColor = COL_TEXT,
                .fontSize = 26,
            });
        }

        button_row();

        CLAY(CLAY_ID("MainGrid"), {
            .layout = {
                .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0) },
                .childGap = 16,
                .childAlignment = { CLAY_ALIGN_X_LEFT, CLAY_ALIGN_Y_TOP },
            },
        }) {
            CLAY(CLAY_ID("LeftCol"), {
                .layout = {
                    .sizing = { CLAY_SIZING_PERCENT(0.42f), CLAY_SIZING_GROW(0) },
                    .layoutDirection = CLAY_TOP_TO_BOTTOM,
                },
            }) {
                choice_panel();
            }
            CLAY(CLAY_ID("RightCol"), {
                .layout = {
                    .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0) },
                    .layoutDirection = CLAY_TOP_TO_BOTTOM,
                },
            }) {
                form_panel();
            }
        }

        status_bar();
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
    if (!ClayGPUCtx_init(&ctx, "KilnUI Component Gallery", 860, 720, font, 16)) {
        SDL_Log("ClayGPUCtx_init failed");
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
            if (!SDL_WaitEvent(&e))
                break;
            dirty = true;
            if (e.type == SDL_EVENT_QUIT)
                break;
            if (e.type == SDL_EVENT_KEY_DOWN && e.key.key == SDLK_ESCAPE)
                break;
            handle_demo_event(&e, &mouse_down, &mouse_released, &mouse_x, &mouse_y);
            ClayGPUCtx_handle_event(&ctx, &e);
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
            ClayGPUCtx_handle_event(&ctx, &e);
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
            ClayGPUCtx_render(&ctx, cmds);

            dirty = false; /* SDL_WaitEvent drives the next frame */
        }
    }

    ClayGPUCtx_destroy(&ctx);
    return 0;
}
