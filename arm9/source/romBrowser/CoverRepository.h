#pragma once
#include "ICoverRepository.h"
#include "RepositoryBase.h"
#include "IconCoverMode.h"

class CoverRepository : public RepositoryBase, public ICoverRepository
{
public:
    /// @param iconCoverMode Whether and how icon fallback covers are created.
    /// @param iconCoverBackgroundColor The XBGR555 background color of icon fallback covers.
    CoverRepository(IconCoverMode iconCoverMode, u16 iconCoverBackgroundColor);

    void Initialize() override;
    FileCover* GetCoverForFile(
        const FileInfo& fileInfo, const InternalFileInfo* internalFileInfo) const override;

private:
    IconCoverMode _iconCoverMode;
    u16 _iconCoverBackgroundColor;
};
