#pragma once
#include <memory>
#include "core/SharedPtr.h"
#include "SdFolder.h"
#include "viewModels/RomBrowserViewModel.h"
#include "RomBrowserStateMachine.h"
#include "core/task/TaskQueue.h"
#include "IRomBrowserController.h"
#include "CoverRepository.h"
#include "FileType/ExtensionFileTypeProvider.h"
#include "services/settings/IAppSettingsService.h"

class RomBrowserController : public IRomBrowserController
{
public:
    RomBrowserController(IAppSettingsService* appSettingsService,
        TaskQueueBase* ioTaskQueue, TaskQueueBase* bgTaskQueue);

    void NavigateUp() override;

    void NavigateToPath(const TCHAR* name) override;
    void LaunchFile(const FileInfo& fileInfo) override;
    void ShowGameInfo() override;
    void HideGameInfo() override;
    void ShowCheats() override;
    void HideCheats() override;
    void ShowCheatDescription(const char* cheatName, const char* description, const char* gameCode, u32 crc,
        int scrollOffset, int cursorIndex, int folderIndex, int rootScrollOffset, int rootCursorIndex,
        bool enabledOnlyMode, int savedViewScrollOffset, int savedViewCursorIndex, int savedViewFolderIndex) override;
    void HideCheatDescription() override;
    void ShowDisplaySettings() override;
    void HideDisplaySettings() override;
    void ToggleFavoritesView() override;
    bool IsFavoritesViewActive() const override { return _favoritesViewActive; }
    void ToggleSelectedFileFavorite() override;
    bool IsSelectedFileFavorite() override;

    bool ConsumeViewModelInvalidated()
    {
        const bool invalidated = _viewModelInvalidated;
        _viewModelInvalidated = false;
        return invalidated;
    }

    void Update() override;

    const SdFolder& GetSdFolder() const override
    {
        return _favoritesViewActive && _favoritesFolder ? *_favoritesFolder : *_sdFolder;
    }

    const RomBrowserStateMachine& GetStateMachine() const override { return _stateMachine; }

    const SharedPtr<RomBrowserViewModel>& GetRomBrowserViewModel() override { return _romBrowserViewModel; }

    TaskQueueBase* GetIoTaskQueue() const override { return _ioTaskQueue; }
    TaskQueueBase* GetBgTaskQueue() const override { return _bgTaskQueue; }
    const ICoverRepository& GetCoverRepository() const override { return *_coverRepository; }

    void SetRomBrowserDisplaySettings(const RomBrowserDisplaySettings& romBrowserDisplaySettings) override;

    void MarkSettingsDirty() override { _saveSettingsPending = true; }

    void RequestThemeReload() override { _themeReloadRequested = true; }

    bool ConsumeThemeReloadRequest()
    {
        const bool requested = _themeReloadRequested;
        _themeReloadRequested = false;
        return requested;
    }

    const RomBrowserDisplaySettings& GetRomBrowserDisplaySettings() const override
    {
        return _appSettingsService->GetAppSettings().romBrowserDisplaySettings;
    }

    const char* GetCheatName() const { return _cheatName; }
    const char* GetCheatDescription() const { return _cheatDescription; }
    const char* GetCheatGameCode() const { return _cheatGameCode; }
    u32 GetCheatCrc() const { return _cheatCrc; }
    int GetCheatFocusScrollOffset() const { return _cheatFocusScrollOffset; }
    int GetCheatFocusCursorIndex() const { return _cheatFocusCursorIndex; }
    int GetCheatFocusFolderIndex() const { return _cheatFocusFolderIndex; }
    int GetCheatFocusRootScrollOffset() const { return _cheatFocusRootScrollOffset; }
    int GetCheatFocusRootCursorIndex() const { return _cheatFocusRootCursorIndex; }
    bool GetCheatFocusEnabledOnlyMode() const { return _cheatFocusEnabledOnlyMode; }
    int GetCheatFocusSavedViewScrollOffset() const { return _cheatFocusSavedViewScrollOffset; }
    int GetCheatFocusSavedViewCursorIndex() const { return _cheatFocusSavedViewCursorIndex; }
    int GetCheatFocusSavedViewFolderIndex() const { return _cheatFocusSavedViewFolderIndex; }

private:
    IAppSettingsService* _appSettingsService;
    TaskQueueBase* _ioTaskQueue;
    TaskQueueBase* _bgTaskQueue;

    std::unique_ptr<SdFolder> _sdFolder;
    std::unique_ptr<SdFolder> _favoritesFolder;
    SharedPtr<RomBrowserViewModel> _romBrowserViewModel;
    std::unique_ptr<SdFolder> _newSdFolder;
    std::unique_ptr<SdFolder> _newFavoritesFolder;
    RomBrowserStateMachine _stateMachine;
    TCHAR _navigatePath[256];
    TCHAR* _navigateFileName;
    FileInfo _launchFileInfo;
    char _cheatName[128];
    char _cheatDescription[512];
    char _cheatGameCode[5];
    u32 _cheatCrc;
    int _cheatFocusScrollOffset = 0;
    int _cheatFocusCursorIndex = 0;
    int _cheatFocusFolderIndex = -1;
    int _cheatFocusRootScrollOffset = 0;
    int _cheatFocusRootCursorIndex = 0;
    bool _cheatFocusEnabledOnlyMode = false;
    int _cheatFocusSavedViewScrollOffset = 0;
    int _cheatFocusSavedViewCursorIndex = 0;
    int _cheatFocusSavedViewFolderIndex = -1;
    QueueTask<void> _navigateTask;
    QueueTask<void> _favoritesTask;
    bool _favoritesViewActive = false;
    bool _favoritesLoadPending = false;
    bool _saveSettingsPending = false;
    bool _viewModelInvalidated = false;
    bool _themeReloadRequested = false;
    std::unique_ptr<CoverRepository> _coverRepository;
    ExtensionFileTypeProvider _fileTypeProvider;

    void HandleTrigger();
    void HandleNavigateTrigger();
    void HandleFolderLoadDoneTrigger();
    void HandleLaunchTrigger();
    void HandleChangeDisplayModeTrigger();
    void StartFavoritesLoad();
    void CompleteFavoritesLoad();
    bool TryBuildFilePath(const FileInfo& fileInfo, char* outPath, u32 outPathSize) const;
    std::unique_ptr<SdFolder> BuildFavoritesFolder();
    bool TryCreateFileInfoFromPath(const char* fullPath, FileInfo*& outFileInfo) const;
    bool IsFavoritePath(const char* fullPath) const;
    void AddFavoritePath(const char* fullPath);
    void RemoveFavoritePath(const char* fullPath);
    void SaveSettingsAsync();
    const FileInfo* GetSelectedFileInfo() const;
};
