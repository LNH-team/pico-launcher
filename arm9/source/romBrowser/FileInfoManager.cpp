#include "common.h"
#include <string.h>
#include "FileType/CustomIconInternalFileInfo.h"
#include "FileInfoManager.h"

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

void FileInfoManager::LoadFileInfo(int index)
{
    if (_extraFileInfo[index].loaded)
    {
        return;
    }

    const InternalFileInfo* internalFileInfo = _items[index]->CreateInternalFileInfo();
    const char* gameCode = internalFileInfo ? internalFileInfo->GetGameCode() : nullptr;

    // A custom banner (.bnr) takes priority and replaces the internal file info entirely.
    auto customBanner = _bannerRepository.GetBannerForFile(*_items[index], gameCode);
    if (customBanner)
    {
        delete internalFileInfo;
        internalFileInfo = customBanner;
    }
    else
    {
        // A custom icon (.bmp) wraps the existing internal file info, overriding only the icon.
        auto iconData = _iconRepository.GetIconForFile(*_items[index], gameCode);
        if (iconData)
        {
            internalFileInfo = new CustomIconInternalFileInfo(std::move(iconData), std::unique_ptr<const InternalFileInfo>(internalFileInfo));
        }
    }

    if (!_extraFileInfo[index].fileCover.Lock())
    {
        _extraFileInfo[index].fileCover = SharedPtr(_coverRepository.GetCoverForFile(*_items[index], internalFileInfo));
    }

    _extraFileInfo[index].internalFileInfo = internalFileInfo;
    _extraFileInfo[index].loaded = true;
}

void FileInfoManager::ReleaseFileInfo(int index)
{
    _extraFileInfo[index].loaded = false;

    auto internalFileInfo = _extraFileInfo[index].internalFileInfo;
    if (internalFileInfo)
    {
        _extraFileInfo[index].internalFileInfo = nullptr;
        delete internalFileInfo;
    }

    _extraFileInfo[index].fileCover.Reset();
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
