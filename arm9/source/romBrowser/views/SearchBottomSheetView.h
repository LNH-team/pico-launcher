#pragma once
#include <array>
#include <memory>
#include "core/SharedPtr.h"
#include "romBrowser/views/BottomSheetView.h"
#include "gui/views/Label2DView.h"
#include "romBrowser/viewModels/SearchViewModel.h"

class MaterialColorScheme;
class IFontRepository;

/// @brief Bottom sheet placeholder for search.
class SearchBottomSheetView : public BottomSheetView
{
    SHARED_ONLY(SearchBottomSheetView)
public:
    SearchBottomSheetView(std::unique_ptr<SearchViewModel> viewModel,
        const MaterialColorScheme* materialColorScheme, const IFontRepository* fontRepository,
        FocusManager* focusManager);

    void InitVram(const VramContext& vramContext) override;
    void Update() override;
    void Draw(GraphicsContext& graphicsContext) override;
    bool HandleInput(const InputProvider& inputProvider, FocusManager& focusManager) override;
    SharedPtr<View> MoveFocus(const SharedPtr<View>& currentFocus,
        FocusMoveDirection direction, View* source) override;

    void Focus(FocusManager& focusManager) override
    {
        focusManager.Focus(_keyLabels[_selectedKey]);
    }

protected:
    void Close() override;

private:
    enum class KeyType
    {
        Character,
        Space,
        Backspace,
        Clear,
        Close
    };

    struct KeySpec
    {
        const char16_t* label;
        KeyType type;
        char character;
    };

    static constexpr int KEYBOARD_COLS = 8;
    static constexpr int KEYBOARD_ROWS = 5;
    static constexpr int KEY_COUNT = KEYBOARD_COLS * KEYBOARD_ROWS;

    std::unique_ptr<SearchViewModel> _viewModel;
    SharedPtr<Label2DView> _titleLabel;
    SharedPtr<Label2DView> _searchLabel;
    SharedPtr<Label2DView> _secondaryLabel;
    std::array<SharedPtr<Label2DView>, KEY_COUNT> _keyLabels;
    const MaterialColorScheme* _materialColorScheme;
    FocusManager* _focusManager;
    int _selectedKey = 0;
    std::array<char16_t, SearchViewModel::QUERY_MAX_LENGTH + 1> _queryBuffer;

    static const std::array<KeySpec, KEY_COUNT> sKeySpecs;

    void UpdateQueryLabel();
    void UpdateStatusLabel();
    int GetKeyIndexFromView(const SharedPtr<View>& view) const;
    void HandleSelectedKey();
};
