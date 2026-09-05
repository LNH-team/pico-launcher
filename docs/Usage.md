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
- START (hold): Save a screenshot of both screens (see [Screenshots](#screenshots)).

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

## Screenshots
Holding START for about half a second saves both screens to `/_pico/screenshots` on your SD card, as BMP files.

Each hold writes two files that share a number: `shotNNN_bot.bmp` for the bottom screen and `shotNNN_top.bmp` for the top one. The number is the lowest one neither screen has taken yet, so a pair is always the two halves of one press. Up to 1000 pairs fit in the folder.

A short message appears at the bottom of the screen once the files are on the card. It confirms the write rather than the button press, so if something goes wrong - a full folder, or a card that cannot be written to - it says that instead.

A few things worth knowing:
- The two screens are recorded a couple of frames apart. The console can only capture one screen at a time, so during a fast animation the two halves of a pair will not match exactly.
- The screen flashes while the picture is taken. That is the capture, not a fault.
- The shortcut works in the file browser and in the settings menu. It does not work in the theme selector, which is a separate screen with its own input handling.
- The hold has to begin while Pico Launcher is running, so a button that was already held down when it started is not read as a request.

## Settings
Settings are stored on your SD card in `/_pico/settings.json`. They can be edited with any text editor. The following settings are available:
- `language` - Display language for Pico Launcher. Currently, only `english` is supported. Other languages may be supported later.
- `romBrowserLayout` - Specified how folder contents are displayed. This setting can be changed in Pico Launcher directly.
- `romBrowserSortMode` - Specified if folder contents should be sorted from A to Z (`NameAscending`), or from Z to A (`NameDescending`). This setting can be changed from within Pico Launcher.
- `theme`: Specifies the folder name of the theme to use. If the theme cannot be found, a default fallback theme will be used.
- `lastUsedFilePath` - Specifies the path of the most recently launched homebrew or game, such that it can be selected the next time Pico Launcher is started. It is automatically updated by Pico Launcher.
- `fileAssociations` - See [FileAssociations.md](/docs/FileAssociations.md) for information about how to use this setting.