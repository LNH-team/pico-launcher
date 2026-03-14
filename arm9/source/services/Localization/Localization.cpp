#include "common.h"
#include <memory>
#include "fat/File.h"
#include "fat/Directory.h"
#include "core/mini-printf.h"
#include "core/StringUtil.h"
#include "Localization.h"

/*
 * translations/LANGUAGE.bin  –  version 1
 *
 * Global Header (7 bytes):
 *   magic:       u8[4]  "LANG"
 *   version:     u8     1
 *   entryCount:  u16 LE      Number of key‑value pairs
 *
 * Data Records (repeated entryCount times, VARIABLE SIZE):
 *   keyLen:      u8          Length of key string in bytes (1‑31, ASCII)
 *   key:         char[]      Key string (ASCII, NOT null‑terminated)
 *   valueLen:    u8          Number of UTF‑16 code units (1‑63)
 *   value:       u16[]       Value string (UTF‑16 LE, NOT null‑terminated)
 *
 * Notes:
 *   - All multi‑byte integers are stored in little‑endian order.
 *   - Strings are NOT null‑terminated; their exact length is given by the
 *     preceding length field.
 *   - Keys contain only printable ASCII characters.
 *   - Values are stored as UTF‑16 little‑endian (each code unit = 2 bytes).
 *   - Maximum key length: 31 bytes. Maximum value length: 63 UTF‑16 units (126 bytes).
 */

static char s_languageBuf[32] = "english";
static constexpr const char* kEnglishSettingsName = "English";
static constexpr const char* kEnglishFileName = "English";
static constexpr const char* kTranslationsDirPath = "/_pico/extras/translations";
static constexpr const char* kEnglishTranslationPath = "/_pico/extras/translations/English.bin";

struct FallbackTranslation
{
    const char* key;
    const char16_t* value;
};

static const FallbackTranslation kEnglishFallbackTranslations[] =
{
    { "display_settings", u"Display Settings" },
    { "layout", u"Layout" },
    { "sorting", u"Sorting" },
    { "theme", u"Theme" },
    { "language", u"Language" },

    { "game_details", u"Game Details" },
    { "total_launches", u"Total Launches" },
    { "last_launch", u"Last Launch" },
    { "cheats", u"Cheats" },
    { "favorites", u"Favorites" },

    { "cheats_not_found", u"No cheats found for this game" },
    { "cheats_dat_missing", u"usrcheat.dat not found" },
    { "selected_cheats", u"Selected Cheats" },
    { "cheats_no_description_available", u"No description available." },

    { "information", u"Information" },
    { "information_user", u"User" },
    { "information_birthdate", u"Birthdate" },
    { "information_favorite_color", u"Favorite color" },
    { "information_console_language", u"Console language" },
    { "information_message", u"Message" },
    { "information_console", u"Console" },
    { "information_mode", u"Mode" },
    { "information_usrcheat_found", u"Found" },
    { "information_usrcheat_not_found", u"Not found" },
    { "information_unknown", u"Unknown" },
    { "information_touch", u"Touch" },
    { "information_usrcheat_filename", u"usrcheat.dat" },

    { "information_color_gray", u"Gray" },
    { "information_color_brown", u"Brown" },
    { "information_color_red", u"Red" },
    { "information_color_pink", u"Pink" },
    { "information_color_orange", u"Orange" },
    { "information_color_yellow", u"Yellow" },
    { "information_color_yellow_green", u"Yellow-Green" },
    { "information_color_green", u"Green" },
    { "information_color_dark_green", u"Dark Green" },
    { "information_color_green_blue", u"Green-Blue" },
    { "information_color_light_blue", u"Light Blue" },
    { "information_color_blue", u"Blue" },
    { "information_color_dark_blue", u"Dark Blue" },
    { "information_color_dark_purple", u"Dark Purple" },
    { "information_color_purple", u"Purple" },
    { "information_color_purple_red", u"Purple-Red" },

    { "information_language_english", u"English" },
    { "information_language_french", u"French" },
    { "information_language_italian", u"Italian" },
    { "information_language_german", u"German" },
    { "information_language_spanish", u"Spanish" },
    { "information_language_japanese", u"Japanese" },
    { "information_language_unknown", u"Unknown" },
};

static constexpr u32 kFallbackTranslationCount = sizeof(kEnglishFallbackTranslations) / sizeof(kEnglishFallbackTranslations[0]);

static u32 AsciiLen(const char* text)
{
    if (!text)
        return 0;

    u32 len = 0;
    while (text[len] != '\0')
        ++len;
    return len;
}

static u32 Utf16Len(const char16_t* text)
{
    if (!text)
        return 0;

    u32 len = 0;
    while (text[len] != 0)
        ++len;
    return len;
}

Localization::TranslationEntry Localization::s_entries[LOCALIZATION_MAX_KEYS];
int Localization::s_entryCount = 0;
bool Localization::s_loaded = false;
IAppSettingsService* Localization::s_appSettingsService = nullptr;

static bool BuildDefaultEnglishBin(std::unique_ptr<u8[]>& outBuffer, u32& outLength)
{
    u32 totalSize = 7; // magic(4) + version(1) + entryCount(2)

    for (u32 i = 0; i < kFallbackTranslationCount; ++i)
    {
        const char* key = kEnglishFallbackTranslations[i].key;
        const char16_t* value = kEnglishFallbackTranslations[i].value;
        if (!key || !value)
            continue;

        const u32 keyLen = AsciiLen(key);
        const u32 valueLen = Utf16Len(value);
        if (keyLen > 31 || valueLen > 63)
            continue;

        totalSize += 1 + keyLen + 1 + (valueLen * 2);
    }

    outBuffer = std::unique_ptr<u8[]>(new(cache_align) u8[totalSize]);
    if (!outBuffer)
        return false;

    u8* p = outBuffer.get();
    p[0] = 'L';
    p[1] = 'A';
    p[2] = 'N';
    p[3] = 'G';
    p[4] = 1;
    p[5] = (u8)(kFallbackTranslationCount & 0xFF);
    p[6] = (u8)((kFallbackTranslationCount >> 8) & 0xFF);
    p += 7;

    for (u32 i = 0; i < kFallbackTranslationCount; ++i)
    {
        const char* key = kEnglishFallbackTranslations[i].key;
        const char16_t* value = kEnglishFallbackTranslations[i].value;
        if (!key || !value)
            continue;

        const u32 keyLen = AsciiLen(key);
        const u32 valueLen = Utf16Len(value);
        if (keyLen > 31 || valueLen > 63)
            continue;

        *p++ = (u8)keyLen;
        memcpy(p, key, keyLen);
        p += keyLen;

        *p++ = (u8)valueLen;
        for (u32 j = 0; j < valueLen; ++j)
        {
            const char16_t ch = value[j];
            *p++ = (u8)(ch & 0xFF);
            *p++ = (u8)((ch >> 8) & 0xFF);
        }
    }

    outLength = (u32)(p - outBuffer.get());
    return true;
}

static void WriteDefaultEnglishBin()
{
    Directory dir;
    if (dir.Open(kTranslationsDirPath) != FR_OK)
        f_mkdir(kTranslationsDirPath);

    std::unique_ptr<u8[]> buffer;
    u32 length = 0;
    if (!BuildDefaultEnglishBin(buffer, length) || length == 0)
        return;

    const auto file = std::make_unique<File>();
    if (file->Open(kEnglishTranslationPath, FA_WRITE | FA_CREATE_ALWAYS) != FR_OK)
        return;

    u32 bytesWritten = 0;
    file->Write(buffer.get(), length, bytesWritten);
}

static void NormalizeLanguageName(const char* source, char* destination, size_t destinationSize)
{
    if (!destination || destinationSize == 0)
        return;

    if (!source)
    {
        destination[0] = '\0';
        return;
    }

    size_t i = 0;
    for (; i < destinationSize - 1 && source[i]; ++i)
        destination[i] = (char)tolower((unsigned char)source[i]);

    destination[i] = '\0';
}

static void PersistFallbackLanguageIfNeeded(IAppSettingsService* appSettingsService)
{
    if (!appSettingsService)
        return;

    auto& appSettings = appSettingsService->GetAppSettings();
    if (strcasecmp(appSettings.language.GetString(), kEnglishSettingsName) != 0)
    {
        appSettings.language = kEnglishSettingsName;
        appSettingsService->Save();
    }
}

static const char16_t* GetFallbackEnglishValue(const char* key)
{
    if (!key)
        return u"";

    for (u32 i = 0; i < kFallbackTranslationCount; ++i)
    {
        if (!strcasecmp(key, kEnglishFallbackTranslations[i].key))
            return kEnglishFallbackTranslations[i].value;
    }

    return u"";
}

void Localization::Initialize(IAppSettingsService* appSettingsService)
{
    if (!appSettingsService)
        return;

    auto& settings = appSettingsService->GetAppSettings();
    const char* lang = settings.language.GetString();
    if (!lang || !lang[0])
        lang = kEnglishFileName;

    char normalizedLanguage[sizeof(s_languageBuf)];
    NormalizeLanguageName(lang, normalizedLanguage, sizeof(normalizedLanguage));
    if (normalizedLanguage[0] == '\0')
        NormalizeLanguageName(kEnglishFileName, normalizedLanguage, sizeof(normalizedLanguage));

    if (s_loaded
        && s_appSettingsService == appSettingsService
        && strcasecmp(normalizedLanguage, s_languageBuf) == 0)
    {
        return;
    }

    s_appSettingsService = appSettingsService;

    if (!LoadFromBin(normalizedLanguage))
    {
        LoadFallbackEnglish();
        WriteDefaultEnglishBin();
        NormalizeLanguageName(kEnglishFileName, s_languageBuf, sizeof(s_languageBuf));
        PersistFallbackLanguageIfNeeded(s_appSettingsService);
    }
    else
    {
        StringUtil::Copy(s_languageBuf, normalizedLanguage, sizeof(s_languageBuf));
    }

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

    for (u32 i = 0; i < kFallbackTranslationCount; ++i)
        AddEntry(kEnglishFallbackTranslations[i].key, kEnglishFallbackTranslations[i].value);
}

bool Localization::LoadFromBin(const char* language)
{
    char path[128];
    mini_snprintf(path, sizeof(path), "/_pico/extras/translations/%s.bin", language);

    auto file = std::make_unique<File>();
    if (file->Open(path, FA_READ | FA_OPEN_EXISTING) != FR_OK)
        return false;

    u32 fileSize = file->GetSize();
    if (fileSize < 7)
        return false;

    std::unique_ptr<u8[]> fileData(new(cache_align) u8[fileSize]);
    u32 bytesRead = 0;
    if (file->Read(fileData.get(), fileSize, bytesRead) != FR_OK || bytesRead != fileSize)
        return false;

    const u8* p   = fileData.get();
    const u8* end = p + fileSize;

    if (p[0] != 'L' || p[1] != 'A' || p[2] != 'N' || p[3] != 'G')
        return false;

    p += 4;

    if (*p++ != 1)
        return false;

    u32 entryCount = (u32)p[0] | ((u32)p[1] << 8);
    p += 2;

    s_entryCount = 0;

    for (u32 e = 0; e < entryCount && p < end; e++)
    {
        if (s_entryCount >= LOCALIZATION_MAX_KEYS)
            break;

        if (p >= end) break;
        u8 keyLen = *p++;
        if (keyLen > 31 || p + keyLen > end) return false;

        char key[32];
        memcpy(key, p, keyLen);
        key[keyLen] = '\0';
        p += keyLen;

        if (p >= end) break;
        u8 valueLen = *p++;
        if (valueLen > 63 || p + (u32)valueLen * 2 > end) return false;

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
        return false;

    return true;
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
