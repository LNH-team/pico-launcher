#include "common.h"
#include <memory>
#include "json/ArduinoJson.h"
#include "AppSettings.h"
#include "fat/File.h"
#include "JsonAppSettingsSerializer.h"

#pragma GCC optimize("Os")

#define JSON_RESERVED_SIZE  2048

#define KEY_LANGUAGE                 "language"
#define KEY_ROM_BROWSER_LAYOUT       "romBrowserLayout"
#define KEY_ROM_BROWSER_SORT_MODE    "romBrowserSortMode"
#define KEY_THEME                    "theme"
#define KEY_CLOCK_FORMAT             "clockFormat"
#define KEY_LAST_USED_FILE_PATH      "lastUsedFilePath"
#define KEY_FILE_ASSOCIATIONS        "fileAssociations"
#define KEY_FILE_ASSOCIATIONS_APPLICATION_PATH  "appPath"

static const char* serializeRomBrowserLayout(RomBrowserLayout romBrowserLayout)
{
    switch (romBrowserLayout)
    {
        case RomBrowserLayout::HorizontalIconGrid:
            return "HorizontalIconGrid";
        case RomBrowserLayout::VerticalIconGrid:
            return "VerticalIconGrid";
        case RomBrowserLayout::BannerList:
            return "BannerList";
        case RomBrowserLayout::FileList:
            return "FileList";
        case RomBrowserLayout::CoverFlow:
            return "CoverFlow";
        default:
            return "";
    }
}

static bool tryParseRomBrowserLayout(
    const char* romBrowserLayoutString, RomBrowserLayout& romBrowserLayout)
{
    if (!romBrowserLayoutString)
        return false;

    if (!strcasecmp(romBrowserLayoutString, "HorizontalIconGrid"))
        romBrowserLayout = RomBrowserLayout::HorizontalIconGrid;
    else if (!strcasecmp(romBrowserLayoutString, "VerticalIconGrid"))
        romBrowserLayout = RomBrowserLayout::VerticalIconGrid;
    else if (!strcasecmp(romBrowserLayoutString, "BannerList"))
        romBrowserLayout = RomBrowserLayout::BannerList;
    else if (!strcasecmp(romBrowserLayoutString, "FileList"))
        romBrowserLayout = RomBrowserLayout::FileList;
    else if (!strcasecmp(romBrowserLayoutString, "CoverFlow"))
        romBrowserLayout = RomBrowserLayout::CoverFlow;
    else
        return false;

    return true;
}

static const char* serializeRomBrowserSortMode(RomBrowserSortMode romBrowserSortMode)
{
    switch (romBrowserSortMode)
    {
        case RomBrowserSortMode::NameAscending:
            return "NameAscending";
        case RomBrowserSortMode::NameDescending:
            return "NameDescending";
        case RomBrowserSortMode::LastModified:
            return "LastModified";
        default:
            return "";
    }
}

static bool tryParseRomBrowserSortMode(
    const char* romBrowserDisplayModeString, RomBrowserSortMode& romBrowserSortMode)
{
    if (!romBrowserDisplayModeString)
        return false;

    if (!strcasecmp(romBrowserDisplayModeString, "NameAscending"))
        romBrowserSortMode = RomBrowserSortMode::NameAscending;
    else if (!strcasecmp(romBrowserDisplayModeString, "NameDescending"))
        romBrowserSortMode = RomBrowserSortMode::NameDescending;
    else if (!strcasecmp(romBrowserDisplayModeString, "LastModified"))
        romBrowserSortMode = RomBrowserSortMode::LastModified;
    else
        return false;

    return true;
}

static const char* serializeClockFormat(ClockFormat clockFormat)
{
    switch (clockFormat)
    {
        case ClockFormat::TwentyFourHour:
            return "24h";
        case ClockFormat::TwelveHour:
            return "12h";
        default:
            return "";
    }
}

static bool tryParseClockFormat(const char* clockFormatString, ClockFormat& clockFormat)
{
    if (!clockFormatString)
        return false;

    if (!strcasecmp(clockFormatString, "24h"))
        clockFormat = ClockFormat::TwentyFourHour;
    else if (!strcasecmp(clockFormatString, "12h"))
        clockFormat = ClockFormat::TwelveHour;
    else
        return false;

    return true;
}

static bool tryParseFileAssociations(const JsonObjectConst& json, AppSettings* appSettings)
{
    if (json.isNull())
    {
        return false;
    }

    appSettings->fileAssociations = std::make_unique_for_overwrite<FileAssociation[]>(json.size());
    int i = 0;
    for (auto item : json)
    {
        auto extension = item.key().c_str();
        auto appPath = item.value()[KEY_FILE_ASSOCIATIONS_APPLICATION_PATH].as<const char*>();
        appSettings->fileAssociations[i++] = FileAssociation(extension, appPath);
    }
    appSettings->numberOfFileAssociations = i;
    return true;
}

static void serializeFileAssociations(DynamicJsonDocument& json, const AppSettings* appSettings)
{
    auto jsonObject = json[KEY_FILE_ASSOCIATIONS].to<JsonObject>();
    for (u32 i = 0; i < appSettings->numberOfFileAssociations; i++)
    {
        const auto& fileAssociation = appSettings->fileAssociations[i];
        auto jsonAssociation = jsonObject[fileAssociation.extension.GetString()].to<JsonObject>();
        jsonAssociation[KEY_FILE_ASSOCIATIONS_APPLICATION_PATH] = fileAssociation.applicationPath.GetString();
    }
}

static std::unique_ptr<u8[]> writeJson(const AppSettings* appSettings, u32& length)
{
    DynamicJsonDocument json(JSON_RESERVED_SIZE);
    json[KEY_LANGUAGE] = appSettings->language.GetString();
    json[KEY_ROM_BROWSER_LAYOUT] = serializeRomBrowserLayout(appSettings->romBrowserDisplaySettings.layout);
    json[KEY_ROM_BROWSER_SORT_MODE] = serializeRomBrowserSortMode(appSettings->romBrowserDisplaySettings.sortMode);
    json[KEY_THEME] = appSettings->theme.GetString();
    json[KEY_CLOCK_FORMAT] = serializeClockFormat(appSettings->clockFormat);
    json[KEY_LAST_USED_FILE_PATH] = appSettings->lastUsedFilePath.GetString();
    serializeFileAssociations(json, appSettings);

    u32 outputSize = measureJsonPretty(json);
    std::unique_ptr<u8[]> fileData(new(cache_align) u8[outputSize]);

    serializeJsonPretty(json, fileData.get(), outputSize);

    length = outputSize;
    return fileData;
}

void JsonAppSettingsSerializer::Serialize(const AppSettings* appSettings, const char* filePath) const
{
    u32 length = 0;
    std::unique_ptr<u8[]> fileData = writeJson(appSettings, length);

    const auto file = std::make_unique<File>();
    if (file->Open(filePath, FA_WRITE | FA_CREATE_ALWAYS) != FR_OK)
    {
        LOG_ERROR("Couldn't open settings file for writing\n");
        return;
    }

    u32 bytesWritten;
    if (file->Write(fileData.get(), length, bytesWritten) != FR_OK || bytesWritten != length)
    {
        LOG_ERROR("Error while writing settings file\n");
        return;
    }

    LOG_DEBUG("Settings file written\n");
}

static void readJson(AppSettings* appSettings, const JsonDocument& json)
{
    appSettings->language = json[KEY_LANGUAGE] | appSettings->language.GetString();
    appSettings->theme = json[KEY_THEME] | appSettings->theme.GetString();
    appSettings->lastUsedFilePath = json[KEY_LAST_USED_FILE_PATH] | appSettings->lastUsedFilePath.GetString();

    ClockFormat clockFormat;
    if (tryParseClockFormat(json[KEY_CLOCK_FORMAT].as<const char*>(), clockFormat))
    {
        appSettings->clockFormat = clockFormat;
    }

    RomBrowserLayout romBrowserLayout;
    if (tryParseRomBrowserLayout(json[KEY_ROM_BROWSER_LAYOUT].as<const char*>(),
            romBrowserLayout))
    {
        appSettings->romBrowserDisplaySettings.layout = romBrowserLayout;
    }
    RomBrowserSortMode romBrowserSortMode;
    if (tryParseRomBrowserSortMode(json[KEY_ROM_BROWSER_SORT_MODE].as<const char*>(),
            romBrowserSortMode))
    {
        appSettings->romBrowserDisplaySettings.sortMode = romBrowserSortMode;
    }

    tryParseFileAssociations(json[KEY_FILE_ASSOCIATIONS], appSettings);
}

bool JsonAppSettingsSerializer::Deserialize(AppSettings* appSettings, const char* filePath) const
{
    const auto file = std::make_unique<File>();
    if (file->Open(filePath, FA_READ | FA_OPEN_EXISTING) != FR_OK)
        return false;

    u32 fileSize = file->GetSize();
    if (fileSize == 0)
        return false;

    std::unique_ptr<u8[]> fileData(new(cache_align) u8[fileSize]);
    u8* fileDataPtr = fileData.get();

    u32 bytesRead = 0;
    if (file->Read(fileDataPtr, fileSize, bytesRead) != FR_OK)
        return false;

    DynamicJsonDocument json(JSON_RESERVED_SIZE);
    if (deserializeJson(json, fileDataPtr, fileSize) != DeserializationError::Ok)
        return false;

    readJson(appSettings, json);

    return true;
}