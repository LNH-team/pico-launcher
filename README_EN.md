# Pico Launcher Chinese Enhanced v3.0

Based on [MattiaTheBest115/pico-launcher](https://github.com/MattiaTheBest115/pico-launcher) develop branch

[中文说明](README_CN.md)

## Features

### Chinese Support
- WenQuanYi bitmap font for full CJK rendering
- Game titles prioritize Chinese banner
- Bilingual UI (Chinese/English toggle)
- Chinese filename support (FatFs code page 936)

### Touch Screen (from Mattia fork)
- Swipe to browse game list
- Tap to select games and toggle settings

### BGM Background Music
- 155 tracks (3DS / DSi / Wii / Wii U / Switch Sports / PS Vita / NS2)
- Tree-style category menu with fold/expand
- Random / specific track / off
- Chinese translated track names

### Theme System
- Multiple themes (built-in + custom)
- Category-based selection menu
- Chinese translated theme names

### Dark Mode
- One-tap toggle in settings, instant effect

### 3D CoverFlow
- 3D perspective cover rotation on Material theme

### Game Covers
- 4,658 NDS covers (GameTDB)
- 2,044 GBA covers (libretro-thumbnails)

### Cheats
- 7,091 entries cheat database
- Press Y to open cheat panel

### GBA Support
- Built-in GBARunner2 loader
- .gba file association configured

### Multi-Launcher
- TWiLight Menu++ v27.23.0
- AKMenu-Next v2.0.5 (Chinese configured)

## Installation

Extract the archive to SD card root and overwrite.

## Downloads

| File | Size | Description |
|------|------|-------------|
| Full | 811 MB | Everything included |
| Lite | 33 MB | No covers/BGM/themes |
| NDS Covers | 36 MB | 4,658 covers |
| GBA Covers | 17 MB | 2,044 covers |
| BGM | 597 MB | 155 tracks |
| Custom Themes | 128 MB | 13 themes |

## Settings Menu

Press SELECT to open, use D-pad or touch screen:

| Option | Description |
|--------|-------------|
| Layout | Display mode (Grid/List/CoverFlow) |
| Sorting | Sort order |
| Theme | Press A to open theme list |
| Language | Press A to toggle Chinese/English |
| Dark Mode | Press A to toggle |
| BGM | Press A to open BGM list |

## Building

```bash
docker run --rm -v "$(pwd):/work" -w /work skylyrac/blocksds:slim-latest make
```

## Credits

- [LNH-team](https://github.com/LNH-team/pico-launcher) — Original pico-launcher
- [MattiaTheBest115](https://github.com/MattiaTheBest115/pico-launcher) — Touch/themes/BGM features
- [DS-Homebrew](https://github.com/DS-Homebrew) — TWiLight Menu++ / nds-bootstrap
- [coderkei](https://github.com/coderkei/akmenu-next) — AKMenu-Next
- Community — Cheat database, cover art, custom themes, BGM music
