# Game Data File
Pico Launcher Enhanced stores per-game data (favorites, launch counts, play time; see [Enhanced.md](Enhanced.md)) in `/_pico/gamedata.json`. This document specifies the format for external tool authors.

Regular users never need to edit this file. Deleting it simply resets all favorites and statistics.

## Example
```json
{
  "games": {
    "Some Game.nds": {
      "gameCode": "ABCE",
      "favorite": true,
      "completed": true,
      "launchCount": 12,
      "playMinutes": 340,
      "lastPlayed": "2026-07-16 21:03",
      "path": "/Games/nds/Some Game.nds"
    },
    "tetris.gb": {
      "name": "Tetris",
      "publisher": "Nintendo",
      "launchCount": 2,
      "lastPlayed": "2026-07-10 18:40",
      "path": "/Games/gb/tetris.gb"
    }
  },
  "sessionGame": "Some Game.nds",
  "sessionGameCode": "ABCE",
  "sessionStart": "2026-07-16 21:03"
}
```

## The `games` object
Each key in `games` is a file name (not a path). All keys inside an entry are optional. The launcher omits any key holding its default value:

| Key | Type | Written when | Meaning |
|---|---|---|---|
| `gameCode` | string | non-empty | Internal game code from the NDS/GBA header, stored as information only: it is never used to identify an entry. Written when it is printable ASCII and not the `####` toolchain placeholder (homebrew ROMs often carry garbage or the placeholder there). |
| `name` | string | non-empty | Name shown instead of the file name, set in the launcher's **Game info** sheet. Stored as information for display only: the entry is still keyed by the file name. |
| `publisher` | string | non-empty | Free-form publisher, shown on the line under the developer when set. Never used to identify anything. |
| `developer` | string | non-empty | Free-form developer, shown on the line under the name when set. Never used to identify anything. |
| `favorite` | bool | `true` | Marked as favorite. Absent means not a favorite; `false` is never written. |
| `completed` | bool | `true` | Marked as completed (finished). Absent means not completed; `false` is never written. |
| `launchCount` | number | > 0 | How many times the game was launched. |
| `playMinutes` | number | > 0 | Accumulated play time in minutes (approximate, see below). |
| `lastPlayed` | string | non-empty | `"YYYY-MM-DD HH:MM"`, 24-hour clock. Lexicographic order equals chronological order, so tools can sort these as plain strings. |
| `path` | string | non-empty | Full path of the file at its last launch. Used by the recently played panel to navigate back to the game. |

An entry whose `favorite` and `completed` are both false, whose `launchCount` and `playMinutes` are both 0, and which has no `name`, `publisher` or `developer` is pruned on the next write. Deleting a game through the launcher also removes its entry.

## Session keys (root level)
While a play session is open, the root object holds:

| Key | Written when | Meaning |
|---|---|---|
| `sessionGame` | session open | File name of the launched game. |
| `sessionGameCode` | session open and code known | Its game code. |
| `sessionStart` | session open | Launch time, `"YYYY-MM-DD HH:MM"`. |

A session opens when a game is launched and closes at the next launcher boot. On boot, the elapsed time since `sessionStart` is added to the game's `playMinutes` if it is between 1 minute and 6 hours (longer means the console was powered off, not playing), and the session keys are removed.

With `launchTracking` set to `false` in `settings.json`, a launch writes nothing here, and a session still open from before the switch is removed on the next boot without being credited.

## Identity
**An entry belongs to one ROM file, and the file name is its identity.** Favorites, completed marks
and play statistics all follow the file. Comparison is case-insensitive.

- Two copies of the same game keep separate entries, with separate favorites and separate play time.
  A ROM hack and its base game never share data either, even though they share a game code.
- `gameCode`, `name`, `publisher` and `developer` are stored as information only. None of them is used to look an entry up: renaming a rom outside the launcher still starts the entry over, whatever `name` says.
- **Renaming a ROM starts it over**: the launcher sees a different file, so the old entry stays behind
  (unused) and the renamed file begins with no favorite and no history.
- Moving a ROM to another folder keeps its data, since the file name does not change, and the browser
  filter keeps working immediately. Its stored `path` still points at the old location until the game is
  launched or re-marked, and until then the favorites panel leaves it out (the panel needs a path that
  resolves).

Earlier versions resolved entries by `gameCode` first and re-keyed them to whatever file was used last.
That made a game's heart appear on every copy while the browser filter, which only ever sees file
names, could not match them, so filtering by favorites could come up empty. Worse, marking such a
file toggled the flag on the *other* copy's entry instead of creating its own.

## Limits
**File names longer than 96 bytes are not tracked at all.** Marking such a game does nothing and
logs an error. They used to be truncated, which silently merged two files sharing a 96-character prefix
into one entry and lost one of their favorites for good; refusing them is the honest failure.

Other values longer than these are truncated (lengths in bytes, so accented characters count double):

- entry key / `sessionGame` (file name): 96 (longer names are refused, see above)
- `gameCode`: 8
- `name`: 48
- `publisher` / `developer`: 32
- `lastPlayed` / `sessionStart`: 20
- `path`: 256

Years in `lastPlayed` are written as `20YY`; dates before the year 2000 do not parse and are ignored for play-time accounting.

## Rules for external tools
- **Unknown keys are not preserved.** The launcher rewrites the whole file on every change and serializes only the keys listed above. Do not store tool-specific data in this file.
- The file is pretty-printed JSON and is replaced in full on every save. Entry order is not meaningful and not preserved.
- **Saves are atomic.** The launcher writes `/_pico/gamedata.tmp` and renames it over `gamedata.json`
  only once it is complete, so losing power mid-save leaves the previous file intact. On the next boot a
  leftover `gamedata.tmp` is deleted when `gamedata.json` is present, but **promoted to
  `gamedata.json` when it is missing**: a crash between the two steps would otherwise leave the temp
  file as the only surviving copy. Do not delete it blindly.
- **If the file cannot be parsed, the launcher refuses to save for the rest of that session** and logs
  the error, rather than overwriting your data with the little it managed to read. Fix or remove the
  file to start saving again. (Earlier versions overwrote it, so a tool writing invalid JSON wiped
  everything.)
- Duplicate `gameCode` values are fine: entries are never merged by code. Entry keys (file names)
  must be unique, which JSON already guarantees.
- The launcher parses the file with a bounded memory budget of 96 KB (about three times the file size); files larger than roughly 32 KB may fail to load. In practice this fits several hundred entries.
