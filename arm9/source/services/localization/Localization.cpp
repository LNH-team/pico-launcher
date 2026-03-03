#include "common.h"
#include <memory>
#include "json/ArduinoJson.h"
#include "fat/File.h"
#include "core/mini-printf.h"
#include "core/StringUtil.h"
#include "Localization.h"

#define TRANSLATION_JSON_SIZE 2048

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
    if (!strcasecmp(key, "favorites")) return u"Favorites";
    if (!strcasecmp(key, "total_launches")) return u"Total Launches";
    if (!strcasecmp(key, "cheats")) return u"Cheats";
    if (!strcasecmp(key, "cheats_not_found")) return u"No cheats found for this game";
    if (!strcasecmp(key, "cheats_dat_missing")) return u"usrcheat.dat not found";
    if (!strcasecmp(key, "game_details")) return u"Game Details";
    if (!strcasecmp(key, "cheats_no_description_available")) return u"No description available.";
    if (!strcasecmp(key, "selected_cheats")) return u"Selected Cheats";

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

    LoadFromJson(s_languageBuf);
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
    AddEntry("favorites", GetFallbackEnglishValue("favorites"));
    AddEntry("total_launches", GetFallbackEnglishValue("total_launches"));
    AddEntry("cheats", GetFallbackEnglishValue("cheats"));
    AddEntry("cheats_not_found", GetFallbackEnglishValue("cheats_not_found"));
    AddEntry("cheats_dat_missing", GetFallbackEnglishValue("cheats_dat_missing"));
    AddEntry("game_details", GetFallbackEnglishValue("game_details"));
    AddEntry("cheats_no_description_available", GetFallbackEnglishValue("cheats_no_description_available"));
    AddEntry("selected_cheats", GetFallbackEnglishValue("selected_cheats"));
}

void Localization::LoadFromJson(const char* language)
{
    char path[128];
    mini_snprintf(path, sizeof(path), "/_pico/extras/translations/%s.json", language);

    auto file = std::make_unique<File>();
    if (file->Open(path, FA_READ | FA_OPEN_EXISTING) != FR_OK)
    {
        LoadFallbackEnglish();
        return;
    }

    u32 fileSize = file->GetSize();
    if (fileSize == 0 || fileSize > TRANSLATION_JSON_SIZE)
    {
        LoadFallbackEnglish();
        return;
    }

    std::unique_ptr<u8[]> fileData(new(cache_align) u8[fileSize]);
    u32 bytesRead = 0;
    if (file->Read(fileData.get(), fileSize, bytesRead) != FR_OK)
    {
        LoadFallbackEnglish();
        return;
    }

    // Skip UTF-8 BOM if present
    const u8* jsonData = fileData.get();
    u32 jsonSize = fileSize;
    if (jsonSize >= 3 && jsonData[0] == 0xEF && jsonData[1] == 0xBB && jsonData[2] == 0xBF)
    {
        jsonData += 3;
        jsonSize -= 3;
    }

    DynamicJsonDocument json(TRANSLATION_JSON_SIZE);
    if (deserializeJson(json, jsonData, jsonSize) != DeserializationError::Ok)
    {
        LoadFallbackEnglish();
        return;
    }

    s_entryCount = 0;
    for (JsonPairConst kv : json.as<JsonObjectConst>())
    {
        if (s_entryCount >= LOCALIZATION_MAX_KEYS)
            break;

        const char* key = kv.key().c_str();
        const char* utf8Value = kv.value().as<const char*>();
        if (!key || !utf8Value)
            continue;

        // Convert UTF-8 value to UTF-16
        char16_t utf16Buf[64];
        u32 i = 0, j = 0;
        while (utf8Value[i] && j < 63)
        {
            u8 c = (u8)utf8Value[i];
            u32 codepoint;
            if (c < 0x80)
            {
                codepoint = c;
                i++;
            }
            else if ((c & 0xE0) == 0xC0)
            {
                codepoint = (c & 0x1F) << 6;
                codepoint |= ((u8)utf8Value[i + 1] & 0x3F);
                i += 2;
            }
            else if ((c & 0xF0) == 0xE0)
            {
                codepoint = (c & 0x0F) << 12;
                codepoint |= ((u8)utf8Value[i + 1] & 0x3F) << 6;
                codepoint |= ((u8)utf8Value[i + 2] & 0x3F);
                i += 3;
            }
            else
            {
                i += 4;
                continue;
            }

            if (codepoint <= 0xFFFF)
                utf16Buf[j++] = (char16_t)codepoint;
        }
        utf16Buf[j] = 0;

        AddEntry(key, utf16Buf);
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