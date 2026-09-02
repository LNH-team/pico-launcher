#pragma once
#include <memory>
#include <string.h>
#include "core/SharedPtr.h"
#include "SdFolder.h"
#include "viewModels/RomBrowserViewModel.h"
#include "RomBrowserStateMachine.h"
#include "core/task/TaskQueue.h"
#include "IRomBrowserController.h"
#include "CoverRepository.h"
#include "IconRepository.h"
#include "BannerRepository.h"
#include "FileType/ExtensionFileTypeProvider.h"
#include "services/settings/IAppSettingsService.h"
#include "services/favorites/IFavoritesService.h"
#include "cheats/ICheatRepository.h"

class RomBrowserController : public IRomBrowserController
{
public:
    RomBrowserController(IAppSettingsService* appSettingsService, IFavoritesService* favoritesService,
        TaskQueueBase* ioTaskQueue, TaskQueueBase* bgTaskQueue);

    void NavigateUp() override
    {
        NavigateToPath(strcmp(_navigatePath, ":favorites") == 0 ? "." : "..");
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
    void GotoSettingsScreen() override;

    void Update() override;

    const SdFolder& GetSdFolder() const override { return *_sdFolder; }
    const TCHAR* GetCurrentPath() const override { return _navigatePath; }

    const RomBrowserStateMachine& GetStateMachine() const override { return _stateMachine; }

    const SharedPtr<RomBrowserViewModel>& GetRomBrowserViewModel() override { return _romBrowserViewModel; }

    TaskQueueBase* GetIoTaskQueue() const override { return _ioTaskQueue; }
    TaskQueueBase* GetBgTaskQueue() const override { return _bgTaskQueue; }
    const ICoverRepository& GetCoverRepository() const override { return *_coverRepository; }
    const IIconRepository& GetIconRepository() const override { return *_iconRepository; }
    const IBannerRepository& GetBannerRepository() const override { return *_bannerRepository; }
    const ICheatRepository& GetCheatRepository() const override { return *_cheatRepository; }

    void SetRomBrowserDisplaySettings(const RomBrowserDisplaySettings& romBrowserDisplaySettings) override;

    const RomBrowserDisplaySettings& GetRomBrowserDisplaySettings() const override
    {
        return _appSettingsService->GetAppSettings().romBrowserDisplaySettings;
    }

    virtual const FileInfo& GetTriggerFileInfo() const override { return _triggerFileInfo; }

private:
    IAppSettingsService* _appSettingsService;
    IFavoritesService* _favoritesService;
    TaskQueueBase* _ioTaskQueue;
    TaskQueueBase* _bgTaskQueue;

    std::unique_ptr<SdFolder> _sdFolder;
    SharedPtr<RomBrowserViewModel> _romBrowserViewModel;
    std::unique_ptr<SdFolder> _newSdFolder;
    RomBrowserStateMachine _stateMachine;
    TCHAR _navigatePath[256];
    TCHAR* _navigateFileName;
    /// @brief Full path of the item to select once the folder finishes loading, or nullptr.
    ///        Only the favorites view sets it - browsed folders select by file name.
    const TCHAR* _navigateFullPath = nullptr;
    /// @brief Full path handed over by a favorites restore, consumed by the next favorites
    ///        navigation. Empty when opening the favorites list by hand, so a regular open
    ///        still lands at the top, exactly like entering a folder does.
    TCHAR _pendingFavoriteSelectionPath[256] = { 0 };
    /// @brief Storage backing _navigateFullPath while a folder load is in flight. The
    ///        pending path is copied here before it is cleared, so the pointer handed to the
    ///        view model cannot be truncated by that clear.
    TCHAR _navigateFavoritePath[256] = { 0 };
    int _pendingFavoriteScrollOffset = 0;
    int _navigateScrollOffset = 0;
    FileInfo _triggerFileInfo;
    QueueTask<void> _navigateTask;
    bool _saveSettingsPending = false;
    volatile bool _isAtRoot = false;
    std::unique_ptr<CoverRepository> _coverRepository;
    std::unique_ptr<IconRepository> _iconRepository;
    std::unique_ptr<BannerRepository> _bannerRepository;
    ExtensionFileTypeProvider _fileTypeProvider;
    std::unique_ptr<ICheatRepository> _cheatRepository;

    void HandleTrigger();
    void HandleNavigateTrigger();
    void HandleFolderLoadDoneTrigger();
    void HandleLaunchTrigger();
    void HandleChangeDisplayModeTrigger();
    void HandleGotoSettingsScreenTrigger();
    /// @brief Builds the full path of a browsed or favorites-view item into pathBuffer.
    /// @return false when the path did not fit the buffer, in which case pathBuffer holds a
    ///         truncated path that must not be used as a favorites key: a prefix can name a
    ///         different file, so storing it would favorite the wrong item.
    bool GetFileInfoPath(const FileInfo& fileInfo, char* pathBuffer, u32 bufferSize) const;
    void PreserveFavoriteSelectionAfterRemoval(const FileInfo& fileInfo);
    void ToggleFavoriteAtPath(const char* path);
    void RemoveFavoriteAtPath(const char* path);
    void UpdateLastUsedFilepath();
    void SetPicoLoaderParams() const;
    void LoadCheats() const;
};
