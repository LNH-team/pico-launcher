#pragma once
#include "core/SharedPtr.h"
#include "FileType/BmpFileIconData.h"

class FileInfo;
class InternalFileInfo;

class IIconRepository
{
public:
    virtual ~IIconRepository() = 0;

    virtual void Initialize() = 0;
    virtual SharedPtr<BmpFileIconData> LoadIconData(
        const FileInfo& fileInfo, const InternalFileInfo* internalFileInfo) const = 0;
};

inline IIconRepository::~IIconRepository() { }
