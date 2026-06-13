#pragma once

class FileInfo;
class InternalFileInfo;

class IBannerRepository
{
public:
    virtual ~IBannerRepository() = 0;

    virtual void Initialize() = 0;
    virtual InternalFileInfo* GetBannerForFile(
        const FileInfo& fileInfo, const InternalFileInfo* internalFileInfo) const = 0;
};

inline IBannerRepository::~IBannerRepository() { }
