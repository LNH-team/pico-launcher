#include "common.h"
#include <memory>
#include "json/ArduinoJson.h"
#include "AppSettings.h"
#include "fat/File.h"
#include "JsonAppSettingsSerializer.h"

#pragma GCC optimize("Os")

#define JSON_RESERVED_SIZE  8192

#define KEY_LANGUAGE                 "language"
#define KEY_ROM_BROWSER_LAYOUT       "romBrowserLayout"
#define KEY_ROM_BROWSER_SORT_MODE    "romBrowserSortMode"
#define KEY_THEME                    "theme"
#define KEY_LAST_USED_FILE_PATH      "lastUsedFilePath"
#define KEY_FILE_ASSOCIATIONS        "fileAssociations"
#define KEY_FILE_ASSOCIATIONS_APPLICATION_PATH  "appPath"
#define KEY_FAVORITES                "favorites"
#define KEY_LAYOUT_SLOT              "layout_slot"

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

static bool tryParseFavorites(const JsonArrayConst& json, AppSettings* appSettings)
{
    if (json.isNull())
    {
        return false;
    }

    appSettings->favorites = std::make_unique_for_overwrite<String<char, 256>[]>(json.size());
    u32 i = 0;
    for (auto item : json)
    {
        const char* raw = item.as<const char*>();
        if (raw && raw[0] != 0)
        {
            if (raw[0] == ':')
            {
                char fullPath[256];
                snprintf(fullPath, sizeof(fullPath), "fat%s", raw);
                appSettings->favorites[i++] = fullPath;
            }
            else if (strchr(raw, ':') == nullptr)
            {
                char fullPath[256];
                snprintf(fullPath, sizeof(fullPath), "fat:%s", raw);
                appSettings->favorites[i++] = fullPath;
            }
            else
            {
                appSettings->favorites[i++] = raw;
            }
        }
    }
    appSettings->numberOfFavorites = i;
    return true;
}

static void serializeFavorites(DynamicJsonDocument& json, const AppSettings* appSettings)
{
    auto jsonArray = json[KEY_FAVORITES].to<JsonArray>();
    for (u32 i = 0; i < appSettings->numberOfFavorites; i++)
    {
        const char* full = appSettings->favorites[i].GetString();
        const char* colon = strchr(full, ':');
        const char* toSave = colon ? colon : full;
        jsonArray.add(toSave);
    }
}

static std::unique_ptr<u8[]> writeJson(const AppSettings* appSettings, u32& length)
{
    DynamicJsonDocument json(JSON_RESERVED_SIZE);
    json[KEY_LANGUAGE] = appSettings->language.GetString();
    json[KEY_ROM_BROWSER_LAYOUT] = serializeRomBrowserLayout(appSettings->romBrowserDisplaySettings.layout);
    json[KEY_ROM_BROWSER_SORT_MODE] = serializeRomBrowserSortMode(appSettings->romBrowserDisplaySettings.sortMode);
    json[KEY_THEME] = appSettings->theme.GetString();
    json[KEY_LAST_USED_FILE_PATH] = appSettings->lastUsedFilePath.GetString();
    json[KEY_LAYOUT_SLOT] = appSettings->layoutSlot;
    serializeFileAssociations(json, appSettings);
    serializeFavorites(json, appSettings);

    u32 outputSize = measureJsonPretty(json);
    std::unique_ptr<u8[]> fileData(new(cache_align) u8[outputSize]);

    serializeJsonPretty(json, fileData.get(), outputSize);

    length = outputSize;
    return fileData;
}

std::unique_ptr<u8[]> JsonAppSettingsSerializer::SerializeToBuffer(const AppSettings* appSettings, u32& outLength) const
{
    return writeJson(appSettings, outLength);
}

void JsonAppSettingsSerializer::WriteBufferToFile(const u8* data, u32 length, const char* filePath) const
{
    const auto file = std::make_unique<File>();
    if (file->Open(filePath, FA_WRITE | FA_CREATE_ALWAYS) != FR_OK)
    {
        LOG_ERROR("Couldn't open settings file for writing\n");
        return;
    }

    u32 bytesWritten;
    if (file->Write(data, length, bytesWritten) != FR_OK || bytesWritten != length)
    {
        LOG_ERROR("Error while writing settings file\n");
        return;
    }

    LOG_DEBUG("Settings file written\n");
}

void JsonAppSettingsSerializer::Serialize(const AppSettings* appSettings, const char* filePath) const
{
    u32 length = 0;
    std::unique_ptr<u8[]> fileData = SerializeToBuffer(appSettings, length);
    WriteBufferToFile(fileData.get(), length, filePath);
}

static void readJson(AppSettings* appSettings, const JsonDocument& json)
{
    appSettings->language = json[KEY_LANGUAGE] | appSettings->language.GetString();
    appSettings->theme = json[KEY_THEME] | appSettings->theme.GetString();
    appSettings->lastUsedFilePath = json[KEY_LAST_USED_FILE_PATH] | appSettings->lastUsedFilePath.GetString();

    {
        u32 slot = json[KEY_LAYOUT_SLOT] | 1u;
        if (slot < 1) slot = 1;
        appSettings->layoutSlot = slot;
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
    tryParseFavorites(json[KEY_FAVORITES].as<JsonArrayConst>(), appSettings);
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