#pragma once
#include <memory>
#include "core/SharedPtr.h"
#include "animation/Animator.h"
#include "gui/views/DialogView.h"

class StackVramManager;
class FocusManager;

/// @brief Class for displaying dialogs.
class DialogPresenter
{
public:
    DialogPresenter(FocusManager* focusManager, StackVramManager* vramManager);

    /// @brief Requests to show the given dialog.
    /// @param dialog The dialog to show.
    void ShowDialog(SharedPtr<DialogView> dialog);

    /// @brief Closes the current dialog.
    void CloseDialog();

    /// @brief Updates the dialog presenter.
    void Update();

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

    /// @brief Handles a pen down event.
    /// @param touchPoint The touch point.
    /// @param focusManager The focus manager.
    void HandlePenDown(const Point& touchPoint, FocusManager& focusManager);

    /// @brief Handles a pen move event.
    /// @param touchPoint The touch point.
    /// @param focusManager The focus manager.
    void HandlePenMove(const Point& touchPoint, FocusManager& focusManager);

    /// @brief Handles a pen up event.
    /// @param lastTouchPoint The last touch point.
    /// @param focusManager The focus manager.
    void HandlePenUp(const Point& lastTouchPoint, FocusManager& focusManager);

    /// @brief Clears the focus that was stored when a dialog was opened.
    void ClearOldFocus()
    {
        _oldFocus = nullptr;
    }

    /// @brief Gets the focus that was stored when a dialog was opened.
    /// @return The view that was focused when the current dialog was opened.
    constexpr SharedPtr<View> GetOldFocus() const
    {
        return _oldFocus;
    }

    bool IsBottomSheetVisible() const
    {
        return _curState != State::Idle;
    }

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
    SharedPtr<DialogView> _currentDialog;
    SharedPtr<DialogView> _nextDialog;
    bool _initVram = false;
    SharedPtr<View> _oldFocus = nullptr;
    Animator<int> _scrimAnimator;
    Animator<int> _yAnimator;
    State _curState = State::Idle;
    State _newState = State::Idle;
};
