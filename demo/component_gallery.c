/* SPDX-License-Identifier: MIT */
/* demo/component_gallery.c - KilnUI component gallery.
 *
 * Shows the reusable UI components together in one Clay + SDL3 GPU demo.
 * Build: cmake --build build --target component_gallery
 * Run:   ./build/component_gallery
 */

#include "../src/kilnui.h"
#include "../src/ui/button.h"
#include "../src/ui/checkbox.h"
#include "../src/ui/container.h"
#include "../src/ui/dropdown.h"
#include "../src/ui/input.h"
#include "../src/ui/progress.h"
#include "../src/ui/radio.h"
#include "../src/ui/slider.h"
#include "../src/ui/tooltip.h"
#include "../src/ui/ui.h"
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <stdio.h>

/* Debug logging */
static int g_dbg_frame = 0;
#define DBG(fmt, ...) fprintf(stderr, "[frame %d] " fmt "\n", g_dbg_frame, ##__VA_ARGS__)

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

static bool g_enabled = true;
static bool g_notify = false;
static int g_mode = 0;
static bool g_input_name_focus = false;
static bool g_input_email_focus = false;
static char g_input_name_buf[128] = "";
static char g_input_email_buf[128] = "demo@kilnui.local";
static float g_volume = 0.62f;
static float g_progress = 0.38f;
static bool g_dropdown_open = false;
static int g_dropdown_choice = 1;
static int g_clicks = 0;
static char g_status[160] = "Interact with any control";

static const char *const g_options[] = {
    "Compact",
    "Comfortable",
    "Dense",
};

static void separator(int uid)
{
    CLAY(CLAY_SIDI(CLAY_STRING("Sep"), uid), {
                                                 .layout = { .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIXED(1) } },
                                                 .backgroundColor = ds_theme->overlay,
                                             })
    {
    }
}

static void button_row(void)
{
    TY_Text(10, "Buttons", TY_CAPTION);
    UI_Container(20, UI_CONTAINER_ROW)
    {
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
    UI_Container(30, UI_CONTAINER_CARD)
    {
        TY_Text(31, "Selection", TY_H3);
        TY_Text(32, "Checkbox and radio components", TY_BODY);

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

static void composite_panel(void)
{
    UI_Container(60, UI_CONTAINER_CARD)
    {
        TY_Text(61, "Composite UI Patterns", TY_H3);
        TY_Text(62, "Stat cards and scrollable lists", TY_BODY);

        separator(63);

        UI_ROW(64, 8)
        {
            UI_StatCard(0, "Active Users", "1,240", ds_theme->mauve);
            UI_StatCard(1, "Server Load", "42%", ds_theme->peach);
        }

        UI_SPACER(8);

        CLAY(CLAY_ID("EventLogPanel"), {
                                           .layout = {
                                               .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIXED(160) },
                                               .padding = { 12, 12, 12, 12 },
                                               .childGap = 4,
                                               .layoutDirection = CLAY_TOP_TO_BOTTOM,
                                           },
                                           .backgroundColor = ds_theme->surface0,
                                           .cornerRadius = CLAY_CORNER_RADIUS(DS_RADIUS_MD),
                                           .clip = { .vertical = true, .childOffset = Clay_GetScrollOffset() },
                                       })
        {
            const char *events[] = {
                "System initialized", "Network connected", "Data synced",
                "User logged in", "Settings updated", "Profile saved",
                "Task completed", "Warning: High memory usage", "Error resolved",
                "Backup created"
            };
            for (int i = 0; i < 10; i++) {
                UI_ListItem(1000 + i, events[i]);
            }
        }
    }
}

static void form_panel(void)
{
    DBG("form_panel: name_focus=%d email_focus=%d name_buf=\"%s\" email_buf=\"%s\"",
        g_input_name_focus, g_input_email_focus, g_input_name_buf, g_input_email_buf);
    UI_Container(40, UI_CONTAINER_CARD)
    {
        TY_Text(41, "Form", TY_H3);
        TY_Text(42, "Input, dropdown, slider, progress, and tooltip", TY_BODY);

        separator(43);

        UIInputResult name = UI_Input(ID_INPUT_NAME, "Name", g_input_name_buf,
                                      "Click to focus name", g_input_name_focus, false);
        DBG("UI_Input(name): clicked=%d focused=%d mouse_released=%d hovered_buf=\"%s\"",
            name.clicked, name.focused, UI__mouse_released, g_input_name_buf);
        if (name.clicked) {
            DBG(">>> Name input CLICKED, setting focus");
            g_input_name_focus = true;
            g_input_email_focus = false;
            snprintf(g_status, sizeof(g_status), "Name input focused");
        }

        UIInputResult email = UI_Input(ID_INPUT_EMAIL, "Email", g_input_email_buf,
                                       "Email address", g_input_email_focus, false);
        DBG("UI_Input(email): clicked=%d focused=%d", email.clicked, email.focused);
        if (email.clicked) {
            DBG(">>> Email input CLICKED, setting focus");
            g_input_email_focus = true;
            g_input_name_focus = false;
            snprintf(g_status, sizeof(g_status), "Email input focused");
        }

        /* If a click happened but neither input was clicked, release focus */
        if (UI__mouse_released && !name.clicked && !email.clicked &&
            (g_input_name_focus || g_input_email_focus)) {
            DBG(">>> Click OUTSIDE inputs, unfocusing");
            g_input_name_focus = false;
            g_input_email_focus = false;
            UI_Input_ResetFocus();
            snprintf(g_status, sizeof(g_status), "Input unfocused");
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

    UI_Container(50, UI_CONTAINER_PANEL)
    {
        TY_Text(51, "Current state", TY_CAPTION);
        TY_Text(52, buf, TY_BODY);
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
                              .backgroundColor = ds_theme->base,
                          })
    {
        CLAY(CLAY_ID("Title"), {
                                   .layout = {
                                       .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIXED(44) },
                                       .childAlignment = { CLAY_ALIGN_X_LEFT, CLAY_ALIGN_Y_CENTER },
                                   },
                               })
        {
            CLAY_TEXT(CLAY_STRING("KilnUI Component Gallery"), {
                                                                   .textColor = ds_theme->text,
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
                                  })
        {
            CLAY(CLAY_ID("LeftCol"), {
                                         .layout = {
                                             .sizing = { CLAY_SIZING_PERCENT(0.42f), CLAY_SIZING_GROW(0) },
                                             .layoutDirection = CLAY_TOP_TO_BOTTOM,
                                         },
                                     })
            {
                choice_panel();
            }
            CLAY(CLAY_ID("RightCol"), {
                                          .layout = {
                                              .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0) },
                                              .layoutDirection = CLAY_TOP_TO_BOTTOM,
                                          },
                                      })
            {
                composite_panel();
                form_panel();
            }
        }

        status_bar();
    }
}

/* Queued text input events — processed after ui_build() so focus state is current */
#define MAX_TEXT_QUEUE 64
typedef struct
{
    char text[32];
} TextEvent;
static TextEvent s_text_queue[MAX_TEXT_QUEUE];
static int s_text_queue_len = 0;

static bool s_backspace_queued = false;

static void handle_demo_event(const SDL_Event *e, bool *mouse_down,
                              bool *mouse_released, float *mouse_x, float *mouse_y)
{
    if (e->type == SDL_EVENT_MOUSE_MOTION) {
        *mouse_x = e->motion.x;
        *mouse_y = e->motion.y;
    } else if (e->type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
        DBG("EVENT: MOUSE_BUTTON_DOWN at (%.0f, %.0f)", e->button.x, e->button.y);
        *mouse_down = true;
        *mouse_x = e->button.x;
        *mouse_y = e->button.y;
    } else if (e->type == SDL_EVENT_MOUSE_BUTTON_UP) {
        DBG("EVENT: MOUSE_BUTTON_UP at (%.0f, %.0f)", e->button.x, e->button.y);
        *mouse_down = false;
        *mouse_released = true;
        *mouse_x = e->button.x;
        *mouse_y = e->button.y;
    } else if (e->type == SDL_EVENT_TEXT_INPUT) {
        DBG("EVENT: TEXT_INPUT text=\"%s\" (queueing, len=%d)", e->text.text, s_text_queue_len);
        /* Queue for processing after ui_build updates focus */
        if (s_text_queue_len < MAX_TEXT_QUEUE) {
            snprintf(s_text_queue[s_text_queue_len].text, 32, "%s", e->text.text);
            s_text_queue_len++;
        }
    } else if (e->type == SDL_EVENT_KEY_DOWN) {
        DBG("EVENT: KEY_DOWN key=%d name=%s", e->key.key, SDL_GetKeyName(e->key.key));
        if (e->key.key == SDLK_BACKSPACE) {
            s_backspace_queued = true;
        }
    }
}

/* Process queued text input — call AFTER ui_build() */
static void process_queued_text_input(void)
{
    if (s_text_queue_len > 0) {
        DBG("process_queued_text_input: %d events, name_focus=%d email_focus=%d",
            s_text_queue_len, g_input_name_focus, g_input_email_focus);
    }

    for (int i = 0; i < s_text_queue_len; i++) {
        char *buf = NULL;
        size_t bufsize = 0;
        if (g_input_name_focus) {
            buf = g_input_name_buf;
            bufsize = sizeof(g_input_name_buf);
        } else if (g_input_email_focus) {
            buf = g_input_email_buf;
            bufsize = sizeof(g_input_email_buf);
        }
        if (buf) {
            size_t len = strlen(buf);
            size_t add = strlen(s_text_queue[i].text);
            if (len + add < bufsize - 1) {
                memcpy(buf + len, s_text_queue[i].text, add + 1);
                DBG("  -> Appended \"%s\", buf now \"%s\"", s_text_queue[i].text, buf);
            }
        } else {
            DBG("  -> No input focused, ignoring \"%s\"", s_text_queue[i].text);
        }
    }
    s_text_queue_len = 0;

    if (s_backspace_queued) {
        char *buf = NULL;
        if (g_input_name_focus)
            buf = g_input_name_buf;
        else if (g_input_email_focus)
            buf = g_input_email_buf;
        if (buf) {
            size_t len = strlen(buf);
            if (len > 0) {
                buf[len - 1] = '\0';
                DBG("  -> Backspace, buf now \"%s\"", buf);
            }
        }
        s_backspace_queued = false;
    }
}

int main(int argc, char *argv[])
{
    (void)argc;
    (void)argv;

    KilnUI ctx;
    static const char *font_candidates[] = {
        "assets/Inter-Regular.ttf",
        "/usr/share/fonts/truetype/noto/NotoSans-Regular.ttf",
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
        "/usr/share/fonts/opentype/noto/NotoSans-Regular.ttf",
        NULL
    };
    const char *font = KilnUI_find_font(font_candidates);
    if (!font) {
        SDL_Log("No usable font found");
        return 1;
    }
    SDL_Log("Using font: %s", font);
    if (!KilnUI_init(&ctx, "KilnUI Component Gallery", 860, 720, font, 16)) {
        SDL_Log("KilnUI_init failed");
        return 1;
    }

    /* Tell UI_Input which window to use for SDL text input */
    UI_SetTextInputWindow(ctx.window);
    DBG("Init: window=%p", (void *)ctx.window);

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
                /* If an input has focus, ESCAPE unfocuses it; otherwise quit */
                if (g_input_name_focus || g_input_email_focus) {
                    DBG("ESCAPE: unfocusing inputs");
                    g_input_name_focus = false;
                    g_input_email_focus = false;
                    UI_Input_ResetFocus();
                } else {
                    DBG("ESCAPE: quitting");
                    running = false;
                    break;
                }
            }
            handle_demo_event(&e, &mouse_down, &mouse_released, &mouse_x, &mouse_y);
            KilnUI_handle_event(&ctx, &e);
        }

        if (!running)
            break;

        if (dirty) {
            g_dbg_frame++;
            Uint64 now = SDL_GetPerformanceCounter();
            float dt = (float)(now - last) / (float)SDL_GetPerformanceFrequency();
            last = now;

            UI_SetPointerState(mouse_down, mouse_released, mouse_x, mouse_y);
            DBG("--- Frame %d start: mouse_down=%d mouse_released=%d ---",
                g_dbg_frame, mouse_down, mouse_released);

            Clay_BeginLayout();
            ui_build();
            Clay_RenderCommandArray cmds = Clay_EndLayout(dt);

            /* Process text input AFTER ui_build so focus state is current */
            process_queued_text_input();

            KilnUI_render(&ctx, cmds);

            dirty = false; /* SDL_WaitEvent drives the next frame */
        }
    }

    KilnUI_destroy(&ctx);
    return 0;
}
