#include "common.h"
#include <array>
#include <libtwl/rtos/rtosIrq.h>
#include "picoLoaderBootstrap.h"
#include "PicoLoaderProcess.h"
#include "settings/SettingsProcess.h"
#include "FileType/ExtensionFileTypeProvider.h"
#include "FileType/FileType.h"
#include "SdFolderFactory.h"
#include "services/settings/IAppSettingsService.h"
#include "cheats/UsrCheatRepositoryFactory.h"
#include "cheats/EmptyCheatRepository.h"
#include "cheats/PicoLoaderCheatDataFactory.h"
#include "RomBrowserController.h"

RomBrowserController::RomBrowserController(
    IAppSettingsService* appSettingsService, IFavoritesService* favoritesService,
    TaskQueueBase* ioTaskQueue, TaskQueueBase* bgTaskQueue)
    : _appSettingsService(appSettingsService)
    , _favoritesService(favoritesService)
    , _ioTaskQueue(ioTaskQueue), _bgTaskQueue(bgTaskQueue)
    , _fileTypeProvider(appSettingsService->GetAppSettings()) { }

void RomBrowserController::NavigateToPath(const TCHAR* name)
{
    StringUtil::Copy(_navigatePath, name, sizeof(_navigatePath) / sizeof(_navigatePath[0]));
    _stateMachine.Fire(RomBrowserStateTrigger::Navigate);
}

void RomBrowserController::LaunchFile(const FileInfo& fileInfo)
{
    _triggerFileInfo = FileInfo(fileInfo);
    _stateMachine.Fire(RomBrowserStateTrigger::Launch);
}

void RomBrowserController::ShowGameInfo(const FileInfo& fileInfo)
{
    _triggerFileInfo = FileInfo(fileInfo);
    _stateMachine.Fire(RomBrowserStateTrigger::ShowGameInfo);
}

void RomBrowserController::HideGameInfo()
{
    _stateMachine.Fire(RomBrowserStateTrigger::HideGameInfo);
}

void RomBrowserController::ShowDisplaySettings()
{
    _stateMachine.Fire(RomBrowserStateTrigger::ShowDisplaySettings);
}

void RomBrowserController::HideDisplaySettings()
{
    if (_saveSettingsPending)
    {
        _saveSettingsPending = false;
        _ioTaskQueue->Enqueue([this] (const vu8& cancelRequested)
        {
            _appSettingsService->Save();
            return TaskResult<void>::Completed();
        });
    }
    _stateMachine.Fire(RomBrowserStateTrigger::HideDisplaySettings);
}

void RomBrowserController::GotoSettingsScreen()
{
    _stateMachine.Fire(RomBrowserStateTrigger::GotoSettingsScreen);
}

void RomBrowserController::SetRomBrowserDisplaySettings(
    const RomBrowserDisplaySettings& romBrowserDisplaySettings)
{
    _appSettingsService->GetAppSettings().romBrowserDisplaySettings = romBrowserDisplaySettings;
    _saveSettingsPending = true;
    _stateMachine.Fire(RomBrowserStateTrigger::ChangeDisplayMode);
}

void RomBrowserController::Update()
{
    _stateMachine.Update();
    if (_stateMachine.HasStateChanged())
    {
        HandleTrigger();
    }
    switch (_stateMachine.GetCurrentState())
    {
        case RomBrowserState::Start:
        {
            LOG_DEBUG("RomBrowserState::Start\n");
            const auto& lastUsed = _appSettingsService->GetAppSettings().lastUsedFilePath;
            if (strlen(lastUsed.GetString()) != 0)
            {
                NavigateToPath(lastUsed.GetString());
            }
            else
            {
                NavigateToPath("/");
            }
            break;
        }
        case RomBrowserState::LoadingFolder:
        {
            if (_navigateTask.GetTask().IsCompletedSuccessfully())
            {
                _navigateTask.Dispose();
                _stateMachine.Fire(RomBrowserStateTrigger::FolderLoadDone);
            }
            break;
        }
        case RomBrowserState::Launching:
        default:
        {
            break;
        }
    }
}

void RomBrowserController::HandleTrigger()
{
    switch (_stateMachine.GetLastTrigger())
    {
        case RomBrowserStateTrigger::Navigate:
            HandleNavigateTrigger();
            break;

        case RomBrowserStateTrigger::FolderLoadDone:
            HandleFolderLoadDoneTrigger();
            break;

        case RomBrowserStateTrigger::Launch:
            HandleLaunchTrigger();
            break;

        case RomBrowserStateTrigger::ChangeDisplayMode:
            HandleChangeDisplayModeTrigger();
            break;

        case RomBrowserStateTrigger::GotoSettingsScreen:
            HandleGotoSettingsScreenTrigger();
            break;

        default:
            break;
    }
}

void RomBrowserController::HandleNavigateTrigger()
{
    LOG_DEBUG("RomBrowserStateTrigger::Navigate\n");

    std::unique_ptr<String<char, 256>[]> favoritesSnapshot;
    u32 favoritesSnapshotCount = 0;
    if (strcmp(_navigatePath, ":favorites") == 0)
    {
        // This runs on the main thread while ToggleFavoriteAtPath()/RemoveFavoriteAtPath() can
        // run on the io thread (or the main thread itself, for favorites-view toggles) - same
        // shape as IsFavorite()'s guard: never allocate with IRQs disabled, only guard the
        // actual array reads, and retry if the count grew between sizing and copying.
        for (;;)
        {
            u32 capacity;
            {
                u32 irq = rtos_disableIrqs();
                capacity = _favoritesService->GetFavorites().numberOfFavorites;
                rtos_restoreIrqs(irq);
            }

            favoritesSnapshot = std::make_unique_for_overwrite<String<char, 256>[]>(capacity);

            u32 irq = rtos_disableIrqs();
            const auto& favorites = _favoritesService->GetFavorites();
            favoritesSnapshotCount = favorites.numberOfFavorites;
            if (favoritesSnapshotCount > capacity)
            {
                rtos_restoreIrqs(irq);
                continue;
            }
            for (u32 i = 0; i < favoritesSnapshotCount; i++)
            {
                favoritesSnapshot[i] = favorites.favorites[i];
            }
            rtos_restoreIrqs(irq);
            break;
        }
    }

    _navigateTask = _ioTaskQueue->Enqueue(
        [this, favoritesSnapshot = std::move(favoritesSnapshot), favoritesSnapshotCount] (const vu8& cancelRequested)
    {
        if (!_coverRepository)
        {
            _coverRepository = std::make_unique<CoverRepository>();
            _coverRepository->Initialize();
        }
        if (!_iconRepository)
        {
            _iconRepository = std::make_unique<IconRepository>();
            _iconRepository->Initialize();
        }
        if (!_bannerRepository)
        {
            _bannerRepository = std::make_unique<BannerRepository>();
            _bannerRepository->Initialize();
        }
        if (!_cheatRepository)
        {
            _cheatRepository = UsrCheatRepositoryFactory().FromUsrCheatDat("/_pico/usrcheat.dat");
            if (!_cheatRepository)
            {
                // When usrcheat.dat is not found or cannot be read use a dummy empty cheat repository
                _cheatRepository = std::make_unique<EmptyCheatRepository>();
            }
        }

        u64 startTick = gTickCounter.GetValue();
        _navigateFileName = nullptr;
        if (strcmp(_navigatePath, ":favorites") == 0)
        {
            auto& settings = _appSettingsService->GetAppSettings();
            if (strcmp(settings.lastUsedFilePath.GetString(), ":favorites") != 0)
            {
                settings.lastUsedFilePath = ":favorites";
                _appSettingsService->Save();
            }

            DIR dir;
            FATFS* fs = nullptr;
            if (f_opendir(&dir, "/") == FR_OK)
            {
                fs = dir.obj.fs;
                f_closedir(&dir);
            }

            u32 count = 0;
            FileInfo** fileInfos = (FileInfo**)malloc(sizeof(FileInfo*) * favoritesSnapshotCount);
            // Paths that failed f_stat() here get removed individually below (against whatever
            // favorites.favorites actually is at that point), rather than replacing the whole
            // array with survivors built from this stale snapshot - that would silently discard
            // any favorite the user toggled during this navigation.
            auto deadPaths = std::make_unique_for_overwrite<String<char, 256>[]>(favoritesSnapshotCount);
            u32 deadCount = 0;

            for (u32 i = 0; i < favoritesSnapshotCount; i++)
            {
                const char* favPath = favoritesSnapshot[i].GetString();
                FILINFO fileInfo;
                if (f_stat(favPath, &fileInfo) == FR_OK)
                {
                    const char* fileName = strrchr(favPath, '/');
                    if (fileName)
                        fileName++;
                    else
                        fileName = favPath;

                    auto fileType = _fileTypeProvider.GetFileType(fileName);
                    fileInfos[count++] = new FileInfo(fileName, fileType, FastFileRef(fs, &fileInfo), fileInfo.fattrib, favPath);
                }
                else
                {
                    deadPaths[deadCount++] = favPath;
                }
            }

            _newSdFolder = std::make_unique<SdFolder>(fileInfos, count);
            if (deadCount > 0)
            {
                for (u32 i = 0; i < deadCount; i++)
                {
                    RemoveFavoriteAtPath(deadPaths[i].GetString());
                }
                _favoritesService->Save();
            }
            // Pressing back from favorites always returns to the real folder browsed before
            // entering it, so it's never a meaningless action here regardless of that folder.
            _isAtRoot = false;
        }
        else
        {
            if (strcmp(_navigatePath, "/") != 0) // can't f_stat on root dir
            {
                FILINFO fileInfo;
                if (f_stat(_navigatePath, &fileInfo) != FR_OK)
                {
                    StringUtil::Copy(_navigatePath, "/", sizeof(_navigatePath) / sizeof(_navigatePath[0]));
                }
                else if (!(fileInfo.fattrib & AM_DIR))
                {
                    _navigateFileName = strrchr(_navigatePath, '/') + 1;
                    _navigateFileName[-1] = 0;
                }
            }
            f_chdir(_navigatePath);
            SdFolderFactory sdFolderFactory { &_fileTypeProvider };
            _newSdFolder = sdFolderFactory.CreateFromPath(".");

            // _navigatePath is the raw navigation argument (could be "..", a folder name, or an
            // absolute path) and isn't reliably resolved here, so check the actual resulting
            // directory instead of the argument string. f_getcwd() always prefixes a volume
            // string (e.g. "fat:/" at root, per FF_STR_VOLUME_ID in ffconf.h), so compare only
            // the part after the last ':' rather than the raw result.
            char cwd[256];
            f_getcwd(cwd, sizeof(cwd));
            const char* cwdPath = strrchr(cwd, ':');
            cwdPath = cwdPath ? cwdPath + 1 : cwd;
            _isAtRoot = strcmp(cwdPath, "/") == 0;
        }
        u64 endTick = gTickCounter.GetValue();
        LOG_DEBUG("Loading files in folder took: %d us\n", (u32)TickCounter::TicksToMicroSeconds(endTick - startTick));
        return TaskResult<void>::Completed();
    });
}

void RomBrowserController::HandleFolderLoadDoneTrigger()
{
    LOG_DEBUG("RomBrowserStateTrigger::FolderLoadDone\n");
    _romBrowserViewModel.Reset();
    _sdFolder = std::move(_newSdFolder);
    _romBrowserViewModel = SharedPtr<RomBrowserViewModel>::MakeShared(this, _navigateFileName);
}

void RomBrowserController::HandleLaunchTrigger()
{
    LOG_DEBUG("RomBrowserStateTrigger::Launch\n");
    _ioTaskQueue->Enqueue([this] (const vu8& cancelRequested)
    {
        UpdateLastUsedFilepath();
        SetPicoLoaderParams();
        LoadCheats();
        return TaskResult<void>::Completed();
    });
}

void RomBrowserController::HandleChangeDisplayModeTrigger()
{
    LOG_DEBUG("RomBrowserStateTrigger::ChangeDisplayMode\n");
    _romBrowserViewModel = SharedPtr<RomBrowserViewModel>::MakeShared(this);
}

void RomBrowserController::HandleGotoSettingsScreenTrigger()
{
    gProcessManager.Goto<SettingsProcess>();
}

void RomBrowserController::UpdateLastUsedFilepath()
{
    // _navigatePath is about to be overwritten with the launched file's path (needed by
    // SetPicoLoaderParams() below), so capture whether we were browsing favorites first.
    bool launchedFromFavorites = strcmp(_navigatePath, ":favorites") == 0;
    if (_triggerFileInfo.GetFullPath())
    {
        char dir[256];
        StringUtil::Copy(dir, _triggerFileInfo.GetFullPath(), sizeof(dir));
        char* lastSlash = strrchr(dir, '/');
        if (lastSlash)
        {
            if (lastSlash == dir)
            {
                dir[1] = 0;
            }
            else
            {
                *lastSlash = 0;
            }
        }
        f_chdir(dir);
        StringUtil::Copy(_navigatePath, _triggerFileInfo.GetFullPath(), sizeof(_navigatePath));
    }
    else
    {
        f_getcwd(_navigatePath, sizeof(_navigatePath) / sizeof(_navigatePath[0]));
        int idx = strlcat(_navigatePath, "/", sizeof(_navigatePath));
        if (_navigatePath[idx - 2] == '/')
        {
            _navigatePath[idx - 1] = 0;
        }
        strlcat(_navigatePath, _triggerFileInfo.GetFileName(), sizeof(_navigatePath));
    }
    auto& settings = _appSettingsService->GetAppSettings();
    if (launchedFromFavorites)
    {
        // The view to restore is the favorites list, and _navigatePath now holds the
        // launched file's full path (set above) - which is what the favorites entries carry
        // and match on, so the next boot can put the selection back on this game.
        settings.lastUsedFilePath = ":favorites";
        settings.lastUsedFavoriteFilePath = _navigatePath;
    }
    else
    {
        settings.lastUsedFilePath = _navigatePath;
    }
    _appSettingsService->Save();
}

void RomBrowserController::SetPicoLoaderParams() const
{
    auto loadParams = pload_getLoadParams();
    loadParams->savePath[0] = 0;
    loadParams->arguments[0] = 0;
    loadParams->argumentsLength = 0;
    if (_triggerFileInfo.GetFileType()->TrySetLaunchParameters(loadParams, _navigatePath))
    {
        gProcessManager.Goto<PicoLoaderProcess>();
    }
    else
    {
        LOG_FATAL("Failed to set launch parameters.\n");
    }
}

void RomBrowserController::LoadCheats() const
{
    auto cheats = _cheatRepository->GetCheatsForGame(_triggerFileInfo.GetFastFileRef());
    auto cheatData = PicoLoaderCheatDataFactory().CreateCheatData(cheats);
    pload_setCheatData(cheatData);
}

bool RomBrowserController::GetFileInfoPath(const FileInfo& fileInfo, char* pathBuffer, u32 bufferSize) const
{
    if (fileInfo.GetFullPath())
    {
        StringUtil::Copy(pathBuffer, fileInfo.GetFullPath(), bufferSize);
        return strlen(fileInfo.GetFullPath()) < bufferSize;
    }

    f_getcwd(pathBuffer, bufferSize);
    int idx = strlcat(pathBuffer, "/", bufferSize);
    if (pathBuffer[idx - 2] == '/')
    {
        pathBuffer[idx - 1] = 0;
    }
    // strlcat() returns the length it *wanted* to build, so a result at or over the buffer
    // size is the truncation signal.
    return (u32)strlcat(pathBuffer, fileInfo.GetFileName(), bufferSize) < bufferSize;
}

bool RomBrowserController::IsFavorite(const FileInfo& fileInfo) const
{
    char path[256];
    if (!GetFileInfoPath(fileInfo, path, sizeof(path)))
    {
        // Too long to have ever been stored (see ToggleFavorite), so it cannot be a
        // favorite - and comparing the truncated prefix could match a different entry.
        return false;
    }

    // ToggleFavoriteAtPath()/RemoveFavoriteAtPath() can replace favorites (freeing the old
    // buffer) from either the main thread or the io thread depending on call site, so this
    // read - like every other read of the array - needs its own guard rather than relying on
    // which thread it happens to run on.
    u32 irq = rtos_disableIrqs();
    const auto& favorites = _favoritesService->GetFavorites();
    bool found = false;
    for (u32 i = 0; i < favorites.numberOfFavorites; i++)
    {
        if (strcmp(favorites.favorites[i].GetString(), path) == 0)
        {
            found = true;
            break;
        }
    }
    rtos_restoreIrqs(irq);
    return found;
}

void RomBrowserController::ToggleFavorite(const FileInfo& fileInfo)
{
    if (fileInfo.GetFullPath())
    {
        // Items from the favorites view already carry their full path, so toggling them
        // needs no filesystem call and can run directly on the calling thread. This also
        // keeps it synchronous with the favorites-view "remove then refresh" flow in
        // RomBrowserItemViewModel::ToggleFavorite(), which re-navigates right after.
        ToggleFavoriteAtPath(fileInfo.GetFullPath());
        _ioTaskQueue->Enqueue([this] (const vu8& cancelRequested)
        {
            _favoritesService->Save();
            return TaskResult<void>::Completed();
        });
        return;
    }

    // Regular browsed items have no stored path, so GetFileInfoPath() needs the current
    // directory - which (like every other path resolution in this file) has to run on the
    // io task thread, since the main thread could otherwise race a navigation's f_chdir()
    // mid-task. FileInfo is copied since fileInfo may not outlive this call (mirrors
    // LaunchFile()).
    _ioTaskQueue->Enqueue([this, fileInfoCopy = FileInfo(fileInfo)] (const vu8& cancelRequested)
    {
        char path[256];
        if (!GetFileInfoPath(fileInfoCopy, path, sizeof(path)))
        {
            // A truncated path is a prefix that can name a different file, so storing it
            // would put the heart on the wrong item - and IsFavorite() would never match it
            // back anyway. Refuse instead.
            LOG_ERROR("Path too long to favorite: %s\n", fileInfoCopy.GetFileName());
            return TaskResult<void>::Completed();
        }
        ToggleFavoriteAtPath(path);
        _favoritesService->Save();
        return TaskResult<void>::Completed();
    });
}

// ToggleFavoriteAtPath() can run on the main thread (favorites-view toggles, see
// ToggleFavorite() below) or the io thread (regular-view toggles), and RemoveFavoriteAtPath()
// runs on the io thread from the favorites-prune path - so favorites.favorites has no single
// writer to assume exclusivity from. The search, decide, and rebuild all happen inside one
// rtos_disableIrqs() section (so a stale foundIndex can never be used against an array that
// changed after it was found), but the heap allocation is sized from a snapshot taken and
// verified outside that section, retrying if the count grew in the meantime - the same
// alloc-outside/read-inside split IsFavorite()'s guard already uses, just closed against a
// second writer too.
void RomBrowserController::ToggleFavoriteAtPath(const char* path)
{
    auto& favorites = _favoritesService->GetFavorites();

    for (;;)
    {
        u32 oldCount;
        {
            u32 irq = rtos_disableIrqs();
            oldCount = favorites.numberOfFavorites;
            rtos_restoreIrqs(irq);
        }

        // Worst case (path not found) needs one more slot than the snapshot.
        auto newFavorites = std::make_unique_for_overwrite<String<char, 256>[]>(oldCount + 1);

        u32 irq = rtos_disableIrqs();
        u32 currentCount = favorites.numberOfFavorites;
        if (currentCount > oldCount)
        {
            rtos_restoreIrqs(irq);
            continue;
        }

        int foundIndex = -1;
        for (u32 i = 0; i < currentCount; i++)
        {
            if (strcmp(favorites.favorites[i].GetString(), path) == 0)
            {
                foundIndex = i;
                break;
            }
        }

        u32 newCount;
        if (foundIndex != -1)
        {
            u32 dst = 0;
            for (u32 src = 0; src < currentCount; src++)
            {
                if (src != (u32)foundIndex)
                {
                    newFavorites[dst++] = favorites.favorites[src];
                }
            }
            newCount = dst;
        }
        else
        {
            for (u32 i = 0; i < currentCount; i++)
            {
                newFavorites[i] = favorites.favorites[i];
            }
            newFavorites[currentCount] = path;
            newCount = currentCount + 1;
        }

        favorites.favorites = newCount > 0 ? std::move(newFavorites) : nullptr;
        favorites.numberOfFavorites = newCount;
        rtos_restoreIrqs(irq);
        return;
    }
}

// Same shape as ToggleFavoriteAtPath() above, minus the add branch - used by the favorites
// navigation prune to remove one confirmed-dead path from whatever favorites.favorites
// currently is, rather than overwriting the whole array from a stale snapshot. A no-op if the
// path is already gone (e.g. the user removed it themselves in the meantime).
void RomBrowserController::RemoveFavoriteAtPath(const char* path)
{
    auto& favorites = _favoritesService->GetFavorites();

    for (;;)
    {
        u32 oldCount;
        {
            u32 irq = rtos_disableIrqs();
            oldCount = favorites.numberOfFavorites;
            rtos_restoreIrqs(irq);
        }
        if (oldCount == 0)
            return;

        auto newFavorites = std::make_unique_for_overwrite<String<char, 256>[]>(oldCount);

        u32 irq = rtos_disableIrqs();
        u32 currentCount = favorites.numberOfFavorites;
        if (currentCount > oldCount)
        {
            rtos_restoreIrqs(irq);
            continue;
        }

        int foundIndex = -1;
        for (u32 i = 0; i < currentCount; i++)
        {
            if (strcmp(favorites.favorites[i].GetString(), path) == 0)
            {
                foundIndex = i;
                break;
            }
        }
        if (foundIndex == -1)
        {
            rtos_restoreIrqs(irq);
            return;
        }

        u32 dst = 0;
        for (u32 src = 0; src < currentCount; src++)
        {
            if (src != (u32)foundIndex)
            {
                newFavorites[dst++] = favorites.favorites[src];
            }
        }
        favorites.favorites = dst > 0 ? std::move(newFavorites) : nullptr;
        favorites.numberOfFavorites = dst;
        rtos_restoreIrqs(irq);
        return;
    }
}
