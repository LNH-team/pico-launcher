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
        u32 cheatActiveCount = 0;
        bool hasCheatStats = false;
    };

    static LaunchStatsService& Instance();

    void Load();
    void Save() const;

    void Increment(const char* path);
    u32 GetCount(const char* path) const;
    bool TryGetLastLaunchDate(const char* path, char* outValue, u32 outValueSize) const;
    bool TryGetLastLaunchTime(const char* path, char* outValue, u32 outValueSize) const;
    void SetCheatStats(const char* path, u32 activeCount);

private:
    LaunchStatsService();
    void EnsureLoaded();

    std::unique_ptr<Info[]> _infos;
    u32 _count = 0;
    bool _loaded = false;
    const char* _filePath = "/_pico/extras/stats.bin";
};
