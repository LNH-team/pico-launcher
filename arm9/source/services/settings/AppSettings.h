#pragma once
#include <memory>
#include "core/String.h"
#include "RomBrowserDisplaySettings.h"
#include "FileAssociation.h"

class AppSettings
{
public:
    String<char, 16> language = "english";
    String<char, 64> theme = "material";
    String<char, 256> lastUsedFilePath = "";
    /// @brief Full path of the game most recently launched from the favorites view, so that
    ///        favorite can be re-selected on the next boot. Kept apart from lastUsedFilePath
    ///        (which holds the ":favorites" sentinel in that case) because a sentinel plus a
    ///        full-length path does not fit the browser's 256 byte navigation buffer.
    String<char, 256> lastUsedFavoriteFilePath = "";
    RomBrowserDisplaySettings romBrowserDisplaySettings;

    std::unique_ptr<FileAssociation[]> fileAssociations;
    u32 numberOfFileAssociations = 0;
};