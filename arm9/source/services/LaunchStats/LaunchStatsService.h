#pragma once
#include "core/String.h"
#include "fat/FastFileRef.h"
#include <memory>

class LaunchStatsService
{
public:
    enum class RomType : u8
    {
        Unknown = 0,
        Nds     = 1,   // .nds / .dsi / .srl
        Gba     = 2,   // .gba
    };

    struct RomMetadata
    {
        u32  headerCrc32    = 0;
        char gameCode[5]    = {};
        u8   romVersion     = 0;
        char gameTitle[13]  = {};
        u8   unitCode       = 0;
    };

    struct Info
    {
        String<char, 256> path;
        u32  launchCount       = 0;
        char lastLaunchDate[11] = {};
        char lastLaunchTime[9]  = {};

        RomType romType    = RomType::Unknown;
        bool metaScanned   = false;
        bool metaValid     = false;
        RomMetadata meta   = {};
    };

    static LaunchStatsService& Instance();

    void Load();
    void Save() const;

    void Increment(const char* path);

    bool TryGetInfo(const char* path,
                    u32* outLaunchCount,
                    char* outDate, u32 outDateSize,
                    char* outTime, u32 outTimeSize) const;

    bool NeedsMetadataScan(const char* path) const;

    bool TryGetRomMetadata(const char* path,
                           RomType* outRomType,
                           RomMetadata* outMeta) const;

    void ScanRomFile(const char* normalizedPath, RomType romType,
                     const FastFileRef& fileRef);

    static RomType GetRomTypeFromExtension(const char* ext);

private:
    LaunchStatsService();
    void EnsureLoaded();

    Info* FindInfo(const char* normalizedPath) const;

    Info& FindOrCreateInfo(const char* normalizedPath);

    static const char* NormalizePath(const char* path);

    std::unique_ptr<Info[]> _infos;
    u32  _count  = 0;
    bool _loaded = false;
    static constexpr const char* kFilePath = "/_pico/extras/stats.bin";
};
