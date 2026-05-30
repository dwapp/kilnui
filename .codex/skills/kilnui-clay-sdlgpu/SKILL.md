---
name: kilnui-clay-sdlgpu
description: Use this skill when working in the kilnui project on Clay UI layout, widgets, demos, or rendering integration. This project uses Clay only through kilnui's SDL3 GPU backend; do not use Clay's SDL2/SDL3 software renderer, OpenGL/GLES, Raylib, Sokol, browser/Wasm, or other renderer paths.
---

# KilnUI Clay SDL GPU

## Scope

This skill is project-specific for `/home/deepin/ui/kilnui`.

KilnUI integrates Clay with SDL3 GPU. Treat Clay as the layout/input command generator and `src/kilnui_render.c` as the renderer. Do not introduce alternate Clay renderers from `/home/deepin/ui/clay/renderers` or examples unless the user explicitly asks for comparison only.

Important local files:

- `3rdparty/clay/clay.h`: vendored Clay single-header library.
- `src/kilnui.h`: public KilnUI API and SDL GPU context.
- `src/kilnui.c`: SDL/TTF/GPU setup, Clay initialization, input routing, text measurement.
- `src/kilnui_render.c`: SDL3 GPU renderer for Clay render commands.
- `src/ui/button.*`: reusable Clay widget pattern.
- `demo/*.c`: usage examples.
- `CMakeLists.txt`: builds `kilnui`, widgets, demos, and GLSL-to-SPIR-V shaders.

## Clay Usage In This Project

Clay owns layout only:

1. Initialize via `KilnUI_init`, which creates SDL window/GPU/TTF resources, allocates Clay arena memory, calls `Clay_Initialize`, and registers `Clay_SetMeasureTextFunction`.
2. Feed SDL events through `KilnUI_handle_event`; it updates Clay pointer state, scroll containers, layout dimensions, and DPI scale.
3. Per frame, call `Clay_BeginLayout()`, declare UI with `CLAY(...)` and `CLAY_TEXT(...)`, then call `Clay_EndLayout(dt)`.
4. Pass the returned `Clay_RenderCommandArray` to `KilnUI_render`.

Minimal frame shape:

```c
KilnUI_handle_event(&ctx, &event);

Clay_BeginLayout();
CLAY(CLAY_ID("Root"), {
    .layout = {
        .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0) },
        .padding = CLAY_PADDING_ALL(16),
        .childGap = 12,
    },
}) {
    CLAY_TEXT(CLAY_STRING("Hello KilnUI"), {
        .fontSize = 18,
        .textColor = { 255, 255, 255, 255 },
    });
}
Clay_RenderCommandArray cmds = Clay_EndLayout(dt);
KilnUI_render(&ctx, cmds);
```

## Local Clay Patterns

- Use stable IDs: `CLAY_ID("Name")` for singleton elements and `CLAY_SIDI(CLAY_STRING("Name"), index)` or `Clay_GetElementIdWithIndex` for repeated elements.
- For hover/click state, query IDs with `Clay_PointerOver(id)` after `KilnUI_handle_event` has processed input for the frame.
- For reusable widgets, follow `src/ui/button.c`: widget functions emit Clay nodes and return interaction state; callers provide unique integer IDs.
- For scrollable regions, use `.clip = { .vertical = true, .childOffset = Clay_GetScrollOffset() }` and rely on `KilnUI_handle_event` to call `Clay_UpdateScrollContainers`.
- Clay strings are length-aware. Use `CLAY_STRING("literal")` for literals or build `Clay_String` with `SDL_strlen` for runtime C strings.

## Renderer Constraints

KilnUI currently handles these Clay command types:

- `CLAY_RENDER_COMMAND_TYPE_RECTANGLE`
- `CLAY_RENDER_COMMAND_TYPE_TEXT`
- `CLAY_RENDER_COMMAND_TYPE_SCISSOR_START`
- `CLAY_RENDER_COMMAND_TYPE_SCISSOR_END`

Before using Clay features that emit images, custom elements, borders, or other command types, inspect `src/kilnui_render.c` and implement the SDL GPU rendering path there. Avoid adding UI declarations that silently rely on unsupported render commands.

Rendering details to preserve:

- Use SDL3 GPU APIs, not `SDL_Renderer`, OpenGL, GLES, Raylib, or Sokol.
- Preserve Clay command order during rendering so text and rectangles keep the intended layering.
- Upload copy data before render passes; SDL GPU does not allow copy work inside an active render pass.
- Keep DPI handling consistent: Clay layout uses logical coordinates, renderer scales to physical pixels via `ctx->dpi_scale`.
- Text measurement and text rendering both use SDL_ttf; keep font size, kerning, and glyph cache behavior aligned.

## Build And Verify

Use the existing CMake flow:

```sh
cmake -S . -B build
cmake --build build
```

For UI/widget changes, prefer compiling at minimum. If runtime verification is needed, run the relevant demo from `build/` so copied shader and asset paths resolve.
