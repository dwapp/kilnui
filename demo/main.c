/* demo/main.c — Clay + SDL3 GPU demo showcasing UI features:
 *   - Rounded-corner containers (SDF rect pipeline)
 *   - Multi-line text (glyph cache + kerning)
 *   - Scrollable list (SCISSOR commands)
 *   - Hover highlight (Clay pointer hit-testing)
 */

#include "../src/kilnui.h"
#include "../src/ui/ui.h"
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <math.h>
#include <stdio.h>

static Clay_Color col_hover(Clay_Color base)
{
    return (Clay_Color){
        fminf(base.r + 30, 255), fminf(base.g + 30, 255),
        fminf(base.b + 30, 255), base.a
    };
}

/* ---- Sidebar navigation ---- */
static void ui_sidebar(void)
{
    CLAY(CLAY_ID("Sidebar"), {
                                 .layout = {
                                     .sizing = { CLAY_SIZING_FIXED(220), CLAY_SIZING_GROW(0) },
                                     .padding = { 16, 16, 20, 20 },
                                     .childGap = 8,
                                     .layoutDirection = CLAY_TOP_TO_BOTTOM,
                                 },
                                 .backgroundColor = ds_theme->surface0,
                                 .cornerRadius = CLAY_CORNER_RADIUS(12),
                             })
    {
        /* Logo area */
        CLAY(CLAY_ID("Logo"), {
                                  .layout = {
                                      .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIXED(48) },
                                      .childAlignment = { CLAY_ALIGN_X_LEFT, CLAY_ALIGN_Y_CENTER },
                                  },
                              })
        {
            CLAY_TEXT(CLAY_STRING("Kiln UI"), {
                                                        .textColor = ds_theme->accent,
                                                        .fontSize = 18,
                                                    });
        }

        /* Separator */
        CLAY(CLAY_ID("Sep"), {
                                 .layout = { .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIXED(1) } },
                                 .backgroundColor = ds_theme->surface2,
                             })
        {
        }

        /* Nav items */
        const char *labels[] = { "Dashboard", "Components", "Settings", "About" };
        Clay_Color colors[] = { ds_theme->mauve, ds_theme->green, ds_theme->peach, ds_theme->blue };
        for (int i = 0; i < 4; i++) {
            Clay_String lbl = { .chars = labels[i], .length = (int32_t)SDL_strlen(labels[i]) };
            Clay_ElementId eid = Clay_GetElementIdWithIndex(CLAY_STRING("Nav"), i);
            bool hovered = Clay_PointerOver(eid);
            CLAY(CLAY_SIDI(CLAY_STRING("Nav"), i), {
                                                       .layout = {
                                                           .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIXED(40) },
                                                           .padding = { 12, 12, 8, 8 },
                                                           .childAlignment = { CLAY_ALIGN_X_LEFT, CLAY_ALIGN_Y_CENTER },
                                                       },
                                                       .backgroundColor = hovered ? ds_theme->surface1 : (Clay_Color){ 0, 0, 0, 0 },
                                                       .cornerRadius = CLAY_CORNER_RADIUS(8),
                                                   })
            {
                CLAY_TEXT(lbl, {
                                   .textColor = hovered ? colors[i] : ds_theme->subtext,
                                   .fontSize = 14,
                               });
            }
        }
    }
}

/* ---- Stat card ---- */
static void ui_stat_card(int idx, const char *title, const char *value,
                         Clay_Color accent)
{
    Clay_String t = { .chars = title, .length = (int32_t)SDL_strlen(title) };
    Clay_String v = { .chars = value, .length = (int32_t)SDL_strlen(value) };
    Clay_ElementId eid = Clay_GetElementIdWithIndex(CLAY_STRING("Stat"), idx);
    bool hov = Clay_PointerOver(eid);

    CLAY(CLAY_SIDI(CLAY_STRING("Stat"), idx), {
                                                  .layout = {
                                                      .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIXED(100) },
                                                      .padding = { 16, 16, 14, 14 },
                                                      .childGap = 8,
                                                      .layoutDirection = CLAY_TOP_TO_BOTTOM,
                                                  },
                                                  .backgroundColor = hov ? col_hover(ds_theme->surface0) : ds_theme->surface0,
                                                  .cornerRadius = CLAY_CORNER_RADIUS(10),
                                              })
    {
        CLAY_TEXT(t, { .textColor = ds_theme->subtext, .fontSize = 13 });
        CLAY_TEXT(v, { .textColor = accent, .fontSize = 28 });
    }
}

/* ---- Progress bar ---- */
static void ui_progress(int idx, const char *label, float pct, Clay_Color color)
{
    Clay_String l = { .chars = label, .length = (int32_t)SDL_strlen(label) };
    CLAY(CLAY_SIDI(CLAY_STRING("Prog"), idx), {
                                                  .layout = {
                                                      .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIT(0) },
                                                      .childGap = 6,
                                                      .layoutDirection = CLAY_TOP_TO_BOTTOM,
                                                  },
                                              })
    {
        CLAY_TEXT(l, { .textColor = ds_theme->subtext, .fontSize = 12 });
        CLAY(CLAY_SIDI(CLAY_STRING("PBg"), idx), {
                                                     .layout = { .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIXED(10) } },
                                                     .backgroundColor = ds_theme->surface1,
                                                     .cornerRadius = CLAY_CORNER_RADIUS(5),
                                                 })
        {
            CLAY(CLAY_SIDI(CLAY_STRING("PFg"), idx), {
                                                         .layout = { .sizing = { CLAY_SIZING_PERCENT(pct), CLAY_SIZING_GROW(0) } },
                                                         .backgroundColor = color,
                                                         .cornerRadius = CLAY_CORNER_RADIUS(5),
                                                     })
            {
            }
        }
    }
}

/* ---- Scrollable list ---- */
static void ui_scroll_list(void)
{
    CLAY(CLAY_ID("ScrollOuter"), {
                                     .layout = {
                                         .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0) },
                                         .layoutDirection = CLAY_TOP_TO_BOTTOM,
                                         .childGap = 4,
                                     },
                                 })
    {
        CLAY_TEXT(CLAY_STRING("Event Log (scroll me)"), {
                                                            .textColor = ds_theme->subtext,
                                                            .fontSize = 13,
                                                        });

        CLAY(CLAY_ID("ScrollArea"), {
                                        .layout = {
                                            .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0) },
                                            .padding = { 8, 8, 8, 8 },
                                            .childGap = 6,
                                            .layoutDirection = CLAY_TOP_TO_BOTTOM,
                                        },
                                        .backgroundColor = ds_theme->surface1,
                                        .cornerRadius = CLAY_CORNER_RADIUS(8),
                                        .clip = { .vertical = true, .childOffset = Clay_GetScrollOffset() },
                                    })
        {
            const char *events[] = {
                "System initialized",
                "GPU device created (Vulkan)",
                "Shader pipelines compiled",
                "Font loaded: Inter-Regular",
                "Glyph cache: 512 slots",
                "Layout engine ready",
                "First frame rendered",
                "Mouse enter detected",
                "Scroll container active",
                "Hover state changed",
                "Render batch: 24 rects",
                "Text batch: 156 glyphs",
                "Frame time: 2.1ms",
                "VSync: enabled",
                "Memory: 12.4 MB used",
            };
            for (int i = 0; i < 15; i++) {
                Clay_String s = { .chars = events[i], .length = (int32_t)SDL_strlen(events[i]) };
                Clay_ElementId eid = Clay_GetElementIdWithIndex(CLAY_STRING("Evt"), i);
                bool hov = Clay_PointerOver(eid);
                CLAY(CLAY_SIDI(CLAY_STRING("Evt"), i), {
                                                           .layout = {
                                                               .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIXED(32) },
                                                               .padding = { 10, 10, 6, 6 },
                                                               .childAlignment = { CLAY_ALIGN_X_LEFT, CLAY_ALIGN_Y_CENTER },
                                                           },
                                                           .backgroundColor = hov ? ds_theme->overlay : (Clay_Color){ 0, 0, 0, 0 },
                                                           .cornerRadius = CLAY_CORNER_RADIUS(4),
                                                       })
                {
                    CLAY_TEXT(s, {
                                     .textColor = hov ? ds_theme->text : ds_theme->subtext,
                                     .fontSize = 13,
                                 });
                }
            }
        }
    }
}

/* ---- Main content area ---- */
static void ui_content(void)
{
    CLAY(CLAY_ID("Content"), {
                                 .layout = {
                                     .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0) },
                                     .padding = { 24, 24, 20, 20 },
                                     .childGap = 20,
                                     .layoutDirection = CLAY_TOP_TO_BOTTOM,
                                 },
                             })
    {
        /* Title */
        CLAY_TEXT(CLAY_STRING("Dashboard"), {
                                                .textColor = ds_theme->text,
                                                .fontSize = 24,
                                            });

        /* Stats row */
        CLAY(CLAY_ID("StatsRow"), {
                                      .layout = {
                                          .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIT(0) },
                                          .childGap = 16,
                                      },
                                  })
        {
            ui_stat_card(0, "Rectangles", "24", ds_theme->mauve);
            ui_stat_card(1, "Glyphs", "156", ds_theme->green);
            ui_stat_card(2, "Frame Time", "2.1ms", ds_theme->peach);
            ui_stat_card(3, "GPU Memory", "12 MB", ds_theme->blue);
        }

        /* Bottom row: progress + scroll list */
        CLAY(CLAY_ID("BottomRow"), {
                                       .layout = {
                                           .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0) },
                                           .childGap = 16,
                                       },
                                   })
        {
            /* Left: progress bars */
            CLAY(CLAY_ID("ProgressPanel"), {
                                               .layout = {
                                                   .sizing = { CLAY_SIZING_PERCENT(0.45f), CLAY_SIZING_GROW(0) },
                                                   .padding = { 16, 16, 16, 16 },
                                                   .childGap = 14,
                                                   .layoutDirection = CLAY_TOP_TO_BOTTOM,
                                               },
                                               .backgroundColor = ds_theme->surface0,
                                               .cornerRadius = CLAY_CORNER_RADIUS(10),
                                           })
            {
                CLAY_TEXT(CLAY_STRING("System Resources"), {
                                                               .textColor = ds_theme->text,
                                                               .fontSize = 15,
                                                           });
                ui_progress(0, "CPU Usage", 0.62f, ds_theme->mauve);
                ui_progress(1, "Memory", 0.45f, ds_theme->green);
                ui_progress(2, "GPU Load", 0.78f, ds_theme->peach);
                ui_progress(3, "Disk I/O", 0.33f, ds_theme->blue);
                ui_progress(4, "Network", 0.55f, ds_theme->teal);
            }

            /* Right: scrollable event log */
            CLAY(CLAY_ID("LogPanel"), {
                                          .layout = {
                                              .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0) },
                                              .padding = { 16, 16, 16, 16 },
                                              .layoutDirection = CLAY_TOP_TO_BOTTOM,
                                          },
                                          .backgroundColor = ds_theme->surface0,
                                          .cornerRadius = CLAY_CORNER_RADIUS(10),
                                      })
            {
                ui_scroll_list();
            }
        }
    }
}

/* ---- Root layout builder ---- */
static void ui_build(void)
{
    CLAY(CLAY_ID("Root"), {
                              .layout = {
                                  .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0) },
                                  .padding = { 16, 16, 16, 16 },
                                  .childGap = 16,
                              },
                              .backgroundColor = ds_theme->base,
                          })
    {
        ui_sidebar();
        ui_content();
    }
}

/* ---- Entry point ---- */
int main(int argc, char *argv[])
{
    (void)argc;
    (void)argv;

    KilnUI ctx;
    static const char *font_candidates[] = {
        "assets/Inter-Regular.ttf",
        "/usr/share/fonts/truetype/noto/NotoSans-Regular.ttf",
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
        "/usr/share/fonts/opentype/noto/NotoSans-Regular.ttf",
        NULL
    };
    const char *font = KilnUI_find_font(font_candidates);
    if (!font) {
        SDL_Log("No usable font found");
        return 1;
    }
    SDL_Log("Using font: %s", font);
    if (!KilnUI_init(&ctx, "Kiln UI", 1280, 720, font, 16)) {
        SDL_Log("Failed to initialize KilnUI");
        return 1;
    }

    bool running = true;
    bool dirty = true; /* render at least one frame on startup */
    Uint64 last = SDL_GetPerformanceCounter();

    while (running) {
        SDL_Event e;

        if (dirty) {
            /* We already have work pending — drain the event queue without
             * blocking so we coalesce all pending input before rendering. */
            while (SDL_PollEvent(&e)) {
                if (e.type == SDL_EVENT_QUIT) {
                    running = false;
                    break;
                }
                if (e.type == SDL_EVENT_KEY_DOWN && e.key.key == SDLK_ESCAPE) {
                    running = false;
                    break;
                }
                KilnUI_handle_event(&ctx, &e);
            }
        } else {
            /* Nothing to paint — block the thread until an event arrives.
             * This is the key idle optimisation: zero CPU when the window
             * is fully static. */
            if (!SDL_WaitEvent(&e))
                break;
            dirty = true;
            if (e.type == SDL_EVENT_QUIT) {
                running = false;
                break;
            }
            if (e.type == SDL_EVENT_KEY_DOWN && e.key.key == SDLK_ESCAPE) {
                running = false;
                break;
            }
            KilnUI_handle_event(&ctx, &e);
            /* Drain remaining events that may have queued up. */
            while (SDL_PollEvent(&e)) {
                if (e.type == SDL_EVENT_QUIT) {
                    running = false;
                    break;
                }
                if (e.type == SDL_EVENT_KEY_DOWN && e.key.key == SDLK_ESCAPE) {
                    running = false;
                    break;
                }
                KilnUI_handle_event(&ctx, &e);
            }
        }

        if (!running)
            break;

        if (dirty) {
            Uint64 now = SDL_GetPerformanceCounter();
            float dt = (float)(now - last) / (float)SDL_GetPerformanceFrequency();
            last = now;

            Clay_BeginLayout();
            ui_build();
            Clay_RenderCommandArray cmds = Clay_EndLayout(dt);
            KilnUI_render(&ctx, cmds);

            dirty = false; /* frame submitted; go idle until next event */
        }
    }

    KilnUI_destroy(&ctx);
    return 0;
}
