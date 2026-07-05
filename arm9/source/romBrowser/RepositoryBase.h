#pragma once
#include <memory>
#include "SdFolder.h"

class RepositoryBase
{
protected:
    std::unique_ptr<SdFolder> _ndsFolder;
    std::unique_ptr<SdFolder> _gbaFolder;
    std::unique_ptr<SdFolder> _userFolder;

    /// @brief Scans directories and populates the folder cache under the given base path.
    /// @param basePath The base path of the folders.
    void InitializeFolders(const char* basePath);

    /// @brief Gets the cached folder corresponding to the file type suffix name.
    /// @param shortName Suffix folder name (e.g. "nds" or "gba").
    /// @return A pointer to the cached \see SdFolder, or \c nullptr otherwise.
    const SdFolder* GetFileTypeFolder(const char* shortName) const;
};
