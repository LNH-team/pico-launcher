#pragma once
#include "InternalFileInfo.h"
#include "BmpFileIcon.h"
#include "BmpFileIconData.h"
#include "core/SharedPtr.h"

/// @brief Wraps any InternalFileInfo and overrides its icon with a custom BMP icon.
class CustomIconInternalFileInfo : public InternalFileInfo
{
public:
    CustomIconInternalFileInfo(SharedPtr<BmpFileIconData> iconData, const InternalFileInfo* wrapped)
        : _iconData(std::move(iconData)), _wrapped(wrapped) {}

    ~CustomIconInternalFileInfo() { delete _wrapped; }

    const char* GetGameCode() const override { return _wrapped ? _wrapped->GetGameCode() : nullptr; }
    const char16_t* GetGameTitle() const override { return _wrapped ? _wrapped->GetGameTitle() : nullptr; }
    FileCover* CreateGameCover() const override { return _wrapped ? _wrapped->CreateGameCover() : nullptr; }

    std::unique_ptr<FileIcon> CreateGameIcon() const override
    {
        return std::make_unique<BmpFileIcon>(_iconData);
    }

private:
    SharedPtr<BmpFileIconData> _iconData;
    const InternalFileInfo* _wrapped;
};
