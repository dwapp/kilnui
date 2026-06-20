/* SPDX-License-Identifier: MIT */
/* src/ui/theme.h — KilnUI Theme Components
 *
 * Provides themed UI components that automatically adapt to ds_theme:
 *
 *   Badge    — inline status pill  (8 color variants)
 *   Chip     — selectable/dismissible tag
 *   Divider  — horizontal rule with optional label
 *   Backdrop — full-screen semi-transparent overlay
 *   Avatar   — initials circle
 *   Alert    — info/warning/error/success banner
 *
 * Usage:
 *   UI_Badge(uid, "New",     DS_VARIANT_PRIMARY);
 *   UI_Badge(uid, "Error",   DS_VARIANT_ERROR);
 *   bool still_shown = UI_Chip(uid, "dark-mode", selected, true);
 *   UI_Divider(uid);
 *   UI_DividerLabel(uid, "OR");
 *   UI_Alert(uid, ICON_WARNING " Disk almost full", DS_VARIANT_WARNING);
 */

#ifndef UI_THEME_H
#define UI_THEME_H

#include "design_system.h"
#include "ui_internal.h"
#include <stdbool.h>

/* ---- Badge ---- */
void UI_Badge(int uid, const char *text, DSVariant variant);

/* ---- Chip ---- */
/* Returns false if the dismiss button was clicked (chip should be removed). */
bool UI_Chip(int uid, const char *text, bool selected, bool dismissible);

/* ---- Divider ---- */
void UI_Divider(int uid);
void UI_DividerLabel(int uid, const char *label);

/* ---- Backdrop ---- */
/* Renders a full-window overlay; place FIRST in the layout stack.
 * visible = false → renders nothing (0-size transparent element). */
void UI_Backdrop(int uid, bool visible);

/* ---- Avatar ---- */
/* initials: 1-2 character string, e.g. "JD" */
void UI_Avatar(int uid, const char *initials, DSVariant color_variant, int size);

/* ---- Alert ---- */
void UI_Alert(int uid, const char *message, DSVariant variant);

/* ---- StatCard ---- */
void UI_StatCard(int uid, const char *title, const char *value, Clay_Color accent);

/* ---- ListItem ---- */
/* Renders a simple list item with hover state. Returns true if clicked. */
bool UI_ListItem(int uid, const char *text);

#endif /* UI_THEME_H */
