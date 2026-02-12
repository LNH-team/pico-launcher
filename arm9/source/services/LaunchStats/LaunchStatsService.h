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
    };

    static LaunchStatsService& Instance();

    void Load();
    void Save() const;

    void Increment(const char* path);
    u32 GetCount(const char* path) const;

private:
    LaunchStatsService();
    void EnsureLoaded();

    std::unique_ptr<Info[]> _infos;
    u32 _count = 0;
    bool _loaded = false;
    const char* _filePath = "/_pico/stats.json";
};
