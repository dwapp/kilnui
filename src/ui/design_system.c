/* src/ui/design_system.c — Theme data for KilnUI Design System */

#include "design_system.h"

/* ---- helper ---- */
#define C(r,g,b,a) ((Clay_Color){r,g,b,a})

/* ================================================================
 * Dark theme — Catppuccin Mocha
 * ================================================================ */
const DSTheme DS_THEME_DARK = {
    .rosewater = C(245, 224, 220, 255),
    .flamingo  = C(242, 205, 205, 255),
    .pink      = C(245, 194, 231, 255),
    .mauve     = C(203, 166, 247, 255),
    .red       = C(243, 139, 168, 255),
    .maroon    = C(235, 160, 172, 255),
    .peach     = C(250, 179, 135, 255),
    .yellow    = C(249, 226, 175, 255),
    .green     = C(166, 227, 161, 255),
    .teal      = C(148, 226, 213, 255),
    .sky       = C(137, 220, 235, 255),
    .sapphire  = C(116, 199, 236, 255),
    .blue      = C(137, 180, 250, 255),
    .lavender  = C(180, 190, 254, 255),

    .text      = C(205, 214, 244, 255),
    .subtext1  = C(186, 194, 222, 255),
    .subtext0  = C(166, 173, 200, 255),
    .overlay2  = C(147, 153, 178, 255),
    .overlay1  = C(127, 132, 156, 255),
    .overlay0  = C(108, 112, 134, 255),
    .surface2  = C( 88,  91, 112, 255),
    .surface1  = C( 69,  71,  90, 255),
    .surface0  = C( 49,  50,  68, 255),
    .base      = C( 30,  30,  46, 255),
    .mantle    = C( 24,  24,  37, 255),
    .crust     = C( 17,  17,  27, 255),

    .overlay    = C( 24,  24,  37, 220),
    .subtext    = C(166, 173, 200, 255), /* subtext0 */
    .muted      = C(127, 132, 156, 255), /* overlay1 */
    .accent     = C(203, 166, 247, 255), /* mauve */
    .accent_alt = C(180, 190, 254, 255), /* lavender */
    .success    = C(166, 227, 161, 255), /* green */
    .warning    = C(249, 226, 175, 255), /* yellow */
    .error      = C(243, 139, 168, 255), /* red */
    .info       = C(116, 199, 236, 255), /* sapphire */
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
    .rosewater = C(220, 138, 120, 255),
    .flamingo  = C(221, 120, 120, 255),
    .pink      = C(234, 118, 203, 255),
    .mauve     = C(136,  57, 239, 255),
    .red       = C(210,  15,  57, 255),
    .maroon    = C(230,  69,  83, 255),
    .peach     = C(254, 100,  11, 255),
    .yellow    = C(223, 142,  29, 255),
    .green     = C( 64, 160,  43, 255),
    .teal      = C( 23, 146, 153, 255),
    .sky       = C(  4, 165, 229, 255),
    .sapphire  = C( 32, 159, 181, 255),
    .blue      = C( 30, 102, 245, 255),
    .lavender  = C(114, 135, 253, 255),

    .text      = C( 76,  79, 105, 255),
    .subtext1  = C( 92,  95, 119, 255),
    .subtext0  = C(108, 111, 133, 255),
    .overlay2  = C(124, 127, 147, 255),
    .overlay1  = C(140, 143, 161, 255),
    .overlay0  = C(156, 160, 176, 255),
    .surface2  = C(172, 176, 190, 255),
    .surface1  = C(188, 192, 204, 255),
    .surface0  = C(204, 208, 218, 255),
    .base      = C(239, 241, 245, 255),
    .mantle    = C(230, 233, 239, 255),
    .crust     = C(220, 224, 232, 255),

    .overlay    = C(220, 224, 232, 230),
    .subtext    = C(108, 111, 133, 255), /* subtext0 */
    .muted      = C(140, 143, 161, 255), /* overlay1 */
    .accent     = C(136,  57, 239, 255), /* mauve */
    .accent_alt = C(114, 135, 253, 255), /* lavender */
    .success    = C( 64, 160,  43, 255), /* green */
    .warning    = C(223, 142,  29, 255), /* yellow */
    .error      = C(210,  15,  57, 255), /* red */
    .info       = C( 32, 159, 181, 255), /* sapphire */
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
