# Using Pico Launcher
This document will outline the different settings and functionalities of Pico Launcher.

## Pico Launcher interface
When Pico Launcher is started, this is how your screen will look like.

![Example screen](./images/Horizontal.png)

From here you can browse your SD card to launch homebrew and games.

- DPAD: Move the selector.
- A: Open a folder, or to launch a homebrew or game.
- B: Go to the parent folder or close a menu.
- L and R: Scroll quickly when there are many items in a folder.
- Y: Open the cheats panel (see [Cheats](Cheats.md)).

The back arrow on the top left of the bottom screen can also be used to go up to the parent folder.

Touch input is also supported.

## Settings menu
The settings menu can be accessed by using the DPAD to move the selector to the cogwheel icon and pressing A. When in the settings menu, press the B button will to return to the file browser.

![Settings menu](./images/SettingsPage.png)

Currently, the only settings available are the display mode, and the sorting mode (More settings are available [in the settings file](#settings)). Here is how each layout looks like.

<table>
    <tr>
        <th>Horizontal Grid</th>
        <th>Vertical Grid</th>
        <th>Banner List</th>
        <th>Coverflow</th>
    </tr>
    <tr>
        <td><img src="./images/Horizontal.png"/></td>
        <td><img src="./images/Vertical.png"/></td>
        <td><img src="./images/List.png"/></td>
        <td><img src="./images/Coverflow.png"/></td>
    </tr>
</table>

## Settings
Settings are stored on your SD card in `/_pico/settings.json`. They can be edited with any text editor.

If you go back to the official Pico Launcher for a while, it rewrites `settings.json` with only the settings it knows, so `romBrowserHideEmptyFolders`, `saveLocation`, `launchTracking`, `backlightLevel` and `loaderExitCombo` are back to their defaults when you return. Favorites, completed marks and play stats in `gamedata.json` are kept. If you use `"saveLocation": "saves"`, set it again before launching a game, or the game won't find its save in the `saves` folder. The official launcher doesn't know that folder either, so games started from it use the save next to the game.

The following settings are available:
- `language` - Display language for Pico Launcher. Currently, only `english` is supported. Other languages may be supported later.
- `romBrowserLayout` - Specified how folder contents are displayed. This setting can be changed in Pico Launcher directly.
- `romBrowserSortMode` - Specified if folder contents should be sorted from A to Z (`NameAscending`), or from Z to A (`NameDescending`). This setting can be changed from within Pico Launcher.
- `romBrowserHideEmptyFolders` - When `true`, folders with no visible content of their own are hidden from the browser. This setting can be changed in Pico Launcher directly.
- `saveLocation` - Where the save file of a DS game is kept: `rom` (the default) puts it next to the game as `<game name>.sav`; `saves` puts it in a `saves` folder inside the game's folder, the layout TWiLight Menu++ uses, so both launchers can share one card. It applies to DS games only: homebrew and DSiWare keep their saves where they are, and games launched through an emulator keep the emulator's own save location. The first time a game is launched with `saves`, a save still next to it is moved into the folder (moved, not copied; nothing is deleted). If the folder already has a save for that game, that one is used and the one next to the game is left alone. Switching back to `rom` does not move saves back: a game whose save is in the folder starts fresh until you move `saves/<game name>.sav` next to it again. While `saves` is on, folders named `saves` are hidden from the browser.
- `launchTracking` - When `false`, the launcher stops keeping track of launches: no launch count, no play time and no last played date are written from then on. What was recorded before stays as it is, and the recently played list, the statistics panel and the star of the most launched game show that. Favorites and completed marks keep working. `true` by default.
- `loaderExitCombo` - The buttons held together to leave a DS game, written as names, for example `"Select+L+R+Right"`. It must be two to four buttons and include `Select`; anything else is ignored and the loader's default is used. It can be changed from the menu's title row (see [In-game exit combo](Enhanced.md#in-game-exit-combo)), and it only reaches the enhanced Pico Loader (API v4): with another loader the setting is kept but the loader's built-in combination stays.
- `theme`: Specifies the folder name of the theme to use. If the theme cannot be found, a default fallback theme will be used.
- `lastUsedFilePath` - Specifies the path of the most recently launched homebrew or game, such that it can be selected the next time Pico Launcher is started. It is automatically updated by Pico Launcher.
- `fileAssociations` - See [FileAssociations.md](/docs/FileAssociations.md) for information about how to use this setting.