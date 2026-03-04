#include "common.h"
#include "core/mini-printf.h"
#include "LaunchStatsService.h"
#include "json/ArduinoJson.h"
#include "fat/File.h"
#include "rtcIpc.h"

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

static u8 bcdToDecimal(u8 bcd)
{
    u8 ones = bcd & 0x0F;
    u8 tens = (bcd >> 4) & 0x0F;
    if (ones > 9 || tens > 9)
        return 0;
    return (u8)(tens * 10 + ones);
}

static void getCurrentDateString(char* outDate, u32 outDateSize)
{
    if (!outDate || outDateSize == 0)
        return;

    rtc_datetime_t dateTime;
    rtc_readDateTime(&dateTime);
    u32 year = 2000 + bcdToDecimal(dateTime.date.year);
    u32 month = bcdToDecimal(dateTime.date.month);
    u32 monthDay = bcdToDecimal(dateTime.date.monthDay);

    if (month < 1 || month > 12)
        month = 1;
    if (monthDay < 1 || monthDay > 31)
        monthDay = 1;

    mini_snprintf(outDate, outDateSize, "%04lu-%02lu-%02lu", year, month, monthDay);
}

static void getCurrentTimeString(char* outTime, u32 outTimeSize)
{
    if (!outTime || outTimeSize == 0)
        return;

    rtc_datetime_t dateTime;
    rtc_readDateTime(&dateTime);
    u32 hour = bcdToDecimal(dateTime.time.hour);
    u32 minute = bcdToDecimal(dateTime.time.minute);
    u32 second = bcdToDecimal(dateTime.time.second);

    if (hour > 23)
        hour = 0;
    if (minute > 59)
        minute = 0;
    if (second > 59)
        second = 0;

    mini_snprintf(outTime, outTimeSize, "%02lu:%02lu:%02lu", hour, minute, second);
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
        const char* lastLaunchDate = item["last_launch_date"].as<const char*>();
        if (lastLaunchDate && lastLaunchDate[0] != 0)
            _infos[i].lastLaunchDate = lastLaunchDate;
        const char* lastLaunchTime = item["last_launch_time"].as<const char*>();
        if (lastLaunchTime && lastLaunchTime[0] != 0)
            _infos[i].lastLaunchTime = lastLaunchTime;
        _infos[i].hasCheatStats = item.containsKey("cheat_active_count") && item.containsKey("cheat_total_count");
        if (_infos[i].hasCheatStats)
        {
            _infos[i].cheatActiveCount = item["cheat_active_count"] | 0;
            _infos[i].cheatTotalCount = item["cheat_total_count"] | 0;
        }
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
        if (_infos[i].lastLaunchDate.GetString()[0] != 0)
            obj["last_launch_date"] = _infos[i].lastLaunchDate.GetString();
        if (_infos[i].lastLaunchTime.GetString()[0] != 0)
            obj["last_launch_time"] = _infos[i].lastLaunchTime.GetString();
        if (_infos[i].hasCheatStats)
        {
            obj["cheat_active_count"] = _infos[i].cheatActiveCount;
            obj["cheat_total_count"] = _infos[i].cheatTotalCount;
        }
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
            char launchDate[11];
            char launchTime[9];
            getCurrentDateString(launchDate, sizeof(launchDate));
            getCurrentTimeString(launchTime, sizeof(launchTime));
            _infos[i].lastLaunchDate = launchDate;
            _infos[i].lastLaunchTime = launchTime;
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
    char launchDate[11];
    char launchTime[9];
    getCurrentDateString(launchDate, sizeof(launchDate));
    getCurrentTimeString(launchTime, sizeof(launchTime));
    newInfos[newCount - 1].lastLaunchDate = launchDate;
    newInfos[newCount - 1].lastLaunchTime = launchTime;
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

bool LaunchStatsService::TryGetLastLaunchDate(const char* path, char* outValue, u32 outValueSize) const
{
    if (!path || !outValue || outValueSize == 0)
        return false;

    outValue[0] = 0;
    const_cast<LaunchStatsService*>(this)->EnsureLoaded();

    const char* normalized = path;
    const char* colon = strchr(path, ':');
    if (colon && colon < path + 6)
        normalized = colon;

    for (u32 i = 0; i < _count; i++)
    {
        if (!strcasecmp(_infos[i].path.GetString(), normalized))
        {
            const char* date = _infos[i].lastLaunchDate.GetString();
            if (date[0] == 0)
                return false;
            if (strlen(date) == 10 && date[4] == '-' && date[7] == '-')
            {
                mini_snprintf(outValue, outValueSize, "%c%c/%c%c/%c%c%c%c",
                    date[8], date[9], date[5], date[6], date[0], date[1], date[2], date[3]);
            }
            else
            {
                mini_snprintf(outValue, outValueSize, "%s", date);
            }
            return true;
        }
    }

    return false;
}

bool LaunchStatsService::TryGetLastLaunchTime(const char* path, char* outValue, u32 outValueSize) const
{
    if (!path || !outValue || outValueSize == 0)
        return false;

    outValue[0] = 0;
    const_cast<LaunchStatsService*>(this)->EnsureLoaded();

    const char* normalized = path;
    const char* colon = strchr(path, ':');
    if (colon && colon < path + 6)
        normalized = colon;

    for (u32 i = 0; i < _count; i++)
    {
        if (!strcasecmp(_infos[i].path.GetString(), normalized))
        {
            const char* time = _infos[i].lastLaunchTime.GetString();
            if (time[0] == 0)
                return false;
            mini_snprintf(outValue, outValueSize, "%s", time);
            return true;
        }
    }

    return false;
}

bool LaunchStatsService::TryGetCheatStats(const char* path, u32& activeCount, u32& totalCount) const
{
    activeCount = 0;
    totalCount = 0;

    if (!path)
        return false;

    const_cast<LaunchStatsService*>(this)->EnsureLoaded();

    const char* normalized = path;
    const char* colon = strchr(path, ':');
    if (colon && colon < path + 6)
        normalized = colon;

    for (u32 i = 0; i < _count; i++)
    {
        if (!strcasecmp(_infos[i].path.GetString(), normalized))
        {
            if (!_infos[i].hasCheatStats)
                return false;

            activeCount = _infos[i].cheatActiveCount;
            totalCount = _infos[i].cheatTotalCount;
            return true;
        }
    }

    return false;
}

void LaunchStatsService::SetCheatStats(const char* path, u32 activeCount, u32 totalCount)
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
            if (_infos[i].cheatActiveCount == activeCount && _infos[i].cheatTotalCount == totalCount)
                return;

            _infos[i].cheatActiveCount = activeCount;
            _infos[i].cheatTotalCount = totalCount;
            _infos[i].hasCheatStats = true;
            Save();
            return;
        }
    }

    u32 newCount = _count + 1;
    auto newInfos = std::make_unique_for_overwrite<Info[]>(newCount);
    for (u32 i = 0; i < _count; i++)
        newInfos[i] = _infos[i];

    newInfos[newCount - 1].path = normalized;
    newInfos[newCount - 1].cheatActiveCount = activeCount;
    newInfos[newCount - 1].cheatTotalCount = totalCount;
    newInfos[newCount - 1].hasCheatStats = true;

    _infos = std::move(newInfos);
    _count = newCount;
    Save();
}
