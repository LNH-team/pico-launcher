#pragma once
#include <string.h>
#include "core/String.h"
#include "core/mini-printf.h"

/// @brief Persisted data for ONE rom file, keyed by its file name (same
///        convention as the /_pico/icons|covers|user folders). Marks and play
///        statistics belong to the file, so two copies of a game are tracked
///        separately and renaming a rom starts it over.
struct GameDataEntry
{
    /// @brief Longest name the game info sheet can store. Also the size of the
    ///        json string, so the editor never offers more than is kept.
    static constexpr u32 kMaxDisplayNameLength = 48;
    static constexpr u32 kMaxPublisherLength = 32;
    static constexpr u32 kMaxDeveloperLength = 32;

    String<char, 96> fileName;
    /// @brief Internal game code (NDS/GBA header), empty when the file has
    ///        none. Stored as information only - it is never used to look an
    ///        entry up, because the browser filter only ever sees file names and
    ///        the two would then disagree (issue #7).
    String<char, 8> gameCode;
    /// @brief The name the user set in the game info sheet, shown instead of
    ///        the file name. Empty means "show the file name", which is also
    ///        what every file gets: only roms without a banner (GBA, SNES,
    ///        emulator shortcuts) have a file name worth replacing.
    String<char, kMaxDisplayNameLength> displayName;
    /// @brief Free-form credits, also from the game info sheet. The developer
    ///        takes the line under the name and the publisher the one under
    ///        that, as a DS banner lays them out; never used to identify
    ///        anything.
    String<char, kMaxPublisherLength> publisher;
    String<char, kMaxDeveloperLength> developer;
    u32 launchCount = 0;
    /// @brief Accumulated play time. A session spans from launching the game
    ///        until the next launcher boot, so it is an approximation.
    u32 playMinutes = 0;
    bool favorite = false;
    /// @brief Marked as finished by the user (long-press X in the browser).
    bool completed = false;
    /// @brief "YYYY-MM-DD HH:MM", empty when never launched. Lexicographic
    ///        order equals chronological order.
    String<char, 20> lastPlayed;
    /// @brief Full path of the file at its last launch, empty when never
    ///        launched. Used by the recents list to navigate back to it.
    String<char, 256> path;

    bool HasDisplayMetadata() const
    {
        return displayName.GetString()[0] != 0
            || publisher.GetString()[0] != 0
            || developer.GetString()[0] != 0;
    }
};

/// @brief The name to show for a file: the user's name when set, the file name
///        otherwise. Never null, so callers can hand the result straight to a
///        label.
inline const char* GameDataDisplayName(const GameDataEntry* entry, const char* fileName)
{
    if (entry && entry->displayName.GetString()[0] != 0)
        return entry->displayName.GetString();
    return fileName;
}

/// @brief Orders games by launches, ties by file name regardless of case, so
///        the star on the top screen and the statistics panel's list are built
///        from the same rule and never name different games.
inline bool LaunchedMoreThan(const GameDataEntry& a, const GameDataEntry& b)
{
    return a.launchCount > b.launchCount ||
        (a.launchCount == b.launchCount &&
            strcasecmp(a.fileName.GetString(), b.fileName.GetString()) < 0);
}
