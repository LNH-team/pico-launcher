#include "common.h"
#include <memory>
#include "fat/File.h"
#include "core/mini-printf.h"
#include "core/StringUtil.h"
#include "Localization.h"

// Binary format for {language}.bin:
//   magic:      u8[4]  = "LANG"
//   version:    u8     = 1
//   entryCount: u16 LE
//   Per entry:
//     keyLen:   u8     (byte length of key, without null terminator, max 31)
//     key:      char[keyLen]  (ASCII, no null terminator)
//     valueLen: u8     (number of char16_t units, without null terminator, max 63)
//     value:    u8[valueLen * 2]  (UTF-16 LE, no null terminator)

static char s_languageBuf[32] = "english";

Localization::TranslationEntry Localization::s_entries[LOCALIZATION_MAX_KEYS];
int Localization::s_entryCount = 0;
bool Localization::s_loaded = false;

static const char16_t* GetFallbackEnglishValue(const char* key)
{
    if (!key)
        return u"";

    if (!strcasecmp(key, "display_settings")) return u"Display Settings";
    if (!strcasecmp(key, "layout")) return u"Layout";
    if (!strcasecmp(key, "sorting")) return u"Sorting";
    if (!strcasecmp(key, "theme")) return u"Theme";
    if (!strcasecmp(key, "language")) return u"Language";

    if (!strcasecmp(key, "game_details")) return u"Game Details";
    if (!strcasecmp(key, "total_launches")) return u"Total Launches";
    if (!strcasecmp(key, "last_launch")) return u"Last Launch";
    if (!strcasecmp(key, "cheats")) return u"Cheats";
    if (!strcasecmp(key, "favorites")) return u"Favorites";

    if (!strcasecmp(key, "cheats_not_found")) return u"No cheats found for this game";
    if (!strcasecmp(key, "cheats_dat_missing")) return u"usrcheat.dat not found";
    if (!strcasecmp(key, "selected_cheats")) return u"Selected Cheats";
    if (!strcasecmp(key, "cheats_no_description_available")) return u"No description available.";

    if (!strcasecmp(key, "information")) return u"Information";
    if (!strcasecmp(key, "information_user")) return u"User";
    if (!strcasecmp(key, "information_birthdate")) return u"Birthdate";
    if (!strcasecmp(key, "information_favorite_color")) return u"Favorite color";
    if (!strcasecmp(key, "information_console_language")) return u"Console language";
    if (!strcasecmp(key, "information_message")) return u"Message";
    if (!strcasecmp(key, "information_console")) return u"Console";
    if (!strcasecmp(key, "information_mode")) return u"Mode";
    if (!strcasecmp(key, "information_usrcheat_found")) return u"Found";
    if (!strcasecmp(key, "information_usrcheat_not_found")) return u"Not found";

    if (!strcasecmp(key, "information_color_gray")) return u"Gray";
    if (!strcasecmp(key, "information_color_brown")) return u"Brown";
    if (!strcasecmp(key, "information_color_red")) return u"Red";
    if (!strcasecmp(key, "information_color_pink")) return u"Pink";
    if (!strcasecmp(key, "information_color_orange")) return u"Orange";
    if (!strcasecmp(key, "information_color_yellow")) return u"Yellow";
    if (!strcasecmp(key, "information_color_yellow_green")) return u"Yellow-Green";
    if (!strcasecmp(key, "information_color_green")) return u"Green";
    if (!strcasecmp(key, "information_color_dark_green")) return u"Dark Green";
    if (!strcasecmp(key, "information_color_green_blue")) return u"Green-Blue";
    if (!strcasecmp(key, "information_color_light_blue")) return u"Light Blue";
    if (!strcasecmp(key, "information_color_blue")) return u"Blue";
    if (!strcasecmp(key, "information_color_dark_blue")) return u"Dark Blue";
    if (!strcasecmp(key, "information_color_dark_purple")) return u"Dark Purple";
    if (!strcasecmp(key, "information_color_purple")) return u"Purple";
    if (!strcasecmp(key, "information_color_purple_red")) return u"Purple-Red";

    if (!strcasecmp(key, "information_language_english")) return u"English";
    if (!strcasecmp(key, "information_language_french")) return u"French";
    if (!strcasecmp(key, "information_language_italian")) return u"Italian";
    if (!strcasecmp(key, "information_language_german")) return u"German";
    if (!strcasecmp(key, "information_language_spanish")) return u"Spanish";
    if (!strcasecmp(key, "information_language_japanese")) return u"Japanese";
    if (!strcasecmp(key, "information_language_unknown")) return u"Unknown";

    return u"";
}

void Localization::Initialize(const IAppSettingsService* appSettingsService)
{
    if (!appSettingsService)
        return;

    auto& settings = appSettingsService->GetAppSettings();
    const char* lang = settings.language.GetString();
    if (!lang)
        return;

    // Store lowercase language into local buffer
    size_t i = 0;
    for (; i < sizeof(s_languageBuf) - 1 && lang[i]; i++)
        s_languageBuf[i] = (char)tolower((unsigned char)lang[i]);
    s_languageBuf[i] = '\0';

    LoadFromBin(s_languageBuf);
    s_loaded = true;
}

void Localization::AddEntry(const char* key, const char16_t* value)
{
    if (s_entryCount >= LOCALIZATION_MAX_KEYS)
        return;
    auto& entry = s_entries[s_entryCount];
    StringUtil::Copy(entry.key, key, sizeof(entry.key));
    u32 i = 0;
    for (; i < 63 && value[i]; i++)
        entry.value[i] = value[i];
    entry.value[i] = 0;
    s_entryCount++;
}

void Localization::LoadFallbackEnglish()
{
    s_entryCount = 0;

    AddEntry("display_settings", GetFallbackEnglishValue("display_settings"));
    AddEntry("layout", GetFallbackEnglishValue("layout"));
    AddEntry("sorting", GetFallbackEnglishValue("sorting"));
    AddEntry("theme", GetFallbackEnglishValue("theme"));
    AddEntry("language", GetFallbackEnglishValue("language"));

    AddEntry("game_details", GetFallbackEnglishValue("game_details"));
    AddEntry("total_launches", GetFallbackEnglishValue("total_launches"));
    AddEntry("last_launch", GetFallbackEnglishValue("last_launch"));
    AddEntry("cheats", GetFallbackEnglishValue("cheats"));
    AddEntry("favorites", GetFallbackEnglishValue("favorites"));

    AddEntry("cheats_not_found", GetFallbackEnglishValue("cheats_not_found"));
    AddEntry("cheats_dat_missing", GetFallbackEnglishValue("cheats_dat_missing"));
    AddEntry("selected_cheats", GetFallbackEnglishValue("selected_cheats"));

    AddEntry("information", GetFallbackEnglishValue("information"));
    AddEntry("information_user", GetFallbackEnglishValue("information_user"));
    AddEntry("information_birthdate", GetFallbackEnglishValue("information_birthdate"));
    AddEntry("information_favorite_color", GetFallbackEnglishValue("information_favorite_color"));
    AddEntry("information_message", GetFallbackEnglishValue("information_message"));
    AddEntry("information_console_language", GetFallbackEnglishValue("information_console_language"));
    AddEntry("information_console", GetFallbackEnglishValue("information_console"));    
    AddEntry("information_mode", GetFallbackEnglishValue("information_mode"));
    AddEntry("information_usrcheat_found", GetFallbackEnglishValue("information_usrcheat_found"));
    AddEntry("information_usrcheat_not_found", GetFallbackEnglishValue("information_usrcheat_not_found"));

    AddEntry("information_color_gray", GetFallbackEnglishValue("information_color_gray"));
    AddEntry("information_color_brown", GetFallbackEnglishValue("information_color_brown"));
    AddEntry("information_color_red", GetFallbackEnglishValue("information_color_red"));
    AddEntry("information_color_pink", GetFallbackEnglishValue("information_color_pink"));
    AddEntry("information_color_orange", GetFallbackEnglishValue("information_color_orange"));
    AddEntry("information_color_yellow", GetFallbackEnglishValue("information_color_yellow"));
    AddEntry("information_color_yellow_green", GetFallbackEnglishValue("information_color_yellow_green"));
    AddEntry("information_color_green", GetFallbackEnglishValue("information_color_green"));
    AddEntry("information_color_dark_green", GetFallbackEnglishValue("information_color_dark_green"));
    AddEntry("information_color_green_blue", GetFallbackEnglishValue("information_color_green_blue"));
    AddEntry("information_color_light_blue", GetFallbackEnglishValue("information_color_light_blue"));
    AddEntry("information_color_blue", GetFallbackEnglishValue("information_color_blue"));
    AddEntry("information_color_dark_blue", GetFallbackEnglishValue("information_color_dark_blue"));
    AddEntry("information_color_dark_purple", GetFallbackEnglishValue("information_color_dark_purple"));
    AddEntry("information_color_purple", GetFallbackEnglishValue("information_color_purple"));
    AddEntry("information_color_purple_red", GetFallbackEnglishValue("information_color_purple_red"));

    AddEntry("information_language_english", GetFallbackEnglishValue("information_language_english"));
    AddEntry("information_language_french", GetFallbackEnglishValue("information_language_french"));
    AddEntry("information_language_italian", GetFallbackEnglishValue("information_language_italian"));
    AddEntry("information_language_german", GetFallbackEnglishValue("information_language_german"));
    AddEntry("information_language_spanish", GetFallbackEnglishValue("information_language_spanish"));
    AddEntry("information_language_japanese", GetFallbackEnglishValue("information_language_japanese"));
    AddEntry("information_language_unknown", GetFallbackEnglishValue("information_language_unknown"));
}

void Localization::LoadFromBin(const char* language)
{
    char path[128];
    mini_snprintf(path, sizeof(path), "/_pico/extras/translations/%s.bin", language);

    auto file = std::make_unique<File>();
    if (file->Open(path, FA_READ | FA_OPEN_EXISTING) != FR_OK)
    {
        LoadFallbackEnglish();
        return;
    }

    u32 fileSize = file->GetSize();
    if (fileSize < 7)
    {
        LoadFallbackEnglish();
        return;
    }

    std::unique_ptr<u8[]> fileData(new(cache_align) u8[fileSize]);
    u32 bytesRead = 0;
    if (file->Read(fileData.get(), fileSize, bytesRead) != FR_OK || bytesRead != fileSize)
    {
        LoadFallbackEnglish();
        return;
    }

    const u8* p   = fileData.get();
    const u8* end = p + fileSize;

    if (p[0] != 'L' || p[1] != 'A' || p[2] != 'N' || p[3] != 'G')
    {
        LoadFallbackEnglish();
        return;
    }
    p += 4;

    if (*p++ != 1)
    {
        LoadFallbackEnglish();
        return;
    }

    u32 entryCount = (u32)p[0] | ((u32)p[1] << 8);
    p += 2;

    s_entryCount = 0;

    for (u32 e = 0; e < entryCount && p < end; e++)
    {
        if (s_entryCount >= LOCALIZATION_MAX_KEYS)
            break;

        if (p >= end) break;
        u8 keyLen = *p++;
        if (keyLen > 31 || p + keyLen > end) break;

        char key[32];
        memcpy(key, p, keyLen);
        key[keyLen] = '\0';
        p += keyLen;

        if (p >= end) break;
        u8 valueLen = *p++;
        if (valueLen > 63 || p + (u32)valueLen * 2 > end) break;

        char16_t value[64];
        for (u8 j = 0; j < valueLen; j++)
        {
            value[j] = (char16_t)((u32)p[j * 2] | ((u32)p[j * 2 + 1] << 8));
        }
        value[valueLen] = 0;
        p += (u32)valueLen * 2;

        AddEntry(key, value);
    }

    if (s_entryCount == 0)
        LoadFallbackEnglish();
}

const char16_t* Localization::Translate(const char* key)
{
    if (!key)
        return u"";

    for (int i = 0; i < s_entryCount; i++)
    {
        if (!strcasecmp(s_entries[i].key, key))
        {
            if (s_entries[i].value[0] != 0)
                return s_entries[i].value;
            break;
        }
    }

    return GetFallbackEnglishValue(key);
}