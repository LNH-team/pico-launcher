#include "common.h"
#include <array>
#include "picoLoaderBootstrap.h"
#include "PicoLoaderProcess.h"
#include "settings/SettingsProcess.h"
#include "FileType/ExtensionFileTypeProvider.h"
#include "FileType/FileType.h"
#include "FileType/Nds/NdsRomHeader.h"
#include "FileType/Nds/NdsFileType.h"
#include "core/Environment.h"
#include "SdFolderFactory.h"
#include "services/settings/IAppSettingsService.h"
#include "cheats/UsrCheatRepositoryFactory.h"
#include "cheats/EmptyCheatRepository.h"
#include "cheats/PicoLoaderCheatDataFactory.h"
#include "services/gamedata/IGameDataService.h"
#include "bgm/IBgmService.h"
#include "core/mini-printf.h"
#include "rtcIpc.h"
#include "backlightIpc.h"
#include "services/settings/KeyCombo.h"
#include "RomBrowserController.h"

RomBrowserController::RomBrowserController(
    IAppSettingsService* appSettingsService, IGameDataService* gameDataService,
    IBgmService* bgmService, TaskQueueBase* ioTaskQueue, TaskQueueBase* bgTaskQueue)
    : _appSettingsService(appSettingsService)
    , _gameDataService(gameDataService)
    , _bgmService(bgmService)
    , _ioTaskQueue(ioTaskQueue), _bgTaskQueue(bgTaskQueue)
    , _fileTypeProvider(appSettingsService->GetAppSettings())
{
    // restore the user's backlight level at boot (harmless to re-apply when
    // this process is re-entered from the settings screen)
    int backlightLevel = appSettingsService->GetAppSettings().backlightLevel;
    if (backlightLevel >= 0)
        backlight_setLevel(backlightLevel);
}

void RomBrowserController::NavigateToPath(const TCHAR* name)
{
    StringUtil::Copy(_navigatePath, name, sizeof(_navigatePath) / sizeof(_navigatePath[0]));
    _stateMachine.Fire(RomBrowserStateTrigger::Navigate);
}

void RomBrowserController::LaunchFile(const FileInfo& fileInfo, const char* gameCode)
{
    _triggerFileInfo = FileInfo(fileInfo);
    StringUtil::Copy(_triggerGameCode, gameCode ? gameCode : "",
        sizeof(_triggerGameCode) / sizeof(_triggerGameCode[0]));
    _stateMachine.Fire(RomBrowserStateTrigger::Launch);
}

void RomBrowserController::LaunchRandomGame()
{
    if (!_romBrowserViewModel.IsValid())
        return;
    auto& fileInfoManager = _romBrowserViewModel->GetFileInfoManager();
    u32 gameCount = fileInfoManager.GetGameCount();
    if (gameCount == 0)
        return;
    u32 pick = gRandomGenerator->NextU32(gameCount);
    for (u32 i = 0; i < fileInfoManager.GetItemCount(); i++)
    {
        const auto& item = fileInfoManager.GetItem(i);
        if (item.GetFileType()->GetClassification() != FileTypeClassification::Game)
            continue;
        if (pick == 0)
        {
            // an off-screen random pick usually has no file info loaded yet, so
            // the code is simply not recorded; it is metadata, and the entry is
            // keyed by file name either way
            const char* gameCode = nullptr;
            if (fileInfoManager.IsFileInfoLoaded(i))
            {
                const auto* info = fileInfoManager.GetInternalFileInfo(i);
                if (info)
                    gameCode = info->GetGameCode();
            }
            LaunchFile(item, gameCode);
            return;
        }
        pick--;
    }
}

void RomBrowserController::BuildCurrentFolderFilePath(const char* fileName,
    TCHAR* buffer, u32 bufferLength) const
{
    buffer[0] = 0;
    f_getcwd(buffer, bufferLength);
    int idx = strlcat(buffer, "/", bufferLength);
    // collapse a "//" at the root; guard idx>=2 in case f_getcwd left the
    // buffer empty (then idx==1 and buffer[idx-2] would read out of bounds)
    if (idx >= 2 && buffer[idx - 2] == '/')
    {
        buffer[idx - 1] = 0;
    }
    strlcat(buffer, fileName, bufferLength);
}

void RomBrowserController::ToggleFavorite(const FileInfo& fileInfo, const char* gameCode)
{
    // the path makes the entry navigable from the favorites panel even for
    // games that were marked but never launched
    TCHAR fullPath[256];
    BuildCurrentFolderFilePath(fileInfo.GetFileName(), fullPath,
        sizeof(fullPath) / sizeof(fullPath[0]));
    _gameDataService->ToggleFavorite(fileInfo.GetFileName(), gameCode, fullPath);
    _gameDataService->SaveAsync(_ioTaskQueue);
    if (_favoritesFilter)
    {
        // an unfavorited game must drop out of the filtered view
        _stateMachine.Fire(RomBrowserStateTrigger::ChangeDisplayMode);
    }
}

void RomBrowserController::ToggleCompleted(const FileInfo& fileInfo, const char* gameCode)
{
    TCHAR fullPath[256];
    BuildCurrentFolderFilePath(fileInfo.GetFileName(), fullPath,
        sizeof(fullPath) / sizeof(fullPath[0]));
    _gameDataService->ToggleCompleted(fileInfo.GetFileName(), gameCode, fullPath);
    _gameDataService->SaveAsync(_ioTaskQueue);
    if (_completedFilter)
    {
        // a game unmarked as completed must drop out of the filtered view
        _stateMachine.Fire(RomBrowserStateTrigger::ChangeDisplayMode);
    }
}

void RomBrowserController::ToggleFavoritesFilter()
{
    _favoritesFilter = !_favoritesFilter;
    _stateMachine.Fire(RomBrowserStateTrigger::ChangeDisplayMode);
}

void RomBrowserController::ToggleCompletedFilter()
{
    _completedFilter = !_completedFilter;
    _stateMachine.Fire(RomBrowserStateTrigger::ChangeDisplayMode);
}

void RomBrowserController::ShowGameInfo(const FileInfo& fileInfo)
{
    // this sheet is the cheats sheet; parental controls can take it away
    if (IsBlocked(ParentalFeature::Cheats))
        return;
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

void RomBrowserController::ShowRecents()
{
    _stateMachine.Fire(RomBrowserStateTrigger::ShowRecents);
}

void RomBrowserController::ShowFavorites()
{
    _stateMachine.Fire(RomBrowserStateTrigger::ShowFavorites);
}

void RomBrowserController::HideFavorites()
{
    _stateMachine.Fire(RomBrowserStateTrigger::HideFavorites);
}

void RomBrowserController::HideRecents()
{
    _stateMachine.Fire(RomBrowserStateTrigger::HideRecents);
}

void RomBrowserController::ShowStatistics()
{
    _stateMachine.Fire(RomBrowserStateTrigger::ShowStatistics);
}

// only games: deleting folders would need recursion, and deleting random
// support files from the launcher is asking for trouble. The app bar asks the
// same question to decide whether to dim its delete button, so the button and
// the action can never disagree about what is deletable.
bool RomBrowserController::CanDeleteSelected() const
{
    // the menu entry is drawn faded from this, but RequestDeleteSelected and
    // ConfirmDelete ask it too, so no other route can delete while blocked
    if (IsBlocked(ParentalFeature::Delete))
        return false;
    return IsSelectedGame();
}

bool RomBrowserController::IsSelectedGame() const
{
    if (!_romBrowserViewModel.IsValid())
        return false;
    int selectedItem = _romBrowserViewModel->GetSelectedItem();
    if (selectedItem < 0)
        return false;
    const auto& item = _romBrowserViewModel->GetFileInfoManager().GetItem(selectedItem);
    return item.GetFileType()->GetClassification() == FileTypeClassification::Game;
}

void RomBrowserController::RequestDeleteSelected()
{
    if (!CanDeleteSelected())
        return;
    int selectedItem = _romBrowserViewModel->GetSelectedItem();
    const auto& item = _romBrowserViewModel->GetFileInfoManager().GetItem(selectedItem);

    StringUtil::Copy(_deleteRomFileName, item.GetFileName(),
        sizeof(_deleteRomFileName) / sizeof(_deleteRomFileName[0]));
    // "<name minus extension>.sav" is the save convention used by the loader
    // and by the gba emulators next to their roms
    StringUtil::Copy(_deleteSaveFileName, _deleteRomFileName,
        sizeof(_deleteSaveFileName) / sizeof(_deleteSaveFileName[0]));
    TCHAR* dot = strrchr(_deleteSaveFileName, '.');
    if (dot)
        *dot = 0;
    strlcat(_deleteSaveFileName, ".sav", sizeof(_deleteSaveFileName));
    // An emulator run through a file association may not follow that convention, and a
    // "<name>.sav" next to its rom may then be the save of a DS game with the same name,
    // so deleting such a rom keeps that save. The dialog says it stays.
    _deleteKeepsSave = !item.GetFileType()->HasKnownSave();
    // With the saves folder on, the loader looks in "saves/" first, so that is the save
    // that goes with the game when it is there. Only when the folder has none does the
    // save next to the game go, as before: with a folder save present, the one next to
    // it may belong to a game of another system with the same name.
    _deleteFolderSaveFileName[0] = 0;
    if (_appSettingsService->GetAppSettings().saveLocation == SaveLocation::SavesFolder &&
        item.GetFileType()->UsesLoaderSave() &&
        strlen(_deleteSaveFileName) + 6 < sizeof(_deleteFolderSaveFileName))
    {
        strcpy(_deleteFolderSaveFileName, "saves/");
        strcat(_deleteFolderSaveFileName, _deleteSaveFileName);
    }
    // Existence checks drive only the confirm dialog's wording; ConfirmDelete
    // deletes the save without checking it is there, so a wrong answer here
    // never leaves the save behind. At worst the dialog names the save next
    // to the game while the one in the folder is what goes.
    FILINFO fileInfo;
    _deleteSaveInFolder = _deleteFolderSaveFileName[0] != 0 &&
        f_stat(_deleteFolderSaveFileName, &fileInfo) == FR_OK;
    _deleteHasSave = _deleteSaveInFolder || f_stat(_deleteSaveFileName, &fileInfo) == FR_OK;

    _stateMachine.Fire(RomBrowserStateTrigger::ShowDeleteConfirm);
}

void RomBrowserController::FormatNowDateTime(TCHAR* buffer, u32 bufferLength) const
{
    rtc_datetime_t dateTime;
    rtc_readDateTime(&dateTime);
    // the rtc registers hold BCD values, which %x renders as decimal digits
    mini_snprintf(buffer, bufferLength, "20%02x-%02x-%02x %02x:%02x",
        dateTime.date.year, dateTime.date.month, dateTime.date.monthDay,
        dateTime.time.hour, dateTime.time.minute);
}

void RomBrowserController::CancelDelete()
{
    _stateMachine.Fire(RomBrowserStateTrigger::HideDeleteConfirm);
}

void RomBrowserController::ConfirmDelete()
{
    // the sheet can only be here because RequestDeleteSelected let it, but the
    // lock is checked again at the point the file actually goes
    if (IsBlocked(ParentalFeature::Delete))
        return;
    _ioTaskQueue->Enqueue([this] (const vu8& cancelRequested)
    {
        FRESULT result = f_unlink(_deleteRomFileName);
        if (result != FR_OK)
        {
            LOG_ERROR("Couldn't delete file (%d)\n", result);
        }
        else if (!_deleteKeepsSave)
        {
            // delete the save without checking it is there: its name is derived from the
            // rom and f_unlink harmlessly returns FR_NO_FILE when there is
            // none. Do NOT gate on a main-thread existence check — SD access
            // from that thread is unreliable and used to skip this. The saves
            // folder goes first; only when it has nothing does the save next
            // to the rom go, as before. Any other failure leaves both alone:
            // the dialog named the folder save, so no other file may go.
            FRESULT saveResult = _deleteFolderSaveFileName[0] == 0
                ? FR_NO_PATH
                : f_unlink(_deleteFolderSaveFileName);
            if (saveResult == FR_NO_FILE || saveResult == FR_NO_PATH)
            {
                f_unlink(_deleteSaveFileName);
            }
            else if (saveResult != FR_OK)
            {
                LOG_ERROR("Couldn't delete %s (%d)\n", _deleteFolderSaveFileName, saveResult);
            }
        }
        _deleteCompleted = true;
        return TaskResult<void>::Completed();
    });
}

void RomBrowserController::HideStatistics()
{
    _stateMachine.Fire(RomBrowserStateTrigger::HideStatistics);
}

void RomBrowserController::ShowMenu()
{
    _stateMachine.Fire(RomBrowserStateTrigger::ShowMenu);
}

void RomBrowserController::HideMenu()
{
    _stateMachine.Fire(RomBrowserStateTrigger::HideMenu);
}

void RomBrowserController::ShowAbout()
{
    _stateMachine.Fire(RomBrowserStateTrigger::ShowAbout);
}

void RomBrowserController::HideAbout()
{
    _stateMachine.Fire(RomBrowserStateTrigger::HideAbout);
}

void RomBrowserController::ShowExitCombo()
{
    _stateMachine.Fire(RomBrowserStateTrigger::ShowExitCombo);
}

void RomBrowserController::HideExitCombo()
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
    _stateMachine.Fire(RomBrowserStateTrigger::HideExitCombo);
}

u16 RomBrowserController::GetLoaderExitCombo() const
{
    u16 combo;
    if (!KeyCombo::TryParse(
            _appSettingsService->GetAppSettings().loaderExitCombo.GetString(), combo))
    {
        return 0;
    }
    return combo;
}

void RomBrowserController::SetLoaderExitCombo(u16 combo)
{
    char text[KeyCombo::kMaxTextLength] = "";
    if (combo != 0)
    {
        // The loader checks these rules too; refusing here keeps an unusable
        // combination out of settings.json in the first place.
        if (!KeyCombo::IsValid(combo))
            return;
        KeyCombo::Format(combo, text, sizeof(text));
    }

    auto& appSettings = _appSettingsService->GetAppSettings();
    if (strcmp(appSettings.loaderExitCombo.GetString(), text) != 0)
    {
        appSettings.loaderExitCombo = text;
        _saveSettingsPending = true;
    }
}

void RomBrowserController::ShowGameInfoEdit()
{
    if (!CanEditSelected())
        return;
    int selectedItem = _romBrowserViewModel->GetSelectedItem();
    const auto& item = _romBrowserViewModel->GetFileInfoManager().GetItem(selectedItem);
    _triggerFileInfo = FileInfo(item);
    _stateMachine.Fire(RomBrowserStateTrigger::ShowGameInfoEdit);
}

void RomBrowserController::HideGameInfoEdit()
{
    _stateMachine.Fire(RomBrowserStateTrigger::HideGameInfoEdit);
}

bool RomBrowserController::CanEditSelected() const
{
    if (IsBlocked(ParentalFeature::GameInfo))
        return false;
    // the same files that can be deleted: a game, not a folder or support file
    return IsSelectedGame();
}

void RomBrowserController::SetGameMetadata(const char* name, const char* publisher,
    const char* developer)
{
    const char* fileName = _triggerFileInfo.GetFileName();
    _gameDataService->SetGameMetadata(fileName, name, publisher, developer);
    _gameDataService->SaveAsync(_ioTaskQueue);
    // Rebuild the browser the way a display-mode change does, but land on the
    // game that was just named: the list rows and the top screen then show the
    // new values without the cost of reloading the folder.
    StringUtil::Copy(_refreshSelectName, fileName,
        sizeof(_refreshSelectName) / sizeof(_refreshSelectName[0]));
    _stateMachine.Fire(RomBrowserStateTrigger::ChangeDisplayMode);
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

void RomBrowserController::SetBacklightLevel(int level)
{
    if (level < 0 || level > 3)
        return;
    auto& appSettings = _appSettingsService->GetAppSettings();
    if (appSettings.backlightLevel != level)
    {
        appSettings.backlightLevel = (s8)level;
        _saveSettingsPending = true;
    }
    // apply immediately; no browser rebuild is needed for this
    backlight_setLevel(level);
}

bool RomBrowserController::HasBacklightLevels() const
{
    return backlight_hasLevels();
}

void RomBrowserController::Update()
{
    if (_deleteCompleted)
    {
        _deleteCompleted = false;
        // the deleted game's favorite/stats entry goes with it
        _gameDataService->RemoveEntry(_deleteRomFileName);
        _gameDataService->SaveAsync(_ioTaskQueue);
        // reload the current folder so the deleted file disappears
        NavigateToPath(".");
    }
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
            // a launcher boot ends the play session the last launch opened;
            // with tracking switched off since, the session is dropped, not
            // credited
            bool sessionChanged;
            if (_appSettingsService->GetAppSettings().launchTracking)
            {
                TCHAR now[20];
                FormatNowDateTime(now, sizeof(now) / sizeof(now[0]));
                sessionChanged = _gameDataService->CloseOpenSession(now);
            }
            else
            {
                sessionChanged = _gameDataService->DiscardOpenSession();
            }
            if (sessionChanged)
            {
                _gameDataService->SaveAsync(_ioTaskQueue);
            }
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
        {
            if (_launchCheckTask.IsValid() && _launchCheckTask.GetTask().IsCompleted())
            {
                _launchCheckTask.Dispose();
                if (_launchDsiOnly)
                {
                    _stateMachine.Fire(RomBrowserStateTrigger::LaunchRefused);
                }
                else
                {
                    BeginLaunch();
                }
            }
            break;
        }
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
    _navigateTask = _ioTaskQueue->Enqueue([this] (const vu8& cancelRequested)
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
        // Going up: land on the folder just left instead of the first entry, so
        // stepping out of a folder does not lose the user's place. The current
        // directory is read here, on the IO thread, because that is the thread
        // that owns the cwd - f_chdir below runs here too, and reading the card
        // from the main thread is unreliable.
        if (strcmp(_navigatePath, "..") == 0)
        {
            TCHAR currentPath[256];
            if (f_getcwd(currentPath, sizeof(currentPath) / sizeof(currentPath[0])) == FR_OK)
            {
                // "fat:/Games/nds" -> "nds"; at the root the separator is the
                // last character, so there is nothing to preselect
                const TCHAR* folderName = strrchr(currentPath, '/');
                if (folderName && folderName[1] != 0)
                {
                    StringUtil::Copy(_navigateSelectName, folderName + 1,
                        sizeof(_navigateSelectName) / sizeof(_navigateSelectName[0]));
                    _navigateFileName = _navigateSelectName;
                }
            }
        }
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
        if (_newSdFolder)
        {
            // Probed unconditionally, not gated on the current "hide empty
            // folders" setting: the cached bit must already be correct
            // whenever the toggle is flipped later (main thread, no I/O -
            // see HandleChangeDisplayModeTrigger), including while the user
            // is already inside the folder. cwd is still _navigatePath here,
            // so bare relative names resolve without building full paths.
            // readBudget is declared once here and shared across every
            // sibling folder below (HasVisibleContent decrements it, never
            // resets it) - otherwise each sibling would get its own fresh
            // allowance and a folder full of large, mostly-empty siblings
            // (photos, DSi system data, ...) could cost their sum instead of
            // a single bounded worst case for the whole navigation.
            int readBudget = SdFolderFactory::kInitialReadBudget;
            FileInfo* const* files = _newSdFolder->GetFiles();
            for (int i = 0; i < _newSdFolder->GetFileCount(); i++)
            {
                FileInfo* file = files[i];
                if (file->GetFileType()->GetClassification() == FileTypeClassification::Folder)
                {
                    // Entries the listing always drops are not worth a single
                    // read: FilterAndSort discards dot-named and hidden ones
                    // unconditionally. On a card that has been in a Mac this is
                    // most of the cost (.Spotlight-V100 alone is a deep tree of
                    // files nobody ever sees) and it changes between sessions,
                    // which made the whole feature look random.
                    if (file->GetFileName()[0] == '.' || file->IsHidden())
                        continue;
                    // Deliberately NOT filter-aware. Emptiness means "has no
                    // content at all", the same answer whatever the favorites or
                    // completed filter is doing, for two reasons: a filter-aware
                    // probe cannot stop at the first game it finds, so a folder
                    // of 200 unmarked roms cost 200 reads and ate the whole
                    // budget (every folder after it then failed open and came
                    // back into view); and the cached answer would go stale the
                    // moment a filter is toggled, since that only rebuilds the
                    // view model - it does not reload the folder. The trade is
                    // that with a filter on, a listed folder may turn out to
                    // hold nothing that matches.
                    file->SetEmptyFolder(!sdFolderFactory.HasVisibleContent(
                        file->GetFileName(), readBudget));
                }
            }
        }
        u64 endTick = gTickCounter.GetValue();
        LOG_DEBUG("Loading files in folder took: %d us\n", (u32)TickCounter::TicksToMicroSeconds(endTick - startTick));

        // folders can bring their own music via a bgm.bcstm inside them
        FILINFO bgmFileInfo;
        if (f_stat("bgm.bcstm", &bgmFileInfo) == FR_OK && !(bgmFileInfo.fattrib & AM_DIR))
        {
            TCHAR bgmPath[256];
            f_getcwd(bgmPath, sizeof(bgmPath) / sizeof(bgmPath[0]));
            int idx = strlcat(bgmPath, "/", sizeof(bgmPath));
            if (bgmPath[idx - 2] == '/')
            {
                bgmPath[idx - 1] = 0;
            }
            strlcat(bgmPath, "bgm.bcstm", sizeof(bgmPath));
            _bgmService->UpdateBgmForFolder(bgmPath);
        }
        else
        {
            _bgmService->UpdateBgmForFolder(nullptr);
        }
        return TaskResult<void>::Completed();
    });
}

void RomBrowserController::HandleFolderLoadDoneTrigger()
{
    LOG_DEBUG("RomBrowserStateTrigger::FolderLoadDone\n");
    _romBrowserViewModel.Reset();
    _sdFolder = std::move(_newSdFolder);
    _romBrowserViewModel = SharedPtr<RomBrowserViewModel>::MakeShared(this, _navigateFileName);
    BackfillFavoritePaths();
}

// Favorites/completed marks made before path recording existed have no
// stored path, so the panels can't navigate to them. The current folder is
// loaded and is the cwd here (main thread, after the navigate IO task), so
// fill in the path of any flagged game found in it — the marks self-heal as
// the user browses, with no card-wide scan.
void RomBrowserController::BackfillFavoritePaths()
{
    // cheap O(entries) gate: once every mark is navigable, skip the
    // O(files x entries) folder scan entirely (steady state on a big library)
    if (!_sdFolder || !_gameDataService->HasUnpathedFlaggedEntry())
        return;
    bool changed = false;
    TCHAR fullPath[256];
    const FileInfo* const* files = _sdFolder->GetFiles();
    for (int i = 0; i < _sdFolder->GetFileCount(); i++)
    {
        const FileInfo* file = files[i];
        if (file->GetFileType()->GetClassification() != FileTypeClassification::Game)
            continue;
        BuildCurrentFolderFilePath(file->GetFileName(), fullPath,
            sizeof(fullPath) / sizeof(fullPath[0]));
        if (_gameDataService->BackfillPath(file->GetFileName(), fullPath))
            changed = true;
    }
    if (changed)
        _gameDataService->SaveAsync(_ioTaskQueue);
}

void RomBrowserController::HandleLaunchTrigger()
{
    LOG_DEBUG("RomBrowserStateTrigger::Launch\n");
    // On a DS or DS Lite a DSi-only game can only end in a white screen, so it
    // is turned away before anything about the launch is recorded. The header
    // is read on the io thread like every other file access; the browser sits
    // in the Launching state, with input off, until the answer is in.
    if (!Environment::IsDsiMode() && _triggerFileInfo.GetFileType() == &NdsFileType::sInstance)
    {
        _launchDsiOnly = false;
        _launchCheckTask = _ioTaskQueue->Enqueue([this] (const vu8& cancelRequested)
        {
            _launchDsiOnly = NdsRomHeader::IsDsiOnly(_triggerFileInfo.GetFastFileRef());
            return TaskResult<void>::Completed();
        });
        return;
    }
    BeginLaunch();
}

void RomBrowserController::BeginLaunch()
{
    if (_appSettingsService->GetAppSettings().launchTracking)
    {
        char lastPlayed[20];
        FormatNowDateTime(lastPlayed, sizeof(lastPlayed));
        // full path into a local buffer: _navigatePath belongs to the navigation
        // flow (same construction the favorite/completed toggles use)
        TCHAR fullPath[256];
        BuildCurrentFolderFilePath(_triggerFileInfo.GetFileName(), fullPath,
            sizeof(fullPath) / sizeof(fullPath[0]));
        _gameDataService->RecordLaunch(_triggerFileInfo.GetFileName(),
            _triggerGameCode[0] != 0 ? _triggerGameCode : nullptr, fullPath, lastPlayed);
        _gameDataService->SaveAsync(_ioTaskQueue);
    }
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
    _romBrowserViewModel = SharedPtr<RomBrowserViewModel>::MakeShared(this,
        _refreshSelectName[0] != 0 ? _refreshSelectName : nullptr);
    _refreshSelectName[0] = 0;
}

void RomBrowserController::HandleGotoSettingsScreenTrigger()
{
    gProcessManager.Goto<SettingsProcess>();
}

void RomBrowserController::UpdateLastUsedFilepath()
{
    f_getcwd(_navigatePath, sizeof(_navigatePath) / sizeof(_navigatePath[0]));
    int idx = strlcat(_navigatePath, "/", sizeof(_navigatePath));
    if (_navigatePath[idx - 2] == '/')
    {
        _navigatePath[idx - 1] = 0;
    }
    strlcat(_navigatePath, _triggerFileInfo.GetFileName(), sizeof(_navigatePath));
    _appSettingsService->GetAppSettings().lastUsedFilePath = _navigatePath;
    _appSettingsService->Save();
}

void RomBrowserController::SetPicoLoaderParams() const
{
    pload_setExitCombo(GetLoaderExitCombo());
    auto loadParams = pload_getLoadParams();
    loadParams->savePath[0] = 0;
    loadParams->arguments[0] = 0;
    loadParams->argumentsLength = 0;
    if (_triggerFileInfo.GetFileType()->TrySetLaunchParameters(loadParams, _navigatePath))
    {
        if (_triggerFileInfo.GetFileType()->UsesLoaderSave() &&
            _appSettingsService->GetAppSettings().saveLocation == SaveLocation::SavesFolder)
        {
            SetSavesFolderPath(loadParams);
        }
        gProcessManager.Goto<PicoLoaderProcess>();
    }
    else
    {
        LOG_FATAL("Failed to set launch parameters.\n");
    }
}

// Points the loader at "<game folder>/saves/<game name>.sav", creating the saves folder.
// The path stays empty, so the loader keeps the save next to the game as it does on its
// own, when the folder can't be made or the path would not fit: a truncated path would
// point the loader at another file, and a save must never end up somewhere unexpected.
void RomBrowserController::SetSavesFolderPath(pload_params_t* loadParams) const
{
    // Homebrew and DSiWare get no card save from the loader, so no folder for them either.
    if (!NdsRomHeader::UsesCardSave(_triggerFileInfo.GetFastFileRef()))
    {
        return;
    }

    const char* fileName = _triggerFileInfo.GetFileName();
    const char* extension = strrchr(fileName, '.');
    u32 baseLength = extension ? extension - fileName : strlen(fileName);
    // _navigatePath is "<game folder>/<file name>", built by UpdateLastUsedFilepath just before
    u32 folderLength = strlen(_navigatePath) - strlen(fileName);
    if (folderLength == 0 || _navigatePath[folderLength - 1] != '/' ||
        strcmp(&_navigatePath[folderLength], fileName) != 0)
    {
        LOG_ERROR("Unexpected launch path, keeping the save next to the game\n");
        return;
    }
    // "<game folder>/" + "saves/" + base + ".sav" + terminator
    if (folderLength + 6 + baseLength + 4 + 1 > sizeof(loadParams->savePath))
    {
        LOG_ERROR("Save path too long, keeping the save next to the game\n");
        return;
    }

    char* savePath = loadParams->savePath;
    memcpy(savePath, _navigatePath, folderLength);
    savePath[folderLength] = 0;
    strcat(savePath, "saves");

    // Create first and look only when something is already there: a folder that exists
    // must be recognised even if one read of the card fails, or the loader would start a
    // second save next to the game while the real one sits in the folder.
    FRESULT mkdirResult = f_mkdir(savePath);
    if (mkdirResult == FR_EXIST)
    {
        FILINFO folderInfo;
        if (f_stat(savePath, &folderInfo) != FR_OK || !(folderInfo.fattrib & AM_DIR))
        {
            LOG_ERROR("%s is not a usable folder, keeping the save next to the game\n", savePath);
            savePath[0] = 0;
            return;
        }
    }
    else if (mkdirResult != FR_OK)
    {
        LOG_ERROR("Couldn't create %s (%d), keeping the save next to the game\n", savePath, mkdirResult);
        savePath[0] = 0;
        return;
    }

    strcat(savePath, "/");
    strncat(savePath, fileName, baseLength);
    strcat(savePath, ".sav");

    // The save that is already there decides. One in the folder is used as it is. One still
    // next to the game is moved in: moved, not copied, so there is a single save to trust,
    // and nothing is ever deleted. If the move fails, the game keeps the save next to it.
    FILINFO saveInfo;
    if (f_stat(savePath, &saveInfo) == FR_OK)
    {
        return;
    }
    char oldPath[sizeof(loadParams->savePath)];
    memcpy(oldPath, _navigatePath, folderLength);
    oldPath[folderLength] = 0;
    strncat(oldPath, fileName, baseLength);
    strcat(oldPath, ".sav");
    if (f_stat(oldPath, &saveInfo) != FR_OK)
    {
        return; // a new game: the loader creates the save in the folder
    }
    FRESULT renameResult = f_rename(oldPath, savePath);
    if (renameResult == FR_EXIST)
    {
        return; // the folder has a save after all: it wins, as above
    }
    if (renameResult != FR_OK)
    {
        LOG_ERROR("Couldn't move %s into the saves folder (%d), keeping it next to the game\n",
            oldPath, renameResult);
        savePath[0] = 0;
    }
}

void RomBrowserController::LoadCheats() const
{
    auto cheats = _cheatRepository->GetCheatsForGame(_triggerFileInfo.GetFastFileRef());
    auto cheatData = PicoLoaderCheatDataFactory().CreateCheatData(cheats);
    pload_setCheatData(cheatData);
}
