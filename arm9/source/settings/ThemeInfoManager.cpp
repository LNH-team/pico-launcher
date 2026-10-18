#include "common.h"
#include <nds/arm9/cache.h>
#include "core/mini-printf.h"
#include "fat/Directory.h"
#include "fat/File.h"
#include "ThemeInfoManager.h"

ThemeInfoManager::ThemeInfoManager(const ThemeRepository& themeRepository)
    : _themeRepository(themeRepository)
    , _extraThemeInfo(std::make_unique<AtomicSharedPtr<ExtraThemeInfo>[]>(themeRepository.GetThemeCount())) { }

void ThemeInfoManager::LoadThemeInfo(int index)
{
    auto extraThemeInfo = SharedPtr<ExtraThemeInfo>::MakeShared();
    extraThemeInfo->themeInfo = _themeRepository.LoadThemeInfo(index);
    if (extraThemeInfo->themeInfo)
    {
        LoadThemeIcon(extraThemeInfo);
        LoadThemePreviewImage(extraThemeInfo);
        _extraThemeInfo[index] = std::move(extraThemeInfo);
    }
}

void ThemeInfoManager::ReleaseThemeInfo(int index)
{
    _extraThemeInfo[index].Reset();
}

void ThemeInfoManager::LoadThemeIcon(SharedPtr<ExtraThemeInfo>& extraThemeInfo)
{
    char pathBuffer[256];
    mini_snprintf(pathBuffer, sizeof(pathBuffer), "/_pico/themes/%s", extraThemeInfo->themeInfo->GetFolderName());
    auto folderDir = std::make_unique<Directory>();
    if (folderDir->Open(pathBuffer) == FR_OK)
    {
        FILINFO folderFileInfo;
        while (folderDir->Read(&folderFileInfo) == FR_OK && folderFileInfo.fname[0] != 0)
        {
            if (!(folderFileInfo.fattrib & AM_DIR) && !strcasecmp(folderFileInfo.fname, "icon.bmp"))
            {
                auto iconData = SharedPtr<BmpFileIconData>::MakeShared(
                    FastFileRef(folderDir->GetFatFsDirectory(), &folderFileInfo));
                if (iconData->IsValid())
                {
                    extraThemeInfo->iconData = std::move(iconData);
                }
                break;
            }
        }
    }
}

void ThemeInfoManager::LoadThemePreviewImage(SharedPtr<ExtraThemeInfo>& extraThemeInfo)
{
    bool previewOk = false;
    char pathBuffer[256];
    mini_snprintf(pathBuffer, sizeof(pathBuffer), "/_pico/themes/%s/preview.bin", extraThemeInfo->themeInfo->GetFolderName());
    auto file = std::make_unique<File>();
    if (file->Open(pathBuffer, FA_READ) == FR_OK &&
        file->ReadExact(extraThemeInfo->previewImage, 256 * 192 * 2))
    {
        previewOk = true;
    }
    file->Close();

    if (!previewOk)
    {
        if (extraThemeInfo->themeInfo->GetType() == ThemeType::Custom)
        {
            mini_snprintf(pathBuffer, sizeof(pathBuffer), "/_pico/themes/%s/topbg.bin", extraThemeInfo->themeInfo->GetFolderName());
            if (file->Open(pathBuffer, FA_READ) == FR_OK &&
                file->ReadExact(extraThemeInfo->previewImage, 256 * 192 * 2))
            {
                previewOk = true;
            }
        }
    }

    if (!previewOk)
    {
        memset(extraThemeInfo->previewImage, 0, 256 * 192 * 2);
    }

    DC_FlushRange(extraThemeInfo->previewImage, 256 * 192 * 2);
}
