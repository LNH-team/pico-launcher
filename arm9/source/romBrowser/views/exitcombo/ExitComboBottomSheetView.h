#pragma once
#include "core/SharedPtr.h"
#include "gui/Alignment.h"
#include "gui/views/Label2DView.h"
#include "romBrowser/views/BottomSheetView.h"
#include "romBrowser/viewModels/ExitComboViewModel.h"

class MaterialColorScheme;
class IFontRepository;

/// @brief Picks the buttons held to leave a DS game, reached from the menu's
///        title row.
///
/// The combo is handed to the loader when the next game starts, as a KEY_* mask;
/// it does not touch a game already running. The sheet shows the combo in use,
/// A starts capturing one, and a combination of two to four keys that includes
/// SELECT is taken once it has been held steady for a second. B cancels, Y puts
/// the loader's default back. While capturing the sheet keeps every key to
/// itself, START included, so the screenshot hold cannot fire (see
/// App::HandleInput).
class ExitComboBottomSheetView : public BottomSheetView
{
    SHARED_ONLY(ExitComboBottomSheetView)

public:
    void Update() override;
    void Draw(GraphicsContext& graphicsContext) override;
    bool HandleInput(const InputProvider& inputProvider, FocusManager& focusManager) override;
    SharedPtr<View> MoveFocus(const SharedPtr<View>& currentFocus,
        FocusMoveDirection direction, View* source) override;
    bool IsCapturingInput() const override { return _capturing; }
    void Focus(FocusManager& focusManager) override;

protected:
    void Close() override;

private:
    /// @brief Frames a valid combination has to stay held before it is taken:
    ///        a second, long enough that a stray press cannot become a combo.
    static constexpr int kHoldFrames = 60;

    SharedPtr<ExitComboViewModel> _viewModel;
    const MaterialColorScheme* _materialColorScheme;
    SharedPtr<Label2DView> _titleLabel;
    SharedPtr<Label2DView> _comboLabel;
    SharedPtr<Label2DView> _statusLabel;
    SharedPtr<Label2DView> _hintButtons[3];
    SharedPtr<Label2DView> _hintDescriptions[3];
    bool _capturing = false;
    u16 _heldKeys = 0;
    u16 _steadyKeys = 0;
    int _steadyFrames = 0;

    ExitComboBottomSheetView(SharedPtr<ExitComboViewModel> viewModel,
        const MaterialColorScheme* materialColorScheme, const IFontRepository* fontRepository);

    SharedPtr<Label2DView> AddLabel(const IFontRepository* fontRepository, FontType fontType,
        u32 width, u32 maxChars, const char* text, Alignment alignment);
    void UpdateText();
};
