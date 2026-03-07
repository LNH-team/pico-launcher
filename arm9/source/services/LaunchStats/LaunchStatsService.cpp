#include "common.h"
#include "core/mini-printf.h"
#include "LaunchStatsService.h"
#include "fat/File.h"
#include "rtcIpc.h"

// Binary format for stats.bin:
//   magic:   u8[4]  = "STAT"
//   version: u8     = 1
//   count:   u32 LE
//   Per entry:
//     pathLen:          u8   (string byte length, without null terminator)
//     path:             char[pathLen]
//     launchCount:      u32 LE
//     hasCheatStats:    u8  (0 or 1)
//     cheatActiveCount: u32 LE
//     dateLen:          u8
//     date:             char[dateLen]
//     timeLen:          u8
//     time:             char[timeLen]

static const u8  STATS_MAGIC[4] = { 'S', 'T', 'A', 'T' };
static const u8  STATS_VERSION  = 1;

static u32 readU32LE(const u8* p)
{
    return (u32)p[0] | ((u32)p[1] << 8) | ((u32)p[2] << 16) | ((u32)p[3] << 24);
}

static void writeU32LE(u8* p, u32 val)
{
    p[0] = (u8)(val);
    p[1] = (u8)(val >> 8);
    p[2] = (u8)(val >> 16);
    p[3] = (u8)(val >> 24);
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
    if (fileSize < 9)
        return;

    std::unique_ptr<u8[]> fileData(new(cache_align) u8[fileSize]);
    u32 bytesRead = 0;
    if (file->Read(fileData.get(), fileSize, bytesRead) != FR_OK || bytesRead != fileSize)
        return;

    const u8* p   = fileData.get();
    const u8* end = p + fileSize;

    if (p[0] != 'S' || p[1] != 'T' || p[2] != 'A' || p[3] != 'T')
        return;
    p += 4;

    if (*p++ != STATS_VERSION)
        return;

    u32 count = readU32LE(p);
    p += 4;

    if (count == 0)
        return;

    _infos = std::make_unique_for_overwrite<Info[]>(count);
    u32 i = 0;

    while (i < count && p < end)
    {
        if (p >= end) break;
        u8 pathLen = *p++;
        if (p + pathLen > end) break;
        char pathBuf[256];
        if (pathLen > 0)
            memcpy(pathBuf, p, pathLen);
        pathBuf[pathLen] = '\0';
        p += pathLen;

        const char* colon = strchr(pathBuf, ':');
        if (colon && colon < pathBuf + 6)
            _infos[i].path = colon;
        else
            _infos[i].path = pathBuf;

        if (p + 4 > end) break;
        _infos[i].launchCount = readU32LE(p);
        p += 4;

        if (p >= end) break;
        _infos[i].hasCheatStats = (*p++ != 0);

        if (p + 4 > end) break;
        _infos[i].cheatActiveCount = readU32LE(p);
        p += 4;

        if (p >= end) break;
        u8 dateLen = *p++;
        if (p + dateLen > end) break;
        if (dateLen > 0)
        {
            char dateBuf[11];
            u8 copyLen = dateLen < 10 ? dateLen : 10;
            memcpy(dateBuf, p, copyLen);
            dateBuf[copyLen] = '\0';
            _infos[i].lastLaunchDate = dateBuf;
        }
        p += dateLen;

        if (p >= end) break;
        u8 timeLen = *p++;
        if (p + timeLen > end) break;
        if (timeLen > 0)
        {
            char timeBuf[9];
            u8 copyLen = timeLen < 8 ? timeLen : 8;
            memcpy(timeBuf, p, copyLen);
            timeBuf[copyLen] = '\0';
            _infos[i].lastLaunchTime = timeBuf;
        }
        p += timeLen;

        i++;
    }
    _count = i;
}

void LaunchStatsService::Save() const
{
    u32 outputSize = 4 + 1 + 4;
    for (u32 i = 0; i < _count; i++)
    {
        u8 pathLen = (u8)strlen(_infos[i].path.GetString());
        u8 dateLen = (u8)strlen(_infos[i].lastLaunchDate.GetString());
        u8 timeLen = (u8)strlen(_infos[i].lastLaunchTime.GetString());
        outputSize += 1u + pathLen   // pathLen field + path bytes
                    + 4u             // launchCount
                    + 1u             // hasCheatStats
                    + 4u             // cheatActiveCount
                    + 1u + dateLen   // dateLen field + date bytes
                    + 1u + timeLen;  // timeLen field + time bytes
    }

    std::unique_ptr<u8[]> fileData(new(cache_align) u8[outputSize]);
    u8* p = fileData.get();

    p[0] = 'S'; p[1] = 'T'; p[2] = 'A'; p[3] = 'T';
    p += 4;

    *p++ = STATS_VERSION;

    writeU32LE(p, _count);
    p += 4;

    for (u32 i = 0; i < _count; i++)
    {
        const char* path = _infos[i].path.GetString();
        u8 pathLen = (u8)strlen(path);
        *p++ = pathLen;
        if (pathLen > 0)
            memcpy(p, path, pathLen);
        p += pathLen;

        writeU32LE(p, _infos[i].launchCount);
        p += 4;

        *p++ = _infos[i].hasCheatStats ? 1u : 0u;

        writeU32LE(p, _infos[i].cheatActiveCount);
        p += 4;

        const char* date = _infos[i].lastLaunchDate.GetString();
        u8 dateLen = (u8)strlen(date);
        *p++ = dateLen;
        if (dateLen > 0)
            memcpy(p, date, dateLen);
        p += dateLen;

        const char* time = _infos[i].lastLaunchTime.GetString();
        u8 timeLen = (u8)strlen(time);
        *p++ = timeLen;
        if (timeLen > 0)
            memcpy(p, time, timeLen);
        p += timeLen;
    }

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

void LaunchStatsService::SetCheatStats(const char* path, u32 activeCount)
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
            if (_infos[i].cheatActiveCount == activeCount)
                return;

            _infos[i].cheatActiveCount = activeCount;
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
    newInfos[newCount - 1].hasCheatStats = true;

    _infos = std::move(newInfos);
    _count = newCount;
    Save();
}
