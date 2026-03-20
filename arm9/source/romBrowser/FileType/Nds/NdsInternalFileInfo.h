#pragma once
#include "../InternalFileInfo.h"
#include "ndsBanner.h"
#include "NdsFileIcon.h"
#include "fat/FastFileRef.h"

/// @brief Internal file info for nds roms.
class alignas(32) NdsInternalFileInfo : public InternalFileInfo
{
public:
    explicit NdsInternalFileInfo(const FastFileRef& fastFileRef);

    constexpr const char* GetGameCode() const override { return _gameCode; }
    const char16_t* GetGameTitle() const override;
    constexpr u8 GetUnitCode() const { return _unitCode; }
    constexpr u8 GetRomVersion() const { return _romVersion; }

    std::unique_ptr<FileIcon> CreateGameIcon() const override
    {
        if (!_hasBanner)
            return nullptr;
        return std::make_unique<NdsFileIcon>(&_banner);
    }

    const nds_banner_t& GetBanner() const { return _banner; }

private:
    nds_banner_t _banner alignas(32);
    bool _hasBanner;
    u8 _unitCode;
    u8 _romVersion;
    char _gameCode[5];
};
