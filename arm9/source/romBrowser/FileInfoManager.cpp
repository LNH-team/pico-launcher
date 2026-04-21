#include "common.h"
#include <string.h>
#include "FileInfoManager.h"

FileInfoManager::FileInfoManager(std::unique_ptr<const FileInfo*[]> items, u32 itemCount, const ICoverRepository& coverRepository)
    : _items(std::move(items)), _itemCount(itemCount)
    , _extraFileInfo(std::make_unique<ExtraFileInfo[]>(itemCount))
    , _coverRepository(coverRepository) { }

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

    _extraFileInfo[index].internalFileInfo = internalFileInfo;
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