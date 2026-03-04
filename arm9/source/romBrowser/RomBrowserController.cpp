#include "common.h"
#include <array>
#include "picoLoaderBootstrap.h"
#include "core/StringUtil.h"
#include "PicoLoaderProcess.h"
#include "services/LaunchStats/LaunchStatsService.h"
#include "FileType/ExtensionFileTypeProvider.h"
#include "FileType/FileType.h"
#include "SdFolderFactory.h"
#include "fat/Directory.h"
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
    , _fileTypeProvider(appSettingsService->GetAppSettings())
    {
    }

void RomBrowserController::NavigateUp()
{
    if (_favoritesViewActive)
    {
        _favoritesViewActive = false;
        _romBrowserViewModel = SharedPtr(new RomBrowserViewModel(this, _navigateFileName));
        _viewModelInvalidated = true;
        return;
    }

    NavigateToPath("..");
}

void RomBrowserController::NavigateToPath(const TCHAR* name)
{
    if (_favoritesLoadPending)
    {
        _favoritesTask.CancelTask();
        _favoritesLoadPending = false;
    }
    _favoritesViewActive = false;
    StringUtil::Copy(_navigatePath, name, sizeof(_navigatePath) / sizeof(_navigatePath[0]));
    _stateMachine.Fire(RomBrowserStateTrigger::Navigate);
}

void RomBrowserController::LaunchFile(const FileInfo& fileInfo)
{
    _launchFileInfo = FileInfo(fileInfo);
    _stateMachine.Fire(RomBrowserStateTrigger::Launch);
}

void RomBrowserController::ShowGameInfo(const FileInfo& fileInfo)
{
    if (fileInfo.GetFileType()->GetClassification() == FileTypeClassification::Folder)
        return;

    _launchFileInfo = FileInfo(fileInfo);
    _stateMachine.Fire(RomBrowserStateTrigger::ShowGameInfo);
}

void RomBrowserController::HideGameInfo()
{
    _stateMachine.Fire(RomBrowserStateTrigger::HideGameInfo);
}

void RomBrowserController::ShowCheats()
{
    _stateMachine.Fire(RomBrowserStateTrigger::ShowCheats);
}

void RomBrowserController::HideCheats()
{
    _stateMachine.Fire(RomBrowserStateTrigger::HideCheats);
}

void RomBrowserController::ShowCheatDescription(const char* cheatName, const char* description, const char* gameCode, u32 crc,
    int scrollOffset, int cursorIndex, int folderIndex, int rootScrollOffset, int rootCursorIndex,
    bool enabledOnlyMode, int savedViewScrollOffset, int savedViewCursorIndex, int savedViewFolderIndex)
{
    StringUtil::Copy(_cheatName, cheatName, sizeof(_cheatName) / sizeof(_cheatName[0]));
    StringUtil::Copy(_cheatDescription, description, sizeof(_cheatDescription) / sizeof(_cheatDescription[0]));
    StringUtil::Copy(_cheatGameCode, gameCode ? gameCode : "", sizeof(_cheatGameCode) / sizeof(_cheatGameCode[0]));
    _cheatCrc = crc;
    _cheatFocusScrollOffset = scrollOffset;
    _cheatFocusCursorIndex = cursorIndex;
    _cheatFocusFolderIndex = folderIndex;
    _cheatFocusRootScrollOffset = rootScrollOffset;
    _cheatFocusRootCursorIndex = rootCursorIndex;
    _cheatFocusEnabledOnlyMode = enabledOnlyMode;
    _cheatFocusSavedViewScrollOffset = savedViewScrollOffset;
    _cheatFocusSavedViewCursorIndex = savedViewCursorIndex;
    _cheatFocusSavedViewFolderIndex = savedViewFolderIndex;
    _stateMachine.Fire(RomBrowserStateTrigger::ShowCheatDescription);
}

void RomBrowserController::HideCheatDescription()
{
    memset(_cheatDescription, 0, sizeof(_cheatDescription));
    _stateMachine.Fire(RomBrowserStateTrigger::HideCheatDescription);
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
    const auto previousDisplaySettings = _appSettingsService->GetAppSettings().romBrowserDisplaySettings;
    if (previousDisplaySettings.layout == romBrowserDisplaySettings.layout
        && previousDisplaySettings.sortMode == romBrowserDisplaySettings.sortMode)
    {
        return;
    }

    _appSettingsService->GetAppSettings().romBrowserDisplaySettings = romBrowserDisplaySettings;
    _saveSettingsPending = true;

    if (previousDisplaySettings.layout != romBrowserDisplaySettings.layout)
    {
        _stateMachine.Fire(RomBrowserStateTrigger::ChangeDisplayMode);
    }
    else
    {
        _romBrowserViewModel = SharedPtr(new RomBrowserViewModel(this));
        _viewModelInvalidated = true;
    }
}

void RomBrowserController::ToggleFavoritesView()
{
    if (_favoritesLoadPending)
        return;

    if (_favoritesViewActive)
    {
        _favoritesViewActive = false;
        _romBrowserViewModel = SharedPtr(new RomBrowserViewModel(this, _navigateFileName));
        _viewModelInvalidated = true;
        return;
    }

    StartFavoritesLoad();
}

void RomBrowserController::ToggleSelectedFileFavorite()
{
    const auto* fileInfo = GetSelectedFileInfo();
    if (!fileInfo)
        return;

    char fullPath[256];
    if (!TryBuildFilePath(*fileInfo, fullPath, sizeof(fullPath)))
        return;

    if (IsFavoritePath(fullPath))
        RemoveFavoritePath(fullPath);
    else
        AddFavoritePath(fullPath);

    SaveSettingsAsync();

    if (_favoritesViewActive && !_favoritesLoadPending)
        StartFavoritesLoad();
}

bool RomBrowserController::IsSelectedFileFavorite()
{
    const auto* fileInfo = GetSelectedFileInfo();
    if (!fileInfo)
        return false;

    char fullPath[256];
    if (!TryBuildFilePath(*fileInfo, fullPath, sizeof(fullPath)))
        return false;

    return IsFavoritePath(fullPath);
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

    if (_favoritesLoadPending && _favoritesTask.IsValid()
        && _favoritesTask.GetTask().IsCompletedSuccessfully())
    {
        CompleteFavoritesLoad();
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
    _navigateTask = _ioTaskQueue->Enqueue([this] (const vu8& cancelRequested)
    {
        if (!_coverRepository)
        {
            _coverRepository = std::make_unique<CoverRepository>();
            _coverRepository->Initialize();
        }
        if (!_cheatRepository)
        {
            _cheatRepository = UsrCheatRepositoryFactory().FromUsrCheatDat("/_pico/extras/usrcheat.dat");
            if (!_cheatRepository)
            {
                // When usrcheat.dat is not found or cannot be read use a dummy empty cheat repository
                _cheatRepository = std::make_unique<EmptyCheatRepository>();
            }
        }

        u64 startTick = gTickCounter.GetValue();
        _navigateFileName = nullptr;
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
    _romBrowserViewModel = SharedPtr(new RomBrowserViewModel(this, _navigateFileName));
}

void RomBrowserController::HandleLaunchTrigger()
{
    LOG_DEBUG("RomBrowserStateTrigger::Launch\n");
    _ioTaskQueue->Enqueue([this] (const vu8& cancelRequested)
    {
        if (!TryBuildFilePath(_launchFileInfo, _navigatePath, sizeof(_navigatePath)))
        {
            LOG_ERROR("Failed to build launch path.\n");
            return TaskResult<void>::Completed();
        }
        auto& appSettings = _appSettingsService->GetAppSettings();
        appSettings.lastUsedFilePath = _navigatePath;
        _appSettingsService->Save();

        LaunchStatsService::Instance().Increment(_navigatePath);

        LoadCheats();

        auto loadParams = pload_getLoadParams();
        loadParams->savePath[0] = 0;
        loadParams->arguments[0] = 0;
        loadParams->argumentsLength = 0;
        if (_launchFileInfo.GetFileType()->TrySetLaunchParameters(loadParams, _navigatePath))
        {
            gProcessManager.Goto<PicoLoaderProcess>();
        }
        else
        {
            LOG_FATAL("Failed to set launch parameters.\n");
        }
        return TaskResult<void>::Completed();
    });
}

void RomBrowserController::HandleChangeDisplayModeTrigger()
{
    LOG_DEBUG("RomBrowserStateTrigger::ChangeDisplayMode\n");
    _romBrowserViewModel = SharedPtr(new RomBrowserViewModel(this));
}

void RomBrowserController::StartFavoritesLoad()
{
    _favoritesLoadPending = true;
    _favoritesTask = _ioTaskQueue->Enqueue([this] (const vu8& cancelRequested)
    {
        _newFavoritesFolder = BuildFavoritesFolder();
        return TaskResult<void>::Completed();
    });
}

void RomBrowserController::CompleteFavoritesLoad()
{
    _favoritesTask.Dispose();
    _favoritesFolder = std::move(_newFavoritesFolder);
    _favoritesLoadPending = false;
    _favoritesViewActive = true;
    _romBrowserViewModel = SharedPtr(new RomBrowserViewModel(this));
    _viewModelInvalidated = true;
}

bool RomBrowserController::TryBuildFilePath(const FileInfo& fileInfo, char* outPath, u32 outPathSize) const
{
    const char* fullPath = fileInfo.GetFullPath();
    if (fullPath && fullPath[0] != 0)
    {
        StringUtil::Copy(outPath, fullPath, outPathSize);
        return true;
    }

    if (f_getcwd(outPath, outPathSize) != FR_OK)
        return false;

    int idx = strlcat(outPath, "/", outPathSize);
    if (idx > 1 && outPath[idx - 2] == '/')
        outPath[idx - 1] = 0;
    strlcat(outPath, fileInfo.GetFileName(), outPathSize);
    return true;
}

std::unique_ptr<SdFolder> RomBrowserController::BuildFavoritesFolder()
{
    auto& appSettings = _appSettingsService->GetAppSettings();
    const u32 favoritesCount = appSettings.numberOfFavorites;

    FileInfo** fileInfos = nullptr;
    if (favoritesCount > 0)
        fileInfos = (FileInfo**)malloc(sizeof(FileInfo*) * favoritesCount);

    u32 fileCount = 0;
    bool favoritesChanged = false;
    std::unique_ptr<String<char, 256>[]> validFavorites;
    u32 validCount = 0;
    if (favoritesCount > 0)
        validFavorites = std::make_unique_for_overwrite<String<char, 256>[]>(favoritesCount);

    for (u32 i = 0; i < favoritesCount; i++)
    {
        const char* favoritePath = appSettings.favorites[i].GetString();
        FileInfo* fileInfo = nullptr;
        if (TryCreateFileInfoFromPath(favoritePath, fileInfo))
        {
            fileInfos[fileCount++] = fileInfo;
            validFavorites[validCount++] = favoritePath;
        }
        else
        {
            favoritesChanged = true;
        }
    }

    if (favoritesChanged)
    {
        if (validCount == 0)
            appSettings.favorites.reset();
        else
            appSettings.favorites = std::move(validFavorites);
        appSettings.numberOfFavorites = validCount;
        _appSettingsService->Save();
    }

    if (fileCount == 0)
    {
        free(fileInfos);
        fileInfos = nullptr;
    }
    else if (fileCount < favoritesCount)
    {
        fileInfos = (FileInfo**)realloc(fileInfos, sizeof(FileInfo*) * fileCount);
    }

    return std::make_unique<SdFolder>(fileInfos, fileCount);
}

bool RomBrowserController::TryCreateFileInfoFromPath(const char* fullPath, FileInfo*& outFileInfo) const
{
    if (!fullPath || fullPath[0] == 0)
        return false;

    const char* fileName = strrchr(fullPath, '/');
    if (!fileName)
        return false;

    char dirPath[256];
    if (fileName == fullPath)
    {
        StringUtil::Copy(dirPath, "/", sizeof(dirPath));
        fileName++;
    }
    else
    {
        u32 dirLength = (u32)(fileName - fullPath);
        if (dirLength + 1 > sizeof(dirPath))
            return false;
        memcpy(dirPath, fullPath, dirLength);
        dirPath[dirLength] = 0;
        fileName++;
    }

    if (fileName[0] == 0)
        return false;

    Directory directory;
    if (directory.Open(dirPath) != FR_OK)
        return false;

    FILINFO fileInfo;
    while (directory.Read(&fileInfo) == FR_OK)
    {
        if (fileInfo.fname[0] == 0)
            break;
        if (strcasecmp(fileInfo.fname, fileName) == 0)
        {
            if (fileInfo.fattrib & AM_DIR)
                return false;
            const auto* fileType = _fileTypeProvider.GetFileType(fileInfo.fname);
            outFileInfo = new FileInfo(fileInfo.fname, fileType,
                FastFileRef(directory.GetFatFsDirectory(), &fileInfo), fullPath);
            return true;
        }
    }

    return false;
}

bool RomBrowserController::IsFavoritePath(const char* fullPath) const
{
    const auto& appSettings = _appSettingsService->GetAppSettings();
    for (u32 i = 0; i < appSettings.numberOfFavorites; i++)
    {
        const char* fav = appSettings.favorites[i].GetString();
        const char* favSuffix = strchr(fav, ':');
        const char* pathSuffix = strchr(fullPath, ':');
        const char* a = favSuffix ? favSuffix : fav;
        const char* b = pathSuffix ? pathSuffix : fullPath;
        if (!strcasecmp(a, b))
            return true;
    }
    return false;
}

void RomBrowserController::AddFavoritePath(const char* fullPath)
{
    if (IsFavoritePath(fullPath))
        return;

    auto& appSettings = _appSettingsService->GetAppSettings();
    u32 newCount = appSettings.numberOfFavorites + 1;
    auto newFavorites = std::make_unique_for_overwrite<String<char, 256>[]>(newCount);
    for (u32 i = 0; i < appSettings.numberOfFavorites; i++)
    {
        newFavorites[i] = appSettings.favorites[i];
    }
    newFavorites[newCount - 1] = fullPath;
    appSettings.favorites = std::move(newFavorites);
    appSettings.numberOfFavorites = newCount;
}

void RomBrowserController::RemoveFavoritePath(const char* fullPath)
{
    auto& appSettings = _appSettingsService->GetAppSettings();
    if (appSettings.numberOfFavorites == 0)
        return;

    bool found = false;
    for (u32 i = 0; i < appSettings.numberOfFavorites; i++)
    {
        const char* fav = appSettings.favorites[i].GetString();
        const char* favSuffix = strchr(fav, ':');
        const char* pathSuffix = strchr(fullPath, ':');
        const char* a = favSuffix ? favSuffix : fav;
        const char* b = pathSuffix ? pathSuffix : fullPath;
        if (!strcasecmp(a, b))
        {
            found = true;
            break;
        }
    }
    if (!found)
        return;

    u32 newCount = appSettings.numberOfFavorites - 1;
    if (newCount == 0)
    {
        appSettings.favorites.reset();
        appSettings.numberOfFavorites = 0;
        return;
    }

    auto newFavorites = std::make_unique_for_overwrite<String<char, 256>[]>(newCount);
    u32 writeIndex = 0;
    for (u32 i = 0; i < appSettings.numberOfFavorites; i++)
    {
        const char* fav = appSettings.favorites[i].GetString();
        const char* favSuffix = strchr(fav, ':');
        const char* pathSuffix = strchr(fullPath, ':');
        const char* a = favSuffix ? favSuffix : fav;
        const char* b = pathSuffix ? pathSuffix : fullPath;
        if (strcasecmp(a, b) == 0)
            continue;
        if (writeIndex < newCount)
            newFavorites[writeIndex++] = appSettings.favorites[i];
    }
    appSettings.favorites = std::move(newFavorites);
    appSettings.numberOfFavorites = writeIndex;
}

void RomBrowserController::SaveSettingsAsync()
{
    u32 length = 0;
    auto data = _appSettingsService->SerializeToBuffer(length);
    auto shared = std::shared_ptr<u8[]>(std::move(data));
    _ioTaskQueue->Enqueue([this, shared, len = length] (const vu8& cancelRequested)
    {
        _appSettingsService->WriteToFile(shared.get(), len);
        return TaskResult<void>::Completed();
    });
}

const FileInfo* RomBrowserController::GetSelectedFileInfo() const
{
    if (!_romBrowserViewModel.IsValid())
        return nullptr;

    int selectedItem = _romBrowserViewModel->GetSelectedItem();
    if (selectedItem < 0)
        return nullptr;

    auto& fileInfoManager = _romBrowserViewModel->GetFileInfoManager();
    if (selectedItem >= (int)fileInfoManager.GetItemCount())
        return nullptr;

    return &fileInfoManager.GetItem(selectedItem);
}

void RomBrowserController::LoadCheats() const
{
    if (!_cheatRepository)
    {
        return;
    }

    auto cheats = _cheatRepository->GetCheatsForGame(_launchFileInfo.GetFastFileRef());
    auto cheatData = PicoLoaderCheatDataFactory().CreateCheatData(cheats);
    pload_setCheatData(cheatData);
}
