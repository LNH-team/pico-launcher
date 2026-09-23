#pragma once
#include <memory>
#include "core/String.h"
#include "RomBrowserDisplaySettings.h"
#include "FileAssociation.h"
#include "picoLoader7.h"

class AppSettings
{
public:
    String<char, 16> language = "english";
    String<char, 64> theme = "material";

    /// @brief The language games see when launched through Pico Loader.
    ///        Separate from \see language, which is the display language of Pico Launcher itself.
    PicoLoaderGameLanguage gameLanguage = PLOAD_GAME_LANGUAGE_AUTO;

    String<char, 256> lastUsedFilePath = "";
    RomBrowserDisplaySettings romBrowserDisplaySettings;

    std::unique_ptr<FileAssociation[]> fileAssociations;
    u32 numberOfFileAssociations = 0;
};