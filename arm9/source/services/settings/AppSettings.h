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
    RomBrowserDisplaySettings romBrowserDisplaySettings;

    std::unique_ptr<FileAssociation[]> fileAssociations;
    u32 numberOfFileAssociations = 0;

    static constexpr u32 MaxHidePatterns = 8;
    String<char, 128> hidePatterns[MaxHidePatterns];
    u32 numberOfHidePatterns = 0;
};