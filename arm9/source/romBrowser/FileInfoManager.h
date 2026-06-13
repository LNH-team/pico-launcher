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

    bool IsFileInfoLoaded(int index) const
    {
        return _extraFileInfo[index].loaded;
    }

    SharedPtr<FileCover> GetFileCover(int index)
    {
        return _extraFileInfo[index].fileCover.Lock();
    }

    // Precondition: IsFileInfoLoaded(index) must have returned true before calling this.
    std::unique_ptr<FileIcon> GetFileIcon(int index);

    void LoadFileInfo(int index);

    // Precondition: must not be called while the render thread may be reading this slot's data.
    void ReleaseFileInfo(int index);

    int GetItemIndex(const char* fileName);

    const FileInfo& GetItem(int index) const { return *_items[index]; }
    u32 GetItemCount() const { return _itemCount; }

private:
    struct ExtraFileInfo
    {
        volatile bool loading = false; // claim flag: IRQ-protected TAS guards ReleaseFileInfo races
        volatile bool loaded = false;
        const InternalFileInfo* internalFileInfo{nullptr};
        AtomicSharedPtr<FileCover> fileCover;
        AtomicSharedPtr<BmpFileIconData> iconData;
    };

    std::unique_ptr<const FileInfo*[]> _items;
    u32 _itemCount;
    std::unique_ptr<ExtraFileInfo[]> _extraFileInfo;
    const ICoverRepository& _coverRepository;
    const IIconRepository& _iconRepository;
    const IBannerRepository& _bannerRepository;
};
