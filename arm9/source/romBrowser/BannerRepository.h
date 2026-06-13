#pragma once
#include <memory>
#include "IBannerRepository.h"
#include "SdFolder.h"

class BannerRepository : public IBannerRepository
{
public:
    void Initialize() override;
    InternalFileInfo* GetBannerForFile(
        const FileInfo& fileInfo, const InternalFileInfo* internalFileInfo) const override;

private:
    static constexpr int MaxSystemBannerFolders = 8;
    struct SystemBannerFolderEntry {
        char name[16] = {};
        std::unique_ptr<SdFolder> folder;
    };
    SystemBannerFolderEntry _systemBannerFolders[MaxSystemBannerFolders];
    int _systemBannerFolderCount = 0;

    std::unique_ptr<SdFolder> _userBannersFolder;

    const SdFolder* GetBannerFolder(const char* bannerFolderName) const;
};
