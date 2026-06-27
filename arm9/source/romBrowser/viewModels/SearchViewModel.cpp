#include "common.h"
#include <cstring>
#include "SearchViewModel.h"

SearchViewModel::SearchViewModel(IRomBrowserController* romBrowserController)
    : _romBrowserController(romBrowserController)
{
}

void SearchViewModel::ActivateSelectedItem()
{
    ApplySearchAndClose();
}

bool SearchViewModel::NavigateUp()
{
    Close();
    return false;
}

void SearchViewModel::Close()
{
    LOG_DEBUG("SearchViewModel::Close\n");
    _romBrowserController->HideSearch();
}

void SearchViewModel::ApplySearchAndClose()
{
    LOG_DEBUG("SearchViewModel::ApplySearchAndClose query='%s'\n", _query);
    _romBrowserController->SetSearchQuery(_query);
    _romBrowserController->RefreshRomBrowserViewModel();
    Close();
}

void SearchViewModel::AppendCharacter(char c)
{
    if (_queryLength >= QUERY_MAX_LENGTH)
    {
        return;
    }

    _query[_queryLength++] = c;
    _query[_queryLength] = 0;
    _state = State::DisplayResults;
    LOG_DEBUG("Search query: %s\n", _query);
}

void SearchViewModel::BackspaceCharacter()
{
    if (_queryLength <= 0)
    {
        return;
    }

    _query[--_queryLength] = 0;
    _state = _queryLength == 0 ? State::NoResults : State::DisplayResults;
    LOG_DEBUG("Search query: %s\n", _query);
}

void SearchViewModel::ClearQuery()
{
    if (_queryLength == 0)
    {
        return;
    }

    std::memset(_query, 0, sizeof(_query));
    _queryLength = 0;
    _state = State::NoResults;
    LOG_DEBUG("Search query cleared\n");
}

void SearchViewModel::ToggleSearch()
{
    if (IsSearchActive())
    {
        _romBrowserController->SetSearchQuery("");
        _romBrowserController->RefreshRomBrowserViewModel();
        _romBrowserController->HideSearch();
        return;
    }

    _romBrowserController->ShowSearch();
}

bool SearchViewModel::IsSearchActive() const
{
    const char* query = _romBrowserController->GetSearchQuery();
    return query && query[0] != 0;
}
