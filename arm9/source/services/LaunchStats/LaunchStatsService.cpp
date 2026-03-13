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
 * Data Record  (repeated entryCount times, VARIABLE SIZE):
 *   pathLen:     u8          Length of the ROM path string
 *   path:        char[]      ROM path (ASCII, NO null-terminator)
 *   launchCount: u32 LE      Total launches
 *   date:        u8[10]      Last launch date (ASCII "YYYY-MM-DD")
 *   time:        u8[8]       Last launch time (ASCII "HH:MM:SS")
 *   romType:     u8          ROM type: 0 = Unknown, 1 = NDS, 2 = GBA
 *   metaFlags:   u8          Bit flags: bit0 = metaScanned, bit1 = metaValid
 *
 *   [if metaScanned (bit0 of metaFlags) == 1]:
 *     headerCrc32: u32 LE    CRC32 of the ROM header
 *     gameCode:    u8[4]     Game code (ASCII, e.g., "AMQE")
 *     romVersion:  u8        Internal ROM version
 *     gameTitle:   u8[12]    Game title (ASCII, padded with spaces)
 *     unitCode:    u8        Unit code
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

static u32 computeCrc32(const u8* p, u32 length)
{
    static const u32 POLY = 0xEDB88320u;
    u32 crc = ~0u;
    while (length--)
    {
        crc ^= *p++;
        for (int i = 0; i < 8; i++)
            crc = (crc >> 1) ^ ((crc & 1) ? POLY : 0);
    }
    return ~crc;
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

const char* LaunchStatsService::NormalizePath(const char* path)
{
    if (!path)
        return path;
    const char* colon = strchr(path, ':');
    if (colon && colon < path + 6)
        return colon;
    return path;
}

LaunchStatsService::Info* LaunchStatsService::FindInfo(const char* normalizedPath) const
{
    for (u32 i = 0; i < _count; i++)
    {
        if (!strcasecmp(_infos[i].path.GetString(), normalizedPath))
            return &_infos[i];
    }
    return nullptr;
}

LaunchStatsService::Info& LaunchStatsService::FindOrCreateInfo(const char* normalizedPath)
{
    Info* existing = FindInfo(normalizedPath);
    if (existing)
        return *existing;

    u32 newCount = _count + 1;
    auto newInfos = std::make_unique_for_overwrite<Info[]>(newCount);
    for (u32 i = 0; i < _count; i++)
        newInfos[i] = _infos[i];

    Info& fresh = newInfos[newCount - 1];
    fresh = Info{};
    fresh.path = normalizedPath;

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

        if (p >= end) break;
        u8 pathLen = *p++;
        if (p + pathLen > end) break;
        {
            char tmp[256];
            u8 len = pathLen < 255 ? pathLen : 255;
            memcpy(tmp, p, len);
            tmp[len] = '\0';
            info.path = tmp;
        }
        p += pathLen;

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

        if (p >= end) break;
        info.romType = (RomType)(*p++);

        if (p >= end) break;
        u8 metaFlags = *p++;
        info.metaScanned = (metaFlags & 0x01) != 0;
        info.metaValid   = (metaFlags & 0x02) != 0;

        if (info.metaScanned)
        {
            if (p + 22 > end) break;

            info.meta.headerCrc32 = readU32LE(p); p += 4;
            memcpy(info.meta.gameCode, p, 4); info.meta.gameCode[4] = '\0'; p += 4;
            info.meta.romVersion = *p++;
            memcpy(info.meta.gameTitle, p, 12); info.meta.gameTitle[12] = '\0'; p += 12;
            info.meta.unitCode = *p++;
        }

        i++;
    }
    _count = i;
}

void LaunchStatsService::Save() const
{
    u32 totalSize = 4 + 1 + 4;
    for (u32 i = 0; i < _count; i++)
    {
        u8 pathLen = (u8)strlen(_infos[i].path.GetString());
        totalSize += 1u + pathLen + 4u + 10u + 8u + 1u + 1u;
        if (_infos[i].metaScanned)
            totalSize += 22u;
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
        u8 pathLen = (u8)strlen(path);
        *p++ = pathLen;
        if (pathLen > 0)
            memcpy(p, path, pathLen);
        p += pathLen;

        writeU32LE(p, info.launchCount);
        p += 4;

        memcpy(p, info.lastLaunchDate, 10);
        p += 10;

        memcpy(p, info.lastLaunchTime, 8);
        p += 8;

        *p++ = (u8)info.romType;

        u8 metaFlags = 0;
        if (info.metaScanned) metaFlags |= 0x01;
        if (info.metaValid)   metaFlags |= 0x02;
        *p++ = metaFlags;

        if (info.metaScanned)
        {
            writeU32LE(p, info.meta.headerCrc32); p += 4;
            memcpy(p, info.meta.gameCode, 4);     p += 4;
            *p++ = info.meta.romVersion;
            memcpy(p, info.meta.gameTitle, 12);   p += 12;
            *p++ = info.meta.unitCode;
        }
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

    const char* norm = NormalizePath(path);
    Info& info = FindOrCreateInfo(norm);

    info.launchCount++;
    getCurrentDateString(info.lastLaunchDate, sizeof(info.lastLaunchDate));
    getCurrentTimeString(info.lastLaunchTime, sizeof(info.lastLaunchTime));

    if (info.romType == RomType::Unknown)
    {
        const char* dot = strrchr(norm, '.');
        if (dot)
            info.romType = GetRomTypeFromExtension(dot + 1);
    }

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

    const char* norm = NormalizePath(path);
    const Info* info = FindInfo(norm);
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

bool LaunchStatsService::NeedsMetadataScan(const char* path) const
{
    if (!path)
        return false;

    const char* dot = strrchr(path, '.');
    if (!dot)
        return false;

    RomType rt = GetRomTypeFromExtension(dot + 1);
    if (rt == RomType::Unknown)
        return false;

    const_cast<LaunchStatsService*>(this)->EnsureLoaded();

    const char* norm = NormalizePath(path);
    const Info* info = FindInfo(norm);
    return (!info || !info->metaScanned);
}

bool LaunchStatsService::TryGetRomMetadata(const char* path,
    RomType* outRomType,
    RomMetadata* outMeta) const
{
    if (!path)
        return false;

    const_cast<LaunchStatsService*>(this)->EnsureLoaded();

    const char* norm = NormalizePath(path);
    const Info* info = FindInfo(norm);
    if (!info || !info->metaScanned || !info->metaValid)
        return false;

    if (outRomType) *outRomType = info->romType;
    if (outMeta)    *outMeta    = info->meta;
    return true;
}

void LaunchStatsService::ScanRomFile(const char* normalizedPath, RomType romType,
    const FastFileRef& fileRef)
{
    if (!normalizedPath || normalizedPath[0] == 0)
        return;
    if (romType == RomType::Unknown)
        return;

    EnsureLoaded();

    {
        const Info* existing = FindInfo(normalizedPath);
        if (existing && existing->metaScanned)
            return;
    }

    RomMetadata meta = {};
    bool valid = false;

    if (romType == RomType::Nds)
    {
        u8 header[512];
        File romFile;
        if (romFile.Open(fileRef, FA_READ) == FR_OK && romFile.GetSize() >= 512)
        {
            u32 br = 0;
            if (romFile.Read(header, 512, br) == FR_OK && br == 512)
            {
                memcpy(meta.gameTitle, header + 0x00, 12);
                meta.gameTitle[12] = '\0';

                memcpy(meta.gameCode, header + 0x0C, 4);
                meta.gameCode[4] = '\0';

                meta.unitCode = header[0x12];

                meta.romVersion = header[0x1E];

                meta.headerCrc32 = computeCrc32(header, 512);

                valid = true;
            }
        }
    }
    else if (romType == RomType::Gba)
    {
        u8 header[0xC0];
        File romFile;
        if (romFile.Open(fileRef, FA_READ) == FR_OK && romFile.GetSize() >= 0xC0)
        {
            u32 br = 0;
            if (romFile.Read(header, 0xC0, br) == FR_OK && br == 0xC0)
            {
                memcpy(meta.gameTitle, header + 0xA0, 12);
                meta.gameTitle[12] = '\0';

                memcpy(meta.gameCode, header + 0xAC, 4);
                meta.gameCode[4] = '\0';

                meta.romVersion = header[0xBC];

                meta.unitCode = 0;

                meta.headerCrc32 = computeCrc32(header, 0xC0);

                valid = true;
            }
        }
    }

    Info& info = FindOrCreateInfo(normalizedPath);
    info.romType    = romType;
    info.metaScanned = true;
    info.metaValid   = valid;
    if (valid)
        info.meta = meta;

    if (info.romType == RomType::Unknown)
        info.romType = romType;

    Save();
}

LaunchStatsService::RomType LaunchStatsService::GetRomTypeFromExtension(const char* ext)
{
    if (!ext)
        return RomType::Unknown;
    if (!strcasecmp(ext, "nds") || !strcasecmp(ext, "dsi") || !strcasecmp(ext, "srl"))
        return RomType::Nds;
    if (!strcasecmp(ext, "gba"))
        return RomType::Gba;
    return RomType::Unknown;
}
