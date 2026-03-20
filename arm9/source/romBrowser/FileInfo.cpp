#include "common.h"
#include <string.h>
#include "core/StringUtil.h"
#include "FileInfo.h"

FileInfo::FileInfo(const FileInfo& fileInfo)
    : _type(fileInfo._type), _fastFileRef(fileInfo._fastFileRef)
{
    u32 bufferLength = strlen(fileInfo.GetFileName()) + 1;
    _name = std::make_unique_for_overwrite<TCHAR[]>(bufferLength);
    StringUtil::Copy(_name.get(), fileInfo.GetFileName(), bufferLength);

    const char* fullPath = fileInfo.GetFullPath();
    if (fullPath)
    {
        u32 fullPathLength = strlen(fullPath) + 1;
        _fullPath = std::make_unique_for_overwrite<TCHAR[]>(fullPathLength);
        StringUtil::Copy(_fullPath.get(), fullPath, fullPathLength);
    }
}

FileInfo::FileInfo(const TCHAR* fileName, const FileType* type, const FastFileRef& fastFileRef)
    : _type(type), _fastFileRef(fastFileRef)
{
    u32 bufferLength = strlen(fileName) + 1;
    _name = std::make_unique_for_overwrite<TCHAR[]>(bufferLength);
    StringUtil::Copy(_name.get(), fileName, bufferLength);
}

FileInfo::FileInfo(const TCHAR* fileName, const FileType* type, const FastFileRef& fastFileRef,
    const TCHAR* fullPath)
    : _type(type), _fastFileRef(fastFileRef)
{
    u32 bufferLength = strlen(fileName) + 1;
    _name = std::make_unique_for_overwrite<TCHAR[]>(bufferLength);
    StringUtil::Copy(_name.get(), fileName, bufferLength);

    if (fullPath && fullPath[0] != 0)
    {
        u32 fullPathLength = strlen(fullPath) + 1;
        _fullPath = std::make_unique_for_overwrite<TCHAR[]>(fullPathLength);
        StringUtil::Copy(_fullPath.get(), fullPath, fullPathLength);
    }
}