#pragma once
#include "../FileType.h"
#include "GbaInternalFileInfo.h"
#include "core/StringUtil.h"
#include "../../Theme/IThemeFileIconFactory.h"

/// @brief File type for gba roms.
class GbaFileType : public FileType
{
public:
    static const GbaFileType sInstance;

    std::unique_ptr<FileIcon> CreateFileIcon(const TCHAR* fileName,
        const IThemeFileIconFactory* themeFileIconFactory) const override
    {
        return themeFileIconFactory->CreateGenericFileIcon(fileName);
    }

    InternalFileInfo* CreateInternalFileInfo(const FastFileRef& fastFileRef) const override
    {
        return new GbaInternalFileInfo(fastFileRef);
    }

private:
    constexpr GbaFileType()
        : FileType("gba", FileTypeClassification::Game) { }
};