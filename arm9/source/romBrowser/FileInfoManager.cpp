#include "common.h"
#include <string.h>
#include "FileInfoManager.h"
#include "FileType/BmpFileIcon.h"

FileInfoManager::FileInfoManager(std::unique_ptr<const FileInfo*[]> items, u32 itemCount, const ICoverRepository& coverRepository,
    const IIconRepository& iconRepository)
    : _items(std::move(items)), _itemCount(itemCount)
    , _extraFileInfo(std::make_unique<ExtraFileInfo[]>(itemCount))
    , _coverRepository(coverRepository)
    , _iconRepository(iconRepository) { }

FileInfoManager::~FileInfoManager()
{
    for (u32 i = 0; i < _itemCount; i++)
    {
        ReleaseFileInfo(i);
    }
}

void FileInfoManager::LoadFileInfo(int index)
{
    auto internalFileInfo = _extraFileInfo[index].internalFileInfo;
    if (!internalFileInfo)
    {
        internalFileInfo = _items[index]->CreateInternalFileInfo();
    }

    if (!_extraFileInfo[index].fileCover.Lock())
    {
        _extraFileInfo[index].fileCover = SharedPtr(_coverRepository.GetCoverForFile(*_items[index], internalFileInfo));
    }

    if (!_extraFileInfo[index].iconData.Lock())
    {
        _extraFileInfo[index].iconData = _iconRepository.LoadIconData(*_items[index], internalFileInfo);
    }

    _extraFileInfo[index].internalFileInfo = internalFileInfo;
}

std::unique_ptr<FileIcon> FileInfoManager::GetFileIcon(int index)
{
    auto iconData = _extraFileInfo[index].iconData.Lock();
    if (iconData)
    {
        return std::make_unique<BmpFileIcon>(std::move(iconData));
    }

    auto internalFileInfo = _extraFileInfo[index].internalFileInfo;
    return internalFileInfo ? internalFileInfo->CreateGameIcon() : nullptr;
}

void FileInfoManager::ReleaseFileInfo(int index)
{
    auto internalFileInfo = _extraFileInfo[index].internalFileInfo;
    if (internalFileInfo)
    {
        _extraFileInfo[index].internalFileInfo = nullptr;
        delete internalFileInfo;
    }

    _extraFileInfo[index].fileCover.Reset();
    _extraFileInfo[index].iconData.Reset();
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