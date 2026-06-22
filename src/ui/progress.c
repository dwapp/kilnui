/* SPDX-License-Identifier: MIT */
/* src/ui/progress.c - Progress bar widget implementation. */

#include "progress.h"
#include "design_system.h"
#include "ui_internal.h"
#include <stdio.h>

/* ---- Size metrics ---- */
typedef struct
{
    int bar_height;
    int radius;
    int font_size;
} ProgressSizeInfo;

static const ProgressSizeInfo PROGRESS_SIZES[3] = {
    /* SM */ { 6, 3, 10 },
    /* MD */ { 10, 5, 12 },
    /* LG */ { 14, 7, 14 },
};

/* ---- Variant colors ---- */
static Clay_Color get_progress_color(UIProgressVariant variant)
{
    switch (variant) {
    case UI_PROGRESS_SUCCESS:
        return ds_theme->success;
    case UI_PROGRESS_WARNING:
        return ds_theme->warning;
    case UI_PROGRESS_ERROR:
        return ds_theme->error;
    case UI_PROGRESS_INFO:
        return ds_theme->info;
    case UI_PROGRESS_PRIMARY:
        return ds_theme->accent;
    case UI_PROGRESS_DEFAULT:
    default:
        return ds_theme->accent;
    }
}

/* ---- Indeterminate animation state ---- */
static float s_indet_time = 0.0f;

/* ---- Extended API ---- */
void UI_ProgressEx(int uid, const char *label, float value, float max_value,
                   const UIProgressOpts *opts)
{
    /* Defaults */
    UIProgressVariant variant = UI_PROGRESS_DEFAULT;
    UIProgressSize size = UI_PROGRESS_MD;
    bool show_pct = false;
    bool indeterminate = false;

    if (opts) {
        variant = opts->variant;
        size = opts->size;
        show_pct = opts->show_percentage;
        indeterminate = opts->indeterminate;
    }

    const ProgressSizeInfo *sz = &PROGRESS_SIZES[size];
    Clay_Color fill_color = get_progress_color(variant);

    /* Calculate percentage for determinate mode */
    float pct = 0.0f;
    if (!indeterminate && max_value > 0.0f) {
        pct = UI__clampf(value / max_value, 0.0f, 1.0f);
    }

    /* Percentage text buffer */
    char pct_buf[16] = "";
    if (show_pct && !indeterminate) {
        snprintf(pct_buf, sizeof(pct_buf), "%.0f%%", pct * 100.0f);
    }

    /* Build layout ID */
    CLAY(CLAY_SIDI(CLAY_STRING("UIProgress"), uid), {
                                                        .layout = {
                                                            .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIT(0) },
                                                            .childGap = DS_SPACE_2,
                                                            .layoutDirection = CLAY_TOP_TO_BOTTOM,
                                                        },
                                                    })
    {
        /* Label row (label + percentage text) */
        bool has_label_row = (label && label[0]) || (show_pct && !indeterminate);
        if (has_label_row) {
            CLAY(CLAY_SIDI(CLAY_STRING("UIProgressLabelRow"), uid), {
                                                                       .layout = {
                                                                           .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIT(0) },
                                                                           .childGap = DS_SPACE_2,
                                                                           .layoutDirection = CLAY_LEFT_TO_RIGHT,
                                                                           .childAlignment = { CLAY_ALIGN_X_LEFT, CLAY_ALIGN_Y_CENTER },
                                                                       },
                                                                   })
            {
                if (label && label[0]) {
                    CLAY_TEXT(UI__str(label), {
                                                  .textColor = ds_theme->muted,
                                                  .fontSize = (uint16_t)sz->font_size,
                                              });
                }

                /* Spacer to push percentage to the right */
                if (label && label[0] && show_pct && !indeterminate) {
                    CLAY(CLAY_SIDI(CLAY_STRING("UIProgressSpacer"), uid), {
                                                                              .layout = {
                                                                                  .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIXED(1) },
                                                                              },
                                                                          })
                    {
                    }
                }

                if (show_pct && !indeterminate) {
                    CLAY_TEXT(UI__str(pct_buf), {
                                                    .textColor = ds_theme->subtext,
                                                    .fontSize = (uint16_t)sz->font_size,
                                                });
                }
            }
        }

        /* Track (background bar) */
        CLAY(CLAY_SIDI(CLAY_STRING("UIProgressTrack"), uid), {
                                                                 .layout = {
                                                                     .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIXED((float)sz->bar_height) },
                                                                     .childAlignment = { CLAY_ALIGN_X_LEFT, CLAY_ALIGN_Y_CENTER },
                                                                 },
                                                                 .backgroundColor = ds_theme->surface0,
                                                                 .cornerRadius = CLAY_CORNER_RADIUS((float)sz->radius),
                                                                 .clip = { .horizontal = true },
                                                             })
        {
            if (indeterminate) {
                /* Indeterminate: animated pulse bar */
                /* Advance time (approximate, tied to frame rate) */
                s_indet_time += 0.032f;
                if (s_indet_time > 2.0f)
                    s_indet_time -= 2.0f;

                /* Position: ping-pong between 0 and 1 */
                float t = s_indet_time < 1.0f ? s_indet_time : 2.0f - s_indet_time;
                float bar_w_pct = 0.35f; /* bar takes 35% of track */
                float pos_pct = t * (1.0f - bar_w_pct);

                /* Spacer to position the pulse bar */
                if (pos_pct > 0.001f) {
                    CLAY(CLAY_SIDI(CLAY_STRING("UIProgressIndetSpace"), uid), {
                                                                                .layout = {
                                                                                    .sizing = { CLAY_SIZING_PERCENT(pos_pct), CLAY_SIZING_GROW(0) },
                                                                                },
                                                                            })
                    {
                    }
                }

                /* Pulse bar */
                Clay_Color pulse_color = fill_color;
                pulse_color.a = 180; /* Slightly transparent for pulse effect */
                CLAY(CLAY_SIDI(CLAY_STRING("UIProgressIndetBar"), uid), {
                                                                           .layout = {
                                                                               .sizing = { CLAY_SIZING_PERCENT(bar_w_pct), CLAY_SIZING_GROW(0) },
                                                                           },
                                                                           .backgroundColor = pulse_color,
                                                                           .cornerRadius = CLAY_CORNER_RADIUS((float)sz->radius),
                                                                       })
                {
                }
            } else {
                /* Determinate: fill bar */
                if (pct > 0.001f) {
                    CLAY(CLAY_SIDI(CLAY_STRING("UIProgressFill"), uid), {
                                                                           .layout = {
                                                                               .sizing = { CLAY_SIZING_PERCENT(pct), CLAY_SIZING_GROW(0) },
                                                                           },
                                                                           .backgroundColor = fill_color,
                                                                           .cornerRadius = CLAY_CORNER_RADIUS((float)sz->radius),
                                                                       })
                    {
                    }
                }
            }
        }
    }
}

/* ---- Simple API (backward compatible) ---- */
void UI_Progress(int uid, const char *label, float value, float max_value)
{
    UI_ProgressEx(uid, label, value, max_value, NULL);
}
