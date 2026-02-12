#include "common.h"
#include "../viewModels/RomBrowserAppBarViewModel.h"
#include "gui/GraphicsContext.h"
#include "gui/VramContext.h"
#include "backIcon.h"
#include "settingsIcon.h"
#include "heartIcon.h"
#include "recentIcon.h"
#include "hGridIcon.h"
#include "vGridIcon.h"
#include "bannerListIcon.h"
#include "coverflowIcon.h"
#include "listIcon.h"
#include "gui/IVramManager.h"
#include "../DisplayMode/RomBrowserDisplayMode.h"
#include "RomBrowserAppBarView.h"
#include "gui/input/InputKey.h"
#include "gui/input/InputProvider.h"

RomBrowserAppBarView::RomBrowserAppBarView(
    RomBrowserAppBarViewModel* viewModel, const RomBrowserDisplayMode& displayMode,
    const IRomBrowserViewFactory* romBrowserViewFactory)
    : _viewModel(viewModel)
{
    _appBarView = displayMode.CreateAppBarView(romBrowserViewFactory, 1, 2);
    _appBarView->SetParent(this);

    _appBarView->SetButtonAction(APP_BAR_BUTTON_BACK, [] (IconButtonView* sender, void* arg)
    {
        ((RomBrowserAppBarViewModel*)arg)->NavigateUp();
    }, _viewModel);
    _appBarView->SetButtonAction(APP_BAR_BUTTON_DISPLAY_SETTINGS, [] (IconButtonView* sender, void* arg)
    {
        ((RomBrowserAppBarViewModel*)arg)->ShowDisplaySettings();
    }, _viewModel);
    _appBarView->SetButtonAction(APP_BAR_BUTTON_FAVORITES, [] (IconButtonView* sender, void* arg)
    {
        ((RomBrowserAppBarViewModel*)arg)->ToggleFavoritesView();
    }, _viewModel);
}

bool RomBrowserAppBarView::HandleInput(const InputProvider& inputProvider, FocusManager& focusManager)
{
    if (inputProvider.Triggered(InputKey::B))
    {
        if (_viewModel->IsFavoritesViewActive())
        {
            _appBarView->Focus(focusManager, APP_BAR_BUTTON_FAVORITES);
            _viewModel->ToggleFavoritesView();
            return true;
        }
    }
    return View::HandleInput(inputProvider, focusManager);
}

void RomBrowserAppBarView::InitVram(const VramContext& vramContext)
{
    _appBarView->InitVram(vramContext);

    const auto objVramManager = vramContext.GetObjVramManager();
    if (objVramManager)
    {
        u32 backIconVramOffset = objVramManager->Alloc(backIconTilesLen);
        dma_ntrCopy32(3, backIconTiles, objVramManager->GetVramAddress(backIconVramOffset), backIconTilesLen);
        _appBarView->SetButtonIcon(APP_BAR_BUTTON_BACK, backIconVramOffset);

        u32 settingsIconVramOffset = objVramManager->Alloc(settingsIconTilesLen);
        dma_ntrCopy32(3, settingsIconTiles, objVramManager->GetVramAddress(settingsIconVramOffset), settingsIconTilesLen);
        _appBarView->SetButtonIcon(APP_BAR_BUTTON_DISPLAY_SETTINGS, settingsIconVramOffset);

        u32 heartIconVramOffset = objVramManager->Alloc(heartIconTilesLen);
        dma_ntrCopy32(3, heartIconTiles, objVramManager->GetVramAddress(heartIconVramOffset), heartIconTilesLen);
        _appBarView->SetButtonIcon(APP_BAR_BUTTON_FAVORITES, heartIconVramOffset);

        // u32 settingsIconVramOffset = objVramManager->Alloc(settingsIconTilesLen);
        // dma_ntrCopy32(3, settingsIconTiles, objVramManager->GetVramAddress(settingsIconVramOffset), settingsIconTilesLen);
        // _appBarView->SetButtonIcon(APP_BAR_BUTTON_SETTINGS, settingsIconVramOffset);

        // u32 heartIconVramOffset = objVramManager->Alloc(heartIconTilesLen);
        // dma_ntrCopy32(3, heartIconTiles, objVramManager->GetVramAddress(heartIconVramOffset), heartIconTilesLen);
        // _appBarView->SetButtonIcon(APP_BAR_BUTTON_FAVORITE, heartIconVramOffset);

        // u32 recentIconVramOffset = objVramManager->Alloc(recentIconTilesLen);
        // dma_ntrCopy32(3, recentIconTiles, objVramManager->GetVramAddress(recentIconVramOffset), recentIconTilesLen);
        // _appBarView->SetButtonIcon(APP_BAR_BUTTON_RECENT, recentIconVramOffset);

        // u32 displaySettingsIconVramOffset;
        // switch (_viewModel->GetRomBrowserLayout())
        // {
        //     case RomBrowserLayout::HorizontalIconGrid:
        //     default:
        //     {
        //         displaySettingsIconVramOffset = objVramManager->Alloc(hGridIconTilesLen);
        //         dma_ntrCopy32(3, hGridIconTiles, objVramManager->GetVramAddress(displaySettingsIconVramOffset), hGridIconTilesLen);
        //         break;
        //     }
        //     case RomBrowserLayout::VerticalIconGrid:
        //     {
        //         displaySettingsIconVramOffset = objVramManager->Alloc(vGridIconTilesLen);
        //         dma_ntrCopy32(3, vGridIconTiles, objVramManager->GetVramAddress(displaySettingsIconVramOffset), vGridIconTilesLen);
        //         break;
        //     }
        //     case RomBrowserLayout::BannerList:
        //     {
        //         displaySettingsIconVramOffset = objVramManager->Alloc(bannerListIconTilesLen);
        //         dma_ntrCopy32(3, bannerListIconTiles, objVramManager->GetVramAddress(displaySettingsIconVramOffset), bannerListIconTilesLen);
        //         break;
        //     }
        //     case RomBrowserLayout::FileList:
        //     {
        //         displaySettingsIconVramOffset = objVramManager->Alloc(listIconTilesLen);
        //         dma_ntrCopy32(3, listIconTiles, objVramManager->GetVramAddress(displaySettingsIconVramOffset), listIconTilesLen);
        //         break;
        //     }
        //     case RomBrowserLayout::CoverFlow:
        //     {
        //         displaySettingsIconVramOffset = objVramManager->Alloc(coverflowIconTilesLen);
        //         dma_ntrCopy32(3, coverflowIconTiles, objVramManager->GetVramAddress(displaySettingsIconVramOffset), coverflowIconTilesLen);
        //         break;
        //     }
        // }
        // _appBarView->SetButtonIcon(APP_BAR_BUTTON_DISPLAY_SETTINGS, displaySettingsIconVramOffset);
    }
}

void RomBrowserAppBarView::Update()
{
    _appBarView->SetButtonState(APP_BAR_BUTTON_FAVORITES,
        _viewModel->IsFavoritesViewActive()
            ? IconButtonView::State::ToggleSelected
            : IconButtonView::State::ToggleUnselected);
    _appBarView->Update();
}

void RomBrowserAppBarView::Draw(GraphicsContext& graphicsContext)
{
    _appBarView->Draw(graphicsContext);
}

void RomBrowserAppBarView::VBlank()
{
    _appBarView->VBlank();
}

View* RomBrowserAppBarView::MoveFocus(View* currentFocus, FocusMoveDirection direction, View* source)
{
    if (currentFocus == nullptr)
    {
        return nullptr;
    }
    if (source == _appBarView.get())
    {
        return View::MoveFocus(currentFocus, direction, source);
    }
    else if (source == GetParent())
    {
        return _appBarView->MoveFocus(currentFocus, direction, this);
    }
    return nullptr;
}

