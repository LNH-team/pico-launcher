#pragma once
#include "services/settings/IAppSettingsService.h"

/// @brief Maximum number of translation keys that can be loaded.
#define LOCALIZATION_MAX_KEYS 16

class Localization
{
public:
    /// @brief Initializes the localization system for the current language.
    ///        Attempts to load translations from /_pico/extras/translations/{language}.json.
    ///        Falls back to hardcoded English if the file is not found.
    /// @param appSettingsService The application settings service.
    static void Initialize(const IAppSettingsService* appSettingsService);

    /// @brief Translates the given key to the current language.
    /// @param key The translation key (e.g. "display_settings").
    /// @return The translated UTF-16 string, or an empty string if not found.
    static const char16_t* Translate(const char* key);

private:
    struct TranslationEntry
    {
        char key[32];
        char16_t value[64];
    };

    static TranslationEntry s_entries[LOCALIZATION_MAX_KEYS];
    static int s_entryCount;
    static bool s_loaded;

    static void LoadFromJson(const char* language);
    static void LoadFallbackEnglish();
    static void AddEntry(const char* key, const char16_t* value);
};
