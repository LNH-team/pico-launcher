#pragma once
#include "core/SharedPtr.h"
#include "services/settings/AppSettings.h"

class SdFolder;
class RomBrowserStateMachine;
class RomBrowserViewModel;
class FileInfo;
class TaskQueueBase;
class ICoverRepository;

class IRomBrowserController
{
public:
    virtual ~IRomBrowserController() = 0;

    virtual void NavigateUp() = 0;
    virtual void NavigateToPath(const TCHAR* name) = 0;
    virtual void LaunchFile(const FileInfo& fileInfo) = 0;
    virtual void ShowGameInfo() = 0;
    virtual void HideGameInfo() = 0;
    virtual void ShowDisplaySettings() = 0;
    virtual void HideDisplaySettings() = 0;
    virtual void ToggleFavoritesView() = 0;
    virtual bool IsFavoritesViewActive() const = 0;
    virtual void ToggleSelectedFileFavorite() = 0;
    virtual bool IsSelectedFileFavorite() = 0;

    virtual void Update() = 0;

    virtual const SdFolder& GetSdFolder() const = 0;

    virtual const RomBrowserStateMachine& GetStateMachine() const = 0;

    virtual const SharedPtr<RomBrowserViewModel>& GetRomBrowserViewModel() = 0;

    virtual TaskQueueBase* GetIoTaskQueue() const = 0;
    virtual TaskQueueBase* GetBgTaskQueue() const = 0;
    virtual const ICoverRepository& GetCoverRepository() const = 0;

    virtual const RomBrowserDisplaySettings& GetRomBrowserDisplaySettings() const = 0;

    virtual void SetRomBrowserDisplaySettings(
        const RomBrowserDisplaySettings& romBrowserDisplaySettings) = 0;

    virtual void MarkSettingsDirty() = 0;

    virtual void RequestThemeReload() = 0;

    virtual void ShowCheats() = 0;
    virtual void HideCheats() = 0;
    virtual void ShowCheatDescription(const char* cheatName, const char* description, const char* gameCode, u32 crc,
        int scrollOffset, int cursorIndex, int folderIndex, int rootScrollOffset, int rootCursorIndex,
        bool enabledOnlyMode, int savedViewScrollOffset, int savedViewCursorIndex, int savedViewFolderIndex) = 0;
    virtual void HideCheatDescription() = 0;
};

inline IRomBrowserController::~IRomBrowserController() { }
