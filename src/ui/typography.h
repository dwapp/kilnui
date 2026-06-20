/* SPDX-License-Identifier: MIT */
/* src/ui/typography.h — KilnUI Typography System
 *
 * 10 text style presets + Label-Value pair component.
 *
 * Styles:
 *   TY_DISPLAY   40 px  accent color       — hero titles
 *   TY_H1        32 px  text               — page heading
 *   TY_H2        24 px  text               — section heading
 *   TY_H3        20 px  text               — card heading
 *   TY_H4        16 px  text               — subsection
 *   TY_BODY      14 px  text               — default prose
 *   TY_SMALL     12 px  subtext            — small print
 *   TY_CAPTION   10 px  muted              — labels, hints
 *   TY_CODE      14 px  accent_alt         — code / mono emphasis
 *   TY_OVERLINE  10 px  muted, uppercased  — category labels
 *
 * Usage:
 *   TY_Text(uid, "Hello, world", TY_BODY);
 *   TY_TextColored(uid, "Error!", TY_H3, ds_theme->error);
 *   TY_LabelValue(uid, "Status", "Active");
 */

#ifndef UI_TYPOGRAPHY_H
#define UI_TYPOGRAPHY_H

#include "design_system.h"
#include "ui_internal.h"

/* ---- Style enum ---- */
typedef enum {
    TY_DISPLAY = 0,
    TY_H1,
    TY_H2,
    TY_H3,
    TY_H4,
    TY_BODY,
    TY_SMALL,
    TY_CAPTION,
    TY_CODE,
    TY_OVERLINE,
    TY_STYLE_COUNT
} TYStyle;

/* ---- Render functions ---- */
void TY_Text(int uid, const char *text, TYStyle style);
void TY_TextColored(int uid, const char *text, TYStyle style, Clay_Color color);

/* Label-Value pair (e.g. "Status" / "Active") */
void TY_LabelValue(int uid, const char *label, const char *value);

/* Centered text block */
void TY_Center(int uid, const char *text, TYStyle style);

/* Truncated single-line text (fills available horizontal space) */
void TY_Truncate(int uid, const char *text, TYStyle style);

#endif /* UI_TYPOGRAPHY_H */
