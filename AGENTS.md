# AGENTS.md — AI Guide for Kilnui

## What is this?

Kilnui is a C23 UI rendering library that bridges **Clay** (CPU layout) with **SDL3 GPU** (GPU rendering). It provides a widget library on top.

## Key Architecture

```
Clay layout → Clay_RenderCommandArray → KilnUI_render() → SDL3 GPU draw calls
```

### Core files

| File | Role |
|------|------|
| `src/kilnui.h` | Public API: `KilnUI_init`, `KilnUI_handle_event`, `KilnUI_render`, `KilnUI_destroy` |
| `src/kilnui.c` | Lifecycle: SDL/GPU/font/Clay init, event routing, teardown. Contains `CLAY_IMPLEMENTATION`. |
| `src/kilnui_render.c` | Hot path: ortho projection, rect/text/shadow/border batching, single-pass GPU upload, render pass |
| `src/glyph_cache.c/h` | Open-addressing hash table of per-glyph GPU textures. Deferred batch upload via `GlyphCache_flush_uploads()`. |
| `src/ui/` | Widget library (button, input, checkbox, radio, slider, dropdown, progress, tooltip, badge, chip, divider, avatar, alert) |
| `src/ui/ui.h` | Master include: pulls in all widgets + layout macros (`UI_ROW`, `UI_COL`, `UI_SPACER`, etc.) |
| `src/ui/design_system.h` | Design tokens: spacing, radii, font sizes, Catppuccin themes |
| `shaders/*.glsl` | GLSL shaders compiled to SPIR-V by `glslc` at build time |

### Render pipeline

1. **Phase 1 — Collect**: Walk Clay commands, build rect vertices into static `s_rect_verts[]` (SDF rounded rects), build text quads per TEXT command via glyph cache. Map each command to a buffer slot.
2. **Phase 2 — Upload**: One transfer buffer, one copy pass for all rect + text geometry.
3. **Phase 3 — Draw**: Single render pass. Iterate Clay commands in order (Z-order preserved). Switch pipelines only on type change. Consecutive RECTANGLE commands are merged into one draw call.

### Key design decisions

- **Persistent GPU buffers** (grow-only, never shrunk mid-session) — avoids per-frame alloc overhead
- **Static batching arrays** (`s_rect_verts[MAX_RECTS * 4]`, `s_text_batches[MAX_TEXT_CMDS]`) — zero hot-path malloc
- **Deferred glyph upload**: `GlyphCache_get()` stages surfaces in `pending[]`, `GlyphCache_flush_uploads()` does one batch upload before the render pass
- **Global state**: `g_measure_ctx`, static arrays — single-window, single-threaded by design
- **HiDPI**: `dpi_scale` for rendering, `mouse_scale` for input. Physical-pixel glyph rasterization.

### Widget pattern

All widgets in `src/ui/` follow the same pattern:
1. Compute Clay_ElementId from name + uid
2. Check hover/press/click via `Clay_PointerOver()` + global mouse state
3. Emit Clay elements with themed colors from `ds_theme`

Global mouse state is set by the app via `UI_SetPointerState()` each frame.

## Build system

CMake 3.20+, C23. Dependencies: SDL3, SDL3_ttf, glslc.

```bash
cmake -B build && cmake --build build
```

Shaders are compiled GLSL → SPIR-V and copied next to demo executables post-build.

## Conventions

- C23 standard (`-Wall -Wextra -Wno-unused-parameter`)
- `.clang-format` present (SDL-style)
- Clay IDs: use `CLAY_ID_LOCAL("name")` for internal elements, `Clay_GetElementIdWithIndex(CLAY_STRING("prefix"), uid)` for stateful widgets
- Colors: always premultiplied alpha (`r = (c.r/255) * (c.a/255)`)
- Font size: physical pixels = logical_size * dpi_scale

## Constants

| Name | Value | Location |
|------|-------|----------|
| MAX_RECTS | 8192 | kilnui.h |
| MAX_TEXT_CMDS | 64 | kilnui.h |
| MAX_CMDS | 2048 | kilnui_render.c |
| MAX_PENDING_GLYPH_UPLOADS | 512 | glyph_cache.h |
| TextBatch max quads | 256 | kilnui_render.c (TextBatch struct) |

## Gotchas

- `Clay_StringSlice` is NOT null-terminated — always use `.length`
- `GlyphCache_get()` requires caller to have called `TTF_SetFontSize()` first
- `measure_text_cb` uses a global pointer (`g_measure_ctx`) — no multi-window support
- Custom render commands (shadow/border) use `KilnUICustomHeader.type` to dispatch
- Shadow/border pools in button.c use static arrays with `unsigned int` index (wrap-safe)
