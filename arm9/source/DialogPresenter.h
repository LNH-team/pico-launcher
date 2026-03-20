#pragma once
#include <memory>
#include "animation/Animator.h"
#include "gui/views/DialogView.h"

class StackVramManager;
class FocusManager;
struct TouchEvent;

/// @brief Class for displaying dialogs.
class DialogPresenter
{
public:
    /// @brief Returns a pointer to the currently active dialog (may be nullptr).
    DialogView* GetCurrentDialog() const { return _currentDialog.get(); }
public:
    DialogPresenter(FocusManager* focusManager, StackVramManager* vramManager);

    /// @brief Requests to show the given dialog.
    /// @param dialog The dialog to show.
    void ShowDialog(std::unique_ptr<DialogView> dialog);

    /// @brief Closes the current dialog.
    void CloseDialog();

    /// @brief Updates the dialog presenter.
    void Update();

    /// @brief Handles a touch event for the dialog area.
    /// @param event The touch event.
    /// @param focusManager The focus manager.
    /// @return True if the touch was handled (dialog is active).
    bool HandleTouch(const TouchEvent& event, FocusManager& focusManager);

    /// @brief Applies the clip area of the currently displayed dialog,
    ///        or does nothing if no dialog is being shown.
    /// @param graphicsContext The graphics context to apply to.
    void ApplyClipArea(GraphicsContext& graphicsContext) const;

    /// @brief If a dialog is currently being shown, draws the dialog.
    /// @param graphicsContext The graphics context to use.
    void Draw(GraphicsContext& graphicsContext)
    {
        if (_currentDialog)
            _currentDialog->Draw(graphicsContext);
    }

    /// @brief Performs vblank processes for the displayed dialog.
    void VBlank();

    /// @brief Initializes vram that is needed for showing dialogs.
    void InitVram();

    /// @brief Clears the focus that was stored when a dialog was opened.
    void ClearOldFocus()
    {
        if (_oldFocus)
            _oldFocus->SetVisualFocusRetained(false);
        _oldFocus = nullptr;
    }

    /// @brief Gets the focus that was stored when a dialog was opened.
    /// @return The view that was focused when the current dialog was opened.
    constexpr View* GetOldFocus() const
    {
        return _oldFocus;
    }

    /// @brief Transfers an existing focus anchor to this presenter so dialog
    ///        transitions can avoid restoring focus in-between overlays.
    void SetOldFocus(View* oldFocus)
    {
        if (_oldFocus && _oldFocus != oldFocus)
            _oldFocus->SetVisualFocusRetained(false);

        _oldFocus = oldFocus;
        if (_oldFocus)
            _oldFocus->SetVisualFocusRetained(true);
    }

    /// @brief Returns true if no dialog is being shown or animated.
    bool IsIdle() const { return _curState == State::Idle && !_nextDialog; }

    /// @brief Returns true while dialogs are opening/closing or pending show.
    bool ShouldBlockNonBInput() const;
    bool CanInterruptOpeningWithB() const;

private:
    void ApplyBottomSheetBg(bool visible);

private:
    enum class State
    {
        Idle,
        BottomSheetVisible,
        BottomSheetClosing
    };

    FocusManager* _focusManager;
    StackVramManager* _vramManager;
    u32 _baseVramState;
    std::unique_ptr<DialogView> _currentDialog;
    std::unique_ptr<DialogView> _nextDialog;
    bool _initVram = false;
    View* _oldFocus = nullptr;
    Animator<int> _scrimAnimator;
    Animator<int> _yAnimator;
    int _scrimTargetBlend = 5;
    bool _useBottomSheetBg = true;
    State _curState = State::Idle;
    State _newState = State::Idle;

    bool _touchDraggingDialog = false;
    bool _touchCapturedByDialogContent = false;
    int _touchStartYAnimatorValue = 0;

    static constexpr int kVisibleY = 32;
    static constexpr int kHiddenY = 192;
};
