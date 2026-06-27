#pragma once
#include <memory>
#include "SdFolder.h"

class RepositoryBase
{
protected:
    void InitializeFolders(const char* basePath);
    const SdFolder* GetSystemFolder(const char* shortName) const;

    std::unique_ptr<SdFolder> _ndsFolder;
    std::unique_ptr<SdFolder> _gbaFolder;
    std::unique_ptr<SdFolder> _userFolder;
};
