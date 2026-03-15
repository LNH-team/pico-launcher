#pragma once
#include "core/String.h"
#include <memory>

class LaunchStatsService
{
public:
    struct Info
    {
        String<char, 256> path;
        u32  launchCount       = 0;
        char lastLaunchDate[11] = {};
        char lastLaunchTime[9]  = {};
    };

    static LaunchStatsService& Instance();

    void Increment(const char* path);

    bool TryGetInfo(const char* path,
                    u32* outLaunchCount,
                    char* outDate, u32 outDateSize,
                    char* outTime, u32 outTimeSize) const;

private:
    LaunchStatsService();
    void EnsureLoaded();
    void Load();
    void Save() const;

    Info* FindInfo(const char* normalizedPath) const;
    Info& FindOrCreateInfo(const char* normalizedPath);
    static const char* NormalizePath(const char* path);

    std::unique_ptr<Info[]> _infos;
    u32  _count  = 0;
    bool _loaded = false;

    static constexpr const char* kFilePath = "/_pico/extras/stats.bin";
};