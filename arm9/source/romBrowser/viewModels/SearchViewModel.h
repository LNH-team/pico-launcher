#pragma once
#include "romBrowser/IRomBrowserController.h"

/// @brief View model for the search screen.
class SearchViewModel
{
public:
    /// @brief Enum representing the state of the search panel.
    enum class State
    {
        /// @brief Search results are being loaded.
        Loading,

        /// @brief No results were found.
        NoResults,

        /// @brief Search results are being displayed.
        DisplayResults
    };

    explicit SearchViewModel(IRomBrowserController* romBrowserController);

    static constexpr int QUERY_MAX_LENGTH = 32;

    /// @brief Activates the selected search result or category.
    void ActivateSelectedItem();

    /// @brief Navigates up in the search hierarchy, or closes the search panel when at the root.
    /// @return \c true when navigation happened in the search tree, or \c false when the search panel was closed.
    bool NavigateUp();

    /// @brief Closes the search panel.
    void Close();

    /// @brief Applies current query to rom browser filtering and closes the panel.
    void ApplySearchAndClose();

    /// @brief Appends a character to the query.
    void AppendCharacter(char c);

    /// @brief Removes the last character from the query.
    void BackspaceCharacter();

    /// @brief Clears the query string.
    void ClearQuery();

    /// @brief Toggles the search panel visibility.
    void ToggleSearch();

    /// @brief Returns whether the search panel is active.
    bool IsSearchActive() const;

    /// @brief Gets the current query.
    const char* GetQuery() const { return _query; }

    /// @brief Gets query length.
    int GetQueryLength() const { return _queryLength; }

    /// @brief Gets the current state of the search panel.
    /// @return The current state of the search panel.
    State GetState() const { return _state; }

    /// @brief Gets the index of the selected item.
    /// @return The index of the selected item.
    constexpr int GetSelectedItem() const { return _selectedItem; }

    /// @brief Sets the index of the selected item.
    /// @param selectedItem The index of the selected item to set.
    void SetSelectedItem(int selectedItem) { _selectedItem = selectedItem; }

    /// @brief Returns whether the category name should be displayed.
    /// @return \c true when the category name should be displayed, or \c false otherwise.
    bool ShouldShowCategoryName() const
    {
        return false;
    }

private:
    IRomBrowserController* _romBrowserController;
    State _state = State::NoResults;
    int _selectedItem = -1;
    char _query[QUERY_MAX_LENGTH + 1] = { 0 };
    int _queryLength = 0;
};
