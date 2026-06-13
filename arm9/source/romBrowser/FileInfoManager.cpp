#include "common.h"
#include <string.h>
#include <libtwl/rtos/rtosIrq.h>
#include "FileInfoManager.h"
#include "FileType/BmpFileIcon.h"

FileInfoManager::FileInfoManager(std::unique_ptr<const FileInfo*[]> items, u32 itemCount, const ICoverRepository& coverRepository,
    const IIconRepository& iconRepository, const IBannerRepository& bannerRepository)
    : _items(std::move(items)), _itemCount(itemCount)
    , _extraFileInfo(std::make_unique<ExtraFileInfo[]>(itemCount))
    , _coverRepository(coverRepository)
    , _iconRepository(iconRepository)
    , _bannerRepository(bannerRepository) { }

FileInfoManager::~FileInfoManager()
{
    for (u32 i = 0; i < _itemCount; i++)
    {
        ReleaseFileInfo(i);
    }
}

// IRQ-protected test-and-set. All RTOS threads share the ARM9 core, so disabling
// IRQs prevents context switches and gives us an atomic claim window.
static bool TryClaimSlot(volatile bool& flag)
{
    u32 irq = rtos_disableIrqs();
    bool claimed = !flag;
    if (claimed)
        flag = true;
    rtos_restoreIrqs(irq);
    return claimed;
}

void FileInfoManager::LoadFileInfo(int index)
{
    // Fast path: data already ready. Loads are serialised on the IO task queue so
    // this branch is taken on any redundant second enqueue for the same slot.
    if (_extraFileInfo[index].loaded)
        return;

    // Claim the slot against ReleaseFileInfo, which runs on the main thread and can
    // context-switch in at any point while we are writing to this slot.
    if (!TryClaimSlot(_extraFileInfo[index].loading))
        return;

    auto internalFileInfo = _extraFileInfo[index].internalFileInfo;
    if (!internalFileInfo)
    {
        internalFileInfo = _items[index]->CreateInternalFileInfo();
        auto customBanner = _bannerRepository.GetBannerForFile(*_items[index], internalFileInfo);
        if (customBanner)
        {
            if (internalFileInfo)
            {
                delete internalFileInfo;
            }
            internalFileInfo = customBanner;
        }
    }

    if (!_extraFileInfo[index].fileCover.Lock())
    {
        _extraFileInfo[index].fileCover = SharedPtr(_coverRepository.GetCoverForFile(*_items[index], internalFileInfo));
    }

    if (!internalFileInfo || !internalFileInfo->IsCustomBanner())
    {
        if (!_extraFileInfo[index].iconData.Lock())
        {
            _extraFileInfo[index].iconData = _iconRepository.LoadIconData(*_items[index], internalFileInfo);
        }
    }

    _extraFileInfo[index].internalFileInfo = internalFileInfo;
    _extraFileInfo[index].loaded = true;
    _extraFileInfo[index].loading = false;
}

std::unique_ptr<FileIcon> FileInfoManager::GetFileIcon(int index)
{
    auto internalFileInfo = _extraFileInfo[index].internalFileInfo;
    if (internalFileInfo && internalFileInfo->IsCustomBanner())
    {
        auto icon = internalFileInfo->CreateGameIcon();
        if (icon)
        {
            return icon;
        }
    }

    auto iconData = _extraFileInfo[index].iconData.Lock();
    if (iconData)
    {
        return std::make_unique<BmpFileIcon>(std::move(iconData));
    }

    return internalFileInfo ? internalFileInfo->CreateGameIcon() : nullptr;
}

void FileInfoManager::ReleaseFileInfo(int index)
{
    // Claim the slot so that an in-progress LoadFileInfo on the IO task thread
    // cannot write to the slot while we are tearing it down.
    if (!TryClaimSlot(_extraFileInfo[index].loading))
        return;

    _extraFileInfo[index].loaded = false;

    auto internalFileInfo = _extraFileInfo[index].internalFileInfo;
    if (internalFileInfo)
    {
        _extraFileInfo[index].internalFileInfo = nullptr;
        delete internalFileInfo;
    }

    _extraFileInfo[index].fileCover.Reset();
    _extraFileInfo[index].iconData.Reset();
    _extraFileInfo[index].loading = false;
}

int FileInfoManager::GetItemIndex(const char* fileName)
{
    if (fileName == nullptr)
    {
        return -1;
    }
    for (u32 i = 0; i < _itemCount; i++)
    {
        if (strcmp(fileName, _items[i]->GetFileName()) == 0)
        {
            return i;
        }
    }
    return -1;
}
