#include "common.h"
#include <string.h>
#include "IconGridItemView.h"
#include "gui/GraphicsContext.h"
#include "gui/input/InputProvider.h"
#include "gui/views/Label3DView.h"
#include "themes/FontType.h"
#include "themes/IFontRepository.h"
#include "themes/material/MaterialColorScheme.h"
#include "RomBrowserView.h"

RomBrowserView::RomBrowserView(
    SharedPtr<RomBrowserViewModel> viewModel,
    const RomBrowserDisplayMode& displayMode,
    const IThemeFileIconFactory* themeFileIconFactory,
    const IRomBrowserViewFactory* romBrowserViewFactory,
    VBlankTextureLoader* vblankTextureLoader,
    const IFontRepository* fontRepository,
    const MaterialColorScheme* materialColorScheme)
    : _viewModel(std::move(viewModel))
    , _isVertical(displayMode.IsVertical())
    , _fontRepository(fontRepository)
{
    _fileGridView = displayMode.CreateRecyclerView(romBrowserViewFactory);
    AddChildTail(_fileGridView.GetPointer());
    _fileRecyclerAdapter = displayMode.CreateRecyclerAdapter(
        _viewModel.GetPointer(), themeFileIconFactory, romBrowserViewFactory, vblankTextureLoader);

    if (_isVertical)
    {
        _emptyStateLabel = Label3DView::CreateShared(208, 16, 50, _fontRepository->GetFont(FontType::Regular10), vblankTextureLoader);
        _emptyStateLabel->SetPosition(40, 88);
    }
    else
    {
        _emptyStateLabel = Label3DView::CreateShared(240, 16, 50, _fontRepository->GetFont(FontType::Regular10), vblankTextureLoader);
        _emptyStateLabel->SetPosition(8, 88);
    }
    _emptyStateLabel->SetHorizontalAlignment(Alignment::Center);
    _emptyStateLabel->SetForegroundColor(materialColorScheme->onSurfaceVariant);
    _emptyStateLabel->SetParent(this);
}

void RomBrowserView::InitVram(const VramContext& vramContext)
{
    _fileRecyclerAdapter->InitVram(vramContext); // first initialize the shared vram for the items
    _fileGridView->SetAdapter(_fileRecyclerAdapter, _viewModel->GetSelectedItem()); // set the adapter of the recycler
    _fileGridView->InitVram(vramContext); // init the vram for the recycler and its items
    _emptyStateLabel->InitVram(vramContext);
}

void RomBrowserView::Update()
{
    _fileRecyclerAdapter->SetIconFrameCounter(_viewModel->GetIconFrameCounter());
    if (_viewModel->GetFileInfoManager().GetItemCount() == 0)
    {
        const char* currentPath = _viewModel->GetRomBrowserController()->GetCurrentPath();
        if (strcmp(currentPath, ":favorites") == 0)
        {
            _emptyStateLabel->SetText("No favorites added yet");
        }
        else
        {
            _emptyStateLabel->SetText("This folder is empty");
        }
        _emptyStateLabel->Update();
    }
    else
    {
        _fileGridView->Update();
        _viewModel->SetSelectedItem(_fileGridView->GetSelectedItem());
    }
}

void RomBrowserView::Draw(GraphicsContext& graphicsContext)
{
    if (_viewModel->GetFileInfoManager().GetItemCount() == 0)
    {
        _emptyStateLabel->Draw(graphicsContext);
    }
    else
    {
        ViewContainer::Draw(graphicsContext);
    }
}

void RomBrowserView::VBlank()
{
    if (_viewModel->GetFileInfoManager().GetItemCount() == 0)
    {
        _emptyStateLabel->VBlank();
    }
    else
    {
        ViewContainer::VBlank();
    }
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
