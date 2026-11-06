# Enhanced Features
This fork adds a number of quality-of-life features on top of Pico Launcher. This document describes each of them.

## Controls
These controls are available in the rom browser, on top of the standard ones (see [Usage](Usage.md)):

| Input | Action |
|---|---|
| L / R | Jump to the previous or next initial (see [Jumping by initial](#jumping-by-initial)) |
| X (short press) | Toggle favorite for the highlighted game |
| X (hold ~half a second) | Toggle completed for the highlighted game |
| SELECT + A | Launch a random game from the current folder |
| START (hold ~half a second) | Save a screenshot of both screens (see [Screenshots](#screenshots)) |
| Menu button (app bar, the three dots) | Open the menu: recently played, favorites, statistics, delete game, game info, the quit combo and the two filters (see [The menu](#the-menu)) |
| Quit combo (menu) | Pick the buttons held to leave a DS game (see [In-game exit combo](#in-game-exit-combo)) |
| Game info (menu) | Name a ROM that has no banner, and its developer and publisher (see [Game info](#game-info)) |
| Light row (display settings) | Set the DS Lite backlight level (4 levels) |
| Folder button (display settings) | Toggle hiding empty folders |

## The menu
The app bar has three buttons: back, the menu (three dots) and display settings. The menu opens a sheet with everything the bar used to hold as icons, each with its name, in two columns: **Recently played** and **Favorites** over **Statistics** and **Delete game**, then **Game info** beside **Quit combo** (the keys held to leave a DS game, see [In-game exit combo](#in-game-exit-combo)), and **Only favorites** and **Only completed** under them, their colour saying whether they are on (red for favorites, green for completed). Tap an entry, or highlight it with the d-pad and press A. Picking a panel closes the menu and opens the panel in its place; a filter applies at once and closes the menu. B, or a tap outside the sheet, closes it. No app bar button hides a second action behind a hold any more; the X and START holds in the table above are unchanged.

The small button at the right of the menu's title opens the **about** sheet: Pico Launcher by the LNH team on one side, Enhanced by rasalopa on the other, then the version with the commit it was built from and the repository the build came from, and a cheat sheet of the controls that have no button of their own, three at a time; up and down scroll it. A build made from another repository names that repository there, so you can always tell where a build came from.

## Game info
Roms that have no banner, such as GB and SNES games run by an emulator or anything else opened through a file association, only have their file name to be shown by. The browser uses it as the title, which is often an abbreviated release name or something that means nothing.

**Game info** in the menu edits three fields for the highlighted game: **Name**, **Developer** and **Publisher**, the order they show in. The sheet opens on those three entries; picking one puts up the on-screen keyboard the launcher has instead of text input, and while it is up the keyboard owns the d-pad and the face buttons, so typing never leaves the keys:

| Input | Entries | Editing |
|---|---|---|
| D-pad | Move between the three entries | Move over the keys |
| A | Edit the selected entry | Type the selected key |
| B | Go back, storing the entries | Delete the last character |
| X | — | Type a space |
| Y | — | Switch capitals and lower case |
| SELECT | Go back, storing the entries | Back to the three entries |
| START | Edit the selected entry | Confirm the entry, back to the entries |

SELECT twice in a row leaves the sheet, so a name can be backed straight out of. The keys are a compact keyboard of three rows of ten, which leaves each key room for one character at a size the small screen can still show cleanly: the letters sit where a keyboard puts them, ten then nine then seven, and the last three keys of the bottom row hold space, delete and the page switch. The pages are capitals, lower case, digits with the punctuation a name uses, then the rest of the symbols; Y swaps the letters between the two cases without paging. The focused key is marked by a rounded bar in the theme's accent colour, with its character in the colour paired with that accent, so the key being typed on always stands out against the other twenty-nine.

A name replaces the file name in the list, on the top screen, in the recently played and favorites panels and in the statistics panel; the developer and the publisher show on the two lines under it when set, developer first, the way a DS banner lays the pair out. The top screen keeps the file name on its small line, so the file is still identifiable. Values are stored in `gamedata.json` per file, next to the game's favorites and play time. They are written when the sheet is left with B or SELECT, which stores whatever was typed, and clearing all three returns the file to its own name.

## Parental controls
Parental controls have no screen of their own. They are one flag in `settings.json`, set from a PC before the console is handed to a child:

```json
"parentalControls": true
```

While it is `true`, the three actions that can destroy work are refused:

| Action | With parental controls on |
|---|---|
| Delete game | Refused, from the menu and at the confirmation |
| Cheats (Y on a game) | The cheats sheet does not open |
| Game info (menu, and Y on a game) | The sheet does not open, so a name cannot be changed |

Everything else stays as it is, and there is nothing to switch one at a time: the favorite and completed marks (X) are never blocked, and neither is any other button, panel or setting. The three actions are refused where they are carried out, not only faded in the menu, so no other route can reach them while the flag is on.

Switch it off by setting the flag back to `false` on the SD card and starting the launcher again. There is no PIN and no menu row to get past, so there is nothing to remember and nothing to forget; this is a simple lock against a child poking at the console, not a secret that stands up to someone with the SD card.

## In-game exit combo
Holding L, R, Right and SELECT together in a DS game asks Pico Loader to close it and come back to the launcher. That combination is now a setting: **Quit combo** in the menu opens a sheet with the combination in use, A starts picking a new one, and Y puts the default back.

Hold the buttons you want together and keep them held for about a second. A combination has to be two to four buttons and include SELECT, so it cannot fire by accident; SELECT on its own is never enough. The sheet shows the buttons as they go down and says what is still missing, and B cancels while picking. START is allowed and never saves a screenshot while the sheet is picking.

The change reaches the loader at the next launch; a game that is already running keeps the combination it started with. It needs the enhanced Pico Loader (v1.1 or newer, Pico Loader API v4). With another loader the setting is saved, but L + R + Right + SELECT stays. Games the loader cannot patch to exit (DSi mode, DSi-enhanced games, DSiWare, homebrew and games with NAND saves) ignore the setting, just as they ignore the built-in combination.

A combination a game uses itself still fires during play if held that long, so avoid the buttons a game needs for its own shortcuts.

## Jumping by initial
In a folder with hundreds of games, paging through the list a screen at a time takes a
while. L and R now jump to where the previous or next initial starts, so crossing a large
folder takes a few presses instead of dozens.

R moves to the first game filed under the next initial. L moves to the top of the current
initial, and again to the top of the one before it, so getting back to the start of a long
run of games sharing an initial does not need a detour.

As you jump, the initial you landed on shows for a moment at the bottom of the touch screen,
the same way the screenshot message does, so it stays easy to see where you are without
watching the list.

Folders and games are stepped through separately, since folders are always listed first.
Games are grouped by the first character of their file name, which is what the list is
sorted by, so the jump always follows the order on screen. This means a game whose file
name starts with an article or a number is filed under that, not under its title.

L and R keep paging in the cheats, favorites and recently played lists.

## Favorites
Press X on a highlighted game to mark it as a favorite (press again to unmark). Favorites show a small heart on the top screen when highlighted. The mark belongs to that ROM **file**: moving it to another folder keeps it, renaming it starts over, and a second copy of the same game is marked separately. See [Data storage](#data-storage) if a mark is not where you expect it.

## Favorites panel
**Favorites** in the menu opens a panel listing your favorites from **every** folder, alphabetically, each with its total play time, handy when the collection is spread across many folders. Tap an entry (or highlight it and press A) to jump to that game's folder with the game preselected; press B to close.

Favorites marked before this feature existed appear in the panel after you toggle them again or launch them once (the panel needs the game's stored path). An entry whose file has moved or is gone still appears, but selecting it does nothing instead of jumping to the card root; re-mark or launch the game from its new location to update it.

## Completed games
Hold X on a highlighted game for about half a second to mark it as completed (hold again to unmark). Completed games show a small green check on the top screen when highlighted, next to the heart. Like favorites, the mark belongs to the ROM file.

## Favorites and completed filters
**Only favorites** in the menu filters the browser down to favorites, and **Only completed** to completed games. Each row takes its colour while the filter is active, red for favorites and green for completed, and the browser behind it updates as soon as the menu closes. With both filters on, only games that are favorite *and* completed remain. The filters apply per folder, and folders themselves always stay visible.

Marks, play counts and play time all belong to the ROM file, so what the top screen shows and what the filter matches are always the same thing (see [GameData.md](GameData.md)). Two copies of a game are marked separately, and a ROM hack no longer inherits its base game's mark. Renaming a ROM outside the launcher starts it over, and games whose file name is longer than 96 bytes cannot be marked at all (accented characters count double).

## Random game
Hold SELECT and press A to launch a random game from the folder you are currently viewing. With the favorites filter active, it picks a random favorite. SELECT on its own does nothing, so the DSi brightness shortcut (SELECT + volume) stays free.

## DSi-only games on a DS
On a DS or DS Lite, a game made for the DSi only cannot run: the console never enters DSi mode, and launching it ends in a white screen. The launcher now checks the game's own header when you press A and, if the game is DSi-only, stays where it is and says `Needs a DSi or 3DS` at the bottom of the screen. DSiWare titles count as DSi-only too. Homebrew is left alone: many homebrew apps say DSi only in their header and still run on a DS. On a DSi or 3DS nothing changes.

## Launch tracking and play time
Every launch is recorded automatically, unless you switch that off (see below). The launch count and play time used to show at the top-right of the top screen (`3x 2h05`, or `3x · 16 Jul` before any play time was recorded); that text is switched off in this version because the play time overstates, as explained below. The data is still kept. The favorite heart and the completed check now sit above the game's icon, astride the card's edge, where a gold star also marks the most launched game, the one that heads the statistics panel.

Play time is approximate: a session starts when a game is launched and ends the next time the launcher boots. Because of that:
- Sessions longer than 6 hours are discarded: that was a power-off, not a play session.
- Time spent in sleep mode counts as play time.
- A session is lost if the console is powered off without booting back into the launcher.

If you would rather the launcher kept no record of your launches, set `"launchTracking": false` in `settings.json` (see [Usage.md](/docs/Usage.md#settings)). Launching a game then writes nothing to the game data: no launch count, no play time, no last played date. (The launcher still remembers the last game you launched, as it always did, so it can highlight it on the next boot.) The recently played list, the statistics panel and the star of the most launched game keep showing what was recorded before, and favorites and completed marks keep working.

## Recently played
**Recently played** in the menu opens a list of up to 20 recently played games, most recent first, each with the date and time it was last played. Tap an entry (or highlight it and press A) to jump to that game's folder with the game preselected. Press B to close the panel.

## Statistics
**Statistics** in the menu opens a summary panel: a row of four figures with an icon each (games in the folder you are in, played, favorites, completed), your three most launched games with their counts, and the last game you played with its date and time. File names show without their extension there. The launcher's version sits at the top-right of the panel. The version with the commit it was built from shows on the bottom screen while the launcher boots, and in the about sheet, for when you report something and need to say which build you have. The total launches and play time line is switched off in this version, for the reason given above. Press B to close it.

## Screenshots
Hold START for about half a second to save both screens to `/_pico/screenshots` on your SD card, as BMP files.

Each hold writes two files that share a number: `shotNNN_bot.bmp` for the bottom screen and `shotNNN_top.bmp` for the top one. The number is the lowest one neither screen has taken yet, so a pair is always the two halves of one press. Up to 1000 pairs fit in the folder.

A short message appears at the bottom of the lower screen once the files are on the card. It confirms the write rather than the button press, so if something goes wrong (a full folder, a card that cannot be written to) it tells you it could not save instead; the reason only goes to the log. Holding START again while the previous pair is still being written shows `Still saving the last one`; wait a moment and try again.

A few things worth knowing:
- The two screens are recorded a couple of frames apart. The console can only capture one screen at a time, so during a fast animation the two halves of a pair will not match exactly.
- The screen flashes while the picture is taken. That is the capture, not a fault.
- The shortcut works in the file browser and in every panel that opens over it (the menu, display settings, cheats, favorites, recently played, statistics, delete confirmation). It does not work in the theme selector, which is a separate screen with its own input handling.
- The hold has to begin while the launcher is running, so a button that was already held down when it started is not read as a request.

## Deleting games
**Delete game** in the menu deletes the highlighted game; the entry is faded on anything else, such as a folder or a text file. A confirmation sheet opens first: press **X** to confirm, or A or B to cancel. Games are DS and GBA games and files opened through any other file association, except a fixed list of text, picture, video and music extensions (see [FileAssociations.md](FileAssociations.md)).

Deleting a DS or GBA game also deletes its save file (same name with a `.sav` extension, next to the ROM) and removes the game's entry from `gamedata.json`. Deleting any other ROM opened through a file association, such as a GB or SNES ROM, removes the ROM and its entry but keeps its save: the emulator may name its save its own way, and a `.sav` with the same name may belong to a DS game. When there is one, the confirmation names it as kept. Note that saves are matched by name without the extension: if `Game.gba` and `Game.nds` sit in the same folder, they share `Game.sav`, and deleting either game deletes it. With `saveLocation` set to `saves` (see [Usage.md](/docs/Usage.md#settings)), deleting a DS game deletes its save in the `saves` folder when there is one, and a save still next to the ROM is then left alone. When the folder has none, because the game was not launched since the setting was turned on, the save next to the ROM is deleted as before. The confirmation names the file that will go.

## Deleting themes

In the theme selector, the trash button at the bottom of the app bar deletes the highlighted theme's folder, with everything in it. A confirmation sheet opens first, naming the theme and its folder, since two themes can share a name: press **X** to confirm, or A or B to cancel. The selector then starts again without it, on the theme that took its place.

The theme you are using and the two that come with the launcher, `material` and `raspberry`, can't be deleted; the button is faded on them. A folder whose `theme.json` can't be read shows under its folder name and can be deleted too. Nothing is deleted when a file in the folder is read-only, when it has more than four levels of folders or 1024 files and folders, when a name can't be read back, or when the card looks damaged around it; the sheet says why, and the folder can still be deleted on a computer. The theme's `theme.json` goes first, so if a delete is cut short, what is left shows as a folder name and can be deleted again.

## Cheats
The cheat list wraps around at both ends: pressing up on the first entry jumps to the last
one, and pressing down on the last entry comes back to the first, so the bottom of a long
cheat database is one press away from the top. Inside a sub-category, pressing up from the
first entry still moves to the back button, as before; the wrap happens where there is
nothing above the list to move to.

The sheet also shows a small `X: all off` hint next to the cheat description while cheats
are listed. Pressing X disables every cheat at once. The launcher supported this already,
but nothing on screen said so. Handy to make sure no code is active before going online or
starting a speedrun.

## Screen brightness (DS Lite)
The display settings sheet (gear button in the app bar) has a **Light** row with the DS Lite's four backlight levels. Tapping a level applies it immediately, and the choice is remembered and restored on every boot. It also stays active inside the game you launch, until the console powers off.

Until you pick a level the launcher leaves the firmware's brightness untouched. On an original DS, a DSi, or a 3DS running the launcher from a DSpico, the row is not shown: the original DS has no brightness levels, and the DSi and 3DS set brightness from their own system menu.

## Hide empty folders
The display settings sheet has a folder toggle that hides folders containing no visible games, homebrew or media of their own (banner, BGM and other system files don't count as content). It's off by default; toggling it refreshes the folder you're currently viewing immediately. While it is on, its circle takes the same color as the chosen options below it.

Subfolders are followed a few levels deep, so a folder containing only other empty folders is hidden too. Launcher support folders (names starting with `_`) are always kept.

Emptiness means "has nothing in it", independently of the favorites and completed filters. With one of those filters on you can therefore still see a folder that turns out to hold nothing matching it. Checking the filters here meant reading every ROM in every folder on each navigation, which was slow enough that folders started reappearing.

## Per-folder music
Place a `bgm.bcstm` file directly inside a folder to give it its own background music. It uses the same DSP-ADPCM `.bcstm` format as theme music (see [Themes](Themes.md)) and supports looping. The music starts when you enter the folder and switches back to the theme music when you leave. Each folder is checked independently: subfolders do not inherit their parent's music.

The `bgm.bcstm` file itself is not shown in the rom browser (file extensions without an association are hidden).

## Time-of-day theme backgrounds
Custom themes can provide night variants of their backgrounds: place `topbg_night.bin` and/or `bottombg_night.bin` next to `topbg.bin` and `bottombg.bin` in the theme folder (same 256x192, 15 bpp format). Between 20:00 and 6:59 the night variants are used when present. The time is checked when the launcher starts. Delete the `_night` files to disable the effect.

`tools/make_night_bg.py` can generate night variants from a theme's existing backgrounds; see [Tools.md](Tools.md).

## Data storage
Favorites, completed marks, launch counts, play time, the recents list and the edited game names are all stored in a single file, `/_pico/gamedata.json`, written by the launcher itself. Saves are atomic, and if the file ever fails to parse the launcher refuses to overwrite it rather than starting over. Deleting the file resets all favorites and statistics.

**Each entry belongs to one ROM file, identified by its file name.** That single rule explains most surprises:

| What you see | Why |
|---|---|
| A game lost its heart and its play time | The file was renamed outside the launcher. The launcher sees a different file, so it starts over. The old entry stays in the file, unused |
| The same game in two folders has separate favorites | They are two files. Marking one does not mark the other |
| A ROM hack does not inherit the base game's marks | Same reason: different files, even though they share an internal game code |
| Two copies with the *same* file name share one entry | The name is the identity, so same name means same entry. Deleting one through the launcher removes the entry both were using |
| Pressing X does nothing on some game | Its file name is longer than 96 bytes, which cannot be stored (accented characters count as two). The launcher logs it |
| A favorite is missing from the favorites panel | The panel only lists entries with a stored path. Marks made before that existed get one the next time you launch or re-mark the game. Entries whose file has moved or is gone are still listed, but selecting them does nothing |

Earlier versions identified a game by its internal game code instead, which made a ROM hack and its base game share one entry, and could leave the browser filter and the top screen disagreeing about the same game.

The file format is documented in [GameData.md](GameData.md) for anyone writing external tools.
