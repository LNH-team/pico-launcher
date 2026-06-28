# Banners

Pico Launcher supports custom banners (which contain custom game titles, subtitles, and animated icons) for your games by placing `.bnr` files in the right locations.

This is especially useful for adding custom titles and animated icons to GBA ROMs or other systems.

## Where to place banners

### System banners (by game code)
Place a `<gamecode>.bnr` file under `/_pico/banners/nds/` or `/_pico/banners/gba/`. For example:
- `/_pico/banners/gba/BPEE.bnr` - Pokémon Emerald (GBA)
- `/_pico/banners/nds/ADAE.bnr` - Pokémon Diamond (NDS, overrides the built-in banner)


### User banners (by filename)
Place a banner file under `/_pico/banners/user/`. You can name it in two ways:
- **With the ROM extension**: `myGame.gba.bnr`
- **Without the ROM extension**: `myGame.bnr` (which matches `myGame.gba` or any extension)

### Folder banners
Place a file named `folder.bnr` directly inside the folder you want to customise. For example:
- `/GBA Games/folder.bnr` - custom icon and title for the `GBA Games` folder

This gives the folder a custom icon (static or animated) and replaces its displayed name with the banner's title, anywhere the folder's name is shown. The folder's actual name on the SD card (used for navigation) is unchanged. Folder banners do not affect the folder's cover image, which always shows the same generic graphic.

### Priority
User banner (`/_pico/banners/user/`) > system banner (`/_pico/banners/nds/` or `/_pico/banners/gba/`) > built-in ROM banner.
Folder banners (`folder.bnr`) are looked up independently and do not participate in the above priority chain.

## Banner format
Banners must be standard Nintendo DS banner files (usually named `.bnr` or `banner.bin`). Both static icons and animated DSi-style icons are supported.
