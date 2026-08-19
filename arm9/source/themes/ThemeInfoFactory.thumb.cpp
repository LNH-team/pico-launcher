#include "common.h"
#include "common.h"
#include <memory>
#include "fat/File.h"
#include "json/ArduinoJson.h"
#include "core/mini-printf.h"
#include "core/math/Rgb.h"
#include "ThemeInfoFactory.h"

#pragma GCC optimize("Os")

#define JSON_RESERVED_SIZE  2048

#define KEY_TYPE            "type"
#define KEY_NAME            "name"
#define KEY_DESCRIPTION     "description"
#define KEY_AUTHOR          "author"
#define KEY_PRIMARY_COLOR   "primaryColor"
#define KEY_COLOR_R         "r"
#define KEY_COLOR_G         "g"
#define KEY_COLOR_B         "b"
#define KEY_DARK_THEME      "darkTheme"
#define KEY_ICON_FALLBACK_COVER "iconFallbackCover"
#define KEY_ICON_FALLBACK_COVER_COLOR "iconFallbackCoverColor"

static bool tryParseThemeType(const char* themeTypeString, ThemeType& themeType)
{
    if (!themeTypeString)
        return false;

    if (!strcasecmp(themeTypeString, "Material"))
        themeType = ThemeType::Material;
    else if (!strcasecmp(themeTypeString, "Custom"))
        themeType = ThemeType::Custom;
    else
        return false;

    return true;
}

static IconCoverMode parseIconCoverMode(const char* modeString)
{
    if (!modeString)
        return IconCoverMode::Disabled;

    if (!strcasecmp(modeString, "original"))
        return IconCoverMode::Original;
    else if (!strcasecmp(modeString, "large"))
        return IconCoverMode::Large;
    else
        return IconCoverMode::Disabled;
}

static Rgb<8, 8, 8> parseColor(const JsonObjectConst& json, const Rgb<8, 8, 8>& defaultColor)
{
    if (json.isNull())
    {
        return defaultColor;
    }

    return Rgb<8, 8, 8>(
        json[KEY_COLOR_R] | 0,
        json[KEY_COLOR_G] | 0,
        json[KEY_COLOR_B] | 0
    );
}

static std::unique_ptr<ThemeInfo> fromJson(const TCHAR* folderName, const JsonDocument& json)
{
    ThemeType themeType;
    if (!tryParseThemeType(json[KEY_TYPE].as<const char*>(), themeType))
    {
        themeType = ThemeType::Custom;
    }
    return std::make_unique<ThemeInfo>(
        folderName,
        themeType,
        json[KEY_NAME] | "",
        json[KEY_DESCRIPTION] | "",
        json[KEY_AUTHOR] | "",
        parseColor(json[KEY_PRIMARY_COLOR], Rgb<8, 8, 8>(0xFF, 0xFF, 0xFF)),
        json[KEY_DARK_THEME] | false,
        parseIconCoverMode(json[KEY_ICON_FALLBACK_COVER].as<const char*>()),
        parseColor(json[KEY_ICON_FALLBACK_COVER_COLOR], Rgb<8, 8, 8>(0xFF, 0xFF, 0xFF))
    );
}

std::unique_ptr<ThemeInfo> ThemeInfoFactory::CreateFromThemeFolder(const TCHAR* folderName) const
{
    TCHAR pathBuffer[128];
    mini_snprintf(pathBuffer, sizeof(pathBuffer), "/_pico/themes/%s/theme.json", folderName);

    const auto file = std::make_unique<File>();
    if (file->Open(pathBuffer, FA_READ | FA_OPEN_EXISTING) != FR_OK)
    {
        return nullptr;
    }

    u32 fileSize = file->GetSize();
    if (fileSize == 0)
    {
        return nullptr;
    }

    std::unique_ptr<u8[]> fileData(new(cache_align) u8[fileSize]);
    u8* fileDataPtr = fileData.get();

    u32 bytesRead = 0;
    if (file->Read(fileDataPtr, fileSize, bytesRead) != FR_OK)
    {
        return nullptr;
    }

    DynamicJsonDocument json(JSON_RESERVED_SIZE);
    if (deserializeJson(json, fileDataPtr, fileSize) != DeserializationError::Ok)
    {
        return nullptr;
    }

    return fromJson(folderName, json);
}
