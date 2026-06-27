#pragma once
#include "IIconRepository.h"
#include "RepositoryBase.h"

class IconRepository : public IIconRepository, public RepositoryBase
{
public:
    void Initialize() override;
    SharedPtr<BmpFileIconData> LoadIconData(
        const FileInfo& fileInfo, const InternalFileInfo* internalFileInfo) const override;
};
