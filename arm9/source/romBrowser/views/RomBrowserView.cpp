#include "common.h"
#include "IconGridItemView.h"
#include "gui/GraphicsContext.h"
#include "gui/input/InputProvider.h"
#include "RomBrowserView.h"

RomBrowserView::RomBrowserView(
    SharedPtr<RomBrowserViewModel> viewModel,
    const RomBrowserDisplayMode& displayMode,
    const IThemeFileIconFactory* themeFileIconFactory,
    const IRomBrowserViewFactory* romBrowserViewFactory,
    VBlankTextureLoader* vblankTextureLoader)
    : _viewModel(std::move(viewModel)), _isVertical(displayMode.IsVertical())
{
    _fileGridView = displayMode.CreateRecyclerView(romBrowserViewFactory);
    AddChildTail(_fileGridView.GetPointer());
    _fileRecyclerAdapter = displayMode.CreateRecyclerAdapter(
        _viewModel.GetPointer(), themeFileIconFactory, romBrowserViewFactory, vblankTextureLoader);
}

void RomBrowserView::InitVram(const VramContext& vramContext)
{
    _fileRecyclerAdapter->InitVram(vramContext); // first initialize the shared vram for the items
    _fileGridView->SetAdapter(_fileRecyclerAdapter, _viewModel->GetSelectedItem()); // set the adapter of the recycler
    _fileGridView->InitVram(vramContext); // init the vram for the recycler and its items
}

void RomBrowserView::Update()
{
    _fileRecyclerAdapter->SetIconFrameCounter(_viewModel->GetIconFrameCounter());
    _fileGridView->Update();
    _viewModel->SetSelectedItem(_fileGridView->GetSelectedItem());
}

SharedPtr<View> RomBrowserView::MoveFocus(const SharedPtr<View>& currentFocus, FocusMoveDirection direction, View* source)
{
    if (!currentFocus)
    {
        return nullptr;
    }
    if (source == GetParent())
    {
        if (_isVertical)
        {
            if (direction == FocusMoveDirection::Right)
            {
                return _fileGridView->MoveFocus(currentFocus, direction, this);
            }
        }
        else
        {
            if (direction == FocusMoveDirection::Down)
            {
                return _fileGridView->MoveFocus(currentFocus, direction, this);
            }
        }
        return nullptr;
    }
    else if (source == _fileGridView.GetPointer())
    {
        return View::MoveFocus(currentFocus, direction, source);
    }
    return nullptr;
}
