#pragma once
#include "IBannerRepository.h"
#include "RepositoryBase.h"

class BannerRepository : public RepositoryBase, public IBannerRepository
{
public:
    void Initialize() override;
    InternalFileInfo* GetBannerForFile(const FileInfo& fileInfo, const char* gameCode) const override;
};
