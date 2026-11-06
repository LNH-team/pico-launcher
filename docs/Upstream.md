# The fork against upstream

Everything Pico Launcher Enhanced adds or changes on top of [upstream Pico Launcher](https://github.com/LNH-team/pico-launcher), one entry per feature, so each can be looked at on its own, though some build on others (the toast, the panels, the delete sheet, the game data file). Each entry says what it does, which commits make it up, whether upstream has an issue or a pull request about the same thing, and which upstream files it touches, which is where a merge can conflict.

This page is up to date with v1.11.0, against upstream `develop` at `dc64a34`. The fork is not behind upstream: it merged upstream's latest commit, `dc64a34`, in `8f7d705`, so everything listed here is a change upstream does not have. Documentation files are left out of the file lists.

## Browsing

### Jump by initial with L and R

In a folder sorted by name, L and R move by initial letter instead of paging: R jumps to the next initial, and L to the top of the current one, then to the initial before it. A folder of hundreds of games takes a few presses to cross. The letter you land on shows for a moment in a small message at the bottom of the touch screen.

- **Commits:** `6857e49`, `8d3f15d`, `a134d64`
- **Upstream:** Asked for in [LNH-team/pico-launcher#75](https://github.com/LNH-team/pico-launcher/issues/75), with the d-pad there; L and R came up in that thread. Search, [LNH-team/pico-launcher#34](https://github.com/LNH-team/pico-launcher/issues/34) and [PR #50](https://github.com/LNH-team/pico-launcher/pull/50), takes on long folders another way.
- **Notes:** Adds a big-step hook to `RecyclerAdapter` that only the file list answers, and only when the folder is sorted by name, so the cheats, favorites and recents lists keep paging. The letter is shown with the toast from the screenshots entry.

<details><summary>Files: 11 upstream, 1 new</summary>

Upstream files touched:

- `arm9/source/App.cpp`
- `arm9/source/gui/views/RecyclerAdapter.h`
- `arm9/source/gui/views/RecyclerView.cpp`
- `arm9/source/romBrowser/FileRecyclerAdapter.cpp`
- `arm9/source/romBrowser/FileRecyclerAdapter.h`
- `arm9/source/romBrowser/IRomBrowserController.h`
- `arm9/source/romBrowser/RomBrowserController.h`
- `arm9/source/romBrowser/Theme/Material/CarouselRecyclerView.cpp`
- `arm9/source/romBrowser/views/CoverFlowRecyclerView.cpp`
- `arm9/source/romBrowser/views/RomBrowserTopScreenView.cpp`
- `arm9/source/romBrowser/views/RomBrowserTopScreenView.h`

New files:

- `arm9/source/gui/views/ToastView.cpp`

</details>

### Hide empty folders

A folder toggle in the display settings sheet hides folders with nothing the browser would list (no DS games or homebrew, and no files with a file association such as GBA games or emulator ROMs), including folders that only contain other empty folders. It is off by default and ignores the favorites and completed filters.

- **Commits:** `3ee6785`, `bba92b6`, `3c8363f`, `c3c733b`
- **Upstream:** [PR #39](https://github.com/LNH-team/pico-launcher/pull/39) hides folders by name patterns and [PR #45](https://github.com/LNH-team/pico-launcher/pull/45) hides names starting with `_`; this one hides folders that turn out to be empty. All three edit the folder listing code.
- **Settings and data:** `romBrowserHideEmptyFolders` in `settings.json`.
- **Fork issues:** [#6](https://github.com/rasalopa/pico-launcher-enhanced/issues/6)
- **Notes:** The emptiness check runs on the IO thread while the folder loads and looks up to four levels down. It raised the IO task stack in `App.h` from 2 KB to 4 KB after a freeze on hardware; keep that on merge. Folders starting with `_` are always shown.

<details><summary>Files: 15 upstream, 2 new</summary>

Upstream files touched:

- `arm9/source/App.h`
- `arm9/source/romBrowser/FileInfo.cpp`
- `arm9/source/romBrowser/FileInfo.h`
- `arm9/source/romBrowser/RomBrowserController.cpp`
- `arm9/source/romBrowser/SdFolder.cpp`
- `arm9/source/romBrowser/SdFolder.h`
- `arm9/source/romBrowser/SdFolderFactory.cpp`
- `arm9/source/romBrowser/SdFolderFactory.h`
- `arm9/source/romBrowser/SdFolderFilterSortParams.h`
- `arm9/source/romBrowser/viewModels/DisplaySettingsViewModel.h`
- `arm9/source/romBrowser/viewModels/RomBrowserViewModel.cpp`
- `arm9/source/romBrowser/views/DisplaySettingsBottomSheetView.cpp`
- `arm9/source/romBrowser/views/DisplaySettingsBottomSheetView.h`
- `arm9/source/services/settings/JsonAppSettingsSerializer.thumb.cpp`
- `arm9/source/services/settings/RomBrowserDisplaySettings.h`

New files:

- `arm9/gfx/hideEmptyFoldersIcon.grit`
- `arm9/gfx/hideEmptyFoldersIcon.png`

</details>

### Game deletion

Delete game in the menu deletes the highlighted game after a confirmation sheet (X confirms), removing the ROM, its save and its favorite and play data. The entry is faded unless a DS game (homebrew included) or a GBA game is highlighted, since only those can be deleted; folders, and files opened through another file association such as GB or SNES ROMs, can't be. Later widened: ROMs opened through any other file association can be deleted too and keep their save; a fixed list of text, picture, video and music extensions still can't.

- **Commits:** `94f405e`
- **Upstream:** None found.
- **Settings and data:** Removes the game's entry from `gamedata.json`.
- **Notes:** It moved from the app bar into the menu with the menu entry, and the save removal follows `saveLocation` since the saves folder entry. Games with the same name and a different extension in one folder share one `.sav`.

<details><summary>Files: 12 upstream, 5 new</summary>

Upstream files touched:

- `arm9/source/App.cpp`
- `arm9/source/App.h`
- `arm9/source/romBrowser/IRomBrowserController.h`
- `arm9/source/romBrowser/RomBrowserController.cpp`
- `arm9/source/romBrowser/RomBrowserController.h`
- `arm9/source/romBrowser/RomBrowserState.h`
- `arm9/source/romBrowser/RomBrowserStateMachine.cpp`
- `arm9/source/romBrowser/RomBrowserStateTrigger.h`
- `arm9/source/romBrowser/viewModels/RomBrowserAppBarViewModel.h`
- `arm9/source/romBrowser/viewModels/RomBrowserBottomScreenViewModel.h`
- `arm9/source/romBrowser/views/RomBrowserAppBarView.cpp`
- `arm9/source/romBrowser/views/RomBrowserAppBarView.h`

New files:

- `arm9/gfx/trashIcon.grit`
- `arm9/gfx/trashIcon.png`
- `arm9/source/romBrowser/viewModels/DeleteConfirmViewModel.h`
- `arm9/source/romBrowser/views/deleteconfirm/DeleteConfirmBottomSheetView.cpp`
- `arm9/source/romBrowser/views/deleteconfirm/DeleteConfirmBottomSheetView.h`

</details>

### Going up lands on the folder you left

Going up a folder puts the highlight on the folder you just came out of instead of the first entry, so you keep your place in a long list.

- **Commits:** `da90e33`
- **Upstream:** None found. [PR #84](https://github.com/LNH-team/pico-launcher/pull/84) touches the same go-up path but not this.
- **Notes:** Also adds the predicate the menu uses to fade Delete game on anything that is not a DS or GBA game (later widened to ROMs opened through any other file association, except a fixed list of text, picture, video and music extensions), and the faded, inert icon button state (`SetEnabled` in `IconButtonView`, `SetButtonEnabled` in `AppBarView`) that the theme selector's delete button uses, so Deleting themes needs this commit. The app bar focus look it set was later replaced by `3a5d6da`.

<details><summary>Files: 12 upstream, 0 new</summary>

Upstream files touched:

- `arm9/source/romBrowser/IRomBrowserController.h`
- `arm9/source/romBrowser/RomBrowserController.cpp`
- `arm9/source/romBrowser/RomBrowserController.h`
- `arm9/source/romBrowser/Theme/custom/CustomAppBarView.cpp`
- `arm9/source/romBrowser/Theme/Material/MaterialAppBarView.cpp`
- `arm9/source/romBrowser/viewModels/RomBrowserAppBarViewModel.h`
- `arm9/source/romBrowser/views/AppBarView.h`
- `arm9/source/romBrowser/views/IconButton2DView.cpp`
- `arm9/source/romBrowser/views/IconButton3DView.cpp`
- `arm9/source/romBrowser/views/IconButtonView.cpp`
- `arm9/source/romBrowser/views/IconButtonView.h`
- `arm9/source/romBrowser/views/RomBrowserAppBarView.cpp`

</details>

### Random game (SELECT + A)

Hold SELECT and press A to launch a random DS or GBA game from the folder you are in, homebrew included; ROMs opened through another file association, such as GB or SNES, are never picked. Later widened: they are picked too, except a fixed list of text, picture, video and music extensions. With Only favorites or Only completed on, it picks from those games in that folder. SELECT alone does nothing, so the DSi's SELECT + volume brightness shortcut never launches a game.

- **Commits:** `780999c`, `012396f`
- **Upstream:** None found.
- **Fork issues:** [#11](https://github.com/rasalopa/pico-launcher-enhanced/issues/11)

<details><summary>Files: 8 upstream, 0 new</summary>

Upstream files touched:

- `arm9/source/App.cpp`
- `arm9/source/romBrowser/IRomBrowserController.h`
- `arm9/source/romBrowser/RomBrowserController.cpp`
- `arm9/source/romBrowser/RomBrowserController.h`
- `arm9/source/romBrowser/viewModels/RomBrowserBottomScreenViewModel.h`
- `arm9/source/romBrowser/views/IconButtonView.cpp`
- `arm9/source/romBrowser/views/RomBrowserBottomScreenView.cpp`
- `arm9/source/romBrowser/views/RomBrowserItemInputHandler.cpp`

</details>

### ROMs opened by an emulator count as games

A file opened through a file association, such as a GB or SNES ROM run by an emulator, counts as a game: X marks it favorite, holding X marks it completed, Delete game deletes it, SELECT + A can pick it, and the statistics panel counts it in the folder. Deleting one keeps its save, since an emulator may name its save its own way and a `.sav` with the same name may belong to a DS game; when there is such a `.sav`, the confirmation names it as kept. A fixed list of text, picture, video and music extensions stays out, listed in [FileAssociations.md](FileAssociations.md). The idea and the file type change come from djaysan's [pico-launcher-dsbios](https://github.com/djaysan/pico-launcher-dsbios).

- **Commits:** `1d38255`
- **Upstream:** None found.
- **Fork issues:** [#29](https://github.com/rasalopa/pico-launcher-enhanced/issues/29)

<details><summary>Files: 8 upstream, 0 new</summary>

Upstream files touched:

- `arm9/source/romBrowser/FileType/CustomFileType.h`
- `arm9/source/romBrowser/FileType/ExtensionFileTypeProvider.cpp`
- `arm9/source/romBrowser/FileType/FileType.h`
- `arm9/source/romBrowser/FileType/Gba/GbaFileType.h`
- `arm9/source/romBrowser/FileType/Nds/NdsFileType.h`
- `arm9/source/romBrowser/IRomBrowserController.h`
- `arm9/source/romBrowser/RomBrowserController.cpp`
- `arm9/source/romBrowser/RomBrowserController.h`

</details>

## Favorites, stats and game data

### Favorites (mark, filter and panel)

Press X on a DS or GBA game (homebrew included; later widened to ROMs opened through any other file association, except a fixed list of text, picture, video and music extensions) to mark it as a favorite, shown as a heart above its icon on the top screen. Only favorites in the menu filters the folder's games down to favorites, with subfolders still listed, and Favorites in the menu lists favorites from every folder, with their play time, so you can jump straight to one.

- **Commits:** `7e516f5`, `68e2f04`, `8e7ef8d`, `b0ef3fc`
- **Upstream:** Asked for in [LNH-team/pico-launcher#33](https://github.com/LNH-team/pico-launcher/issues/33). [PR #67](https://github.com/LNH-team/pico-launcher/pull/67) by tasken, still being worked on, adds favorites its own way: heart badges on the list items, a heart button on the app bar and a virtual favorites folder, stored by its own service. [PR #37](https://github.com/LNH-team/pico-launcher/pull/37) also has favorites, marked from the game details sheet, listed from a heart button and kept in `/_pico/extras/state.bin`, and [#81](https://github.com/LNH-team/pico-launcher/issues/81) proposes a favorites folder of shortcut files toggled with X. Taking any of them needs a migration of the data.
- **Settings and data:** Creates `/_pico/gamedata.json`, see [Game data](GameData.md).
- **Fork issues:** [#3](https://github.com/rasalopa/pico-launcher-enhanced/issues/3)

<details><summary>Files: 32 upstream, 7 new</summary>

Upstream files touched:

- `arm9/source/App.cpp`
- `arm9/source/App.h`
- `arm9/source/gui/views/Label2DView.cpp`
- `arm9/source/gui/views/View.h`
- `arm9/source/romBrowser/FileInfoManager.cpp`
- `arm9/source/romBrowser/FileInfoManager.h`
- `arm9/source/romBrowser/IRomBrowserController.h`
- `arm9/source/romBrowser/RomBrowserController.cpp`
- `arm9/source/romBrowser/RomBrowserController.h`
- `arm9/source/romBrowser/RomBrowserState.h`
- `arm9/source/romBrowser/RomBrowserStateMachine.cpp`
- `arm9/source/romBrowser/RomBrowserStateTrigger.h`
- `arm9/source/romBrowser/SdFolder.cpp`
- `arm9/source/romBrowser/SdFolderFilterSortParams.h`
- `arm9/source/romBrowser/viewModels/IRomBrowserItemViewModel.h`
- `arm9/source/romBrowser/viewModels/RomBrowserAppBarViewModel.h`
- `arm9/source/romBrowser/viewModels/RomBrowserBottomScreenViewModel.h`
- `arm9/source/romBrowser/viewModels/RomBrowserItemViewModel.cpp`
- `arm9/source/romBrowser/viewModels/RomBrowserItemViewModel.h`
- `arm9/source/romBrowser/viewModels/RomBrowserViewModel.cpp`
- `arm9/source/romBrowser/views/AppBarView.h`
- `arm9/source/romBrowser/views/IconButton2DView.cpp`
- `arm9/source/romBrowser/views/IconButton3DView.cpp`
- `arm9/source/romBrowser/views/IconButtonView.cpp`
- `arm9/source/romBrowser/views/IconButtonView.h`
- `arm9/source/romBrowser/views/RomBrowserAppBarView.cpp`
- `arm9/source/romBrowser/views/RomBrowserAppBarView.h`
- `arm9/source/romBrowser/views/RomBrowserItemInputHandler.cpp`
- `arm9/source/romBrowser/views/RomBrowserTopScreenView.cpp`
- `arm9/source/romBrowser/views/RomBrowserTopScreenView.h`
- `arm9/source/services/process/ProcessFactory.thumb.cpp`
- `arm9/source/settings/viewModels/ThemeListItemViewModel.h`

New files:

- `arm9/source/services/gamedata/GameDataEntry.h`
- `arm9/source/services/gamedata/IGameDataService.h`
- `arm9/source/services/gamedata/JsonGameDataService.h`
- `arm9/source/services/gamedata/JsonGameDataService.thumb.cpp`
- `arm9/source/romBrowser/viewModels/RecentsViewModel.h`
- `arm9/source/romBrowser/views/recents/RecentListItemView.cpp`
- `arm9/source/romBrowser/views/recents/RecentsBottomSheetView.cpp`

</details>

### Completed games

Hold X for about half a second on a DS or GBA game (homebrew included; later widened to ROMs opened through any other file association, except a fixed list of text, picture, video and music extensions) to mark it as completed, shown as a green check above its icon next to the heart. Only completed in the menu filters the folder's games to completed ones, with subfolders still listed, and the statistics panel counts them.

- **Commits:** `6fd8022`
- **Upstream:** None found.
- **Settings and data:** `completed` in `gamedata.json`, stored only when true.

<details><summary>Files: 17 upstream, 8 new</summary>

Upstream files touched:

- `arm9/source/romBrowser/IRomBrowserController.h`
- `arm9/source/romBrowser/RomBrowserController.cpp`
- `arm9/source/romBrowser/RomBrowserController.h`
- `arm9/source/romBrowser/SdFolder.cpp`
- `arm9/source/romBrowser/SdFolderFilterSortParams.h`
- `arm9/source/romBrowser/viewModels/IRomBrowserItemViewModel.h`
- `arm9/source/romBrowser/viewModels/RomBrowserAppBarViewModel.h`
- `arm9/source/romBrowser/viewModels/RomBrowserItemViewModel.cpp`
- `arm9/source/romBrowser/viewModels/RomBrowserItemViewModel.h`
- `arm9/source/romBrowser/viewModels/RomBrowserViewModel.cpp`
- `arm9/source/romBrowser/views/RomBrowserAppBarView.cpp`
- `arm9/source/romBrowser/views/RomBrowserAppBarView.h`
- `arm9/source/romBrowser/views/RomBrowserItemInputHandler.cpp`
- `arm9/source/romBrowser/views/RomBrowserItemInputHandler.h`
- `arm9/source/romBrowser/views/RomBrowserTopScreenView.cpp`
- `arm9/source/romBrowser/views/RomBrowserTopScreenView.h`
- `arm9/source/settings/viewModels/ThemeListItemViewModel.h`

New files:

- `arm9/gfx/checkIcon.grit`
- `arm9/gfx/checkIcon.png`
- `arm9/source/romBrowser/viewModels/StatisticsViewModel.h`
- `arm9/source/romBrowser/views/statistics/StatisticsBottomSheetView.cpp`
- `arm9/source/services/gamedata/GameDataEntry.h`
- `arm9/source/services/gamedata/IGameDataService.h`
- `arm9/source/services/gamedata/JsonGameDataService.h`
- `arm9/source/services/gamedata/JsonGameDataService.thumb.cpp`

</details>

### Recently played panel

Recently played in the menu lists up to 20 games you played last, newest first, with the date and time, and picking one takes you straight to it in its folder.

- **Commits:** `8266671`
- **Upstream:** [LNH-team/pico-launcher#81](https://github.com/LNH-team/pico-launcher/issues/81), opened later, asks for recently played too, as a folder of shortcut files. Upstream's app bar already has a recents button commented out.
- **Settings and data:** Adds `path` to each game in `gamedata.json`, written at every launch, and reads it with `lastPlayed`.

<details><summary>Files: 12 upstream, 10 new</summary>

Upstream files touched:

- `arm9/source/App.cpp`
- `arm9/source/App.h`
- `arm9/source/romBrowser/IRomBrowserController.h`
- `arm9/source/romBrowser/RomBrowserController.cpp`
- `arm9/source/romBrowser/RomBrowserController.h`
- `arm9/source/romBrowser/RomBrowserState.h`
- `arm9/source/romBrowser/RomBrowserStateMachine.cpp`
- `arm9/source/romBrowser/RomBrowserStateTrigger.h`
- `arm9/source/romBrowser/viewModels/RomBrowserAppBarViewModel.h`
- `arm9/source/romBrowser/viewModels/RomBrowserBottomScreenViewModel.h`
- `arm9/source/romBrowser/views/RomBrowserAppBarView.cpp`
- `arm9/source/romBrowser/views/RomBrowserAppBarView.h`

New files:

- `arm9/source/romBrowser/viewModels/RecentsViewModel.h`
- `arm9/source/romBrowser/views/recents/RecentListItemView.cpp`
- `arm9/source/romBrowser/views/recents/RecentListItemView.h`
- `arm9/source/romBrowser/views/recents/RecentsAdapter.h`
- `arm9/source/romBrowser/views/recents/RecentsBottomSheetView.cpp`
- `arm9/source/romBrowser/views/recents/RecentsBottomSheetView.h`
- `arm9/source/services/gamedata/GameDataEntry.h`
- `arm9/source/services/gamedata/IGameDataService.h`
- `arm9/source/services/gamedata/JsonGameDataService.h`
- `arm9/source/services/gamedata/JsonGameDataService.thumb.cpp`

</details>

### Statistics panel

Statistics in the menu opens a panel with four figures (the games in this folder, counting only what an active filter shows, then the games played, favorites and completed across the whole card), your three most launched games with their counts, the last game you played and the launcher version. The total launches and play time line is switched off because play time overstates.

- **Commits:** `c9fa19b`, `8438b11`, `0be3b79`, `f2e5642`
- **Upstream:** None found.
- **Settings and data:** Reads `gamedata.json` only.
- **Fork issues:** [#9](https://github.com/rasalopa/pico-launcher-enhanced/issues/9)
- **Notes:** The totals line is switched off with `SHOW_STATISTICS_LAUNCHES_AND_TIME` in `StatisticsBottomSheetView.cpp`.

<details><summary>Files: 14 upstream, 4 new</summary>

Upstream files touched:

- `arm9/source/App.cpp`
- `arm9/source/App.h`
- `arm9/source/romBrowser/IRomBrowserController.h`
- `arm9/source/romBrowser/RomBrowserController.cpp`
- `arm9/source/romBrowser/RomBrowserController.h`
- `arm9/source/romBrowser/RomBrowserState.h`
- `arm9/source/romBrowser/RomBrowserStateMachine.cpp`
- `arm9/source/romBrowser/RomBrowserStateTrigger.h`
- `arm9/source/romBrowser/viewModels/RomBrowserAppBarViewModel.h`
- `arm9/source/romBrowser/viewModels/RomBrowserBottomScreenViewModel.h`
- `arm9/source/romBrowser/views/IconButtonView.cpp`
- `arm9/source/romBrowser/views/RomBrowserAppBarView.cpp`
- `arm9/source/romBrowser/views/RomBrowserBottomScreenView.cpp`
- `arm9/source/romBrowser/views/RomBrowserItemInputHandler.cpp`

New files:

- `arm9/source/romBrowser/viewModels/StatisticsViewModel.h`
- `arm9/source/romBrowser/views/statistics/StatisticsBottomSheetView.cpp`
- `arm9/source/romBrowser/views/statistics/StatisticsBottomSheetView.h`
- `arm9/source/romBrowser/views/recents/RecentsBottomSheetView.cpp`

</details>

### Top screen markers (heart, check and star)

When a game is highlighted, its favorite heart and completed check sit above its icon on the top screen as crisp outlined pixel markers that read on any theme, and a gold star marks your most launched game. The game count and launch text that used to sit on the top screen are gone from it; the count now heads the statistics panel.

- **Commits:** `3985737`, `9d72a51`, `bf37a9f`, `25b31f7`, `f1ec46f`, `2173910`
- **Upstream:** None found. It builds on upstream's positioning of top screen elements for custom themes ([LNH-team/pico-launcher#40](https://github.com/LNH-team/pico-launcher/issues/40)). [PR #78](https://github.com/LNH-team/pico-launcher/pull/78) adds its time and battery bar to the same top screen view and the same view factory headers, and [PR #67](https://github.com/LNH-team/pico-launcher/pull/67) draws its own hearts.
- **Settings and data:** `topLaunchInfo` in a custom theme's `theme.json` (optional). `topGameCount` is no longer read.
- **Fork issues:** [#4](https://github.com/rasalopa/pico-launcher-enhanced/issues/4), [#18](https://github.com/rasalopa/pico-launcher-enhanced/issues/18)

<details><summary>Files: 13 upstream, 15 new</summary>

Upstream files touched:

- `arm9/gfx/smallHeartIconFilled.png`
- `arm9/source/App.cpp`
- `arm9/source/PicoLoaderProcess.cpp`
- `arm9/source/romBrowser/Theme/custom/CustomRomBrowserViewFactory.h`
- `arm9/source/romBrowser/Theme/IRomBrowserViewFactory.h`
- `arm9/source/romBrowser/Theme/Material/MaterialRomBrowserViewFactory.h`
- `arm9/source/romBrowser/views/ChipView.cpp`
- `arm9/source/romBrowser/views/RomBrowserTopScreenView.cpp`
- `arm9/source/romBrowser/views/RomBrowserTopScreenView.h`
- `arm9/source/settings/views/ThemeListTopView.cpp`
- `arm9/source/themes/custom/CustomTheme.cpp`
- `arm9/source/themes/custom/CustomThemeInfo.h`
- `Makefile.arm9`

New files:

- `arm9/gfx/checkMarker.grit`
- `arm9/gfx/checkMarker.png`
- `arm9/gfx/heartMarker.grit`
- `arm9/gfx/heartMarker.png`
- `arm9/gfx/starMarker.grit`
- `arm9/gfx/starMarker.png`
- `arm9/gfx/stripChipBg.grit`
- `arm9/gfx/stripChipBg.png`
- `arm9/source/themes/custom/CustomTopStripElementInfo.h`
- `arm9/source/romBrowser/viewModels/StatisticsViewModel.h`
- `arm9/source/romBrowser/views/statistics/StatisticsBottomSheetView.cpp`
- `arm9/source/romBrowser/views/statistics/StatisticsBottomSheetView.h`
- `arm9/source/services/gamedata/GameDataEntry.h`
- `arm9/source/Version.cpp`
- `arm9/source/Version.h`

</details>

### Launch tracking and play time

Every launch is recorded: launch count, last played date and a rough play time, which feed the recently played list, the statistics panel and the star. A game whose file name is longer than 96 bytes is left out. With `"launchTracking": false` in `settings.json` nothing new is recorded, while earlier records, favorites and completed marks stay.

- **Commits:** `c61d1f6`, `0b65c7e`
- **Upstream:** [PR #37](https://github.com/LNH-team/pico-launcher/pull/37) has similar work: it records a launch count and the last launch date per game, in its own file.
- **Settings and data:** `launchCount`, `lastPlayed`, `playMinutes` and the session keys in `gamedata.json`; `launchTracking` in `settings.json` (on by default).
- **Fork issues:** [#9](https://github.com/rasalopa/pico-launcher-enhanced/issues/9)
- **Notes:** The launch count and last played date came earlier, with `7e516f5` under Favorites; these commits add play time and the setting. Play time counts from launch until the launcher boots again, so it counts the clock and not play, and only a gap between 1 minute and 6 hours adds to it: a longer one is taken as the console being off. The top screen text and the statistics totals are switched off for that reason; the favorites panel still shows it per game as an approximate figure.

<details><summary>Files: 5 upstream, 6 new</summary>

Upstream files touched:

- `arm9/source/romBrowser/RomBrowserController.cpp`
- `arm9/source/romBrowser/RomBrowserController.h`
- `arm9/source/romBrowser/views/RomBrowserTopScreenView.cpp`
- `arm9/source/services/settings/AppSettings.h`
- `arm9/source/services/settings/JsonAppSettingsSerializer.thumb.cpp`

New files:

- `arm9/source/romBrowser/viewModels/StatisticsViewModel.h`
- `arm9/source/romBrowser/views/statistics/StatisticsBottomSheetView.cpp`
- `arm9/source/services/gamedata/GameDataEntry.h`
- `arm9/source/services/gamedata/IGameDataService.h`
- `arm9/source/services/gamedata/JsonGameDataService.h`
- `arm9/source/services/gamedata/JsonGameDataService.thumb.cpp`

</details>

### Game data per ROM file (identity and safe saving)

Favorites, completed marks and play data are keyed by the ROM's file name, so the filters always agree with the top screen, and a copy or a hack with a different file name keeps its own marks. Saving is crash safe, and a file that fails to parse is never overwritten.

- **Commits:** `e83353f`, `a6bdc05`, `510f9e8`
- **Upstream:** None found. [PR #67](https://github.com/LNH-team/pico-launcher/pull/67) keeps its favorites in a file of its own.
- **Settings and data:** The `gamedata.json` format, documented in [Game data](GameData.md): entries by file name, saved through a temporary file that boot promotes when `gamedata.json` is missing (a save cut short between deleting the old file and renaming the new one) and deletes otherwise.
- **Fork issues:** [#1](https://github.com/rasalopa/pico-launcher-enhanced/issues/1), [#7](https://github.com/rasalopa/pico-launcher-enhanced/issues/7)

<details><summary>Files: 7 upstream, 5 new</summary>

Upstream files touched:

- `arm9/source/romBrowser/IRomBrowserController.h`
- `arm9/source/romBrowser/RomBrowserController.cpp`
- `arm9/source/romBrowser/RomBrowserController.h`
- `arm9/source/romBrowser/viewModels/RomBrowserItemViewModel.cpp`
- `arm9/source/romBrowser/viewModels/RomBrowserItemViewModel.h`
- `arm9/source/romBrowser/views/RomBrowserTopScreenView.cpp`
- `arm9/source/romBrowser/views/RomBrowserTopScreenView.h`

New files:

- `arm9/source/services/gamedata/GameDataEntry.h`
- `arm9/source/services/gamedata/IGameDataService.h`
- `arm9/source/services/gamedata/JsonGameDataService.h`
- `arm9/source/services/gamedata/JsonGameDataService.thumb.cpp`
- `arm9/source/romBrowser/viewModels/RecentsViewModel.h`

</details>

## Saves

### Saves folder (TWiLight Menu++ layout)

With `"saveLocation": "saves"` in `settings.json`, a DS game's save lives in a saves folder inside the game's folder, the layout TWiLight Menu++ uses, so both launchers can share one card. An old save next to the game is moved in on first launch, deleting a game removes the save from the folder, and saves folders stay out of the browser.

- **Commits:** `3612680`, `cef2fe5`, `2119a08`, `b15dfbc`, `d9d8be7`
- **Upstream:** Asked for in [LNH-team/pico-launcher#63](https://github.com/LNH-team/pico-launcher/issues/63), still open upstream.
- **Settings and data:** `saveLocation` in `settings.json`: `"rom"` (default) or `"saves"`.
- **Notes:** A save next to the game moves into the folder on first launch unless the folder already has one. Switching back to `"rom"` does not move saves back.

<details><summary>Files: 10 upstream, 3 new</summary>

Upstream files touched:

- `arm9/source/romBrowser/FileType/FileType.h`
- `arm9/source/romBrowser/FileType/Nds/NdsFileType.h`
- `arm9/source/romBrowser/IRomBrowserController.h`
- `arm9/source/romBrowser/RomBrowserController.cpp`
- `arm9/source/romBrowser/RomBrowserController.h`
- `arm9/source/romBrowser/SdFolder.cpp`
- `arm9/source/romBrowser/SdFolderFilterSortParams.h`
- `arm9/source/romBrowser/viewModels/RomBrowserViewModel.cpp`
- `arm9/source/services/settings/AppSettings.h`
- `arm9/source/services/settings/JsonAppSettingsSerializer.thumb.cpp`

New files:

- `arm9/source/services/settings/SaveLocation.h`
- `arm9/source/romBrowser/FileType/Nds/NdsRomHeader.cpp`
- `arm9/source/romBrowser/FileType/Nds/NdsRomHeader.h`

</details>

## Menu and panels

### Three-dot menu and a three-button app bar

The app bar holds back, a three-dot menu and display settings. The menu lists Recently played, Favorites, Statistics and Delete game by name, plus the Only favorites and Only completed filters saying on or off, and no app bar button hides a second action behind a long press any more (holding X on a game still marks it completed).

- **Commits:** `089eb53`, `0b90347`, `eb691fb`
- **Upstream:** None found for the bar. [PR #37](https://github.com/LNH-team/pico-launcher/pull/37) has a loosely similar sheet of named entries, and [PR #67](https://github.com/LNH-team/pico-launcher/pull/67) would add a heart button to the bar.
- **Notes:** Moving everything into the menu left some of the six-button bar's plumbing in `AppBarView` and `IconButtonView` with no callers.

<details><summary>Files: 13 upstream, 7 new</summary>

Upstream files touched:

- `arm9/source/App.cpp`
- `arm9/source/App.h`
- `arm9/source/DialogPresenter.h`
- `arm9/source/romBrowser/IRomBrowserController.h`
- `arm9/source/romBrowser/RomBrowserController.cpp`
- `arm9/source/romBrowser/RomBrowserController.h`
- `arm9/source/romBrowser/RomBrowserState.h`
- `arm9/source/romBrowser/RomBrowserStateMachine.cpp`
- `arm9/source/romBrowser/RomBrowserStateTrigger.h`
- `arm9/source/romBrowser/viewModels/RomBrowserAppBarViewModel.h`
- `arm9/source/romBrowser/viewModels/RomBrowserBottomScreenViewModel.h`
- `arm9/source/romBrowser/views/RomBrowserAppBarView.cpp`
- `arm9/source/romBrowser/views/RomBrowserAppBarView.h`

New files:

- `arm9/gfx/moreIcon.grit`
- `arm9/gfx/moreIcon.png`
- `arm9/gfx/statsIcon.grit`
- `arm9/gfx/statsIcon.png`
- `arm9/source/romBrowser/viewModels/MenuViewModel.h`
- `arm9/source/romBrowser/views/menu/MenuBottomSheetView.cpp`
- `arm9/source/romBrowser/views/menu/MenuBottomSheetView.h`

</details>

### About sheet

A small button in the menu's title row opens an about sheet: Pico Launcher by the LNH team and Enhanced by rasalopa, the version with its commit and the repository the build came from, and a cheat sheet of the controls that have no button of their own.

- **Commits:** `e3dd62f`, `5f29717`
- **Upstream:** None found.
- **Settings and data:** `LAUNCHER_REPO`, passed in by `Makefile.arm9` from the remote the branch tracks.

<details><summary>Files: 10 upstream, 14 new</summary>

Upstream files touched:

- `arm9/source/App.cpp`
- `arm9/source/App.h`
- `arm9/source/romBrowser/IRomBrowserController.h`
- `arm9/source/romBrowser/RomBrowserController.cpp`
- `arm9/source/romBrowser/RomBrowserController.h`
- `arm9/source/romBrowser/RomBrowserState.h`
- `arm9/source/romBrowser/RomBrowserStateMachine.cpp`
- `arm9/source/romBrowser/RomBrowserStateTrigger.h`
- `arm9/source/romBrowser/viewModels/RomBrowserBottomScreenViewModel.h`
- `Makefile.arm9`

New files:

- `arm9/gfx/infoIcon.grit`
- `arm9/gfx/infoIcon.png`
- `arm9/gfx/lnhChipLogo.grit`
- `arm9/gfx/lnhChipLogo.png`
- `arm9/gfx/rasalopaAvatar.grit`
- `arm9/gfx/rasalopaAvatar.png`
- `arm9/source/Version.cpp`
- `arm9/source/Version.h`
- `arm9/source/romBrowser/viewModels/AboutViewModel.h`
- `arm9/source/romBrowser/viewModels/MenuViewModel.h`
- `arm9/source/romBrowser/views/about/AboutBottomSheetView.cpp`
- `arm9/source/romBrowser/views/about/AboutBottomSheetView.h`
- `arm9/source/romBrowser/views/menu/MenuBottomSheetView.cpp`
- `arm9/source/romBrowser/views/menu/MenuBottomSheetView.h`

</details>

### In-game exit combo

The menu's title row has a second small button, opening a sheet that picks the buttons held to leave a DS game. The combination is stored in `settings.json` and handed to the loader in a new API v4 header field at launch; a loader that does not report version 4 is never written to, so it keeps the combo it was built with. This exists for consoles whose L or R button no longer works.

- **Commits:** pending.
- **Upstream:** None found. The combination is compiled into the loader's ARM7 exit patch.
- **Settings and data:** `loaderExitCombo` in `settings.json`, as names (`"Select+L+R+Right"`).
- **Notes:** The loader side lives in `rasalopa/pico-loader` and is a separate PR; without it the setting is stored but changes nothing. While the sheet is picking buttons it keeps every one to itself, which is why `App::HandleInput` asks `DialogPresenter::IsCapturingInput()` before counting the START screenshot hold.

<details><summary>Files: 18 upstream, 5 new</summary>

Upstream files touched:

- `arm9/source/App.cpp`
- `arm9/source/App.h`
- `arm9/source/DialogPresenter.h`
- `arm9/source/gui/views/DialogView.h`
- `arm9/source/picoLoaderBootstrap.cpp`
- `arm9/source/picoLoaderBootstrap.h`
- `arm9/source/romBrowser/IRomBrowserController.h`
- `arm9/source/romBrowser/RomBrowserController.cpp`
- `arm9/source/romBrowser/RomBrowserController.h`
- `arm9/source/romBrowser/RomBrowserState.h`
- `arm9/source/romBrowser/RomBrowserStateMachine.cpp`
- `arm9/source/romBrowser/RomBrowserStateTrigger.h`
- `arm9/source/romBrowser/viewModels/MenuViewModel.h`
- `arm9/source/romBrowser/views/menu/MenuBottomSheetView.cpp`
- `arm9/source/romBrowser/views/menu/MenuBottomSheetView.h`
- `arm9/source/services/settings/AppSettings.h`
- `arm9/source/services/settings/JsonAppSettingsSerializer.thumb.cpp`
- `common/picoLoader7.h`

New files:

- `arm9/source/romBrowser/viewModels/ExitComboViewModel.h`
- `arm9/source/romBrowser/views/exitcombo/ExitComboBottomSheetView.cpp`
- `arm9/source/romBrowser/views/exitcombo/ExitComboBottomSheetView.h`
- `arm9/source/services/settings/KeyCombo.cpp`
- `arm9/source/services/settings/KeyCombo.h`

</details>

### Game info

Roms without a banner (GB, SNES and anything else run through a file association) are shown by their file name. The menu's **Game info** sheet edits a name, developer and publisher, stored with the file's favorites and play time in `gamedata.json`; the browser, the top screen, the recents and favorites panels and the statistics panel then show the name instead, with the developer and the publisher on the two lines under it. The sheet carries its own on-screen keyboard because the launcher has no text input.

- **Commits:** pending.
- **Upstream:** None found. The game data file and the panels exist only in the fork.
- **Settings and data:** `name`, `publisher` and `developer` in `gamedata.json`, per file name. No `settings.json` keys.
- **Notes:** Saving rebuilds the browser the way a display-mode change does, but lands on the renamed game instead of the top of the folder; B closes without touching the stored values.

<details><summary>Files: 21 upstream, 3 new</summary>

Upstream files touched:

- `arm9/source/App.cpp`
- `arm9/source/App.h`
- `arm9/source/romBrowser/DisplayMode/BannerListFileRecyclerAdapter.cpp`
- `arm9/source/romBrowser/IRomBrowserController.h`
- `arm9/source/romBrowser/RomBrowserController.cpp`
- `arm9/source/romBrowser/RomBrowserController.h`
- `arm9/source/romBrowser/RomBrowserState.h`
- `arm9/source/romBrowser/RomBrowserStateMachine.cpp`
- `arm9/source/romBrowser/RomBrowserStateTrigger.h`
- `arm9/source/romBrowser/viewModels/MenuViewModel.h`
- `arm9/source/romBrowser/viewModels/RomBrowserBottomScreenViewModel.h`
- `arm9/source/romBrowser/views/BannerView.h`
- `arm9/source/romBrowser/views/RomBrowserTopScreenView.cpp`
- `arm9/source/romBrowser/views/menu/MenuBottomSheetView.cpp`
- `arm9/source/romBrowser/views/menu/MenuBottomSheetView.h`
- `arm9/source/romBrowser/views/recents/RecentListItemView.cpp`
- `arm9/source/romBrowser/views/statistics/StatisticsBottomSheetView.cpp`
- `arm9/source/services/gamedata/GameDataEntry.h`
- `arm9/source/services/gamedata/IGameDataService.h`
- `arm9/source/services/gamedata/JsonGameDataService.thumb.cpp`
- `arm9/source/services/gamedata/JsonGameDataService.h`

New files:

- `arm9/source/romBrowser/viewModels/GameInfoEditViewModel.h`
- `arm9/source/romBrowser/views/gameinfo/GameInfoEditBottomSheetView.cpp`
- `arm9/source/romBrowser/views/gameinfo/GameInfoEditBottomSheetView.h`

</details>

### Focus lands on the game after picking it from a panel

Choosing a game from the recently played or favorites panel puts the highlight on that game, so the next A launches it instead of reopening the menu. After a game is deleted from its confirmation sheet, the highlight also goes back into the list instead of staying on the menu button.

- **Commits:** `8347bd4`
- **Upstream:** None found; the panels exist only in the fork.
- **Notes:** Shares the `_focusListAfterFolderLoad` flag name with [PR #84](https://github.com/LNH-team/pico-launcher/pull/84); a merge has to combine the two comments.

<details><summary>Files: 2 upstream, 0 new</summary>

Upstream files touched:

- `arm9/source/App.cpp`
- `arm9/source/App.h`

</details>

### Launcher version on screen

The launcher says which build it is: the version with its commit on the bottom screen while it boots and in the about sheet, and the version at the top-right of the statistics panel, so a bug report can name the build.

- **Commits:** `6152e92`, `a4087d7`
- **Upstream:** None found.
- **Settings and data:** `LAUNCHER_VERSION` in `arm9/source/Version.h`, bumped when the changelog is rolled; `LAUNCHER_BUILD`, the short commit hash, passed in by `Makefile.arm9`.

<details><summary>Files: 5 upstream, 4 new</summary>

Upstream files touched:

- `arm9/source/App.cpp`
- `arm9/source/App.h`
- `arm9/source/settings/views/ThemeListTopView.cpp`
- `arm9/source/settings/views/ThemeListTopView.h`
- `Makefile.arm9`

New files:

- `arm9/source/Version.cpp`
- `arm9/source/Version.h`
- `arm9/source/romBrowser/views/statistics/StatisticsBottomSheetView.cpp`
- `arm9/source/romBrowser/views/statistics/StatisticsBottomSheetView.h`

</details>

## Look and themes

### Focus look on every icon button

The colored circle behind a button now only means an option is chosen, and the button you are on gets its icon in the theme's accent color with a light tint behind it, so focus is easy to see on every theme. The menu rows follow the same rule.

- **Commits:** `3a5d6da`
- **Upstream:** None found as such, but [PR #67](https://github.com/LNH-team/pico-launcher/pull/67) also changes what the circle behind an icon button means, with an active and a disabled state in `IconButtonView`.

<details><summary>Files: 6 upstream, 1 new</summary>

Upstream files touched:

- `arm9/source/romBrowser/Theme/Material/MaterialAppBarView.cpp`
- `arm9/source/romBrowser/Theme/custom/CustomAppBarView.cpp`
- `arm9/source/romBrowser/views/IconButton2DView.cpp`
- `arm9/source/romBrowser/views/IconButton3DView.cpp`
- `arm9/source/romBrowser/views/IconButtonView.cpp`
- `arm9/source/romBrowser/views/IconButtonView.h`

New files:

- `arm9/source/romBrowser/views/menu/MenuBottomSheetView.cpp`

</details>

### Theme selector (opens on your theme, always readable)

The theme selector opens with the theme you use highlighted, and it is always drawn as a Material theme in that theme's colors, so a custom theme's own backgrounds can't make the list hard to read. The top screen still previews the highlighted theme.

- **Commits:** `8f3a614`, `30792cb`
- **Upstream:** None found.
- **Settings and data:** Reads the active theme's `primaryColor` and `darkTheme`; no new keys.
- **Fork issues:** [#17](https://github.com/rasalopa/pico-launcher-enhanced/issues/17)

<details><summary>Files: 6 upstream, 0 new</summary>

Upstream files touched:

- `arm9/source/settings/SettingsProcess.cpp`
- `arm9/source/settings/viewModels/ThemeListViewModel.cpp`
- `arm9/source/settings/viewModels/ThemeListViewModel.h`
- `arm9/source/settings/views/ThemeListBottomView.cpp`
- `arm9/source/themes/ThemeRepository.cpp`
- `arm9/source/themes/ThemeRepository.h`

</details>

### Deleting themes

In the theme selector, a trash button at the bottom of the app bar deletes the highlighted theme's folder after a confirmation that names the theme and its folder. The theme in use, `material` and `raspberry` can't be deleted. The folder is read in full first and nothing is deleted when anything looks wrong: a read-only file, too deep or too many entries, a name that can't be read back, or a damaged folder link. Then `theme.json` goes first, the rest one entry at a time with every path checked to be inside the folder, and the selector starts again on the theme that took its place.

- **Commits:** `7883afa`, `6de7ef2`, `da2e05b`, `caf3a66`, `3199e3b`, `9a5e88a`
- **Upstream:** None found.
- **Fork issues:** [#5](https://github.com/rasalopa/pico-launcher-enhanced/issues/5)
- **Notes:** Shares the game delete's confirmation sheet, which now reads its texts from a small view model interface. The selector gets its own dialog presenter, and its IO stack grows from 2 KB to 4 KB. Memory for the check comes from malloc: `new (std::nothrow)` pulls in the C++ exception runtime, which the launcher doesn't have.

<details><summary>Files: 13 upstream, 8 new</summary>

Upstream files touched:

- `arm9/source/App.cpp`
- `arm9/source/romBrowser/views/IconButtonView.cpp`
- `arm9/source/settings/ISettingsController.h`
- `arm9/source/settings/SettingsController.cpp`
- `arm9/source/settings/SettingsController.h`
- `arm9/source/settings/SettingsProcess.cpp`
- `arm9/source/settings/SettingsProcess.h`
- `arm9/source/settings/ThemeInfoManager.h`
- `arm9/source/settings/viewModels/ThemeListViewModel.cpp`
- `arm9/source/settings/viewModels/ThemeListViewModel.h`
- `arm9/source/settings/views/SettingsAppBarView.cpp`
- `arm9/source/settings/views/SettingsAppBarView.h`
- `arm9/source/settings/views/ThemeListBottomView.cpp`

New files:

- `arm9/source/romBrowser/viewModels/DeleteConfirmViewModel.h`
- `arm9/source/romBrowser/viewModels/IDeleteConfirmViewModel.h`
- `arm9/source/romBrowser/views/deleteconfirm/DeleteConfirmBottomSheetView.cpp`
- `arm9/source/romBrowser/views/deleteconfirm/DeleteConfirmBottomSheetView.h`
- `arm9/source/settings/ThemeFolderDeleter.cpp`
- `arm9/source/settings/ThemeFolderDeleter.h`
- `arm9/source/settings/ThemeFolderRules.h`
- `arm9/source/settings/viewModels/ThemeDeleteConfirmViewModel.h`

</details>

### Per-folder background music

Put a bgm.bcstm file inside a folder and that music plays while you browse it, switching back to the theme's music when you leave. Subfolders do not inherit it.

- **Commits:** `ef47b2f`
- **Upstream:** [PR #37](https://github.com/LNH-team/pico-launcher/pull/37) adds a background music picker with bundled tracks. Both edit `BgmService`.
- **Settings and data:** A `bgm.bcstm` file inside the folder.

<details><summary>Files: 7 upstream, 0 new</summary>

Upstream files touched:

- `arm9/source/App.cpp`
- `arm9/source/bgm/BgmService.cpp`
- `arm9/source/bgm/BgmService.h`
- `arm9/source/bgm/IBgmService.h`
- `arm9/source/romBrowser/IRomBrowserController.h`
- `arm9/source/romBrowser/RomBrowserController.cpp`
- `arm9/source/romBrowser/RomBrowserController.h`

</details>

### Night theme backgrounds

A custom theme can ship night versions of its backgrounds (topbg_night.bin, bottombg_night.bin) that the launcher uses when it loads the theme between 20:00 and 6:59, at boot or on coming back from the theme selector, and a PC tool makes them from the day ones.

- **Commits:** `93d8f33`
- **Upstream:** None found.
- **Settings and data:** Optional `topbg_night.bin` and `bottombg_night.bin` in a custom theme's folder.

<details><summary>Files: 2 upstream, 3 new</summary>

Upstream files touched:

- `arm9/source/themes/custom/CustomMainBackground.cpp`
- `arm9/source/themes/custom/CustomSubBackground.cpp`

New files:

- `arm9/source/themes/custom/ThemeTimeOfDay.cpp`
- `arm9/source/themes/custom/ThemeTimeOfDay.h`
- `tools/make_night_bg.py`

</details>

## Console support

### Lid sleep (close the lid to sleep)

Closing the lid while the launcher is open puts the console to sleep with the screens off and the power LED blinking, and opening it wakes it where it was. On a DS Lite the chosen brightness level comes back after waking.

- **Commits:** `149600d`, `5872f40`
- **Upstream:** Asked for in [LNH-team/pico-launcher#9](https://github.com/LNH-team/pico-launcher/issues/9), still open.
- **Fork issues:** PR [#23](https://github.com/rasalopa/pico-launcher-enhanced/issues/23)
- **Notes:** Written by marlooonxdd.

<details><summary>Files: 1 upstream, 0 new</summary>

Upstream files touched:

- `arm7/source/main.cpp`

</details>

### DS Lite brightness control

On a DS Lite, a Light row in the display settings sheet sets the backlight to one of its four levels, each with its own icon. The choice applies at once, is remembered across boots and stays on inside the game; on an original DS, a DSi, or a 3DS running from a DSpico the row is not shown.

- **Commits:** `e991d51`, `952fcc4`, `f266c48`
- **Upstream:** Asked for in [LNH-team/pico-launcher#73](https://github.com/LNH-team/pico-launcher/issues/73). [PR #43](https://github.com/LNH-team/pico-launcher/pull/43) does the same through its own IPC service, with levels on a DSi through the MCU as well; after review there it leaves the level to the console, where this stores it in `settings.json` and applies it at boot.
- **Settings and data:** `backlightLevel` in `settings.json`, 0 to 3.
- **Fork issues:** [#2](https://github.com/rasalopa/pico-launcher-enhanced/issues/2), [#27](https://github.com/rasalopa/pico-launcher-enhanced/issues/27)
- **Notes:** Adds an IPC channel numbered 21 in `common/ipcChannels.h`, the same number [PR #78](https://github.com/LNH-team/pico-launcher/pull/78) uses for its own channel.

<details><summary>Files: 11 upstream, 10 new</summary>

Upstream files touched:

- `arm7/source/main.cpp`
- `arm9/source/main.cpp`
- `arm9/source/romBrowser/IRomBrowserController.h`
- `arm9/source/romBrowser/RomBrowserController.cpp`
- `arm9/source/romBrowser/RomBrowserController.h`
- `arm9/source/romBrowser/viewModels/DisplaySettingsViewModel.h`
- `arm9/source/romBrowser/views/DisplaySettingsBottomSheetView.cpp`
- `arm9/source/romBrowser/views/DisplaySettingsBottomSheetView.h`
- `arm9/source/services/settings/AppSettings.h`
- `arm9/source/services/settings/JsonAppSettingsSerializer.thumb.cpp`
- `common/ipcChannels.h`

New files:

- `arm9/gfx/brightness1Icon.grit`
- `arm9/gfx/brightness1Icon.png`
- `arm9/gfx/brightness2Icon.grit`
- `arm9/gfx/brightness2Icon.png`
- `arm9/gfx/brightness3Icon.grit`
- `arm9/gfx/brightness3Icon.png`
- `arm9/gfx/brightness4Icon.grit`
- `arm9/gfx/brightness4Icon.png`
- `arm9/source/backlightIpc.cpp`
- `arm9/source/backlightIpc.h`

</details>

### DSi-only games refused on a DS

On a DS or DS Lite, pressing A on a game made only for the DSi no longer launches it into a white screen: the launcher stays in the browser and says Needs a DSi or 3DS. Homebrew is left alone, since many homebrew apps say DSi only and still run on a DS. Nothing changes on a DSi or 3DS.

- **Commits:** `6e199bb`, `a5671d3`
- **Upstream:** None found.
- **Fork issues:** [#22](https://github.com/rasalopa/pico-launcher-enhanced/issues/22), for the DS side; [#30](https://github.com/rasalopa/pico-launcher-enhanced/issues/30), the homebrew exception

<details><summary>Files: 5 upstream, 2 new</summary>

Upstream files touched:

- `arm9/source/App.cpp`
- `arm9/source/romBrowser/RomBrowserController.cpp`
- `arm9/source/romBrowser/RomBrowserController.h`
- `arm9/source/romBrowser/RomBrowserStateMachine.cpp`
- `arm9/source/romBrowser/RomBrowserStateTrigger.h`

New files:

- `arm9/source/romBrowser/FileType/Nds/NdsRomHeader.cpp`
- `arm9/source/romBrowser/FileType/Nds/NdsRomHeader.h`

</details>

## Cheats

### Cheats list wrap-around and X: all off hint

The cheat list wraps at both ends, so the bottom of a long cheat database is one press up from the top; inside a sub-category, Up on the first cheat still goes to the back button. The cheats sheet shows X: all off, making the existing turn-everything-off button visible.

- **Commits:** `df56d71`, `1de108b`
- **Upstream:** Asked for in [LNH-team/pico-launcher#83](https://github.com/LNH-team/pico-launcher/issues/83). X already turned every cheat off upstream; this makes it visible. [PR #77](https://github.com/LNH-team/pico-launcher/pull/77) works on the same sheet's layout and parser.

<details><summary>Files: 4 upstream, 0 new</summary>

Upstream files touched:

- `arm9/source/gui/views/RecyclerView.cpp`
- `arm9/source/gui/views/RecyclerView.h`
- `arm9/source/romBrowser/views/cheats/CheatsBottomSheetView.cpp`
- `arm9/source/romBrowser/views/cheats/CheatsBottomSheetView.h`

</details>

## Screenshots

### Screenshots of both screens by holding START

Hold START for about half a second to save both screens as a pair of numbered BMP files in /_pico/screenshots. A short message at the bottom of the lower screen confirms the save or says it failed.

- **Commits:** `003ebee`, `4c6e6dc`, `3bed3ae`, `3bbafdf`, `798d990`
- **Upstream:** Sent upstream as [PR #85](https://github.com/LNH-team/pico-launcher/pull/85), for [LNH-team/pico-launcher#27](https://github.com/LNH-team/pico-launcher/issues/27). The later fix for the message's descenders is not in that PR.
- **Settings and data:** Writes `/_pico/screenshots/shotNNN_top.bmp` and `shotNNN_bot.bmp`.
- **Notes:** Adds the toast message that other entries use.

<details><summary>Files: 6 upstream, 4 new</summary>

Upstream files touched:

- `arm9/source/App.cpp`
- `arm9/source/App.h`
- `arm9/source/gui/views/Label3DView.cpp`
- `arm9/source/gui/views/Label3DView.h`
- `arm9/source/romBrowser/views/RomBrowserTopScreenView.cpp`
- `arm9/source/romBrowser/views/RomBrowserTopScreenView.h`

New files:

- `arm9/source/Screenshot.cpp`
- `arm9/source/Screenshot.h`
- `arm9/source/gui/views/ToastView.cpp`
- `arm9/source/gui/views/ToastView.h`

</details>

## Fixes to upstream behavior

### A theme row no longer keeps the theme it held before

The theme selector reuses its rows as the list scrolls, and a row whose folder had no readable `theme.json` kept the text and the theme of whatever it showed before, so A could apply that other theme. The row now shows the folder name, and A does nothing on it.

- **Commits:** `3fec27b`
- **Upstream:** None found. Upstream has the same code.

<details><summary>Files: 3 upstream, 0 new</summary>

Upstream files touched:

- `arm9/source/settings/views/ThemeAdapter.cpp`
- `arm9/source/themes/ThemeRepository.cpp`
- `arm9/source/themes/ThemeRepository.h`

</details>

### `.agb` files open with an agb association

An association was looked up by its short name, and an `agb` association's short name is `gba`, so `.agb` files didn't match it and stayed hidden, while `.gba` files could open with it. An association now matches on its own extension: an `agb` one opens `.agb` files, and `.gba` files go by their own `gba` one.

- **Commits:** `72e45b4`
- **Upstream:** None found. Upstream has the same code.

<details><summary>Files: 1 upstream, 0 new</summary>

Upstream files touched:

- `arm9/source/romBrowser/FileType/ExtensionFileTypeProvider.cpp`

</details>

### Custom covers and icons from more image editors

Custom 8 bpp BMP covers with a color table shorter than 256 entries, or saved with the newer header GIMP writes, now draw correctly instead of as colored noise, and covers saved top to bottom are no longer upside down. Covers are still read only at 8 bpp and 128×96, icons at 4 bpp and 32×32, uncompressed. A cover or icon the launcher can't read gives way to the next one in line instead of drawing garbage or blanking the game's own icon.

- **Commits:** `f391482`
- **Upstream:** None found as a request. [PR #76](https://github.com/LNH-team/pico-launcher/pull/76) also checks whether a BMP icon was read, but only to leave it out of its coverflow icon covers; an unreadable icon still replaces the game's own icon there.

<details><summary>Files: 8 upstream, 0 new</summary>

Upstream files touched:

- `arm9/source/romBrowser/CoverRepository.cpp`
- `arm9/source/romBrowser/FileType/BmpFileCover.cpp`
- `arm9/source/romBrowser/FileType/BmpFileCover.h`
- `arm9/source/romBrowser/FileType/BmpFileIconData.cpp`
- `arm9/source/romBrowser/FileType/BmpFileIconData.h`
- `arm9/source/romBrowser/FileType/BmpHeader.h`
- `arm9/source/romBrowser/IconRepository.cpp`
- `arm9/source/settings/ThemeInfoManager.cpp`

</details>

### Focus stays in the list when leaving an empty folder

Going back up from an empty folder puts the highlight on the folder you just left, instead of stranding it on the app bar's back arrow where A kept climbing up. The back arrow shares that path, so going up with it, by touch or with A, now also ends on the list, and a second A opens the folder you just left instead of climbing another level.

- **Commits:** `1e61848`
- **Upstream:** Sent upstream as [PR #84](https://github.com/LNH-team/pico-launcher/pull/84).
- **Fork issues:** [#10](https://github.com/rasalopa/pico-launcher-enhanced/issues/10)

<details><summary>Files: 1 upstream, 0 new</summary>

Upstream files touched:

- `arm9/source/App.cpp`

</details>

### Focus stays on the app bar above an empty cover flow

In the cover flow layout, pressing Down from the app bar in an empty folder keeps the highlight on the button. It used to vanish, and A did nothing until Up or B.

- **Commits:** `55de101`
- **Upstream:** None found.

<details><summary>Files: 1 upstream, 0 new</summary>

Upstream files touched:

- `arm9/source/romBrowser/views/CoverFlowRecyclerViewBase.cpp`

</details>

### No stale sprite flash during the boot splash

A leftover sprite no longer flashes at the top left of the screen while the boot splash shows or while a slow theme loads.

- **Commits:** `fe02536`
- **Upstream:** None found for this symptom; [LNH-team/pico-launcher#82](https://github.com/LNH-team/pico-launcher/issues/82) is about the splash too, with a different symptom.

<details><summary>Files: 1 upstream, 0 new</summary>

Upstream files touched:

- `arm9/source/App.cpp`

</details>

## Tools

### Desktop helper tools (covers, banners, icons, night backgrounds)

Python scripts you run on a PC to turn art into covers and icons, make folder banners and night backgrounds, and download missing GBA and retro system covers from libretro-thumbnails onto the SD card. The cover downloaders take the card's path with `--sd`, so they no longer need a Mac volume; the Windows fixes for #12, #13 and #14 are in the code, and none of them has been run on Windows.

- **Commits:** `585cc48`, `2a877ec`, `de5b491`, `94a153d`, `34c7a04`, `a89257d`, `7974caa`, `82a01f5`, `e20b77c`, `9f72ac5`, `e43784d`, `842e7e1`, `f5d66aa`
- **Upstream:** None found; the `tools` folder exists only in the fork. Most of what they make is for upstream features: covers, and the custom icons and banners for games and folders from merged PR #64. The night backgrounds are for the fork's own night theme backgrounds, and `png2icon.py` prepares the launcher's own cartridge icon for ndstool.
- **Fork issues:** [#12](https://github.com/rasalopa/pico-launcher-enhanced/issues/12), [#13](https://github.com/rasalopa/pico-launcher-enhanced/issues/13), [#14](https://github.com/rasalopa/pico-launcher-enhanced/issues/14)

<details><summary>Files: 0 upstream, 8 new</summary>

New files:

- `tools/png2icon.py`
- `tools/png2iconbmp.py`
- `tools/img2cover.py`
- `tools/fetch_covers_gba.py`
- `tools/fetch_covers.py`
- `tools/make_banner.py`
- `tools/make_night_bg.py`
- `tools/assets/snes-console.png`

</details>

## Housekeeping

Commits with no feature of their own, listed so every commit is accounted for. Two of them could still go upstream on their own: the error log and the VRAM offset fix.

- **Fork documentation (readme, docs pages, screenshots, contributing).** The fork's readme, its docs pages and the contributing guide. `ebddb97`, `37b5a5d`, `50acb27`, `34e04db`, `cd5ce1a`, `95bfbca`, `5557e97`, `e07bfb9`, `799148e`, `4e7a08a`
- **Changelog release rolls.** Release headings in the changelog, and the version bump with the last one. `d8a7744`, `0b51e82`, `7bd4a5b`, `6fd0165`, `ad218ea`, `83e5c03`
- **Error log on the SD card.** Errors go to `/_pico/launcher.log` on real hardware, so what was logged before a crash can be read afterwards; the crash itself is not recorded, and the next boot that logs an error starts the file again. `3a4ce98`
- **Enhanced line in the cartridge banner.** The boot banner's second line says Enhanced, so tools can tell the fork from the stock launcher. `1966ccd`
- **Icon button VRAM offset initialized.** An icon button's selector offset starts at zero instead of whatever was in memory; nothing changes on screen. From marlooonxdd ([#24](https://github.com/rasalopa/pico-launcher-enhanced/pull/24)). `47e8944`, `12e9203`
- **Personal launcher title and icon.** A personal title and icon for a few days in July, put back since. `6d6e58d`, `76da0d0`

---

Checked for this update: every commit the fork adds sits in exactly one entry above, apart from the two merges and the commits that write this page, and each entry was compared with the code.
