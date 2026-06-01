/* demo/design_system_demo.c — KilnUI Design System Showcase
 *
 * Demonstrates all design system components:
 *   - Spacing scale swatches
 *   - Corner radius previews
 *   - Typography scale (all 10 presets)
 *   - Color palette (surfaces + semantic variants)
 *   - Icon gallery (by category)
 *   - Badge & Chip variants
 *   - Alert types
 *   - Avatar sizes
 *   - Theme toggle (dark / light)
 *
 * Build:  cmake --build build --target design_system_demo
 * Run:    ./build/design_system_demo
 */

#include "../src/kilnui.h"
#include "../src/ui/ui.h"
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <stdio.h>

/* ---- State ---- */
static bool  g_dark_mode    = true;
static bool  g_chip_a       = true;
static bool  g_chip_b       = true;
static bool  g_chip_c       = true;
static int   g_section      = 0;   /* active tab */

#define NUM_SECTIONS 7
static const char *const g_section_labels[NUM_SECTIONS] = {
    "Colors", "Typography", "Spacing", "Icons", "Components", "Badges", "Alerts"
};

/* ================================================================
 * Color section
 * ================================================================ */
static void section_colors(void)
{
    /* Surface palette */
    TY_Text(100, "Surfaces", TY_H3);

    UI_ROW(101, 8) {
        UI_ColorSwatch(110, ds_theme->crust,    48, 48, DS_RADIUS_MD);
        UI_ColorSwatch(111, ds_theme->mantle,   48, 48, DS_RADIUS_MD);
        UI_ColorSwatch(112, ds_theme->base,     48, 48, DS_RADIUS_MD);
        UI_ColorSwatch(113, ds_theme->surface0, 48, 48, DS_RADIUS_MD);
        UI_ColorSwatch(114, ds_theme->surface1, 48, 48, DS_RADIUS_MD);
        UI_ColorSwatch(115, ds_theme->surface2, 48, 48, DS_RADIUS_MD);
        UI_ColorSwatch(116, ds_theme->overlay,  48, 48, DS_RADIUS_MD);
    }

    TY_Text(120, "Text", TY_H3);
    UI_ROW(121, 8) {
        UI_ColorSwatch(122, ds_theme->text,    48, 48, DS_RADIUS_MD);
        UI_ColorSwatch(123, ds_theme->subtext, 48, 48, DS_RADIUS_MD);
        UI_ColorSwatch(124, ds_theme->muted,   48, 48, DS_RADIUS_MD);
    }

    TY_Text(130, "Semantic", TY_H3);
    UI_ROW(131, 8) {
        UI_ColorSwatch(132, ds_theme->accent,    48, 48, DS_RADIUS_MD);
        UI_ColorSwatch(133, ds_theme->accent_alt,48, 48, DS_RADIUS_MD);
        UI_ColorSwatch(134, ds_theme->success,   48, 48, DS_RADIUS_MD);
        UI_ColorSwatch(135, ds_theme->warning,   48, 48, DS_RADIUS_MD);
        UI_ColorSwatch(136, ds_theme->error,     48, 48, DS_RADIUS_MD);
        UI_ColorSwatch(137, ds_theme->info,      48, 48, DS_RADIUS_MD);
    }

    TY_Text(140, "Variant Backgrounds", TY_H3);
    UI_ROW(141, 8) {
        for (int v = 0; v < DS_VARIANT_COUNT; v++) {
            UI_ColorSwatch(150 + v, DS_VariantBg((DSVariant)v), 48, 48, DS_RADIUS_MD);
        }
    }
}

/* ================================================================
 * Typography section
 * ================================================================ */
static void section_typography(void)
{
    static const struct { TYStyle style; const char *label; const char *sample; } rows[] = {
        { TY_DISPLAY,  "Display  40px",  "The quick brown fox"    },
        { TY_H1,       "H1  32px",       "The quick brown fox"    },
        { TY_H2,       "H2  24px",       "The quick brown fox"    },
        { TY_H3,       "H3  20px",       "The quick brown fox"    },
        { TY_H4,       "H4  16px",       "The quick brown fox"    },
        { TY_BODY,     "Body  14px",     "The quick brown fox jumps over the lazy dog." },
        { TY_SMALL,    "Small  12px",    "The quick brown fox jumps over the lazy dog." },
        { TY_CAPTION,  "Caption  10px",  "The quick brown fox jumps over the lazy dog." },
        { TY_CODE,     "Code  14px",     "fn main() { println!(\"hello\"); }"           },
        { TY_OVERLINE, "Overline  10px", "SECTION LABEL"          },
    };
    for (int i = 0; i < 10; i++) {
        UI_ROW(200 + i * 3, 16) {
            /* Style tag */
            CLAY(CLAY_SIDI(CLAY_STRING("TyTag"), 200 + i * 3 + 1), {
                .layout = {
                    .sizing  = { CLAY_SIZING_FIXED(130), CLAY_SIZING_FIT(0) },
                    .childAlignment = { CLAY_ALIGN_X_LEFT, CLAY_ALIGN_Y_CENTER },
                },
            }) {
                CLAY_TEXT(UI__str(rows[i].label),
                          { .textColor = ds_theme->muted, .fontSize = 11 });
            }
            TY_Text(200 + i * 3 + 2, rows[i].sample, rows[i].style);
        }
    }

    UI_Divider(290);
    TY_Text(291, "Label-Value pairs", TY_H3);
    UI_ROW(292, 32) {
        TY_LabelValue(293, "Username",  "kilnui_user");
        TY_LabelValue(294, "Status",    "Active");
        TY_LabelValue(295, "Version",   "0.4.0");
        TY_LabelValue(296, "License",   "MIT");
    }
}

/* ================================================================
 * Spacing section
 * ================================================================ */
static void section_spacing(void)
{
    static const struct { int val; const char *label; } spaces[] = {
        { DS_SPACE_1,  "space-1  4px"  },
        { DS_SPACE_2,  "space-2  8px"  },
        { DS_SPACE_3,  "space-3  12px" },
        { DS_SPACE_4,  "space-4  16px" },
        { DS_SPACE_5,  "space-5  20px" },
        { DS_SPACE_6,  "space-6  24px" },
        { DS_SPACE_8,  "space-8  32px" },
        { DS_SPACE_10, "space-10 40px" },
        { DS_SPACE_12, "space-12 48px" },
        { DS_SPACE_16, "space-16 64px" },
    };
    TY_Text(300, "Spacing Scale", TY_H3);
    for (int i = 0; i < 10; i++) {
        UI_ROW(310 + i * 3, 12) {
            /* Bar */
            CLAY(CLAY_SIDI(CLAY_STRING("SpBar"), 310 + i * 3 + 1), {
                .layout = {
                    .sizing = {
                        CLAY_SIZING_FIXED((float)spaces[i].val),
                        CLAY_SIZING_FIXED(20),
                    },
                },
                .backgroundColor = ds_theme->accent,
                .cornerRadius    = CLAY_CORNER_RADIUS(DS_RADIUS_XS),
            }) {}
            /* Label */
            CLAY(CLAY_SIDI(CLAY_STRING("SpLbl"), 310 + i * 3 + 2), {
                .layout = { .sizing = { CLAY_SIZING_FIT(0), CLAY_SIZING_FIT(0) },
                            .childAlignment = { CLAY_ALIGN_X_LEFT, CLAY_ALIGN_Y_CENTER } },
            }) {
                CLAY_TEXT(UI__str(spaces[i].label),
                          { .textColor = ds_theme->subtext, .fontSize = 12 });
            }
        }
    }

    UI_Divider(380);
    TY_Text(381, "Corner Radii", TY_H3);
    UI_ROW(382, 16) {
        static const struct { int r; const char *label; } radii[] = {
            { DS_RADIUS_NONE, "none" },
            { DS_RADIUS_XS,   "xs 2" },
            { DS_RADIUS_SM,   "sm 4" },
            { DS_RADIUS_MD,   "md 8" },
            { DS_RADIUS_LG,   "lg 12" },
            { DS_RADIUS_XL,   "xl 16" },
            { DS_RADIUS_2XL,  "2xl 24" },
            { DS_RADIUS_FULL, "full" },
        };
        for (int i = 0; i < 8; i++) {
            CLAY(CLAY_SIDI(CLAY_STRING("RadWrap"), 390 + i), {
                .layout = {
                    .sizing = { CLAY_SIZING_FIT(0), CLAY_SIZING_FIT(0) },
                    .layoutDirection = CLAY_TOP_TO_BOTTOM,
                    .childAlignment  = { CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_TOP },
                    .childGap = 6,
                },
            }) {
                CLAY(CLAY_SIDI(CLAY_STRING("Rad"), 390 + i), {
                    .layout = { .sizing = { CLAY_SIZING_FIXED(48), CLAY_SIZING_FIXED(48) } },
                    .backgroundColor = ds_theme->accent,
                    .cornerRadius    = CLAY_CORNER_RADIUS((float)radii[i].r),
                }) {}
                CLAY(CLAY_SIDI(CLAY_STRING("RadLbl"), 490 + i), {
                    .layout = { .sizing = { CLAY_SIZING_FIT(0), CLAY_SIZING_FIT(0) } },
                }) {
                    CLAY_TEXT(UI__str(radii[i].label),
                              { .textColor = ds_theme->muted, .fontSize = 10 });
                }
            }
        }
    }
}

/* ================================================================
 * Icons section
 * ================================================================ */
static void section_icons(void)
{
    struct { const char *category; const char *icons[12]; } groups[] = {
        { "Navigation", {
            ICON_ARROW_LEFT, ICON_ARROW_RIGHT, ICON_ARROW_UP, ICON_ARROW_DOWN,
            ICON_CHEVRON_LEFT, ICON_CHEVRON_RIGHT, ICON_TRI_UP, ICON_TRI_DOWN,
            ICON_HOME, ICON_ARROW_BACK, ICON_ARROW_REFRESH, ICON_EXPAND_H,
        }},
        { "Actions", {
            ICON_PLUS, ICON_MINUS, ICON_CLOSE, ICON_CHECK,
            ICON_EDIT, ICON_CUT, ICON_DELETE, ICON_SEARCH,
            ICON_ADD_CIRCLE, ICON_REMOVE_CIRCLE, ICON_LOCK, ICON_PIN,
        }},
        { "UI Controls", {
            ICON_MENU, ICON_MORE_V, ICON_MORE_H, ICON_SETTINGS,
            ICON_FILTER, ICON_SORT, ICON_GRID, ICON_LIST,
            ICON_TOGGLE_ON, ICON_CHECKBOX_ON, ICON_RADIO_ON, ICON_KEYBOARD,
        }},
        { "Status", {
            ICON_WARNING, ICON_INFO, ICON_SUCCESS, ICON_ERROR,
            ICON_STAR, ICON_STAR_EMPTY, ICON_HEART, ICON_HEART_EMPTY,
            ICON_BOLT, ICON_HOURGLASS, ICON_SPINNER, ICON_STOP_SIGN,
        }},
        { "Media", {
            ICON_PLAY, ICON_PAUSE, ICON_STOP, ICON_RECORD,
            ICON_SKIP_NEXT, ICON_SKIP_PREV, ICON_FAST_FORWARD, ICON_REWIND,
            ICON_EJECT, ICON_VOLUME, ICON_MIC, NULL,
        }},
    };

    for (int g = 0; g < 5; g++) {
        TY_Text(500 + g * 20, groups[g].category, TY_CAPTION);
        UI_ROW(501 + g * 20, 6) {
            for (int i = 0; i < 12; i++) {
                if (!groups[g].icons[i]) break;
                UI_Icon(510 + g * 20 + i, groups[g].icons[i],
                        DS_FS_LG, ds_theme->subtext);
            }
        }
    }
}

/* ================================================================
 * Components section
 * ================================================================ */
static void section_components(void)
{
    TY_Text(600, "Buttons", TY_H3);
    UI_ROW(601, 8) {
        UI_Button(610, "Primary",   UI_BTN_PRIMARY,   UI_BTN_MD, false);
        UI_Button(611, "Secondary", UI_BTN_SECONDARY, UI_BTN_MD, false);
        UI_Button(612, "Ghost",     UI_BTN_GHOST,     UI_BTN_MD, false);
        UI_Button(613, "Danger",    UI_BTN_DANGER,    UI_BTN_MD, false);
        UI_Button(614, "Disabled",  UI_BTN_PRIMARY,   UI_BTN_MD, true);
    }

    UI_ROW(615, 8) {
        UI_Button(616, "SM", UI_BTN_PRIMARY, UI_BTN_SM, false);
        UI_Button(617, "MD", UI_BTN_PRIMARY, UI_BTN_MD, false);
        UI_Button(618, "LG", UI_BTN_PRIMARY, UI_BTN_LG, false);
    }

    TY_Text(620, "Icon Buttons", TY_H3);
    UI_ROW(621, 8) {
        UI_IconButton(630, ICON_SETTINGS, DS_FS_LG, UI_BTN_GHOST,     false);
        UI_IconButton(631, ICON_EDIT,     DS_FS_LG, UI_BTN_SECONDARY, false);
        UI_IconButton(632, ICON_DELETE,   DS_FS_LG, UI_BTN_DANGER,    false);
        UI_IconButton(633, ICON_CHECK,    DS_FS_LG, UI_BTN_PRIMARY,   false);
        UI_IconButton(634, ICON_CLOSE,    DS_FS_LG, UI_BTN_GHOST,     true);
    }

    TY_Text(640, "Avatars", TY_H3);
    UI_ROW(641, 12) {
        UI_Avatar(650, "JD", DS_VARIANT_PRIMARY,  DS_HEIGHT_SM);
        UI_Avatar(651, "AB", DS_VARIANT_SUCCESS,  DS_HEIGHT_MD);
        UI_Avatar(652, "XY", DS_VARIANT_ERROR,    DS_HEIGHT_LG);
        UI_Avatar(653, "KL", DS_VARIANT_WARNING,  DS_HEIGHT_XL);
        UI_Avatar(654, "?",  DS_VARIANT_MUTED,    DS_HEIGHT_SM);
    }
}

/* ================================================================
 * Badges section
 * ================================================================ */
static void section_badges(void)
{
    TY_Text(700, "Badges", TY_H3);
    UI_ROW(701, 8) {
        UI_Badge(710, "Default", DS_VARIANT_DEFAULT);
        UI_Badge(711, "Primary", DS_VARIANT_PRIMARY);
        UI_Badge(712, "Success", DS_VARIANT_SUCCESS);
        UI_Badge(713, "Warning", DS_VARIANT_WARNING);
        UI_Badge(714, "Error",   DS_VARIANT_ERROR);
        UI_Badge(715, "Info",    DS_VARIANT_INFO);
        UI_Badge(716, "Muted",   DS_VARIANT_MUTED);
        UI_Badge(717, "Accent",  DS_VARIANT_ACCENT);
    }

    UI_ROW(720, 8) {
        UI_Badge(721, "New",    DS_VARIANT_PRIMARY);
        UI_Badge(722, "12",     DS_VARIANT_ERROR);
        UI_Badge(723, "Beta",   DS_VARIANT_WARNING);
        UI_Badge(724, "v2.1",   DS_VARIANT_INFO);
        UI_Badge(725, "Active", DS_VARIANT_SUCCESS);
        UI_Badge(726, "Deprecated", DS_VARIANT_MUTED);
    }

    TY_Text(730, "Chips", TY_H3);
    UI_ROW(731, 8) {
        if (!UI_Chip(740, "dark-mode",   g_chip_a, true))  g_chip_a = false;
        if (!UI_Chip(741, "typescript",  g_chip_b, true))  g_chip_b = false;
        if (!UI_Chip(742, "open-source", g_chip_c, true))  g_chip_c = false;
        UI_Chip(743, "selected",  true,  false);
        UI_Chip(744, "unselected",false, false);
    }

    TY_Text(750, "Dividers", TY_H3);
    UI_Divider(751);
    UI_DividerLabel(752, "OR");
    UI_Divider(753);
    UI_DividerLabel(754, "CONTINUE WITH");
}

/* ================================================================
 * Alerts section
 * ================================================================ */
static void section_alerts(void)
{
    TY_Text(800, "Alerts", TY_H3);
    UI_Alert(810, ICON_INFO    " This is an informational message.",   DS_VARIANT_INFO);
    UI_Alert(811, ICON_SUCCESS " Operation completed successfully.",   DS_VARIANT_SUCCESS);
    UI_Alert(812, ICON_WARNING " Your session will expire in 5 minutes.", DS_VARIANT_WARNING);
    UI_Alert(813, ICON_ERROR   " Failed to connect to the server.",   DS_VARIANT_ERROR);
    UI_Alert(814, ICON_STAR    " You have unlocked a new achievement!", DS_VARIANT_ACCENT);
    UI_Alert(815, ICON_INFO    " Default style notification.",         DS_VARIANT_DEFAULT);
}

/* ================================================================
 * Tab bar
 * ================================================================ */
static void tab_bar(void)
{
    UI_ROW(10, 4) {
        for (int i = 0; i < NUM_SECTIONS; i++) {
            bool active = (g_section == i);
            Clay_ElementId tid = Clay_GetElementIdWithIndex(CLAY_STRING("Tab"), i);
            bool hov = Clay_PointerOver(tid);
            if (hov && UI__mouse_released) g_section = i;

            Clay_Color bg = active ? ds_theme->surface1
                          : hov    ? ds_theme->surface0
                          :          (Clay_Color){0,0,0,0};
            Clay_Color fg = active ? ds_theme->accent : ds_theme->subtext;

            CLAY(tid, {
                .layout = {
                    .sizing  = { CLAY_SIZING_FIT(DS_SPACE_6), CLAY_SIZING_FIXED(32) },
                    .padding = { DS_SPACE_3, DS_SPACE_3, 0, 0 },
                    .childAlignment = { CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER },
                },
                .backgroundColor = bg,
                .cornerRadius    = CLAY_CORNER_RADIUS(DS_RADIUS_SM),
                .border = {
                    .color = ds_theme->accent,
                    .width = { .bottom = (uint16_t)(active ? 2 : 0) },
                },
            }) {
                CLAY_TEXT(UI__str(g_section_labels[i]),
                          { .textColor = fg, .fontSize = 13 });
            }
        }

        UI_SPACER(19);

        /* Theme toggle */
        Clay_ElementId thm_id = Clay_GetElementIdWithIndex(CLAY_STRING("ThemeToggle"), 0);
        bool thm_hov = Clay_PointerOver(thm_id);
        if (thm_hov && UI__mouse_released)
            DS_SetTheme(g_dark_mode ? &DS_THEME_LIGHT : &DS_THEME_DARK);
        if (thm_hov && UI__mouse_released)
            g_dark_mode = !g_dark_mode;

        CLAY(thm_id, {
            .layout = {
                .sizing  = { CLAY_SIZING_FIT(0), CLAY_SIZING_FIXED(32) },
                .padding = { DS_SPACE_3, DS_SPACE_3, 0, 0 },
                .childGap = 6,
                .childAlignment = { CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER },
            },
            .backgroundColor = thm_hov ? ds_theme->surface1 : (Clay_Color){0,0,0,0},
            .cornerRadius    = CLAY_CORNER_RADIUS(DS_RADIUS_SM),
        }) {
            CLAY_TEXT(g_dark_mode ? CLAY_STRING("\xe2\x98\x80") /* ☀ */
                                  : CLAY_STRING("\xe2\x98\xbd") /* ☽ */,
                      { .textColor = ds_theme->subtext, .fontSize = 14 });
            CLAY_TEXT(g_dark_mode ? CLAY_STRING("Light") : CLAY_STRING("Dark"),
                      { .textColor = ds_theme->subtext, .fontSize = 12 });
        }
    }
}

/* ================================================================
 * Full UI build
 * ================================================================ */
static void ui_build(void)
{
    CLAY(CLAY_ID("Root"), {
        .layout = {
            .sizing          = { CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0) },
            .padding         = { DS_SPACE_6, DS_SPACE_6, DS_SPACE_4, DS_SPACE_4 },
            .childGap        = DS_SPACE_4,
            .layoutDirection = CLAY_TOP_TO_BOTTOM,
        },
        .backgroundColor = ds_theme->base,
    }) {
        /* Header */
        UI_ROW(1, 12) {
            TY_Text(2, "KilnUI", TY_DISPLAY);
            UI_SPACER(3);
            TY_Text(4, "Design System", TY_H2);
        }
        UI_Divider(5);

        /* Tab bar */
        tab_bar();
        UI_Divider(20);

        /* Content (scrollable) */
        UI_SCROLLCOL(30, DS_SPACE_4) {
            switch (g_section) {
                case 0: section_colors();     break;
                case 1: section_typography(); break;
                case 2: section_spacing();    break;
                case 3: section_icons();      break;
                case 4: section_components(); break;
                case 5: section_badges();     break;
                case 6: section_alerts();     break;
            }
        }
    }
}

/* ================================================================
 * Main
 * ================================================================ */
int main(int argc, char *argv[])
{
    (void)argc; (void)argv;

    KilnUI ctx;
    static const char *fonts[] = {
        "assets/Inter-Regular.ttf",
        "/usr/share/fonts/truetype/noto/NotoSans-Regular.ttf",
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
        "/usr/share/fonts/opentype/noto/NotoSans-Regular.ttf",
        NULL
    };
    const char *font = KilnUI_find_font(fonts);
    if (!font) { SDL_Log("No font found"); return 1; }
    if (!KilnUI_init(&ctx, "KilnUI Design System", 1100, 760, font, 14)) return 1;

    bool running = true;
    bool dirty   = true;
    bool mouse_down = false, mouse_released = false;
    float mx = 0, my = 0;
    Uint64 last = SDL_GetPerformanceCounter();

    while (running) {
        SDL_Event e;
        mouse_released = false;

        if (!dirty) {
            if (!SDL_WaitEvent(&e)) break;
            dirty = true;
            if (e.type == SDL_EVENT_QUIT) break;
            if (e.type == SDL_EVENT_KEY_DOWN && e.key.key == SDLK_ESCAPE) break;
            if (e.type == SDL_EVENT_MOUSE_MOTION)
                { mx = e.motion.x; my = e.motion.y; }
            else if (e.type == SDL_EVENT_MOUSE_BUTTON_DOWN)
                { mouse_down = true;  mx = e.button.x; my = e.button.y; }
            else if (e.type == SDL_EVENT_MOUSE_BUTTON_UP)
                { mouse_down = false; mouse_released = true; mx = e.button.x; my = e.button.y; }
            KilnUI_handle_event(&ctx, &e);
        }

        while (SDL_PollEvent(&e)) {
            dirty = true;
            if (e.type == SDL_EVENT_QUIT)   { running = false; break; }
            if (e.type == SDL_EVENT_KEY_DOWN && e.key.key == SDLK_ESCAPE) { running = false; break; }
            if (e.type == SDL_EVENT_MOUSE_MOTION)
                { mx = e.motion.x; my = e.motion.y; }
            else if (e.type == SDL_EVENT_MOUSE_BUTTON_DOWN)
                { mouse_down = true;  mx = e.button.x; my = e.button.y; }
            else if (e.type == SDL_EVENT_MOUSE_BUTTON_UP)
                { mouse_down = false; mouse_released = true; mx = e.button.x; my = e.button.y; }
            KilnUI_handle_event(&ctx, &e);
        }
        if (!running) break;

        if (dirty) {
            Uint64 now = SDL_GetPerformanceCounter();
            float dt = (float)(now - last) / (float)SDL_GetPerformanceFrequency();
            last = now;
            UI_SetPointerState(mouse_down, mouse_released, mx, my);

            Clay_BeginLayout();
            ui_build();
            Clay_RenderCommandArray cmds = Clay_EndLayout(dt);
            KilnUI_render(&ctx, cmds);
            dirty = false;
        }
    }

    KilnUI_destroy(&ctx);
    return 0;
}
