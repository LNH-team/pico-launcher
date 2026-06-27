#pragma once
#include <memory>
#include "FileIcon.h"
#include "FileCover.h"

/// @brief Abstract base class representing internal file information.
class InternalFileInfo
{
public:
    virtual ~InternalFileInfo() = 0;

    /// @brief Returns the game code of the file.
    /// @return A pointer to the game code if available, or \c nullptr otherwise.
    virtual const char* GetGameCode() const { return nullptr; }

    /// @brief Returns the game title of the file.
    /// @return A pointer to the game title if available, or \c nullptr otherwise.
    virtual const char16_t* GetGameTitle() const { return nullptr; }

    /// @brief Creates an icon based on the internal file information.
    /// @return A unique pointer to the created icon when successful, or \c nullptr otherwise.
    virtual std::unique_ptr<FileIcon> CreateGameIcon() const { return nullptr; }

    /// @brief Creates a cover based on the internal file information.
    /// @return A unique pointer to the created cover when successful, or \c nullptr otherwise.
    virtual FileCover* CreateGameCover() const { return nullptr; }
};

inline InternalFileInfo::~InternalFileInfo() { }