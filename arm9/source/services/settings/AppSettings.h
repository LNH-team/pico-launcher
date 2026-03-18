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
    bool darkMode = false;
    String<char, 128> bgm = "";
    RomBrowserDisplaySettings romBrowserDisplaySettings;

    std::unique_ptr<FileAssociation[]> fileAssociations;
    u32 numberOfFileAssociations = 0;

    u32 layoutSlot = 1;

    std::unique_ptr<String<char, 256>[]> favorites;
    u32 numberOfFavorites = 0;
};