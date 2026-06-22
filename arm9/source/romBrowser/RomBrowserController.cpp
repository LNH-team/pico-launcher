#include "common.h"
#include <array>
#include <libtwl/rtos/rtosIrq.h>
#include "picoLoaderBootstrap.h"
#include "PicoLoaderProcess.h"
#include "FileType/ExtensionFileTypeProvider.h"
#include "FileType/FileType.h"
#include "SdFolderFactory.h"
#include "services/settings/IAppSettingsService.h"
#include "cheats/UsrCheatRepositoryFactory.h"
#include "cheats/EmptyCheatRepository.h"
#include "cheats/PicoLoaderCheatDataFactory.h"
#include "RomBrowserController.h"

RomBrowserController::RomBrowserController(
    IAppSettingsService* appSettingsService, TaskQueueBase* ioTaskQueue,
    TaskQueueBase* bgTaskQueue)
    : _appSettingsService(appSettingsService)
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
        const auto& settings = _appSettingsService->GetAppSettings();
        favoritesSnapshotCount = settings.numberOfFavorites;
        favoritesSnapshot = std::make_unique_for_overwrite<String<char, 256>[]>(favoritesSnapshotCount);
        for (u32 i = 0; i < favoritesSnapshotCount; i++)
        {
            favoritesSnapshot[i] = settings.favorites[i];
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
            DIR dir;
            FATFS* fs = nullptr;
            if (f_opendir(&dir, "/") == FR_OK)
            {
                fs = dir.obj.fs;
                f_closedir(&dir);
            }

            u32 count = 0;
            FileInfo** fileInfos = (FileInfo**)malloc(sizeof(FileInfo*) * favoritesSnapshotCount);
            auto survivors = std::make_unique_for_overwrite<String<char, 256>[]>(favoritesSnapshotCount);
            u32 survivorCount = 0;

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
                    survivors[survivorCount++] = favPath;
                }
            }

            _newSdFolder = std::make_unique<SdFolder>(fileInfos, count);
            _newFavoritesSurvivors = std::move(survivors);
            _newFavoritesSurvivorCount = survivorCount;
            _favoritesPruneNeeded = (survivorCount != favoritesSnapshotCount);
            // Pressing back from favorites always returns to the real folder browsed before
            // entering it, so it's never a meaningless action here regardless of that folder.
            _isAtRoot = false;
        }
        else
        {
            _favoritesPruneNeeded = false;
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

    if (_favoritesPruneNeeded)
    {
        _favoritesPruneNeeded = false;
        auto& settings = _appSettingsService->GetAppSettings();
        u32 irq = rtos_disableIrqs();
        settings.favorites = std::move(_newFavoritesSurvivors);
        settings.numberOfFavorites = _newFavoritesSurvivorCount;
        rtos_restoreIrqs(irq);
        _ioTaskQueue->Enqueue([this] (const vu8& cancelRequested)
        {
            _appSettingsService->Save();
            return TaskResult<void>::Completed();
        });
    }
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

void RomBrowserController::UpdateLastUsedFilepath()
{
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
    _appSettingsService->GetAppSettings().lastUsedFilePath = _navigatePath;
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

void RomBrowserController::GetFileInfoPath(const FileInfo& fileInfo, char* pathBuffer, u32 bufferSize) const
{
    if (fileInfo.GetFullPath())
    {
        StringUtil::Copy(pathBuffer, fileInfo.GetFullPath(), bufferSize);
        return;
    }

    f_getcwd(pathBuffer, bufferSize);
    int idx = strlcat(pathBuffer, "/", bufferSize);
    if (pathBuffer[idx - 2] == '/')
    {
        pathBuffer[idx - 1] = 0;
    }
    strlcat(pathBuffer, fileInfo.GetFileName(), bufferSize);
}

bool RomBrowserController::IsFavorite(const FileInfo& fileInfo) const
{
    char path[256];
    GetFileInfoPath(fileInfo, path, sizeof(path));

    const auto& settings = _appSettingsService->GetAppSettings();
    for (u32 i = 0; i < settings.numberOfFavorites; i++)
    {
        if (strcmp(settings.favorites[i].GetString(), path) == 0)
        {
            return true;
        }
    }
    return false;
}

void RomBrowserController::ToggleFavorite(const FileInfo& fileInfo)
{
    char path[256];
    GetFileInfoPath(fileInfo, path, sizeof(path));

    auto& settings = _appSettingsService->GetAppSettings();
    int foundIndex = -1;
    for (u32 i = 0; i < settings.numberOfFavorites; i++)
    {
        if (strcmp(settings.favorites[i].GetString(), path) == 0)
        {
            foundIndex = i;
            break;
        }
    }

    if (foundIndex != -1)
    {
        u32 newCount = settings.numberOfFavorites - 1;
        std::unique_ptr<String<char, 256>[]> newFavorites = nullptr;
        if (newCount > 0)
        {
            newFavorites = std::make_unique_for_overwrite<String<char, 256>[]>(newCount);
            u32 dst = 0;
            for (u32 src = 0; src < settings.numberOfFavorites; src++)
            {
                if (src != (u32)foundIndex)
                {
                    newFavorites[dst++] = settings.favorites[src];
                }
            }
        }
        // RomBrowserItemViewModel::SetIndex() reads settings.favorites/numberOfFavorites
        // from the io task thread; disabling IRQs makes this swap appear atomic to it,
        // so it never observes a count from one array paired with the pointer of the other.
        u32 irq = rtos_disableIrqs();
        settings.favorites = std::move(newFavorites);
        settings.numberOfFavorites = newCount;
        rtos_restoreIrqs(irq);
    }
    else
    {
        u32 newCount = settings.numberOfFavorites + 1;
        auto newFavorites = std::make_unique_for_overwrite<String<char, 256>[]>(newCount);
        for (u32 i = 0; i < settings.numberOfFavorites; i++)
        {
            newFavorites[i] = settings.favorites[i];
        }
        newFavorites[settings.numberOfFavorites] = path;
        u32 irq = rtos_disableIrqs();
        settings.favorites = std::move(newFavorites);
        settings.numberOfFavorites = newCount;
        rtos_restoreIrqs(irq);
    }

    _ioTaskQueue->Enqueue([this] (const vu8& cancelRequested)
    {
        _appSettingsService->Save();
        return TaskResult<void>::Completed();
    });
}
