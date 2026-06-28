#include "common.h"
#include <string.h>
#include "core/StringUtil.h"
#include "FileType/NullFileTypeProvider.h"
#include "SdFolderFactory.h"
#include "RepositoryBase.h"

void RepositoryBase::InitializeFolders(const char* basePath)
{
    NullFileTypeProvider fileTypeProvider;
    SdFolderFactory folderFactory(&fileTypeProvider);
    char path[64];

    u32 len = StringUtil::Copy(path, basePath, sizeof(path));

    StringUtil::Copy(path + len, "nds", sizeof(path) - len);
    _ndsFolder = folderFactory.CreateFromPath(path);
    if (_ndsFolder)
    {
        _ndsFolder->SortByNameInPlace();
    }

    StringUtil::Copy(path + len, "gba", sizeof(path) - len);
    _gbaFolder = folderFactory.CreateFromPath(path);
    if (_gbaFolder)
    {
        _gbaFolder->SortByNameInPlace();
    }

    StringUtil::Copy(path + len, "user", sizeof(path) - len);
    _userFolder = folderFactory.CreateFromPath(path);
    if (_userFolder)
    {
        _userFolder->SortByNameInPlace();
    }
}

const SdFolder* RepositoryBase::GetFileTypeFolder(const char* shortName) const
{
    if (!strcmp(shortName, "nds"))
    {
        return _ndsFolder.get();
    }
    if (!strcmp(shortName, "gba"))
    {
        return _gbaFolder.get();
    }
    return nullptr;
}
