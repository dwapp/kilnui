# KilnUI Design System

A design token–driven UI system for the KilnUI Clay + SDL3 GPU rendering backend.

---

## File Overview

| File | Purpose |
|------|---------|
| `design_system.h/.c` | Design tokens + dual-theme data |
| `typography.h/.c` | 10 text presets + LabelValue component |
| `icons.h` | 120+ Unicode icon constants + helpers |
| `theme.h/.c` | Badge, Chip, Divider, Backdrop, Avatar, Alert |
| `ui.h` | Unified include + layout macros |

---

## Design Tokens (`design_system.h`)

### Spacing
```c
DS_SPACE_1  =  4px    DS_SPACE_6  = 24px
DS_SPACE_2  =  8px    DS_SPACE_8  = 32px
DS_SPACE_3  = 12px    DS_SPACE_10 = 40px
DS_SPACE_4  = 16px    DS_SPACE_12 = 48px
DS_SPACE_5  = 20px    DS_SPACE_16 = 64px
```

### Corner Radii
```c
DS_RADIUS_NONE=0  DS_RADIUS_XS=2  DS_RADIUS_SM=4  DS_RADIUS_MD=8
DS_RADIUS_LG=12   DS_RADIUS_XL=16 DS_RADIUS_2XL=24 DS_RADIUS_FULL=9999
```

### Font Sizes
```c
DS_FS_XS=10  DS_FS_SM=12  DS_FS_MD=14  DS_FS_LG=16
DS_FS_XL=20  DS_FS_2XL=24 DS_FS_3XL=32 DS_FS_4XL=40
```

### Component Heights
```c
DS_HEIGHT_XS=24  DS_HEIGHT_SM=32  DS_HEIGHT_MD=40
DS_HEIGHT_LG=48  DS_HEIGHT_XL=56
```

---

## Themes

```c
DS_SetTheme(&DS_THEME_DARK);   // Catppuccin Mocha (default)
DS_SetTheme(&DS_THEME_LIGHT);  // Catppuccin Latte

// Access active theme colors
ds_theme->base       // app background
ds_theme->surface0   // card background
ds_theme->accent     // primary accent (mauve/purple)
ds_theme->text       // primary text
ds_theme->error      // red / danger
```

### Variant Colors (8 roles)
```c
DS_VARIANT_DEFAULT  DS_VARIANT_PRIMARY  DS_VARIANT_SUCCESS
DS_VARIANT_WARNING  DS_VARIANT_ERROR    DS_VARIANT_INFO
DS_VARIANT_MUTED    DS_VARIANT_ACCENT

Clay_Color bg = DS_VariantBg(DS_VARIANT_SUCCESS);
Clay_Color fg = DS_VariantFg(DS_VARIANT_SUCCESS);
```

---

## Typography (`typography.h`)

```c
// 10 presets
TY_Text(uid, "Hello", TY_DISPLAY);   // 40px accent
TY_Text(uid, "Title", TY_H1);        // 32px
TY_Text(uid, "Section", TY_H2);      // 24px
TY_Text(uid, "Card", TY_H3);         // 20px
TY_Text(uid, "Sub", TY_H4);          // 16px
TY_Text(uid, "Body text", TY_BODY);  // 14px (default)
TY_Text(uid, "Small", TY_SMALL);     // 12px subtext
TY_Text(uid, "Hint", TY_CAPTION);    // 10px muted
TY_Text(uid, "code()", TY_CODE);     // 14px accent_alt
TY_Text(uid, "LABEL", TY_OVERLINE);  // 10px muted

// Custom color override
TY_TextColored(uid, "Error!", TY_H3, ds_theme->error);

// Label-Value pair
TY_LabelValue(uid, "Status", "Active");

// Layout helpers
TY_Center(uid, "Centered", TY_BODY);
TY_Truncate(uid, "Long text...", TY_BODY);
```

---

## Icons (`icons.h`)

Over 120 Unicode icons organized by category. Works with Noto Sans.

```c
// Navigation
ICON_ARROW_LEFT  ICON_ARROW_RIGHT  ICON_HOME  ICON_ARROW_REFRESH
ICON_CHEVRON_LEFT  ICON_CHEVRON_RIGHT  ICON_TRI_UP  ICON_TRI_DOWN

// Actions
ICON_PLUS  ICON_MINUS  ICON_CLOSE  ICON_CHECK  ICON_EDIT
ICON_DELETE  ICON_SEARCH  ICON_ADD_CIRCLE  ICON_REMOVE_CIRCLE

// UI Controls
ICON_MENU  ICON_SETTINGS  ICON_MORE_V  ICON_MORE_H
ICON_FILTER  ICON_SORT  ICON_TOGGLE_ON  ICON_CHECKBOX_ON

// Status
ICON_WARNING  ICON_INFO  ICON_SUCCESS  ICON_ERROR
ICON_STAR  ICON_HEART  ICON_BOLT  ICON_HOURGLASS

// Media
ICON_PLAY  ICON_PAUSE  ICON_STOP  ICON_RECORD
ICON_SKIP_NEXT  ICON_SKIP_PREV
```

### Icon Helpers

```c
// Render icon at given size and color
UI_Icon(uid, ICON_CHECK, DS_FS_LG, ds_theme->success);

// Clickable icon button
if (UI_IconButton(uid, ICON_SETTINGS, DS_FS_LG, UI_BTN_GHOST, false)) {
    // handle click
}
```

---

## Theme Components (`theme.h`)

### Badge
```c
UI_Badge(uid, "New",    DS_VARIANT_PRIMARY);
UI_Badge(uid, "Error",  DS_VARIANT_ERROR);
UI_Badge(uid, "Beta",   DS_VARIANT_WARNING);
```

### Chip (dismissible)
```c
static bool shown = true;
if (!UI_Chip(uid, "dark-mode", selected, true))
    shown = false;  // dismissed
```

### Divider
```c
UI_Divider(uid);
UI_DividerLabel(uid, "OR");
```

### Alert
```c
UI_Alert(uid, ICON_WARNING " Disk almost full", DS_VARIANT_WARNING);
UI_Alert(uid, ICON_SUCCESS " Saved!", DS_VARIANT_SUCCESS);
```

### Avatar
```c
UI_Avatar(uid, "JD", DS_VARIANT_PRIMARY, DS_HEIGHT_MD);
```

### Backdrop
```c
UI_Backdrop(uid, modal_open);  // full-screen overlay
```

---

## Layout Helpers (`ui.h`)

```c
// Horizontal row with gap
UI_ROW(uid, 8) {
    UI_Button(...);
    UI_SPACER(uid);     // flex-grow filler
    UI_Badge(...);
}

// Vertical column
UI_COL(uid, 16) {
    TY_Text(...);
    UI_Divider(...);
}

// Centered container
UI_CENTER(uid) {
    TY_Text(uid, "Loading...", TY_H3);
}

// Padded panel (surface0 background)
UI_PANEL(uid, DS_SPACE_4, DS_SPACE_3) {
    TY_Text(...);
}

// Vertical scrollable column
UI_SCROLLCOL(uid, DS_SPACE_3) {
    // long content
}
```

---

## Demo

```sh
cmake --build build --target design_system_demo
./build/design_system_demo
```

Tabs: **Colors · Typography · Spacing · Icons · Components · Badges · Alerts**

Press ☀/☽ button in the top-right to toggle dark/light theme.
