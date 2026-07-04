#pragma once
#include "core/SharedPtr.h"
#include "FileType/BmpFileIconData.h"

class FileInfo;
class InternalFileInfo;

class IIconRepository
{
public:
    virtual ~IIconRepository() = default;

    virtual void Initialize() = 0;
    virtual SharedPtr<BmpFileIconData> GetIconForFile(
        const FileInfo& fileInfo, const InternalFileInfo* internalFileInfo) const = 0;

protected:
    IIconRepository() = default;
};
