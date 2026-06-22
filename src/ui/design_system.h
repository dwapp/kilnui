/* SPDX-License-Identifier: MIT */
/* src/ui/design_system.h — KilnUI Design System Tokens
 *
 * Provides spacing, corner radii, font sizes, component heights, shadow
 * descriptors, and a dual-theme (dark/light) system based on Catppuccin.
 *
 * Usage:
 *   #include "design_system.h"
 *   DS_SetTheme(&DS_THEME_LIGHT);
 *   Clay_Color bg = ds_theme->base;
 *   padding = DS_SPACE_4;
 */

#ifndef UI_DESIGN_SYSTEM_H
#define UI_DESIGN_SYSTEM_H

#include "clay.h"
#include <stdbool.h>

/* ================================================================
 * Spacing scale  (multiples of 4 px)
 * ================================================================ */
#define DS_SPACE_1  4
#define DS_SPACE_2  8
#define DS_SPACE_3  12
#define DS_SPACE_4  16
#define DS_SPACE_5  20
#define DS_SPACE_6  24
#define DS_SPACE_8  32
#define DS_SPACE_10 40
#define DS_SPACE_12 48
#define DS_SPACE_16 64

/* ================================================================
 * Corner radii
 * ================================================================ */
#define DS_RADIUS_NONE 0
#define DS_RADIUS_XS   2
#define DS_RADIUS_SM   4
#define DS_RADIUS_MD   8
#define DS_RADIUS_LG   12
#define DS_RADIUS_XL   16
#define DS_RADIUS_2XL  24
#define DS_RADIUS_FULL 9999

/* ================================================================
 * Font sizes  (px at 1× DPI)
 * ================================================================ */
#define DS_FS_XS  10
#define DS_FS_SM  12
#define DS_FS_MD  14
#define DS_FS_LG  16
#define DS_FS_XL  20
#define DS_FS_2XL 24
#define DS_FS_3XL 32
#define DS_FS_4XL 40
#define DS_FS_5XL 48

/* ================================================================
 * Component heights
 * ================================================================ */
#define DS_HEIGHT_XS 24
#define DS_HEIGHT_SM 32
#define DS_HEIGHT_MD 40
#define DS_HEIGHT_LG 48
#define DS_HEIGHT_XL 56

/* ---- Color Tokens ---- */
/* ================================================================
 * Color variant enum  (8 semantic roles)
 * ================================================================ */
typedef enum
{
    DS_VARIANT_DEFAULT = 0,
    DS_VARIANT_PRIMARY, /* accent / brand */
    DS_VARIANT_SUCCESS, /* green */
    DS_VARIANT_WARNING, /* yellow / peach */
    DS_VARIANT_ERROR,   /* red */
    DS_VARIANT_INFO,    /* blue / sapphire */
    DS_VARIANT_MUTED,   /* surface2 / subtext */
    DS_VARIANT_ACCENT,  /* mauve / lavender */
    DS_VARIANT_COUNT
} DSVariant;

/* ================================================================
 * Theme structure
 * ================================================================ */
typedef struct
{
    /* Standard Catppuccin Palette */
    Clay_Color rosewater;
    Clay_Color flamingo;
    Clay_Color pink;
    Clay_Color mauve;
    Clay_Color red;
    Clay_Color maroon;
    Clay_Color peach;
    Clay_Color yellow;
    Clay_Color green;
    Clay_Color teal;
    Clay_Color sky;
    Clay_Color sapphire;
    Clay_Color blue;
    Clay_Color lavender;

    Clay_Color text;
    Clay_Color subtext1;
    Clay_Color subtext0;
    Clay_Color overlay2;
    Clay_Color overlay1;
    Clay_Color overlay0;
    Clay_Color surface2;
    Clay_Color surface1;
    Clay_Color surface0;
    Clay_Color base;
    Clay_Color mantle;
    Clay_Color crust;

    /* Semantic Aliases */
    Clay_Color overlay;    /* Legacy generic overlay (often translucent) */
    Clay_Color subtext;    /* Primary subtext (maps to subtext0) */
    Clay_Color muted;      /* Muted text (maps to overlay1 or overlay0) */
    Clay_Color accent;     /* Primary accent (mauve) */
    Clay_Color accent_alt; /* Secondary accent (lavender) */
    Clay_Color success;    /* Success state (green) */
    Clay_Color warning;    /* Warning state (yellow/peach) */
    Clay_Color error;      /* Error state (red) */
    Clay_Color info;       /* Info state (sapphire) */

    /* Semantic variant palette  [DS_VARIANT_COUNT] */
    Clay_Color variant_bg[DS_VARIANT_COUNT]; /* background fill   */
    Clay_Color variant_fg[DS_VARIANT_COUNT]; /* foreground / text */

    bool is_dark;
} DSTheme;

/* ================================================================
 * Built-in themes
 * ================================================================ */
extern const DSTheme DS_THEME_DARK;  /* Catppuccin Mocha  */
extern const DSTheme DS_THEME_LIGHT; /* Catppuccin Latte  */

/* Active theme pointer — defaults to &DS_THEME_DARK on startup. */
extern const DSTheme *ds_theme;

/* Switch the active theme.  Pass &DS_THEME_DARK or &DS_THEME_LIGHT. */
void DS_SetTheme(const DSTheme *theme);

/* ================================================================
 * Convenience accessors
 * ================================================================ */
static inline Clay_Color DS_VariantBg(DSVariant v)
{
    return ds_theme->variant_bg[v < DS_VARIANT_COUNT ? v : 0];
}
static inline Clay_Color DS_VariantFg(DSVariant v)
{
    return ds_theme->variant_fg[v < DS_VARIANT_COUNT ? v : 0];
}

#endif /* UI_DESIGN_SYSTEM_H */
