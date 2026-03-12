#pragma once
#include <memory>
#include "common.h"
#include "core/String.h"

/// @brief In-memory representation of state.bin (/_pico/extras/state.bin).
/// Holds runtime state that persists across sessions but is NOT stored in settings.json:
struct AppStateBin
{
    /// Name of the theme that was loaded at last startup (e.g. "material").
    String<char, 64> appliedThemeName;

    /// Primary color of the applied theme (from theme.json at load time).
    u8 primaryColorR = 0xFF;
    u8 primaryColorG = 0xFF;
    u8 primaryColorB = 0xFF;

    /// Dark-theme flag of the applied theme (from theme.json at load time).
    u8 darkTheme = 0;

    /// Currently selected layout slot (1-based).
    u32 layoutSlot = 1;

    /// Favorite file paths.
    std::unique_ptr<String<char, 256>[]> favorites;
    u32 numberOfFavorites = 0;
};
