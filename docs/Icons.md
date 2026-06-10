# Icons
Pico Launcher supports showing custom icons for files and folders by placing `.bmp` files in the right locations.
DS roms already show the icon from their internal banner, so custom icons are mainly useful for GBA roms and other file types.

## Where to place icons

### System icons (by game code)
Place a `<gamecode>.bmp` file under `/_pico/icons/<system>/`. The system name must match the file association's short name. For example:
- `/_pico/icons/gba/BPEE.bmp` — Pokémon Emerald (GBA)
- `/_pico/icons/nds/ADAE.bmp` — Pokémon Diamond (NDS, overrides the built-in banner)

Each subdirectory under `/_pico/icons/` (other than `user/`) is auto-discovered at startup.

### User icons (by filename)
Place a `<filename>.bmp` file under `/_pico/icons/user/`. For example, `myGame.gba.bmp` will be used for any file named `myGame.gba`. This takes precedence over a game-code icon and over the internal banner icon of DS roms.

### Folder icons
Place a file named `folder.bmp` directly inside the folder you want to customise. For example:
- `/GBA Games/folder.bmp` — icon for the `GBA Games` folder
- `/GBA Games/Pokemon/folder.bmp` — icon for the `Pokemon` subfolder

Each folder's `folder.bmp` is independent — two folders with the same name can have different icons. Since `.bmp` files are not a recognised ROM type, `folder.bmp` is never shown as a browser entry.

### Priority
User icon (`/_pico/icons/user/`) > system icon (`/_pico/icons/<system>/`) > internal banner > theme default.
Folder icons (`folder.bmp`) are looked up independently and do not participate in the above priority chain.

## Icon format
Icons must be **32×32 pixels, 4 bpp (16 colors), uncompressed `.bmp`** files. Files with a different size, bit depth, or compression are rejected and shown as blank.
