# Kilnui

A lightweight UI rendering library built on [Clay](https://github.com/nicbarker/clay) and SDL3 GPU.

Kilnui gives you a minimal, high-performance path from layout to pixels: Clay handles layout, SDL3 GPU handles rendering, and Kilnui connects them with efficient batching, glyph caching, and HiDPI support.

## Features

- **GPU-accelerated rendering** via SDL3's Vulkan-backed GPU API
- **SDF rounded rectangles** with per-corner radius
- **Crisp text** with per-glyph GPU texture caching
- **Shadows, borders, gradients** through custom shader passes
- **HiDPI / fractional scaling** out of the box
- **Design system** with Catppuccin themes, spacing tokens, and reusable widgets
- **Zero heap allocations** on the hot path (persistent GPU buffers, static batching)

## Component Library

Ready-to-use widgets built on top of the core renderer:

Button, Input, Checkbox, Radio, Slider, Dropdown, Progress, Tooltip, Badge, Chip, Divider, Avatar, Alert

## Building

Dependencies: SDL3, SDL3_ttf, glslc (from Vulkan SDK)

```bash
cmake -B build
cmake --build build
```

Run the demos:

```bash
./build/clay_demo
./build/component_gallery
./build/design_system_demo
```

## Project Structure

```
kilnui/
├── src/
│   ├── kilnui.h            # Public API
│   ├── kilnui.c            # Context lifecycle (init, events, destroy)
│   ├── kilnui_render.c     # Hot-path rendering (batching, GPU upload, draw)
│   ├── glyph_cache.c/h     # Per-glyph GPU texture cache
│   └── ui/                 # Reusable widget library
├── shaders/                # GLSL shaders (compiled to SPIR-V at build time)
├── demo/                   # Demo applications
├── 3rdparty/clay/          # Clay layout library (vendored)
└── CMakeLists.txt
```

## Usage

```c
#include "kilnui.h"
#include "ui/ui.h"

KilnUI ctx;
KilnUI_init(&ctx, "My App", 800, 600, "font.ttf", 16);

// In your render loop:
Clay_BeginLayout();
UI_ROW(1, 8) {
    UI_Button(2, "Click me", UI_BTN_PRIMARY, UI_MD, false);
    CLAY_TEXT(CLAY_STRING("Hello"), &(Clay_TextElementConfig){ .fontSize = 16 });
}
Clay_RenderCommandArray cmds = Clay_EndLayout();
KilnUI_render(&ctx, cmds);

KilnUI_destroy(&ctx);
```

## License

See individual source files for licensing.
