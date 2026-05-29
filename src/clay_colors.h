/* clay_colors.h — Catppuccin Mocha color palette for Clay UI.
 *
 * Reference: https://catppuccin.com/palette
 * All colors are in sRGB with alpha=255 (fully opaque).
 *
 * Usage:
 *   #include "clay_colors.h"
 *   CLAY({ .backgroundColor = COL_BASE }) { ... }
 */

#ifndef CLAY_COLORS_H
#define CLAY_COLORS_H

#include "clay.h"

/* ---- Base shades (darkest → lightest) ---- */
#define COL_CRUST      ((Clay_Color){ 17, 17, 27, 255 })
#define COL_MANTLE     ((Clay_Color){ 24, 24, 37, 255 })
#define COL_BASE       ((Clay_Color){ 30, 30, 46, 255 })
#define COL_SURFACE0   ((Clay_Color){ 49, 50, 68, 255 })
#define COL_SURFACE1   ((Clay_Color){ 69, 71, 90, 255 })
#define COL_SURFACE2   ((Clay_Color){ 88, 91, 112, 255 })
#define COL_OVERLAY    ((Clay_Color){ 88, 91, 112, 255 })
#define COL_SUBTEXT    ((Clay_Color){ 166, 173, 200, 255 })
#define COL_TEXT       ((Clay_Color){ 205, 214, 244, 255 })
#define COL_LAVENDER   ((Clay_Color){ 180, 190, 254, 255 })

/* ---- Accent colors ---- */
#define COL_ROSEWATER  ((Clay_Color){ 245, 224, 220, 255 })
#define COL_FLAMINGO   ((Clay_Color){ 242, 205, 205, 255 })
#define COL_PINK       ((Clay_Color){ 245, 194, 231, 255 })
#define COL_MAUVE      ((Clay_Color){ 203, 166, 247, 255 })
#define COL_RED        ((Clay_Color){ 243, 139, 168, 255 })
#define COL_MAROON     ((Clay_Color){ 235, 160, 172, 255 })
#define COL_PEACH      ((Clay_Color){ 250, 179, 135, 255 })
#define COL_YELLOW     ((Clay_Color){ 249, 226, 175, 255 })
#define COL_GREEN      ((Clay_Color){ 166, 227, 161, 255 })
#define COL_TEAL       ((Clay_Color){ 148, 226, 213, 255 })
#define COL_SKY        ((Clay_Color){ 137, 220, 235, 255 })
#define COL_SAPPHIRE   ((Clay_Color){ 116, 199, 236, 255 })
#define COL_BLUE       ((Clay_Color){ 137, 180, 250, 255 })

#endif /* CLAY_COLORS_H */
