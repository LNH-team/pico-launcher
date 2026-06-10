#pragma once
#include <memory>
#include "IIconRepository.h"
#include "SdFolder.h"

class IconRepository : public IIconRepository
{
public:
    void Initialize() override;
    SharedPtr<BmpFileIconData> LoadIconData(
        const FileInfo& fileInfo, const InternalFileInfo* internalFileInfo) const override;

private:
    static constexpr int MaxSystemIconFolders = 8;
    struct SystemIconFolderEntry {
        char name[16] = {};
        std::unique_ptr<SdFolder> folder;
    };
    SystemIconFolderEntry _systemIconFolders[MaxSystemIconFolders];
    int _systemIconFolderCount = 0;

    std::unique_ptr<SdFolder> _userIconsFolder;

    const SdFolder* GetIconFolder(const char* iconFolderName) const;
};
