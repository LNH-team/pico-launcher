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
    /// @brief Enum representing the state of the cheats panel.
    enum class State
    {
        /// @brief Cheats are being loaded.
        Loading,

        /// @brief No cheats were found.
        NoCheats,

        /// @brief Cheats are being displayed.
        DisplayCheats
    };

    CheatsViewModel(const FileInfo& romFileInfo, IRomBrowserController* romBrowserController);

    /// @brief Activates the cheat or category at the specified \p index.
    void ActivateItem(int index);

    /// @brief Navigates up in the cheat hierachy, or closes the cheats panel when at the root.
    /// @return \c true when navigation happened in the cheats tree, or \c false when the cheats panel was closed.
    bool NavigateUp();

    /// @brief Closes the cheats panel.
    void Close();

    /// @brief Disables all cheats.
    void DisableAllCheats();

    /// @brief Gets the current state of the cheats panel.
    /// @return The current state of the cheats panel.
    State GetState() const { return _state; }

    /// @brief Gets the current cheat category.
    /// @return The current cheat category.
    const CheatEntry* GetCurrentCheatCategory() const { return _categoryStack[_categoryStackLevel].cheatCategory; }

    /// @brief Gets the index of the selected item.
    /// @return The index of the selected item.
    constexpr int GetSelectedItem() const { return _selectedItem; }

    /// @brief Sets the index of the selected item.
    /// @param selectedItem The index of the selected item to set.
    void SetSelectedItem(int selectedItem) { _selectedItem = selectedItem; }

    /// @brief Returns whether the current displayed category is a sub-category.
    /// @return \c true when the current displayed category is a sub-category, or \c false otherwise.
    bool IsInSubCategory() const
    {
        return _categoryStackLevel > 0;
    }

private:
    struct CategoryStackEntry
    {
        const CheatEntry* cheatCategory;
        u32 index;
    };

    FileInfo _romFileInfo;
    IRomBrowserController* _romBrowserController;
    QueueTask<void> _loadCheatsTask;
    std::unique_ptr<GameCheats> _cheats;
    State _state = State::Loading;
    int _selectedItem = -1;
    bool _changed = false;
    u32 _categoryStackLevel = 0;
    std::array<CategoryStackEntry, 8> _categoryStack;

    void DisableAllCheats(const CheatEntry* cheatCategory);
};
