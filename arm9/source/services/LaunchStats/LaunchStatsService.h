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
        String<char, 4> gameCode;
        bool hasGameCode = false;
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
        char* outTime, u32 outTimeSize,
        u32* outCheatActiveCount, bool* outHasCheatStats) const;
    bool TryGetGameIdentity(const char* path,
        char* outGameCode, u32 outGameCodeSize, bool* outHasGameCode,
        u32* outHeaderCrc, bool* outHasHeaderCrc) const;
    void SetCheatStats(const char* path, u32 activeCount);
    void SetGameIdentity(const char* path,
        const char* gameCode, bool hasGameCode,
        u32 headerCrc, bool hasHeaderCrc);

private:
    LaunchStatsService();
    void EnsureLoaded();

    std::unique_ptr<Info[]> _infos;
    u32 _count = 0;
    bool _loaded = false;
    const char* _filePath = "/_pico/extras/stats.bin";
};
