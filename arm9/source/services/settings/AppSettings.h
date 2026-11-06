#pragma once
#include <memory>
#include "core/String.h"
#include "RomBrowserDisplaySettings.h"
#include "SaveLocation.h"
#include "FileAssociation.h"
#include "ParentalControls.h"

class AppSettings
{
public:
    String<char, 16> language = "english";
    String<char, 64> theme = "material";
    String<char, 256> lastUsedFilePath = "";
    /// @brief DS Lite backlight level (0 = low .. 3 = max), or -1 to leave
    ///        the firmware's level untouched (the default until the user
    ///        picks one in display settings).
    s8 backlightLevel = -1;
    SaveLocation saveLocation = SaveLocation::NextToRom;
    /// @brief Whether a launch is recorded in the game data (launch count,
    ///        play time, last played). Off, nothing about launches is
    ///        written; favorites and completed marks keep working.
    bool launchTracking = true;
    /// @brief Keys held to exit a DS game, as names ("Select+L+R+Right").
    ///        Empty means the loader's own default. The combo only reaches a
    ///        loader that reports API version 4; older ones keep the combo
    ///        they were built with.
    String<char, 32> loaderExitCombo = "";
    /// @brief Whether the destructive actions are locked: see ParentalControls.
    ParentalControls parentalControls;
    RomBrowserDisplaySettings romBrowserDisplaySettings;

    std::unique_ptr<FileAssociation[]> fileAssociations;
    u32 numberOfFileAssociations = 0;
};