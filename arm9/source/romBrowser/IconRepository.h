#pragma once
#include "IIconRepository.h"
#include "RepositoryBase.h"

class IconRepository : public RepositoryBase, public IIconRepository
{
public:
    void Initialize() override;
    SharedPtr<BmpFileIconData> LoadIconData(
        const FileInfo& fileInfo, const InternalFileInfo* internalFileInfo) const override;
};
