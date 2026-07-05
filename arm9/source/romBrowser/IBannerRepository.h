#pragma once

class FileInfo;
class InternalFileInfo;

class IBannerRepository
{
public:
    virtual ~IBannerRepository() = default;

    virtual void Initialize() = 0;
    virtual InternalFileInfo* GetBannerForFile(const FileInfo& fileInfo, const char* gameCode) const = 0;

protected:
    IBannerRepository() = default;
};
