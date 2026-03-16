#include "common.h"
#include "core/mini-printf.h"
#include "LaunchStatsService.h"
#include "fat/File.h"
#include "rtcIpc.h"

/*
 * stats.bin  –  version 1
 *
 * Global Header (9 bytes):
 *   magic:       u8[4]  "STAT"
 *   version:     u8     1
 *   entryCount:  u32 LE
 *
 * Data Record (repeated entryCount times, VARIABLE SIZE):
 *   path:        char[]      ROM file name (ASCII, null-terminated)
 *   launchCount: u32 LE      Total launches
 *   date:        u8[10]      Last launch date (ASCII "YYYY-MM-DD")
 *   time:        u8[8]       Last launch time (ASCII "HH:MM:SS")
 */
static const u8 STATS_VERSION = 1;

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

static void getCurrentDateString(char* out, u32 outSize)
{
    if (!out || outSize == 0)
        return;
    rtc_datetime_t dt;
    rtc_readDateTime(&dt);
    u32 year     = 2000 + bcdToDecimal(dt.date.year);
    u32 month    = bcdToDecimal(dt.date.month);
    u32 monthDay = bcdToDecimal(dt.date.monthDay);
    if (month   < 1 || month   > 12) month   = 1;
    if (monthDay < 1 || monthDay > 31) monthDay = 1;
    mini_snprintf(out, outSize, "%04lu-%02lu-%02lu", year, month, monthDay);
}

static void getCurrentTimeString(char* out, u32 outSize)
{
    if (!out || outSize == 0)
        return;
    rtc_datetime_t dt;
    rtc_readDateTime(&dt);
    u32 hour   = bcdToDecimal(dt.time.hour);
    u32 minute = bcdToDecimal(dt.time.minute);
    u32 second = bcdToDecimal(dt.time.second);
    if (hour   > 23) hour   = 0;
    if (minute > 59) minute = 0;
    if (second > 59) second = 0;
    mini_snprintf(out, outSize, "%02lu:%02lu:%02lu", hour, minute, second);
}

LaunchStatsService& LaunchStatsService::Instance()
{
    static LaunchStatsService instance;
    return instance;
}

LaunchStatsService::LaunchStatsService() { }

void LaunchStatsService::EnsureLoaded()
{
    if (_loaded)
        return;
    Load();
}

LaunchStatsService::Info* LaunchStatsService::FindInfo(const char* fileName) const
{
    for (u32 i = 0; i < _count; i++)
    {
        if (!strcasecmp(_infos[i].path.GetString(), fileName))
            return &_infos[i];
    }
    return nullptr;
}

LaunchStatsService::Info& LaunchStatsService::FindOrCreateInfo(const char* fileName)
{
    Info* existing = FindInfo(fileName);
    if (existing)
        return *existing;

    u32 newCount = _count + 1;
    auto newInfos = std::make_unique_for_overwrite<Info[]>(newCount);
    for (u32 i = 0; i < _count; i++)
        newInfos[i] = _infos[i];

    Info& fresh = newInfos[newCount - 1];
    fresh = Info{};
    fresh.path = fileName;

    _infos = std::move(newInfos);
    _count = newCount;
    return _infos[newCount - 1];
}

void LaunchStatsService::Load()
{
    _loaded = true;

    const auto file = std::make_unique<File>();
    if (file->Open(kFilePath, FA_READ | FA_OPEN_EXISTING) != FR_OK)
        return;

    u32 fileSize = file->GetSize();
    if (fileSize < 9)
        return;

    std::unique_ptr<u8[]> buf(new(cache_align) u8[fileSize]);
    u32 bytesRead = 0;
    if (file->Read(buf.get(), fileSize, bytesRead) != FR_OK || bytesRead != fileSize)
        return;

    const u8* p   = buf.get();
    const u8* end = p + fileSize;

    if (p[0] != 'S' || p[1] != 'T' || p[2] != 'A' || p[3] != 'T')
        return;
    p += 4;

    u8 fileVersion = *p++;
    if (fileVersion != STATS_VERSION)
        return;

    u32 count = readU32LE(p);
    p += 4;
    if (count == 0)
        return;

    _infos = std::make_unique_for_overwrite<Info[]>(count);
    u32 i = 0;

    while (i < count && p < end)
    {
        Info& info = _infos[i];
        info = Info{};

        // Read null-terminated path
        const u8* pathStart = p;
        while (p < end && *p != '\0')
            p++;
        if (p >= end) break; // No null terminator found
        u32 pathLen = p - pathStart;
        {
            char tmp[256];
            u32 len = pathLen < 255 ? pathLen : 255;
            memcpy(tmp, pathStart, len);
            tmp[len] = '\0';
            info.path = tmp;
        }
        p++; // Skip null terminator

        if (p + 4 > end) break;
        info.launchCount = readU32LE(p);
        p += 4;

        if (p + 10 > end) break;
        memcpy(info.lastLaunchDate, p, 10);
        info.lastLaunchDate[10] = '\0';
        p += 10;

        if (p + 8 > end) break;
        memcpy(info.lastLaunchTime, p, 8);
        info.lastLaunchTime[8] = '\0';
        p += 8;

        i++;
    }
    _count = i;
}

void LaunchStatsService::Save() const
{
    u32 totalSize = 4 + 1 + 4; // magic + version + entryCount
    for (u32 i = 0; i < _count; i++)
    {
        totalSize += strlen(_infos[i].path.GetString()) + 1; // path + null terminator
        totalSize += 4 + 10 + 8; // launchCount + date + time
    }

    std::unique_ptr<u8[]> buf(new(cache_align) u8[totalSize]);
    u8* p = buf.get();

    p[0] = 'S'; p[1] = 'T'; p[2] = 'A'; p[3] = 'T';
    p += 4;
    *p++ = STATS_VERSION;
    writeU32LE(p, _count);
    p += 4;

    for (u32 i = 0; i < _count; i++)
    {
        const Info& info = _infos[i];

        const char* path = info.path.GetString();
        u32 pathLen = strlen(path);
        memcpy(p, path, pathLen);
        p += pathLen;
        *p++ = '\0'; // null terminator

        writeU32LE(p, info.launchCount);
        p += 4;

        memcpy(p, info.lastLaunchDate, 10);
        p += 10;

        memcpy(p, info.lastLaunchTime, 8);
        p += 8;
    }

    const auto file = std::make_unique<File>();
    if (file->Open(kFilePath, FA_WRITE | FA_CREATE_ALWAYS) != FR_OK)
        return;

    u32 bytesWritten = 0;
    file->Write(buf.get(), totalSize, bytesWritten);
}

void LaunchStatsService::Increment(const char* path)
{
    if (!path || path[0] == 0)
        return;
    EnsureLoaded();

    Info& info = FindOrCreateInfo(path);

    info.launchCount++;
    getCurrentDateString(info.lastLaunchDate, sizeof(info.lastLaunchDate));
    getCurrentTimeString(info.lastLaunchTime, sizeof(info.lastLaunchTime));

    Save();
}

bool LaunchStatsService::TryGetInfo(const char* path,
    u32* outLaunchCount,
    char* outDate, u32 outDateSize,
    char* outTime, u32 outTimeSize) const
{
    if (!path)
        return false;

    const_cast<LaunchStatsService*>(this)->EnsureLoaded();

    if (outLaunchCount) *outLaunchCount = 0;
    if (outDate && outDateSize > 0) outDate[0] = '\0';
    if (outTime && outTimeSize > 0) outTime[0] = '\0';

    const Info* info = FindInfo(path);
    if (!info)
        return false;

    if (outLaunchCount)
        *outLaunchCount = info->launchCount;

    const char* date = info->lastLaunchDate;
    if (outDate && outDateSize > 0 && date[0] != '\0')
    {
        if (date[4] == '-' && date[7] == '-')
        {
            mini_snprintf(outDate, outDateSize, "%c%c/%c%c/%c%c%c%c",
                date[8], date[9], date[5], date[6], date[0], date[1], date[2], date[3]);
        }
        else
        {
            mini_snprintf(outDate, outDateSize, "%s", date);
        }
    }

    if (outTime && outTimeSize > 0 && info->lastLaunchTime[0] != '\0')
        mini_snprintf(outTime, outTimeSize, "%s", info->lastLaunchTime);

    return true;
}