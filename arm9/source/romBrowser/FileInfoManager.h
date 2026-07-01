#pragma once
#include "common.h"
#include <memory>
#include "FileInfo.h"
#include "FileType/FileCover.h"
#include "ICoverRepository.h"
#include "IIconRepository.h"
#include "IBannerRepository.h"
#include "core/AtomicSharedPtr.h"
#include "FileType/InternalFileInfo.h"

class FileInfoManager
{
public:
    FileInfoManager(std::unique_ptr<const FileInfo*[]> items, u32 itemCount, const ICoverRepository& coverRepository,
        const IIconRepository& iconRepository, const IBannerRepository& bannerRepository);
    ~FileInfoManager();

    const InternalFileInfo* GetInternalFileInfo(int index)
    {
        return _extraFileInfo[index].internalFileInfo;
    }

    /// @brief Whether LoadFileInfo() has finished for this item. Unlike checking
    ///        GetInternalFileInfo() for null, this distinguishes "still loading" from
    ///        "loaded, and there's legitimately nothing" (e.g. a folder with no custom icon).
    bool IsFileInfoLoaded(int index) const
    {
        return _extraFileInfo[index].loaded;
    }

    SharedPtr<FileCover> GetFileCover(int index)
    {
        return _extraFileInfo[index].fileCover.Lock();
    }

    void LoadFileInfo(int index);
    void ReleaseFileInfo(int index);

    int GetItemIndex(const char* fileName);

    const FileInfo& GetItem(int index) const { return *_items[index]; }
    u32 GetItemCount() const { return _itemCount; }

private:
    struct ExtraFileInfo
    {
        bool loaded = false;
        const InternalFileInfo* internalFileInfo = nullptr;
        AtomicSharedPtr<FileCover> fileCover;
    };

    std::unique_ptr<const FileInfo*[]> _items;
    u32 _itemCount;
    std::unique_ptr<ExtraFileInfo[]> _extraFileInfo;
    const ICoverRepository& _coverRepository;
    const IIconRepository& _iconRepository;
    const IBannerRepository& _bannerRepository;
};
