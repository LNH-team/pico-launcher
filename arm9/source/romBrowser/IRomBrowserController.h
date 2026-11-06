#pragma once
#include "core/SharedPtr.h"
#include "services/settings/AppSettings.h"

class SdFolder;
class RomBrowserStateMachine;
class RomBrowserViewModel;
class FileInfo;
class TaskQueueBase;
class ICoverRepository;
class IIconRepository;
class IBannerRepository;
class ICheatRepository;
class IGameDataService;
class IBgmService;

class IRomBrowserController
{
public:
    virtual ~IRomBrowserController() = 0;

    virtual void NavigateUp() = 0;
    virtual void NavigateToPath(const TCHAR* name) = 0;
    /// @brief gameCode is the file's internal game code when the caller has
    ///        it loaded, or nullptr; it keys the game's persisted data.
    virtual void LaunchFile(const FileInfo& fileInfo, const char* gameCode = nullptr) = 0;
    virtual void LaunchRandomGame() = 0;
    virtual void ToggleFavorite(const FileInfo& fileInfo, const char* gameCode = nullptr) = 0;
    virtual void ToggleCompleted(const FileInfo& fileInfo, const char* gameCode = nullptr) = 0;
    virtual void ToggleFavoritesFilter() = 0;
    virtual bool IsFavoritesFilterEnabled() const = 0;
    virtual void ToggleCompletedFilter() = 0;
    virtual bool IsCompletedFilterEnabled() const = 0;
    virtual void ShowGameInfo(const FileInfo& fileInfo) = 0;
    virtual void HideGameInfo() = 0;
    virtual void ShowDisplaySettings() = 0;
    virtual void HideDisplaySettings() = 0;
    virtual void ShowRecents() = 0;
    virtual void HideRecents() = 0;
    virtual void ShowFavorites() = 0;
    virtual void HideFavorites() = 0;
    virtual void ShowStatistics() = 0;
    virtual void HideStatistics() = 0;
    /// @brief The menu behind the app bar's "more" button.
    virtual void ShowMenu() = 0;
    virtual void HideMenu() = 0;
    /// @brief The about sheet, reached from the menu.
    virtual void ShowAbout() = 0;
    virtual void HideAbout() = 0;
    /// @brief The in-game exit combo sheet, reached from the menu's title row.
    virtual void ShowExitCombo() = 0;
    virtual void HideExitCombo() = 0;
    /// @brief Keys held to leave a DS game, as a KEY_* mask (see KeyCombo).
    ///        Zero means the enhanced loader's default.
    virtual u16 GetLoaderExitCombo() const = 0;
    /// @brief Stores the combo, as names, in settings.json. Zero puts the
    ///        loader's default back. It reaches a game at its next launch.
    virtual void SetLoaderExitCombo(u16 combo) = 0;
    /// @brief The game info sheet, reached from the menu: edits the name,
    ///        publisher and developer shown for the highlighted game.
    virtual void ShowGameInfoEdit() = 0;
    virtual void HideGameInfoEdit() = 0;
    /// @brief Stores the edited values for the game the sheet was opened on
    ///        and rebuilds the browser on the same game, so the new name shows
    ///        without reloading the folder. Empty strings clear a field.
    virtual void SetGameMetadata(const char* name, const char* publisher,
        const char* developer) = 0;
    /// @brief Whether the highlighted entry has game info worth editing. Games
    ///        yes, folders and support files no, and nothing while parental
    ///        controls take the sheet away.
    virtual bool CanEditSelected() const = 0;
    /// @brief Whether parental controls take a feature away right now. Every
    ///        action a blocked feature can be reached by asks this, not just
    ///        the menu that draws it faded. The lock itself is the
    ///        parentalControls flag in settings.json; no screen sets it.
    virtual bool IsBlocked(ParentalFeature feature) const = 0;
    /// @brief Whether the highlighted entry can be deleted at all. Folders and
    ///        support files cannot, so the app bar dims its delete button rather
    ///        than offering one that does nothing.
    virtual bool CanDeleteSelected() const = 0;
    virtual void RequestDeleteSelected() = 0;
    virtual void CancelDelete() = 0;
    virtual void ConfirmDelete() = 0;
    /// @brief File that will be deleted. Deliberately a dedicated buffer, NOT
    ///        _triggerFileInfo: the launch flows overwrite that one, and a
    ///        delete must never act on anything but the file the user saw.
    virtual const char* GetDeleteRomFileName() const = 0;
    /// @brief Save file that goes with the game, empty when there is none. It is
    ///        deleted along with the game unless DeleteKeepsSave says it stays.
    ///        Valid while the delete confirmation is shown.
    virtual const char* GetDeleteSaveFileName() const = 0;
    /// @brief Whether deleting the game leaves its save alone, as for a rom an
    ///        emulator runs. Valid while the delete confirmation is shown.
    virtual bool DeleteKeepsSave() const = 0;
    virtual void GotoSettingsScreen() = 0;

    virtual void Update() = 0;

    /// @brief Records that the last list move was a big jump by L/R, so the
    ///        letter chip shows for those and not for a d-pad step.
    virtual void NotifyBigStepJump() = 0;
    /// @brief Returns whether a big jump has happened since the last call, and
    ///        clears the flag.
    virtual bool ConsumeBigStepJump() = 0;

    virtual const SdFolder& GetSdFolder() const = 0;

    virtual const RomBrowserStateMachine& GetStateMachine() const = 0;

    virtual const SharedPtr<RomBrowserViewModel>& GetRomBrowserViewModel() = 0;

    virtual TaskQueueBase* GetIoTaskQueue() const = 0;
    virtual TaskQueueBase* GetBgTaskQueue() const = 0;
    virtual const ICoverRepository& GetCoverRepository() const = 0;
    virtual const IIconRepository& GetIconRepository() const = 0;
    virtual const IBannerRepository& GetBannerRepository() const = 0;
    virtual const ICheatRepository& GetCheatRepository() const = 0;
    virtual IGameDataService* GetGameDataService() = 0;

    virtual const RomBrowserDisplaySettings& GetRomBrowserDisplaySettings() const = 0;

    /// @brief Returns where the launcher keeps the save file of a DS game.
    virtual SaveLocation GetSaveLocation() const = 0;

    virtual void SetRomBrowserDisplaySettings(
        const RomBrowserDisplaySettings& romBrowserDisplaySettings) = 0;

    /// @brief DS Lite backlight level (0..3), or -1 when the user never
    ///        picked one (the firmware's level is left untouched then).
    virtual int GetBacklightLevel() const = 0;
    virtual void SetBacklightLevel(int level) = 0;
    /// @brief Whether this console has backlight levels to set (the DS Lite
    ///        does; the original DS, the DSi and the 3DS do not).
    virtual bool HasBacklightLevels() const = 0;

    virtual const FileInfo& GetTriggerFileInfo() const = 0;
};

inline IRomBrowserController::~IRomBrowserController() { }
