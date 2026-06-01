/* src/ui/icons.h — KilnUI Icon System
 *
 * 120+ Unicode icons encoded as UTF-8 string literals.
 * Works with any font that covers the relevant Unicode blocks
 * (Noto Sans covers all symbols defined here).
 *
 * Categories:
 *   Navigation  — arrows, chevrons, page controls
 *   Actions     — edit, delete, add, copy, share, refresh
 *   UI          — menu, search, filter, settings, close
 *   Objects     — file, folder, image, mail, calendar, user
 *   Status      — check, warning, error, info, star, heart
 *   Media       — play, pause, stop, next, previous
 *   Symbols     — math, misc punctuation
 *
 * Usage:
 *   UI_Icon(uid, ICON_CHECK, DS_FS_LG, ds_theme->success);
 *   if (UI_IconButton(uid, ICON_SETTINGS, DS_FS_LG, UI_BTN_GHOST, false)) { ... }
 */

#ifndef UI_ICONS_H
#define UI_ICONS_H

#include "design_system.h"
#include "button.h"
#include "ui_internal.h"

/* ================================================================
 * Navigation
 * ================================================================ */
#define ICON_ARROW_LEFT     "\xe2\x86\x90"   /* ← U+2190 */
#define ICON_ARROW_RIGHT    "\xe2\x86\x92"   /* → U+2192 */
#define ICON_ARROW_UP       "\xe2\x86\x91"   /* ↑ U+2191 */
#define ICON_ARROW_DOWN     "\xe2\x86\x93"   /* ↓ U+2193 */
#define ICON_ARROW_LEFT2    "\xe2\xac\x85"   /* ⬅ U+2B05 */
#define ICON_ARROW_RIGHT2   "\xe2\x9e\xa1"   /* ➡ U+27A1 */
#define ICON_ARROW_UP2      "\xe2\xac\x86"   /* ⬆ U+2B06 */
#define ICON_ARROW_DOWN2    "\xe2\xac\x87"   /* ⬇ U+2B07 */
#define ICON_ARROW_BACK     "\xe2\x86\xa9"   /* ↩ U+21A9 */
#define ICON_ARROW_FORWARD  "\xe2\x86\xaa"   /* ↪ U+21AA */
#define ICON_ARROW_REFRESH  "\xe2\x86\xba"   /* ↺ U+21BA */
#define ICON_ARROW_REDO     "\xe2\x86\xbb"   /* ↻ U+21BB */
#define ICON_CHEVRON_LEFT   "\xe2\x80\xb9"   /* ‹ U+2039 */
#define ICON_CHEVRON_RIGHT  "\xe2\x80\xba"   /* › U+203A */
#define ICON_CHEVRON_LEFT2  "\xc2\xab"       /* « U+00AB */
#define ICON_CHEVRON_RIGHT2 "\xc2\xbb"       /* » U+00BB */
#define ICON_CHEVRON_UP     "\xe2\x8c\x83"   /* ⌃ U+2303 */
#define ICON_CHEVRON_DOWN   "\xe2\x8c\x84"   /* ⌄ U+2304 */
#define ICON_TRI_UP         "\xe2\x96\xb2"   /* ▲ U+25B2 */
#define ICON_TRI_DOWN       "\xe2\x96\xbc"   /* ▼ U+25BC */
#define ICON_TRI_LEFT       "\xe2\x97\x80"   /* ◀ U+25C0 */
#define ICON_TRI_RIGHT      "\xe2\x96\xb6"   /* ▶ U+25B6 */
#define ICON_HOME           "\xe2\x8c\x82"   /* ⌂ U+2302 */
#define ICON_EXPAND_H       "\xe2\xac\x8c"   /* ⬌ U+2B0C */
#define ICON_EXPAND_V       "\xe2\xac\x8d"   /* ⬍ U+2B0D */

/* ================================================================
 * Actions
 * ================================================================ */
#define ICON_PLUS           "+"
#define ICON_MINUS          "-"
#define ICON_CLOSE          "\xe2\x9c\x95"   /* ✕ U+2715 */
#define ICON_CLOSE2         "\xe2\x9c\x96"   /* ✖ U+2716 */
#define ICON_CHECK          "\xe2\x9c\x93"   /* ✓ U+2713 */
#define ICON_CHECK2         "\xe2\x9c\x94"   /* ✔ U+2714 */
#define ICON_CROSS          "\xe2\x9c\x97"   /* ✗ U+2717 */
#define ICON_EDIT           "\xe2\x9c\x8e"   /* ✎ U+270E */
#define ICON_EDIT2          "\xe2\x9c\x8d"   /* ✍ U+270D */
#define ICON_COPY           "\xe2\x8a\x9f"   /* ⊟ repurposed */
#define ICON_CUT            "\xe2\x9c\x82"   /* ✂ U+2702 */
#define ICON_DELETE         "\xe2\x8c\xab"   /* ⌫ U+232B */
#define ICON_SHARE          "\xe2\x86\x91"   /* ↑ (upload/share) */
#define ICON_DOWNLOAD       "\xe2\x86\x93"   /* ↓ */
#define ICON_UPLOAD         "\xe2\x86\x91"   /* ↑ */
#define ICON_ADD_CIRCLE     "\xe2\x8a\x95"   /* ⊕ U+2295 */
#define ICON_REMOVE_CIRCLE  "\xe2\x8a\x96"   /* ⊖ U+2296 */
#define ICON_SEARCH         "\xe2\x8a\x99"   /* ⊙ U+2299 */
#define ICON_ZOOM_IN        "\xe2\x8a\x95"   /* ⊕ */
#define ICON_ZOOM_OUT       "\xe2\x8a\x96"   /* ⊖ */
#define ICON_LINK           "\xe2\x80\xa6"   /* … U+2026 ellipsis */
#define ICON_FULLSCREEN     "\xe2\x8a\xa0"   /* ⊠ U+22A0 */
#define ICON_LOCK           "\xe2\x8c\x96"   /* ⌖ U+2316 */
#define ICON_PIN            "\xe2\x80\xa2"   /* • U+2022 */

/* ================================================================
 * UI Controls
 * ================================================================ */
#define ICON_MENU           "\xe2\x98\xb0"   /* ☰ U+2630 */
#define ICON_MENU_ALT       "\xe2\x89\xa1"   /* ≡ U+2261 */
#define ICON_MORE_V         "\xe2\x8b\xae"   /* ⋮ U+22EE */
#define ICON_MORE_H         "\xe2\x8b\xaf"   /* ⋯ U+22EF */
#define ICON_SETTINGS       "\xe2\x9a\x99"   /* ⚙ U+2699 */
#define ICON_FILTER         "\xe2\x89\x8b"   /* ≋ U+224B */
#define ICON_SORT           "\xe2\x87\x85"   /* ⇅ U+21C5 */
#define ICON_SORT_ASC       "\xe2\x86\x91"   /* ↑ */
#define ICON_SORT_DESC      "\xe2\x86\x93"   /* ↓ */
#define ICON_GRID           "\xe2\x8a\x9e"   /* ⊞ repurposed */
#define ICON_LIST           "\xe2\x96\xa4"   /* ▤ U+25A4 */
#define ICON_TOGGLE_ON      "\xe2\x97\x89"   /* ◉ U+25C9 */
#define ICON_TOGGLE_OFF     "\xe2\x97\x8b"   /* ○ U+25CB */
#define ICON_CHECKBOX_ON    "\xe2\x98\x91"   /* ☑ U+2611 */
#define ICON_CHECKBOX_OFF   "\xe2\x98\x90"   /* ☐ U+2610 */
#define ICON_RADIO_ON       "\xe2\x97\x89"   /* ◉ U+25C9 */
#define ICON_RADIO_OFF      "\xe2\x97\x8b"   /* ○ U+25CB */
#define ICON_KEYBOARD       "\xe2\x8c\xa8"   /* ⌨ U+2328 */
#define ICON_RETURN         "\xe2\x8f\x8e"   /* ⏎ U+23CE */

/* ================================================================
 * Objects
 * ================================================================ */
#define ICON_FILE           "\xe2\x96\xab"   /* ▫ U+25AB */
#define ICON_FOLDER         "\xe2\x96\xaa"   /* ▪ U+25AA */
#define ICON_MAIL           "\xe2\x9c\x89"   /* ✉ U+2709 */
#define ICON_CALENDAR       "\xe2\x8c\x9b"   /* ⌛ U+231B */
#define ICON_CLOCK          "\xe2\x8c\x9a"   /* ⌚ U+231A */
#define ICON_LOCATION       "\xe2\x8c\x96"   /* ⌖ U+2316 */
#define ICON_USER           "\xe2\x8a\x99"   /* ⊙ */
#define ICON_USERS          "\xe2\x8a\x9a"   /* ⊚ U+229A */
#define ICON_TAG            "\xe2\x97\x86"   /* ◆ U+25C6 */
#define ICON_FLAG           "\xe2\x9a\x91"   /* ⚑ U+2691 */
#define ICON_GLOBE          "\xe2\x8a\x95"   /* ⊕ */
#define ICON_IMAGE          "\xe2\x96\xa3"   /* ▣ U+25A3 */
#define ICON_CAMERA         "\xe2\x8c\xa6"   /* ⌦ U+2326 */
#define ICON_PHONE          "\xe2\x8c\x95"   /* ⌕ U+2315 */
#define ICON_CHART_BAR      "\xe2\x96\x88"   /* █ U+2588 */
#define ICON_CHART_LINE     "\xe2\x88\xbc"   /* ∼ U+223C */
#define ICON_DATABASE       "\xe2\x8a\x9f"   /* ⊟ */
#define ICON_CODE           "\xe2\x89\x88"   /* ≈ U+2248 */
#define ICON_TERMINAL       "\xe2\x8c\x83"   /* ⌃ */
#define ICON_PACKAGE        "\xe2\x97\x87"   /* ◇ U+25C7 */
#define ICON_CLOUD          "\xe2\x98\x81"   /* ☁ U+2601 */
#define ICON_WIFI           "\xe2\x88\xbe"   /* ∾ U+223E */
#define ICON_BLUETOOTH      "\xe2\x9c\xb3"   /* ✳ U+2733 */
#define ICON_BATTERY        "\xe2\x96\xae"   /* ▮ U+25AE */

/* ================================================================
 * Status / Feedback
 * ================================================================ */
#define ICON_WARNING        "\xe2\x9a\xa0"   /* ⚠ U+26A0 */
#define ICON_INFO           "\xe2\x84\xb9"   /* ℹ U+2139 */
#define ICON_SUCCESS        "\xe2\x9c\x93"   /* ✓ */
#define ICON_ERROR          "\xe2\x9c\x97"   /* ✗ */
#define ICON_STAR           "\xe2\x98\x85"   /* ★ U+2605 */
#define ICON_STAR_EMPTY     "\xe2\x98\x86"   /* ☆ U+2606 */
#define ICON_HEART          "\xe2\x99\xa5"   /* ♥ U+2665 */
#define ICON_HEART_EMPTY    "\xe2\x99\xa1"   /* ♡ U+2661 */
#define ICON_LIKE           "\xe2\x96\xb2"   /* ▲ */
#define ICON_DISLIKE        "\xe2\x96\xbc"   /* ▼ */
#define ICON_FIRE           "\xe2\x9c\xb4"   /* ✴ U+2734 */
#define ICON_SHIELD         "\xe2\x9b\x89"   /* ⛉ U+26C9 */
#define ICON_BOLT           "\xe2\x9a\xa1"   /* ⚡ U+26A1 */
#define ICON_CIRCLE_DOT     "\xe2\x8a\x99"   /* ⊙ */
#define ICON_CIRCLE_RING    "\xe2\x8a\x9a"   /* ⊚ */
#define ICON_STOP_SIGN      "\xe2\x9b\x94"   /* ⛔ U+26D4 */
#define ICON_HOURGLASS      "\xe2\x8c\x9b"   /* ⌛ */
#define ICON_SPINNER        "\xe2\x97\x8c"   /* ◌ U+25CC */

/* ================================================================
 * Media
 * ================================================================ */
#define ICON_PLAY           "\xe2\x96\xb6"   /* ▶ U+25B6 */
#define ICON_PAUSE          "\xe2\x80\x96"   /* ‖ U+2016 */
#define ICON_STOP           "\xe2\x96\xa0"   /* ■ U+25A0 */
#define ICON_RECORD         "\xe2\x97\x8f"   /* ● U+25CF */
#define ICON_SKIP_NEXT      "\xe2\x8f\xad"   /* ⏭ U+23ED */
#define ICON_SKIP_PREV      "\xe2\x8f\xae"   /* ⏮ U+23EE */
#define ICON_FAST_FORWARD   "\xe2\x8f\xa9"   /* ⏩ U+23E9 */
#define ICON_REWIND         "\xe2\x8f\xaa"   /* ⏪ U+23EA */
#define ICON_EJECT          "\xe2\x8f\x8f"   /* ⏏ U+23CF */
#define ICON_VOLUME         "\xe2\x99\xaa"   /* ♪ U+266A */
#define ICON_VOLUME_MUTE    "\xe2\x99\xb2"   /* ♲ U+2672 repurposed */
#define ICON_MIC            "\xe2\x9c\x86"   /* ✆ U+2706 */

/* ================================================================
 * Symbols
 * ================================================================ */
#define ICON_PLUS_SIGN      "\xc2\xb1"       /* ± U+00B1 */
#define ICON_MULTIPLY       "\xc3\x97"       /* × U+00D7 */
#define ICON_DIVIDE         "\xc3\xb7"       /* ÷ U+00F7 */
#define ICON_INFINITY       "\xe2\x88\x9e"   /* ∞ U+221E */
#define ICON_EMPTY_SET      "\xe2\x88\x85"   /* ∅ U+2205 */
#define ICON_COPYRIGHT      "\xc2\xa9"       /* © U+00A9 */
#define ICON_REGISTERED     "\xc2\xae"       /* ® U+00AE */
#define ICON_TRADEMARK      "\xe2\x84\xa2"   /* ™ U+2122 */
#define ICON_DEGREE         "\xc2\xb0"       /* ° U+00B0 */
#define ICON_BULLET         "\xe2\x80\xa2"   /* • U+2022 */
#define ICON_ELLIPSIS       "\xe2\x80\xa6"   /* … U+2026 */
#define ICON_EM_DASH        "\xe2\x80\x94"   /* — U+2014 */
#define ICON_SECTION        "\xc2\xa7"       /* § U+00A7 */
#define ICON_DIAMOND        "\xe2\x97\x86"   /* ◆ U+25C6 */
#define ICON_DIAMOND_EMPTY  "\xe2\x97\x87"   /* ◇ U+25C7 */
#define ICON_SQUARE         "\xe2\x96\xa0"   /* ■ U+25A0 */
#define ICON_SQUARE_EMPTY   "\xe2\x96\xa1"   /* □ U+25A1 */
#define ICON_COMMAND        "\xe2\x8c\x98"   /* ⌘ U+2318 */
#define ICON_OPTION         "\xe2\x8c\xa5"   /* ⌥ U+2325 */
#define ICON_SHIFT          "\xe2\x87\xa7"   /* ⇧ U+21E7 */

/* ================================================================
 * Widget helpers
 * ================================================================ */

/* Render an icon glyph as text at given size and color. */
static inline void UI_Icon(int uid, const char *icon, int size, Clay_Color color)
{
    CLAY(CLAY_SIDI(CLAY_STRING("UIIcon"), uid), {
        .layout = {
            .sizing = { CLAY_SIZING_FIT(0), CLAY_SIZING_FIT(0) },
            .childAlignment = { CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER },
        },
    }) {
        CLAY_TEXT(UI__str(icon), { .textColor = color, .fontSize = (uint16_t)size });
    }
}

/* Icon button: renders icon inside a button shell. Returns true on click. */
static inline bool UI_IconButton(int uid, const char *icon, int size,
                                 UIBtnVariant variant, bool disabled)
{
    Clay_ElementId id = Clay_GetElementIdWithIndex(CLAY_STRING("UIIconBtn"), uid);
    bool hovered = !disabled && Clay_PointerOver(id);
    bool pressed = hovered && UI__mouse_down;
    bool clicked = hovered && UI__mouse_released && !disabled;

    static const Clay_Color BG_NRM[4] = {
        {137,112,194,255}, {49,50,68,160}, {0,0,0,0}, {210,99,128,255}
    };
    static const Clay_Color BG_HOV[4] = {
        {155,130,210,255}, {69,71,90,200}, {88,91,112,80}, {230,115,143,255}
    };
    static const Clay_Color FG_COL[4] = {
        {255,255,255,255}, {180,190,254,255}, {166,173,200,255}, {255,255,255,255}
    };

    Clay_Color bg = pressed ? BG_HOV[variant] : hovered ? BG_HOV[variant] : BG_NRM[variant];
    if (variant == UI_BTN_GHOST && !hovered) bg = (Clay_Color){0,0,0,0};
    Clay_Color fg = disabled ? (Clay_Color){166,173,200,80} : FG_COL[variant];

    CLAY(id, {
        .layout = {
            .sizing = { CLAY_SIZING_FIXED((float)(size + 12)),
                        CLAY_SIZING_FIXED((float)(size + 12)) },
            .childAlignment = { CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER },
        },
        .backgroundColor = bg,
        .cornerRadius = CLAY_CORNER_RADIUS(6),
    }) {
        CLAY_TEXT(UI__str(icon), { .textColor = fg, .fontSize = (uint16_t)size });
    }
    return clicked;
}

#endif /* UI_ICONS_H */
