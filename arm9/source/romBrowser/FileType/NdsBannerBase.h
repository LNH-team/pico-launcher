#pragma once
#include <memory>
#include "InternalFileInfo.h"
#include "Nds/ndsBanner.h"

class File;

class alignas(32) NdsBannerBase : public InternalFileInfo
{
public:
    std::unique_ptr<FileIcon> CreateGameIcon() const override;
    const char* GetGameCode() const override;
    const char16_t* GetGameTitle() const override;
    bool HasBanner() const { return _hasBanner; }

protected:
    // Reads banner chunks from the current file position.
    // availableSize is the number of bytes remaining from that position.
    // Returns true on success and sets _hasBanner.
    bool ReadBannerChunks(File& file, u32 availableSize);

    nds_banner_t _banner alignas(32);
    bool _hasBanner = false;
    char _gameCode[5] = {};
};
