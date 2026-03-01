#pragma once
#include <array>
#include <memory>
#include "core/task/TaskQueue.h"
#include "cheats/GameCheats.h"
#include "romBrowser/FileInfo.h"
#include "romBrowser/IRomBrowserController.h"

/// @brief View model for the cheats screen.
class CheatsViewModel
{
public:
    enum class State
    {
        Loading,
        NoCheats,
        DisplayCheats
    };

    CheatsViewModel(const FileInfo& romFileInfo, IRomBrowserController* romBrowserController);

    bool ItemActivated();
    void Back();
    void Close();
    void SetSelectedOnlyMode(bool selectedOnlyMode);

    State GetState() const { return _state; }
    const ICheatCategory* GetCurrentCheatCategory() const { return _categoryStack[_categoryStackLevel]; }
    const char* GetCurrentFolderName() const;
    bool GetIsSelectedOnlyMode() const { return _selectedOnlyMode; }
    const Cheat* GetSelectedCheats(u32& numberOfCheats) const
    {
        numberOfCheats = _numberOfSelectedCheats;
        return _selectedCheats.get();
    }

    constexpr int GetSelectedItem() const { return _selectedItem; }
    void SetSelectedItem(int selectedItem) { _selectedItem = selectedItem; }

private:
    FileInfo _romFileInfo;
    IRomBrowserController* _romBrowserController;
    QueueTask<void> _loadCheatsTask;
    std::unique_ptr<GameCheats> _cheats;
    State _state = State::Loading;
    int _selectedItem = -1;
    bool _changed = false;
    bool _selectedOnlyMode = false;
    u32 _categoryStackLevel = 0;
    std::array<const ICheatCategory*, 8> _categoryStack;
    std::array<const char*, 8> _categoryNameStack;
    std::unique_ptr<Cheat[]> _selectedCheats;
    u32 _numberOfSelectedCheats = 0;

    u32 CountActiveCheats(const ICheatCategory* category) const;
    void CopyActiveCheats(const ICheatCategory* category, Cheat* cheats, u32& offset) const;
    void BuildSelectedCheatsList();
};
