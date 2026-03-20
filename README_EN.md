# Pico Launcher Chinese Enhanced v3.1

Based on [MattiaTheBest115/pico-launcher](https://github.com/MattiaTheBest115/pico-launcher) develop branch

[中文说明](README_CN.md)

## Features

### Chinese Support
- WenQuanYi bitmap font (GB2312 full set, 6763 chars)
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

### Theme System
- Multiple themes (built-in + custom)
- Category-based selection menu

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
- Built-in GBARunner3 loader
- .gba file association configured

### Multi-Launcher
- TWiLight Menu++ v27.23.0
- AKMenu-Next v2.0.5 (Chinese configured)

## Installation

Extract the archive to SD card root and overwrite.

## Downloads

| File | Size | Description |
|------|------|-------------|
| Full | 847 MB | Everything included |
| Lite | 32 MB | No covers/BGM/themes |

## Building

```bash
docker run --rm -v "$(pwd):/work" -w /work skylyrac/blocksds:slim-v1.15.7 make
```
