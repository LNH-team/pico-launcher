#pragma once
#include <memory>
#include "InternalFileInfo.h"
#include "Nds/ndsBanner.h"

class File;

class alignas(32) NdsBannerInternalFileInfo : public InternalFileInfo
{
public:
    std::unique_ptr<FileIcon> CreateGameIcon() const override;
    const char* GetGameCode() const override;
    const char16_t* GetGameTitle() const override;
    bool HasBanner() const { return _hasBanner; }

    bool HasIconData() const override { return _hasBanner; }
    const u8* GetIconGfx() const override { return _banner.iconGfx; }
    const u16* GetIconPalette() const override { return _banner.iconPltt; }

protected:
    bool _hasBanner = false;
    char _gameCode[5] = {};
    nds_banner_t _banner alignas(32);

    /// @brief Reads banner chunks from the current file position.
    /// @param file The file to read from.
    /// @param availableSize The number of bytes remaining from that position.
    /// @return True on success, or false otherwise.
    bool ReadBannerChunks(File& file, u32 availableSize);
};
