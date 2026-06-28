#pragma once
#include "ICoverRepository.h"
#include "RepositoryBase.h"

class CoverRepository : public RepositoryBase, public ICoverRepository
{
public:
    void Initialize() override;
    FileCover* GetCoverForFile(
        const FileInfo& fileInfo, const InternalFileInfo* internalFileInfo) const override;
};
