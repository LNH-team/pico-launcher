#pragma once
#include "core/StringUtil.h"
#include "../FileInfo.h"
#include "../IRomBrowserController.h"
#include "services/gamedata/IGameDataService.h"

/// @brief View model for the game info sheet: the name, publisher and
///        developer currently stored for the selected game, and where the
///        edited values go. The sheet keeps its own copy while editing, so a
///        cancel leaves the stored values untouched.
class GameInfoEditViewModel
{
public:
    explicit GameInfoEditViewModel(IRomBrowserController* romBrowserController)
        : _romBrowserController(romBrowserController)
    {
        const auto* entry = romBrowserController->GetGameDataService()->GetEntry(
            romBrowserController->GetTriggerFileInfo().GetFileName());
        if (entry)
        {
            StringUtil::Copy(_name, entry->displayName.GetString(), sizeof(_name));
            StringUtil::Copy(_publisher, entry->publisher.GetString(), sizeof(_publisher));
            StringUtil::Copy(_developer, entry->developer.GetString(), sizeof(_developer));
        }
    }

    const char* GetName() const { return _name; }
    const char* GetPublisher() const { return _publisher; }
    const char* GetDeveloper() const { return _developer; }

    /// @brief Stores the values and rebuilds the browser on the same game.
    void Save(const char* name, const char* publisher, const char* developer)
    {
        _romBrowserController->SetGameMetadata(name, publisher, developer);
    }

    /// @brief Leaves the stored values as they were.
    void Close() { _romBrowserController->HideGameInfoEdit(); }

private:
    IRomBrowserController* _romBrowserController;
    char _name[GameDataEntry::kMaxDisplayNameLength + 1] = {};
    char _publisher[GameDataEntry::kMaxPublisherLength + 1] = {};
    char _developer[GameDataEntry::kMaxDeveloperLength + 1] = {};
};
