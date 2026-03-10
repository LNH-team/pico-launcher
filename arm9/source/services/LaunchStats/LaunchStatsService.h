#pragma once
#include "core/String.h"
#include <memory>

class LaunchStatsService
{
public:
    struct Info
    {
        String<char, 256> path;
        u32 launchCount = 0;
        String<char, 11> lastLaunchDate;
        String<char, 9> lastLaunchTime;
        u8 romVersion = 0;
        bool hasRomVersion = false;
        String<char, 16> id;
        bool hasID = false;
        u32 headerCrc = 0;
        bool hasHeaderCrc = false;
    };

    static LaunchStatsService& Instance();

    void Load();
    void Save() const;

    void Increment(const char* path);
    u32 GetCount(const char* path) const;
    bool TryGetLastLaunchDate(const char* path, char* outValue, u32 outValueSize) const;
    bool TryGetLastLaunchTime(const char* path, char* outValue, u32 outValueSize) const;
    bool TryGetInfo(const char* path, u32* outLaunchCount,
        char* outDate, u32 outDateSize,
        char* outTime, u32 outTimeSize) const;
    bool TryGetCachedData(const char* path,
        u8* outRomVersion, bool* outHasRomVersion,
        char* outId, u32 outIdSize, bool* outHasID,
        u32* outHeaderCrc, bool* outHasHeaderCrc) const;
    void SetCachedData(const char* path,
        u8 romVersion, bool hasRomVersion,
        const char* id, bool hasID,
        u32 headerCrc, bool hasHeaderCrc);

private:
    LaunchStatsService();
    void EnsureLoaded();

    std::unique_ptr<Info[]> _infos;
    u32 _count = 0;
    bool _loaded = false;
    const char* _filePath = "/_pico/extras/stats.bin";
};
