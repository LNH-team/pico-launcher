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
#include "cheats/ICheatRepository.h"

class RomBrowserController : public IRomBrowserController
{
public:
    RomBrowserController(IAppSettingsService* appSettingsService,
        TaskQueueBase* ioTaskQueue, TaskQueueBase* bgTaskQueue);

    void NavigateUp() override
    {
        NavigateToPath("..");
    }

    void NavigateToPath(const TCHAR* name) override;
    bool IsAtRoot() const override { return _isAtRoot; }
    void LaunchFile(const FileInfo& fileInfo) override;
    void ShowGameInfo(const FileInfo& fileInfo) override;
    void HideGameInfo() override;
    bool IsFavorite(const FileInfo& fileInfo) const override;
    void ToggleFavorite(const FileInfo& fileInfo) override;
    void ShowDisplaySettings() override;
    void HideDisplaySettings() override;

    void Update() override;

    const SdFolder& GetSdFolder() const override { return *_sdFolder; }
    const TCHAR* GetCurrentPath() const override { return _navigatePath; }

    const RomBrowserStateMachine& GetStateMachine() const override { return _stateMachine; }

    const SharedPtr<RomBrowserViewModel>& GetRomBrowserViewModel() override { return _romBrowserViewModel; }

    TaskQueueBase* GetIoTaskQueue() const override { return _ioTaskQueue; }
    TaskQueueBase* GetBgTaskQueue() const override { return _bgTaskQueue; }
    const ICoverRepository& GetCoverRepository() const override { return *_coverRepository; }
    const ICheatRepository& GetCheatRepository() const override { return *_cheatRepository; }

    void SetRomBrowserDisplaySettings(const RomBrowserDisplaySettings& romBrowserDisplaySettings) override;

    const RomBrowserDisplaySettings& GetRomBrowserDisplaySettings() const override
    {
        return _appSettingsService->GetAppSettings().romBrowserDisplaySettings;
    }

    virtual const FileInfo& GetTriggerFileInfo() const override { return _triggerFileInfo; }

private:
    IAppSettingsService* _appSettingsService;
    TaskQueueBase* _ioTaskQueue;
    TaskQueueBase* _bgTaskQueue;

    std::unique_ptr<SdFolder> _sdFolder;
    SharedPtr<RomBrowserViewModel> _romBrowserViewModel;
    std::unique_ptr<SdFolder> _newSdFolder;
    RomBrowserStateMachine _stateMachine;
    TCHAR _navigatePath[256];
    TCHAR* _navigateFileName;
    FileInfo _triggerFileInfo;
    QueueTask<void> _navigateTask;
    bool _saveSettingsPending = false;
    std::unique_ptr<String<char, 256>[]> _newFavoritesSurvivors;
    u32 _newFavoritesSurvivorCount = 0;
    bool _favoritesPruneNeeded = false;
    bool _isAtRoot = false;
    std::unique_ptr<CoverRepository> _coverRepository;
    ExtensionFileTypeProvider _fileTypeProvider;
    std::unique_ptr<ICheatRepository> _cheatRepository;

    void HandleTrigger();
    void HandleNavigateTrigger();
    void HandleFolderLoadDoneTrigger();
    void HandleLaunchTrigger();
    void HandleChangeDisplayModeTrigger();
    void GetFileInfoPath(const FileInfo& fileInfo, char* pathBuffer, u32 bufferSize) const;
    void UpdateLastUsedFilepath();
    void SetPicoLoaderParams() const;
    void LoadCheats() const;
};
