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

    const RomBrowserDisplaySettings& GetRomBrowserDisplaySettings() const override
    {
        return _appSettingsService->GetAppSettings().romBrowserDisplaySettings;
    }

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
    QueueTask<void> _navigateTask;
    QueueTask<void> _favoritesTask;
    bool _favoritesViewActive = false;
    bool _favoritesLoadPending = false;
    bool _saveSettingsPending = false;
    bool _viewModelInvalidated = false;
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
