/* src/ui/design_system.c — Theme data for KilnUI Design System */

#include "design_system.h"

/* ---- helper ---- */
#define C(r,g,b,a) ((Clay_Color){r,g,b,a})

/* ================================================================
 * Dark theme — Catppuccin Mocha
 * ================================================================ */
const DSTheme DS_THEME_DARK = {
    .base     = C( 30,  30,  46, 255),
    .mantle   = C( 24,  24,  37, 255),
    .crust    = C( 17,  17,  27, 255),
    .surface0 = C( 49,  50,  68, 255),
    .surface1 = C( 69,  71,  90, 255),
    .surface2 = C( 88,  91, 112, 255),
    .overlay  = C( 24,  24,  37, 220),
    .text     = C(205, 214, 244, 255),
    .subtext  = C(166, 173, 200, 255),
    .muted    = C(127, 132, 156, 255),
    .accent   = C(203, 166, 247, 255),   /* mauve   */
    .accent_alt = C(180, 190, 254, 255), /* lavender*/
    .success  = C(166, 227, 161, 255),   /* green   */
    .warning  = C(249, 226, 175, 255),   /* yellow  */
    .error    = C(243, 139, 168, 255),   /* red     */
    .info     = C(116, 199, 236, 255),   /* sapphire*/
    .is_dark  = true,
    .variant_bg = {
        /* DEFAULT  */ C( 49,  50,  68, 200),
        /* PRIMARY  */ C(203, 166, 247, 255),
        /* SUCCESS  */ C( 64, 120,  60, 220),
        /* WARNING  */ C(140, 100,  30, 220),
        /* ERROR    */ C(140,  40,  60, 220),
        /* INFO     */ C( 40, 100, 140, 220),
        /* MUTED    */ C( 88,  91, 112, 160),
        /* ACCENT   */ C(180, 190, 254, 255),
    },
    .variant_fg = {
        /* DEFAULT  */ C(205, 214, 244, 255),
        /* PRIMARY  */ C( 30,  30,  46, 255),
        /* SUCCESS  */ C(166, 227, 161, 255),
        /* WARNING  */ C(249, 226, 175, 255),
        /* ERROR    */ C(243, 139, 168, 255),
        /* INFO     */ C(116, 199, 236, 255),
        /* MUTED    */ C(166, 173, 200, 255),
        /* ACCENT   */ C( 30,  30,  46, 255),
    },
};

/* ================================================================
 * Light theme — Catppuccin Latte
 * ================================================================ */
const DSTheme DS_THEME_LIGHT = {
    .base     = C(239, 241, 245, 255),
    .mantle   = C(230, 233, 239, 255),
    .crust    = C(220, 224, 232, 255),
    .surface0 = C(204, 208, 218, 255),
    .surface1 = C(188, 192, 204, 255),
    .surface2 = C(172, 176, 190, 255),
    .overlay  = C(220, 224, 232, 230),
    .text     = C( 76,  79, 105, 255),
    .subtext  = C(108, 111, 133, 255),
    .muted    = C(140, 143, 161, 255),
    .accent   = C(136,  57, 239, 255),   /* mauve   */
    .accent_alt = C(114, 135, 253, 255), /* lavender*/
    .success  = C( 64, 160,  43, 255),   /* green   */
    .warning  = C(223, 142,  29, 255),   /* yellow  */
    .error    = C(210,  15,  57, 255),   /* red     */
    .info     = C( 32, 159, 181, 255),   /* sapphire*/
    .is_dark  = false,
    .variant_bg = {
        /* DEFAULT  */ C(204, 208, 218, 255),
        /* PRIMARY  */ C(136,  57, 239, 255),
        /* SUCCESS  */ C(200, 240, 196, 255),
        /* WARNING  */ C(250, 235, 200, 255),
        /* ERROR    */ C(250, 200, 210, 255),
        /* INFO     */ C(200, 230, 245, 255),
        /* MUTED    */ C(172, 176, 190, 255),
        /* ACCENT   */ C(114, 135, 253, 255),
    },
    .variant_fg = {
        /* DEFAULT  */ C( 76,  79, 105, 255),
        /* PRIMARY  */ C(239, 241, 245, 255),
        /* SUCCESS  */ C( 64, 160,  43, 255),
        /* WARNING  */ C(223, 142,  29, 255),
        /* ERROR    */ C(210,  15,  57, 255),
        /* INFO     */ C( 32, 159, 181, 255),
        /* MUTED    */ C(108, 111, 133, 255),
        /* ACCENT   */ C(239, 241, 245, 255),
    },
};

/* ---- Active theme pointer ---- */
const DSTheme *ds_theme = &DS_THEME_DARK;

void DS_SetTheme(const DSTheme *theme)
{
    if (theme) ds_theme = theme;
}
