#pragma once
#include "../InternalFileInfo.h"
#include "fat/FastFileRef.h"
#include "../Nds/ndsBanner.h"

class File;

/// @brief Internal file info loaded from an external .bnr file patch.
class alignas(32) BnrInternalFileInfo : public InternalFileInfo
{
public:
    BnrInternalFileInfo(const FastFileRef& bnrFileRef, const char* originalGameCode = nullptr);
    BnrInternalFileInfo(const TCHAR* path, const char* originalGameCode = nullptr);

    const char16_t* GetGameTitle() const override;
    std::unique_ptr<FileIcon> CreateGameIcon() const override;
    const char* GetGameCode() const override;
    bool IsCustomBanner() const override { return _hasBanner; }

private:
    void Init(const char* originalGameCode);
    void Load(File& file);

    nds_banner_t _banner alignas(32);
    bool _hasBanner = false;
    char _gameCode[5];
};
