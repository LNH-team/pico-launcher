#pragma once
#include "ViewContainer.h"
#include "DialogType.h"

/// @brief View meant to be displayed as a dialog on top of other content.
class DialogView : public ViewContainer
{
public:
    /// @brief Gets the type of dialog.
    /// @return The type of dialog.
    virtual DialogType GetDialogType() const = 0;

    /// @brief Gets the type ID of the dialog for RTTI.
    /// @return The unique ID of the dialog type.
    virtual int GetDialogTypeId() const { return 0; }

    /// @brief Moves the focus to this dialog.
    /// @param focusManager The focus manager to use.
    virtual void Focus(FocusManager& focusManager) = 0;

    /// @brief Gets the area of the screen that will be fully covered by
    ///        this dialog for the purpose of culling views behind it.
    /// @return A rectangle that is fully covered by the dialog.
    virtual Rectangle GetFullyCoveredArea() const = 0;
};
