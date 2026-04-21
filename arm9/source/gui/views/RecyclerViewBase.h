#pragma once
#include "View.h"
#include "RecyclerAdapter.h"
#include "gui/FocusManager.h"
#include "core/SharedPtr.h"

/// @brief Abstract base class for a recycler view that displays a possibly large collection of items
///        provided by an adapter in an efficient way.
class RecyclerViewBase : public View
{
public:
    virtual void SetAdapter(SharedPtr<const RecyclerAdapter> adapter, int initialSelectedIndex = 0) = 0;
    virtual void Focus(FocusManager& focusManager) = 0;
    virtual int GetSelectedItem() const = 0;

protected:
    SharedPtr<const RecyclerAdapter> _adapter;
};
