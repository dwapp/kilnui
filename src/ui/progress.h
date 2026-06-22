/* SPDX-License-Identifier: MIT */
/* src/ui/progress.h - Progress bar widget.
 *
 * Features:
 *   - Variant colors (primary, success, warning, error, info)
 *   - Size presets (SM, MD, LG)
 *   - Optional percentage text (inside or outside the bar)
 *   - Indeterminate mode (pulsing animation for unknown progress)
 *
 * Usage:
 *   // Simple determinate progress
 *   UI_Progress(ID, "Downloading", 0.45f, 1.0f);
 *
 *   // With options
 *   UI_ProgressOpts opts = {
 *       .variant = UI_PROGRESS_SUCCESS,
 *       .size = UI_PROGRESS_LG,
 *       .show_percentage = true,
 *   };
 *   UI_ProgressEx(ID, "Install", 75.0f, 100.0f, &opts);
 *
 *   // Indeterminate (loading)
 *   UI_ProgressOpts opts = { .indeterminate = true };
 *   UI_ProgressEx(ID, "Loading...", 0, 0, &opts);
 */

#ifndef UI_PROGRESS_H
#define UI_PROGRESS_H

#include <stdbool.h>

/* ---- Variant (color scheme) ---- */
typedef enum
{
    UI_PROGRESS_DEFAULT = 0, /* accent color (default) */
    UI_PROGRESS_PRIMARY,     /* accent / brand */
    UI_PROGRESS_SUCCESS,     /* green */
    UI_PROGRESS_WARNING,     /* yellow / peach */
    UI_PROGRESS_ERROR,       /* red */
    UI_PROGRESS_INFO,        /* blue / sapphire */
} UIProgressVariant;

/* ---- Size preset ---- */
typedef enum
{
    UI_PROGRESS_SM = 0, /*  6px bar height */
    UI_PROGRESS_MD = 1, /* 10px bar height (default) */
    UI_PROGRESS_LG = 2, /* 14px bar height */
} UIProgressSize;

/* ---- Options for UI_ProgressEx ---- */
typedef struct
{
    UIProgressVariant variant;
    UIProgressSize size;
    bool show_percentage; /* render "45%" text */
    bool indeterminate;   /* pulsing animation (value/max ignored) */
} UIProgressOpts;

/* ---- Simple API (default variant, no percentage text) ---- */
void UI_Progress(int uid, const char *label, float value, float max_value);

/* ---- Extended API ---- */
void UI_ProgressEx(int uid, const char *label, float value, float max_value,
                   const UIProgressOpts *opts);

#endif /* UI_PROGRESS_H */
