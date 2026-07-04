# Customization
Pico Launcher supports custom icons and banners for files and folders by placing files in the right locations on the SD card. Icons and banners follow the same folder structure: system files by game code, user files by filename, and folder-level overrides.

## Icons
Custom icons are `.bmp` files. DS roms already show the icon from their internal banner, so custom icons are mainly useful for GBA roms and other file types.

### System icons (by game code)
Place a `<gamecode>.bmp` file under `/_pico/icons/nds/` or `/_pico/icons/gba/`. For example:
- `/_pico/icons/gba/BPEE.bmp` - Pokémon Emerald (GBA)
- `/_pico/icons/nds/ADAE.bmp` - Pokémon Diamond (NDS, overrides the built-in banner)

### User icons (by filename)
Place a `<filename>.bmp` file under `/_pico/icons/user/`. For example, `myGame.gba.bmp` will be used for any file named `myGame.gba`. This takes precedence over a game-code icon and over the internal banner icon of DS roms.

### Folder icons
Place a file named `folder.bmp` directly inside the folder you want to customise. For example:
- `/GBA Games/folder.bmp` - icon for the `GBA Games` folder
- `/GBA Games/Pokemon/folder.bmp` - icon for the `Pokemon` subfolder

Each folder's `folder.bmp` is independent - two folders with the same name can have different icons. Since `.bmp` files are not a recognised ROM type, `folder.bmp` is never shown as a browser entry.

A folder can also use a `folder.bnr` file (see [Folder banners](#folder-banners)) for its icon, in the same location. If both `folder.bnr` and `folder.bmp` exist, `folder.bnr` takes priority.

### Icon priority
User icon (`/_pico/icons/user/`) > system icon (`/_pico/icons/nds/` or `/_pico/icons/gba/`) > internal banner > theme default.
Folder icons are looked up independently and do not participate in the above priority chain: `folder.bnr` > `folder.bmp` > theme default.

### Icon format
Icons must be **32×32 pixels, 4 bpp (16 colors), uncompressed `.bmp`** files.

## Banners
Custom banners contain custom game titles, subtitles, and animated icons, placed via `.bnr` files. This is especially useful for adding custom titles and animated icons to GBA ROMs or other systems.

### System banners (by game code)
Place a `<gamecode>.bnr` file under `/_pico/banners/nds/` or `/_pico/banners/gba/`. For example:
- `/_pico/banners/gba/BPEE.bnr` - Pokémon Emerald (GBA)
- `/_pico/banners/nds/ADAE.bnr` - Pokémon Diamond (NDS, overrides the built-in banner)

### User banners (by filename)
Place a `<filename>.bnr` file under `/_pico/banners/user/`. For example, `myGame.gba.bnr` will be used for any file named `myGame.gba`.

### Folder banners
Place a file named `folder.bnr` directly inside the folder you want to customise. For example:
- `/GBA Games/folder.bnr` - custom icon and title for the `GBA Games` folder

This gives the folder a custom icon (static or animated) and replaces its displayed name with the banner's title, anywhere the folder's name is shown. The folder's actual name on the SD card (used for navigation) is unchanged. Folder banners do not affect the folder's cover image, which always shows the same generic graphic.

### Banner priority
User banner (`/_pico/banners/user/`) > system banner (`/_pico/banners/nds/` or `/_pico/banners/gba/`) > built-in ROM banner.
Folder banners (`folder.bnr`) are looked up independently and do not participate in the above priority chain.

### Banner format
Banners must be standard Nintendo DS banner files (usually named `.bnr` or `banner.bin`). Both static icons and animated DSi-style icons are supported.
