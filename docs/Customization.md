# Customization
Pico Launcher supports custom icons and banners for files and folders by placing files in the right locations on the SD card. Icons and banners follow the same folder structure: nds and gba files by game code, user files by filename, and folder-level overrides.

## Icons
Custom icons are `.bmp` files. DS roms already show the icon from their internal banner, so custom icons are mainly useful for GBA roms and other file types.

### Icons by game code
Place a `<gamecode>.bmp` file under `/_pico/icons/nds/` or `/_pico/icons/gba/`. For example:
- `/_pico/icons/gba/ABCD.bmp` - icon for the GBA game with game code `ABCD`
- `/_pico/icons/nds/EFGH.bmp` - icon for the NDS game with game code `EFGH` (overrides the built-in banner icon)

### User icons (by filename)
Place a `<filename>.bmp` file under `/_pico/icons/user/`. For example, `myGame.gba.bmp` will be used for any file named `myGame.gba`. This takes precedence over a game code based icon and over the internal banner icon of DS roms.

### Folder icons
Place a file named `icon.bmp` directly inside the folder you want to customise. For example:
- `/GBA Games/icon.bmp` - icon for the `GBA Games` folder
- `/GBA Games/Racing/icon.bmp` - icon for the `Racing` subfolder

Each folder's `icon.bmp` is independent - two folders with the same name can have different icons. Since `.bmp` files are not a recognised ROM type, `icon.bmp` is never shown in the rom browser.

A folder can also use a `banner.bnr` file (see [Folder banners](#folder-banners)) for its icon, in the same location. If both `banner.bnr` and `icon.bmp` exist, `banner.bnr` takes priority.

### Icon priority
User icon (`/_pico/icons/user/`) > game code based icon (`/_pico/icons/nds/` or `/_pico/icons/gba/`) > internal banner > theme default.
Folder icons are looked up independently and do not participate in the above priority chain: `banner.bnr` > `icon.bmp` > theme default.

### Icon format
Icons must be **32×32 pixels, 4 bpp (16 colors), uncompressed `.bmp`** files, with the first palette color treated as transparent.

## Banners
Custom banners are `.bnr` files containing custom game titles, subtitles and (animated) icons. This is especially useful for adding custom titles and (animated) icons to GBA ROMs or other systems.

### Banners by game code
Place a `<gamecode>.bnr` file under `/_pico/banners/nds/` or `/_pico/banners/gba/`. For example:
- `/_pico/banners/gba/ABCD.bnr` - banner for the GBA game with game code `ABCD`
- `/_pico/banners/nds/EFGH.bnr` - banner for the NDS game with game code `EFGH` (overrides the built-in banner)

### User banners (by filename)
Place a `<filename>.bnr` file under `/_pico/banners/user/`. For example, `myGame.gba.bnr` will be used for any file named `myGame.gba`.

### Folder banners
Place a file named `banner.bnr` directly inside the folder you want to customise. For example:
- `/GBA Games/banner.bnr` - custom banner for the `GBA Games` folder

This gives the folder a custom icon (static or animated) and replaces its displayed name with the banner's title, anywhere the folder's name is shown. The folder's actual name on the SD card (used for navigation) is unchanged. Folder banners do not affect the folder's cover image.

### Banner priority
User banner (`/_pico/banners/user/`) > game code based banner (`/_pico/banners/nds/` or `/_pico/banners/gba/`) > built-in ROM banner.
Folder banners (`banner.bnr`) are looked up independently and do not participate in the above priority chain.

### Banner format
Banners must be standard Nintendo DS(i) banner files (usually named `.bnr` or `banner.bin`). Both static icons and animated DSi-style icons are supported.
