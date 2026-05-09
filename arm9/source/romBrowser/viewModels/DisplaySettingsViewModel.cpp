#include "common.h"
#include <memory>
#include <string.h>
#include "DisplaySettingsViewModel.h"
#include "romBrowser/FileInfo.h"
#include "romBrowser/FileType/NullFileTypeProvider.h"
#include "romBrowser/SdFolderFactory.h"
#include "themes/ThemeInfoFactory.h"

#define THEMES_PATH "/_pico/themes"

static bool isThemeFolder(const FileInfo* fileInfo, ThemeInfoFactory& themeInfoFactory)
{
    return fileInfo && themeInfoFactory.CreateFromThemeFolder(fileInfo->GetFileName()) != nullptr;
}

void DisplaySettingsViewModel::SetTheme(const String<char, 64>& theme)
{
    if (strcasecmp(_appSettings->theme.GetString(), theme.GetString()) != 0)
    {
        _romBrowserController->SetTheme(theme);
    }
}

void DisplaySettingsViewModel::SelectPreviousTheme()
{
    SelectRelativeTheme(-1);
}

void DisplaySettingsViewModel::SelectNextTheme()
{
    SelectRelativeTheme(1);
}

void DisplaySettingsViewModel::SelectRelativeTheme(int offset)
{
    if (offset == 0)
    {
        return;
    }

    NullFileTypeProvider fileTypeProvider;
    auto themesFolder = SdFolderFactory(&fileTypeProvider).CreateFromPath(THEMES_PATH);
    if (!themesFolder)
    {
        return;
    }

    int folderCount = 0;
    auto sortedFolders = themesFolder->FilterAndSort(SdFolderFilterSortParams(), folderCount);
    if (folderCount == 0)
    {
        return;
    }

    auto themeFolders = std::make_unique<const FileInfo*[]>(folderCount);
    ThemeInfoFactory themeInfoFactory;
    int themeCount = 0;
    int currentThemeIndex = -1;
    for (int i = 0; i < folderCount; i++)
    {
        const FileInfo* folder = sortedFolders[i];
        if (!isThemeFolder(folder, themeInfoFactory))
        {
            continue;
        }

        if (strcasecmp(folder->GetFileName(), GetTheme()) == 0)
        {
            currentThemeIndex = themeCount;
        }
        themeFolders[themeCount++] = folder;
    }

    if (themeCount == 0)
    {
        return;
    }

    int nextThemeIndex = currentThemeIndex + offset;
    if (currentThemeIndex < 0)
    {
        nextThemeIndex = offset > 0 ? 0 : themeCount - 1;
    }
    else if (nextThemeIndex < 0)
    {
        nextThemeIndex += themeCount;
    }
    else if (nextThemeIndex >= themeCount)
    {
        nextThemeIndex -= themeCount;
    }

    SetTheme(String<char, 64>(themeFolders[nextThemeIndex]->GetFileName()));
}
