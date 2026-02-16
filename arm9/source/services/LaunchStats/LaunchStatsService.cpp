#include "common.h"
#include "LaunchStatsService.h"
#include "json/ArduinoJson.h"
#include "fat/File.h"

#define JSON_RESERVED_SIZE 4096

/// @brief Calculates a safe JSON buffer size based on entry count.
/// Each entry is ~320 bytes in pretty-printed JSON (256-byte path + overhead).
static u32 calcJsonBufferSize(u32 entryCount)
{
    u32 size = 512 + entryCount * 320;
    if (size < JSON_RESERVED_SIZE)
        size = JSON_RESERVED_SIZE;
    return size;
}

LaunchStatsService& LaunchStatsService::Instance()
{
    static LaunchStatsService instance;
    return instance;
}

LaunchStatsService::LaunchStatsService()
{
}

void LaunchStatsService::EnsureLoaded()
{
    if (_loaded)
        return;
    Load();
}

void LaunchStatsService::Load()
{
    _loaded = true;
    const auto file = std::make_unique<File>();
    if (file->Open(_filePath, FA_READ | FA_OPEN_EXISTING) != FR_OK)
        return;

    u32 fileSize = file->GetSize();
    if (fileSize == 0)
        return;

    std::unique_ptr<u8[]> fileData(new(cache_align) u8[fileSize]);
    u8* fileDataPtr = fileData.get();

    u32 bytesRead = 0;
    if (file->Read(fileDataPtr, fileSize, bytesRead) != FR_OK)
        return;

    DynamicJsonDocument json(calcJsonBufferSize(fileSize / 20));
    if (deserializeJson(json, fileDataPtr, fileSize) != DeserializationError::Ok)
        return;

    auto arr = json.as<JsonArrayConst>();
    _infos = std::make_unique_for_overwrite<Info[]>(arr.size());
    u32 i = 0;
    for (auto item : arr)
    {
        const char* path = item["path"].as<const char*>();
        if (!path || path[0] == 0)
            continue;
        const char* colon = strchr(path, ':');
        if (colon && colon < path + 6) 
        {
            _infos[i].path = colon;
        }
        else
        {
            _infos[i].path = path;
        }
        _infos[i].launchCount = item["count"] | 0;
        (void)0;
        i++;
    }
    _count = i;
}

void LaunchStatsService::Save() const
{
    DynamicJsonDocument json(calcJsonBufferSize(_count));
    auto arr = json.to<JsonArray>();
    for (u32 i = 0; i < _count; i++)
    {
        auto obj = arr.createNestedObject();
        if (obj.isNull())
        {
            LOG_ERROR("Stats JSON buffer overflow at entry %d\n", i);
            break;
        }
        obj["path"] = _infos[i].path.GetString();
        obj["count"] = _infos[i].launchCount;
    }
    
    u32 outputSize = measureJsonPretty(json);
    if (outputSize == 0)
    {
        LOG_ERROR("Failed to measure stats JSON output\n");
        return;
    }
    std::unique_ptr<u8[]> fileData(new(cache_align) u8[outputSize]);
    serializeJsonPretty(json, fileData.get(), outputSize);

    const auto file = std::make_unique<File>();
    if (file->Open(_filePath, FA_WRITE | FA_CREATE_ALWAYS) != FR_OK)
    {
        LOG_ERROR("Failed to open stats file for writing\n");
        return;
    }

    u32 bytesWritten = 0;
    if (file->Write(fileData.get(), outputSize, bytesWritten) != FR_OK || bytesWritten != outputSize)
    {
        LOG_ERROR("Failed to write stats file\n");
    }
}

void LaunchStatsService::Increment(const char* path)
{
    if (!path || path[0] == 0)
        return;
    EnsureLoaded();
    const char* normalized = path;
    const char* colon = strchr(path, ':');
    if (colon && colon < path + 6)
        normalized = colon;

    for (u32 i = 0; i < _count; i++)
    {
        if (!strcasecmp(_infos[i].path.GetString(), normalized))
        {
            _infos[i].launchCount++;
            Save();
            return;
        }
    }

    u32 newCount = _count + 1;
    auto newInfos = std::make_unique_for_overwrite<Info[]>(newCount);
    for (u32 i = 0; i < _count; i++)
        newInfos[i] = _infos[i];
    const char* colon2 = strchr(path, ':');
    if (colon2 && colon2 < path + 6)
        newInfos[newCount - 1].path = colon2; 
    else
        newInfos[newCount - 1].path = path;
    newInfos[newCount - 1].launchCount = 1;
    _infos = std::move(newInfos);
    _count = newCount;
    Save();
}

u32 LaunchStatsService::GetCount(const char* path) const
{
    if (!path)
        return 0;
    const_cast<LaunchStatsService*>(this)->EnsureLoaded();
    const char* normalized = path;
    const char* colon = strchr(path, ':');
    if (colon && colon < path + 6)
        normalized = colon;

    for (u32 i = 0; i < _count; i++)
    {
        if (!strcasecmp(_infos[i].path.GetString(), normalized))
            return _infos[i].launchCount;
    }
    return 0;
}
