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

    /// @brief Returns whether this file info contains raw icon data.
    /// @return True when raw icon data is available, or false otherwise.
    virtual bool HasIconData() const { return false; }

    /// @brief Returns the raw 4 bpp tile graphics of the icon.
    /// @return A pointer to the icon graphics if available, or \c nullptr otherwise.
    virtual const u8* GetIconGfx() const { return nullptr; }

    /// @brief Returns the 16-color XBGR555 palette of the icon for cover rendering.
    /// @return A pointer to the icon palette if available, or \c nullptr otherwise.
    virtual const u16* GetIconPalette() const { return nullptr; }
};

inline InternalFileInfo::~InternalFileInfo() { }
