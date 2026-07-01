#pragma once
#include <memory>
#include "fat/ff.h"
#include "fat/FastFileRef.h"
#include "FileTypeClassification.h"
#include "FileIcon.h"
#include "FileCover.h"
#include "UnknownFileCover.h"
#include "picoLoaderBootstrap.h"

class FileInfo;
class InternalFileInfo;
class IThemeFileIconFactory;
class ICoverRepository;

/// @brief Abstract base class representing a file type.
class FileType
{
public:
    virtual ~FileType() = 0;

    /// @brief Returns the short name for this file type.
    /// @return The short name for this file type.
    constexpr const char* GetShortName() const { return _shortName; }

    /// @brief Returns the classification for this file type.
    /// @return The classification for this file type.
    constexpr FileTypeClassification GetClassification() const { return _classification; }

    /// @brief Creates a generic file icon for a file of this file type.
    /// @param fileName The name of the file.
    /// @param themeFileIconFactory The theme file icon factory to use.
    /// @return The constructed file icon, or \c nullptr when no file icon for this file type is available.
    virtual std::unique_ptr<FileIcon> CreateFileIcon(const TCHAR* fileName,
        const IThemeFileIconFactory* themeFileIconFactory) const
    {
        return nullptr;
    };

    /// @brief Creates a generic file cover for a file of this file type.
    /// @param fileName The name of the file.
    /// @return The constructed file cover.
    virtual FileCover* CreateFileCover(const TCHAR* fileName) const
    {
        return new UnknownFileCover();
    };

    /// @brief Reads the internal file info of the file specified by \p fastFileRef.
    /// @param fastFileRef The file to get the internal file info of.
    /// @return The internal file info of the specified file when successful, or \c nullptr otherwise.
    virtual InternalFileInfo* CreateInternalFileInfo(const FastFileRef& fastFileRef) const { return nullptr; }

    /// @brief Tries to set the launch parameters to launch a file of this file type at the specified \p filePath.
    /// @param launchParameters The launch parameters to set.
    /// @param filePath The file that is going to be launched.
    /// @return \c true when the launch parameters were set successfully, or \c false otherwise.
    virtual bool TrySetLaunchParameters(pload_params_t* launchParameters, const char* filePath) const
    {
        return false;
    }

protected:
    constexpr FileType(const char* shortName, FileTypeClassification classification)
        : _shortName(shortName), _classification(classification) { }

private:
    const char* _shortName;
    FileTypeClassification _classification;
};

inline FileType::~FileType() { }
