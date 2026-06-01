/* src/ui/typography.c — Typography System implementation */

#include "typography.h"

/* ---- Per-style metrics ---- */
typedef struct { uint16_t fs; Clay_Color (*get_color)(void); } TYMeta;

static Clay_Color col_accent(void)   { return ds_theme->accent;     }
static Clay_Color col_text(void)     { return ds_theme->text;        }
static Clay_Color col_subtext(void)  { return ds_theme->subtext;     }
static Clay_Color col_muted(void)    { return ds_theme->muted;       }
static Clay_Color col_code(void)     { return ds_theme->accent_alt;  }

static const uint16_t TY_FS[TY_STYLE_COUNT]     = { 40, 32, 24, 20, 16, 14, 12, 10, 14, 10 };
static Clay_Color (*const TY_COL[TY_STYLE_COUNT])(void) = {
    col_accent,   /* DISPLAY  */
    col_text,     /* H1       */
    col_text,     /* H2       */
    col_text,     /* H3       */
    col_text,     /* H4       */
    col_text,     /* BODY     */
    col_subtext,  /* SMALL    */
    col_muted,    /* CAPTION  */
    col_code,     /* CODE     */
    col_muted,    /* OVERLINE */
};

/* ---- TY_Text ---- */
void TY_Text(int uid, const char *text, TYStyle style)
{
    if (style < 0 || style >= TY_STYLE_COUNT) style = TY_BODY;
    TY_TextColored(uid, text, style, TY_COL[style]());
}

/* ---- TY_TextColored ---- */
void TY_TextColored(int uid, const char *text, TYStyle style, Clay_Color color)
{
    if (style < 0 || style >= TY_STYLE_COUNT) style = TY_BODY;
    CLAY(CLAY_SIDI(CLAY_STRING("TYText"), uid), {
        .layout = {
            .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIT(0) },
        },
    }) {
        CLAY_TEXT(UI__str(text), { .textColor = color, .fontSize = TY_FS[style] });
    }
}

/* ---- TY_LabelValue ---- */
void TY_LabelValue(int uid, const char *label, const char *value)
{
    CLAY(CLAY_SIDI(CLAY_STRING("TYLabelValue"), uid), {
        .layout = {
            .sizing        = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIT(0) },
            .layoutDirection = CLAY_TOP_TO_BOTTOM,
            .childGap       = 2,
        },
    }) {
        /* Label */
        CLAY(CLAY_SIDI(CLAY_STRING("TYLbl"), uid), {
            .layout = { .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIT(0) } },
        }) {
            CLAY_TEXT(UI__str(label),
                      { .textColor = ds_theme->muted, .fontSize = TY_FS[TY_CAPTION] });
        }
        /* Value */
        CLAY(CLAY_SIDI(CLAY_STRING("TYVal"), uid), {
            .layout = { .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIT(0) } },
        }) {
            CLAY_TEXT(UI__str(value),
                      { .textColor = ds_theme->text, .fontSize = TY_FS[TY_BODY] });
        }
    }
}

/* ---- TY_Center ---- */
void TY_Center(int uid, const char *text, TYStyle style)
{
    if (style < 0 || style >= TY_STYLE_COUNT) style = TY_BODY;
    CLAY(CLAY_SIDI(CLAY_STRING("TYCenter"), uid), {
        .layout = {
            .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIT(0) },
            .childAlignment = { CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER },
        },
    }) {
        CLAY_TEXT(UI__str(text),
                  { .textColor = TY_COL[style](), .fontSize = TY_FS[style] });
    }
}

/* ---- TY_Truncate ---- */
void TY_Truncate(int uid, const char *text, TYStyle style)
{
    if (style < 0 || style >= TY_STYLE_COUNT) style = TY_BODY;
    CLAY(CLAY_SIDI(CLAY_STRING("TYTrunc"), uid), {
        .layout = {
            .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIT(0) },
            .childAlignment = { CLAY_ALIGN_X_LEFT, CLAY_ALIGN_Y_CENTER },
        },
        .clip = { .horizontal = true },
    }) {
        CLAY_TEXT(UI__str(text),
                  { .textColor = TY_COL[style](), .fontSize = TY_FS[style] });
    }
}
