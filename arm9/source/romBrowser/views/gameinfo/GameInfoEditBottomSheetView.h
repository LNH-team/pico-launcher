#pragma once
#include "core/SharedPtr.h"
#include "gui/Alignment.h"
#include "gui/views/KeyboardView.h"
#include "gui/views/Label2DView.h"
#include "romBrowser/views/BottomSheetView.h"
#include "romBrowser/viewModels/GameInfoEditViewModel.h"

class MaterialColorScheme;
class IFontRepository;

/// @brief Edits the name, developer and publisher shown for a game, reached
///        from the menu. Roms without a banner (GBA, SNES, emulator shortcuts)
///        only have a file name, which is often not what the game is called;
///        the values entered here are stored in gamedata.json with the game's
///        favorites and play time, keyed by the same file name.
///
/// The sheet has two modes. It opens on the three entries, picked with the
/// d-pad, where A or Start starts editing one and B or Select leaves the sheet
/// and stores what was typed. While editing, the shared KeyboardView (see
/// gui/views) owns the d-pad and A, B, X and Y, so typing stays on the keys;
/// Start and Select come back here and return to the three entries. From the
/// entries Select leaves the sheet, so a second press backs out of an entry
/// being edited.
class GameInfoEditBottomSheetView : public BottomSheetView
{
    SHARED_ONLY(GameInfoEditBottomSheetView)

public:
    void Update() override;
    void Draw(GraphicsContext& graphicsContext) override;
    bool HandleInput(const InputProvider& inputProvider, FocusManager& focusManager) override;
    SharedPtr<View> MoveFocus(const SharedPtr<View>& currentFocus,
        FocusMoveDirection direction, View* source) override;
    void Focus(FocusManager& focusManager) override;

    /// @brief START edits and confirms here, so the screenshot hold must not see it.
    bool IsCapturingInput() const override { return true; }

protected:
    void Close() override;

private:
    static constexpr int kFieldCount = 3;
    /// @brief Bytes the three fields may hold, matching GameDataEntry.
    static constexpr u32 kFieldLimits[kFieldCount] =
    {
        GameDataEntry::kMaxDisplayNameLength,
        GameDataEntry::kMaxDeveloperLength,
        GameDataEntry::kMaxPublisherLength
    };

    SharedPtr<GameInfoEditViewModel> _viewModel;
    const MaterialColorScheme* _materialColorScheme;
    SharedPtr<Label2DView> _titleLabel;
    SharedPtr<Label2DView> _fieldLabels[kFieldCount];
    SharedPtr<Label2DView> _valueLabels[kFieldCount];
    /// @brief The shared on-screen keyboard, editing the selected field.
    SharedPtr<KeyboardView> _keyboard;
    SharedPtr<Label2DView> _hintLabel;
    char _values[kFieldCount][GameDataEntry::kMaxDisplayNameLength + 1] = {};
    int _field = 0;
    /// @brief True while the keyboard is up and the entries are set aside.
    bool _editing = false;

    GameInfoEditBottomSheetView(SharedPtr<GameInfoEditViewModel> viewModel,
        const MaterialColorScheme* materialColorScheme, const IFontRepository* fontRepository);

    SharedPtr<Label2DView> AddLabel(const IFontRepository* fontRepository, FontType fontType,
        u32 width, u32 height, u32 maxChars, const char* text, Alignment alignment);
    void BeginEditing(FocusManager& focusManager);
    void EndEditing(FocusManager& focusManager);
    void SaveAndClose();
    void Refresh();
    static void OnKeyboardTextChanged(void* arg);
};
