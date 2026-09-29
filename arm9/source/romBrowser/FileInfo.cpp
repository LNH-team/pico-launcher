#include "common.h"
#include <string.h>
#include "core/StringUtil.h"
#include "FileInfo.h"

FileInfo::FileInfo(const FileInfo& fileInfo)
    : _type(fileInfo._type), _fastFileRef(fileInfo._fastFileRef), _attributes(fileInfo._attributes)
{
    u32 bufferLength = strlen(fileInfo.GetFileName()) + 1;
    _name = std::make_unique_for_overwrite<TCHAR[]>(bufferLength);
    StringUtil::Copy(_name.get(), fileInfo.GetFileName(), bufferLength);

    if (fileInfo.GetFullPath())
    {
        u32 fullPathBufferLength = strlen(fileInfo.GetFullPath()) + 1;
        _fullPath = std::make_unique_for_overwrite<TCHAR[]>(fullPathBufferLength);
        StringUtil::Copy(_fullPath.get(), fileInfo.GetFullPath(), fullPathBufferLength);
    }
}

FileInfo::FileInfo(const TCHAR* fileName, const FileType* type, const FastFileRef& fastFileRef, u8 attributes,
    const TCHAR* fullPath)
    : _type(type), _fastFileRef(fastFileRef), _attributes(attributes)
{
    u32 bufferLength = strlen(fileName) + 1;
    _name = std::make_unique_for_overwrite<TCHAR[]>(bufferLength);
    StringUtil::Copy(_name.get(), fileName, bufferLength);

    if (fullPath)
    {
        u32 fullPathBufferLength = strlen(fullPath) + 1;
        _fullPath = std::make_unique_for_overwrite<TCHAR[]>(fullPathBufferLength);
        StringUtil::Copy(_fullPath.get(), fullPath, fullPathBufferLength);
    }
}