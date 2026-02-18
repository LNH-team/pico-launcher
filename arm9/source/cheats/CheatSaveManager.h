/*
    CheatSaveManager.h
    Manages saving and loading per-game cheat selections.
    Each ROM stores its selected cheats in:
      /_pico/extras/cheats/{IDCODE}_{FILENAME}.dat
*/

#pragma once
#include <nds/ndstypes.h>
#include "CheatCodelist.h"

class CheatSaveManager
{
public:
    /// @brief Saves the currently selected cheats for the given ROM.
    /// @param cheatList The cheat codelist with selections.
    /// @param gameCode  The 4-character game code (e.g. "ASME").
    /// @param romFileName The ROM file name (without path).
    /// @return True if saved successfully.
    static bool SaveSelections(const CheatCodelist& cheatList, const char* gameCode, const char* romFileName);

    /// @brief Loads previously saved cheat selections for the given ROM.
    /// @param cheatList The cheat codelist to apply selections to.
    /// @param gameCode  The 4-character game code.
    /// @param romFileName The ROM file name (without path).

    /// @brief Writes selected cheat codes to a file for use by the loader.
    /// @param cheatList The cheat codelist with selections.
    static bool WriteLinkerFormat(const CheatCodelist& cheatList, const char* gameCode, const char* romFileName);

private:
    /// @param outputPath The output path for the cheat data.
    /// @return True if written successfully.
    static bool WriteCheatsToFile(const CheatCodelist& cheatList, const char* outputPath);

private:
    /// @brief Builds the save file path.
    /// @param gameCode The game code.
    /// @param romFileName The ROM file name.
    /// @param outPath Output buffer for the path.
    /// @param outPathSize Size of the output buffer.
    static void BuildSavePath(const char* gameCode, const char* romFileName,
        char* outPath, u32 outPathSize);

    /// @brief Ensures the cheats directory exists.
    static void EnsureDirectory();
};
