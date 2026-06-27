#pragma once
#include "ICoverRepository.h"
#include "RepositoryBase.h"

class CoverRepository : public ICoverRepository, public RepositoryBase
{
public:
    void Initialize() override;
    FileCover* GetCoverForFile(
        const FileInfo& fileInfo, const InternalFileInfo* internalFileInfo) const override;
};
