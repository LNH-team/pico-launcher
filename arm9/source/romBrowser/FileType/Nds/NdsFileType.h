#pragma once
#include "../FileType.h"
#include "NdsInternalFileInfo.h"
#include "core/StringUtil.h"
#include "../../Theme/IThemeFileIconFactory.h"

/// @brief File type for nds roms.
class NdsFileType : public FileType
{
public:
    static const NdsFileType sInstance;

    std::unique_ptr<FileIcon> CreateFileIcon(const TCHAR* fileName,
        const IThemeFileIconFactory* themeFileIconFactory) const override
    {
        return themeFileIconFactory->CreateNdsFileIcon(fileName);
    }

    InternalFileInfo* CreateInternalFileInfo(const FastFileRef& fastFileRef) const override
    {
        return new NdsInternalFileInfo(fastFileRef);
    }

    bool TrySetLaunchParameters(pload_params_t* launchParameters, const char* filePath) const override
    {
        StringUtil::Copy(launchParameters->romPath, filePath, sizeof(launchParameters->romPath));
        return true;
    }

private:
    constexpr NdsFileType()
        : FileType("nds", FileTypeClassification::Game) { }

};