#pragma once
#include "core/SharedPtr.h"
#include "services/settings/AppSettings.h"

class SdFolder;
class RomBrowserStateMachine;
class RomBrowserViewModel;
class FileInfo;
class TaskQueueBase;
class ICoverRepository;
class ICheatRepository;

class IRomBrowserController
{
public:
    virtual ~IRomBrowserController() = 0;

    virtual void NavigateUp() = 0;
    virtual void NavigateToPath(const TCHAR* name) = 0;
    virtual void LaunchFile(const FileInfo& fileInfo) = 0;
    virtual void ShowGameInfo(const FileInfo& fileInfo) = 0;
    virtual void HideGameInfo() = 0;
    virtual void ShowDisplaySettings() = 0;
    virtual void HideDisplaySettings() = 0;
    virtual void ShowDisplayInfo() = 0;
    virtual void HideDisplayInfo() = 0;
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
    virtual const ICheatRepository& GetCheatRepository() const = 0;

    virtual const RomBrowserDisplaySettings& GetRomBrowserDisplaySettings() const = 0;

    virtual void SetRomBrowserDisplaySettings(
        const RomBrowserDisplaySettings& romBrowserDisplaySettings) = 0;

    virtual void MarkSettingsDirty() = 0;

    /// @brief Immediately serializes and enqueues an async write of settings.json.
    virtual void SaveSettingsNow() = 0;

    /// @brief Signals that state.bin should be saved.
    virtual void MarkStateDirty() = 0;

    virtual void RequestThemeReload() = 0;
    virtual void RequestChangeDisplayMode() = 0;

    virtual void ShowCheats() = 0;
    virtual void HideCheats() = 0;
    virtual void ShowCheatDescription(const char* cheatName, const char* description, const char* gameCode, u32 crc,
        int scrollOffset, int cursorIndex, int folderIndex, int rootScrollOffset, int rootCursorIndex,
        bool enabledOnlyMode, int savedViewScrollOffset, int savedViewCursorIndex, int savedViewFolderIndex) = 0;
    virtual void HideCheatDescription() = 0;

    virtual void ShowLayoutEditor() = 0;
    virtual void HideLayoutEditor() = 0;

    virtual void ShowQuickMenu() = 0;
    virtual void HideQuickMenu() = 0;

    virtual void SetDirectMenuAccess(bool direct) = 0;
    virtual bool ConsumeDirectMenuAccess() = 0;

    virtual const FileInfo& GetTriggerFileInfo() const = 0;
    virtual void SetActiveFile(const FileInfo& fileInfo) = 0;

    enum class QuickMenuAction
    {
        None,
        GameDetails,
        Cheats,
        DisplaySettings,
        LayoutEditor,
        Information
    };
    virtual void SetQuickMenuAction(QuickMenuAction action) = 0;
    virtual QuickMenuAction ConsumeQuickMenuAction() = 0;
};

inline IRomBrowserController::~IRomBrowserController() { }
