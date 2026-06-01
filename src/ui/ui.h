/* src/ui/ui.h — KilnUI Unified Component Header
 *
 * Single include that pulls in all UI subsystems plus layout helpers.
 *
 * Layout macros (use inside Clay_BeginLayout() / Clay_EndLayout()):
 *
 *   UI_ROW(uid, gap)        horizontal flex row
 *   UI_COL(uid, gap)        vertical flex column
 *   UI_CENTER(uid)          centered container (both axes)
 *   UI_SPACER(uid)          flex-grow filler
 *   UI_PANEL(uid)           padded surface card
 *   UI_SCROLLCOL(uid)       vertical scroll container
 *   UI_STACK(uid)           z-layered floating stack
 *
 * Usage:
 *   #include "ui/ui.h"
 *
 *   UI_ROW(10, 8) {
 *       UI_IconButton(11, ICON_HOME, 16, UI_BTN_GHOST, false);
 *       UI_SPACER(12);
 *       UI_Badge(13, "3", DS_VARIANT_ERROR);
 *   }
 */

#ifndef UI_H
#define UI_H

/* ---- Design system foundation ---- */
#include "design_system.h"

/* ---- All components ---- */
#include "button.h"
#include "checkbox.h"
#include "container.h"
#include "dropdown.h"
#include "icons.h"
#include "input.h"
#include "label.h"
#include "progress.h"
#include "radio.h"
#include "slider.h"
#include "theme.h"
#include "tooltip.h"
#include "typography.h"

/* ================================================================
 * Layout helpers — thin wrappers around CLAY() macros
 * ================================================================ */

/* Horizontal row */
#define UI_ROW(uid, gap) \
    CLAY(CLAY_SIDI(CLAY_STRING("UIRow"), uid), { \
        .layout = { \
            .sizing          = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIT(0) }, \
            .layoutDirection = CLAY_LEFT_TO_RIGHT, \
            .childAlignment  = { CLAY_ALIGN_X_LEFT, CLAY_ALIGN_Y_CENTER }, \
            .childGap        = (uint16_t)(gap), \
        }, \
    })

/* Vertical column */
#define UI_COL(uid, gap) \
    CLAY(CLAY_SIDI(CLAY_STRING("UICol"), uid), { \
        .layout = { \
            .sizing          = { CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0) }, \
            .layoutDirection = CLAY_TOP_TO_BOTTOM, \
            .childAlignment  = { CLAY_ALIGN_X_LEFT, CLAY_ALIGN_Y_TOP }, \
            .childGap        = (uint16_t)(gap), \
        }, \
    })

/* Centered container */
#define UI_CENTER(uid) \
    CLAY(CLAY_SIDI(CLAY_STRING("UICenter"), uid), { \
        .layout = { \
            .sizing         = { CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0) }, \
            .childAlignment = { CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER }, \
        }, \
    })

/* Flex-grow spacer */
#define UI_SPACER(uid) \
    CLAY(CLAY_SIDI(CLAY_STRING("UISpacer"), uid), { \
        .layout = { \
            .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIXED(1) }, \
        }, \
    }) {}

/* Padded surface panel / card */
#define UI_PANEL(uid, pad, gap) \
    CLAY(CLAY_SIDI(CLAY_STRING("UIPanel"), uid), { \
        .layout = { \
            .sizing          = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIT(0) }, \
            .layoutDirection = CLAY_TOP_TO_BOTTOM, \
            .childGap        = (uint16_t)(gap), \
            .padding         = { (uint16_t)(pad), (uint16_t)(pad), \
                                 (uint16_t)(pad), (uint16_t)(pad) }, \
        }, \
        .backgroundColor = ds_theme->surface0, \
        .cornerRadius    = CLAY_CORNER_RADIUS(DS_RADIUS_LG), \
    })

/* Vertical scroll column */
#define UI_SCROLLCOL(uid, gap) \
    CLAY(CLAY_SIDI(CLAY_STRING("UIScroll"), uid), { \
        .layout = { \
            .sizing          = { CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0) }, \
            .layoutDirection = CLAY_TOP_TO_BOTTOM, \
            .childGap        = (uint16_t)(gap), \
        }, \
        .clip = { .vertical = true }, \
    })

/* Z-layered floating stack (position children via .floating) */
#define UI_STACK(uid) \
    CLAY(CLAY_SIDI(CLAY_STRING("UIStack"), uid), { \
        .layout = { \
            .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0) }, \
        }, \
    })

/* ================================================================
 * Utility: padded text label (quick use without ID tracking)
 * ================================================================ */
static inline void UI_Text(int uid, const char *text, uint16_t font_size, Clay_Color color)
{
    CLAY(CLAY_SIDI(CLAY_STRING("UIText"), uid), {
        .layout = { .sizing = { CLAY_SIZING_FIT(0), CLAY_SIZING_FIT(0) } },
    }) {
        CLAY_TEXT(UI__str(text), { .textColor = color, .fontSize = font_size });
    }
}

/* ================================================================
 * Utility: colored rectangle swatch (useful in design system demos)
 * ================================================================ */
static inline void UI_ColorSwatch(int uid, Clay_Color color, int w, int h, int radius)
{
    CLAY(CLAY_SIDI(CLAY_STRING("UISwatch"), uid), {
        .layout = {
            .sizing = { CLAY_SIZING_FIXED((float)w), CLAY_SIZING_FIXED((float)h) },
        },
        .backgroundColor = color,
        .cornerRadius    = CLAY_CORNER_RADIUS((float)radius),
    }) {}
}

#endif /* UI_H */
