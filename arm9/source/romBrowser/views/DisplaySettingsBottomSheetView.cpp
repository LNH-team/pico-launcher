#include "common.h"
#include "gui/GraphicsContext.h"
#include "gui/VramContext.h"
#include "gui/IVramManager.h"
#include "hGridIcon.h"
#include "vGridIcon.h"
#include "bannerListIcon.h"
#include "listIcon.h"
#include "sortNameAscendingIcon.h"
#include "sortNameDescendingIcon.h"
#include "recentIcon.h"
#include "gamesIcon.h"
#include "picturesIcon.h"
#include "musicIcon.h"
#include "moviesIcon.h"
#include "unknownIcon.h"
#include "coverflowIcon.h"
#include "../IRomBrowserController.h"
#include "gui/input/InputProvider.h"
#include "themes/material/MaterialColorScheme.h"
#include "themes/IFontRepository.h"
#include "DisplaySettingsBottomSheetView.h"

#define TITLE_LABEL_X       20
#define TITLE_LABEL_Y       16

#define LAYOUT_LABEL_X      20
#define LAYOUT_LABEL_Y      46

#define SORTING_LABEL_X     20
#define SORTING_LABEL_Y     78

#define FILTERS_LABEL_X     20
#define FILTERS_LABEL_Y     112

static RomBrowserLayout sRomBrowserDisplayModes[4] =
{
    [0] = RomBrowserLayout::HorizontalIconGrid,
    [1] = RomBrowserLayout::VerticalIconGrid,
    [2] = RomBrowserLayout::BannerList,
    [3] = RomBrowserLayout::CoverFlow
};

static RomBrowserSortMode sRomBrowserSortModes[4] =
{
    [0] = RomBrowserSortMode::NameAscending,
    [1] = RomBrowserSortMode::NameDescending,
    [2] = RomBrowserSortMode::LastModified
};

DisplaySettingsBottomSheetView::DisplaySettingsBottomSheetView(
    DisplaySettingsViewModel* viewModel, const MaterialColorScheme* materialColorScheme,
    const IFontRepository* fontRepository)
    : _viewModel(viewModel)
    , _titleLabel(Label2DView::CreateShared(128, 16, 25, fontRepository->GetFont(FontType::Medium11)))
    , _layoutLabel(Label2DView::CreateShared(64, 16, 25, fontRepository->GetFont(FontType::Regular10)))
    , _sortingLabel(Label2DView::CreateShared(64, 16, 25, fontRepository->GetFont(FontType::Regular10)))
    , _materialColorScheme(materialColorScheme)
    // , _filtersLabel(64, 16, 25, fontRepository->GetFont(FontType::Regular10))
{
    _titleLabel->SetText(u"Display Settings");
    AddChildTail(_titleLabel.GetPointer());
    _layoutLabel->SetText(u"Layout");
    AddChildTail(_layoutLabel.GetPointer());
    _sortingLabel->SetText(u"Sorting");
    AddChildTail(_sortingLabel.GetPointer());
    // _filtersLabel.SetText(u"Filters");
    // AddChildTail(&_filtersLabel);

    for (auto& layoutOption : _layoutOptions)
    {
        layoutOption = CreateLayoutOptionIconButton();
        AddChildTail(layoutOption.GetPointer());
    }

    for (auto& sortOption : _sortOptions)
    {
        sortOption = CreateSortOptionIconButton();
        AddChildTail(sortOption.GetPointer());
    }

    // for (auto& filterOption : _filterOptions)
    // {
    //     filterOption = CreateFilterOptionIconButton();
    //     AddChildTail(&filterOption);
    // }

    // _filterOptions[0].SetState(IconButtonView::State::ToggleSelected);
}

SharedPtr<IconButton2DView> DisplaySettingsBottomSheetView::CreateLayoutOptionIconButton()
{
    auto layoutOption = IconButton2DView::CreateShared(
        IconButtonView::Type::Tonal,
        IconButtonView::State::ToggleUnselected,
        md::sys::color::surfaceContainerLow,
        _materialColorScheme
    );
    layoutOption->SetAction([] (IconButtonView* sender, void* arg)
    {
        auto self = reinterpret_cast<DisplaySettingsBottomSheetView*>(arg);
        for (u32 i = 0; i < self->_layoutOptions.size(); i++)
        {
            if (self->_layoutOptions[i].GetPointer() == sender)
            {
                self->_viewModel->SetRomBrowserDisplayMode(sRomBrowserDisplayModes[i]);
                break;
            }
        }
    }, this);
    return layoutOption;
}

SharedPtr<IconButton2DView> DisplaySettingsBottomSheetView::CreateSortOptionIconButton()
{
    auto sortOption = IconButton2DView::CreateShared(
        IconButtonView::Type::Tonal,
        IconButtonView::State::ToggleUnselected,
        md::sys::color::surfaceContainerLow,
        _materialColorScheme
    );
    sortOption->SetAction([] (IconButtonView* sender, void* arg)
    {
        auto self = reinterpret_cast<DisplaySettingsBottomSheetView*>(arg);
        for (u32 i = 0; i < self->_sortOptions.size(); i++)
        {
            if (self->_sortOptions[i].GetPointer() == sender)
            {
                self->_viewModel->SetRomBrowserSortMode(sRomBrowserSortModes[i]);
                break;
            }
        }
    }, this);
    return sortOption;
}

// IconButtonView DisplaySettingsBottomSheetView::CreateFilterOptionIconButton()
// {
//     IconButtonView filterOption
//     {
//         IconButtonView::Type::Tonal,
//         IconButtonView::State::ToggleUnselected,
//         md::sys::color::surfaceContainerLow,
//         _materialColorScheme
//     };
//     return filterOption;
// }

void DisplaySettingsBottomSheetView::InitVram(const VramContext& vramContext)
{
    BottomSheetView::InitVram(vramContext);

    const auto objVramManager = vramContext.GetObjVramManager();
    if (objVramManager)
    {
        // layout options
        _layoutOptions[0]->SetIconVramOffset(LoadIcon(*objVramManager, hGridIconTiles, hGridIconTilesLen));
        _layoutOptions[1]->SetIconVramOffset(LoadIcon(*objVramManager, vGridIconTiles, vGridIconTilesLen));
        _layoutOptions[2]->SetIconVramOffset(LoadIcon(*objVramManager, bannerListIconTiles, bannerListIconTilesLen));
        _layoutOptions[3]->SetIconVramOffset(LoadIcon(*objVramManager, coverflowIconTiles, coverflowIconTilesLen));

        // sort options
        _sortOptions[0]->SetIconVramOffset(LoadIcon(*objVramManager, sortNameAscendingIconTiles, sortNameAscendingIconTilesLen));
        _sortOptions[1]->SetIconVramOffset(LoadIcon(*objVramManager, sortNameDescendingIconTiles, sortNameDescendingIconTilesLen));
        // _sortOptions[2].SetIconVramOffset(LoadIcon(objVramManager, recentIconTiles, recentIconTilesLen));

        // filter options
        // _filterOptions[0].SetIconVramOffset(LoadIcon(objVramManager, gamesIconTiles, gamesIconTilesLen));
        // _filterOptions[1].SetIconVramOffset(LoadIcon(objVramManager, picturesIconTiles, picturesIconTilesLen));
        // _filterOptions[2].SetIconVramOffset(LoadIcon(objVramManager, musicIconTiles, musicIconTilesLen));
        // _filterOptions[3].SetIconVramOffset(LoadIcon(objVramManager, moviesIconTiles, moviesIconTilesLen));
        // _filterOptions[4].SetIconVramOffset(LoadIcon(objVramManager, unknownIconTiles, unknownIconTilesLen));
    }
}

void DisplaySettingsBottomSheetView::UpdateLabels()
{
    _titleLabel->SetPosition(TITLE_LABEL_X, _position.y + TITLE_LABEL_Y);
    _layoutLabel->SetPosition(LAYOUT_LABEL_X, _position.y + LAYOUT_LABEL_Y);
    _sortingLabel->SetPosition(SORTING_LABEL_X, _position.y + SORTING_LABEL_Y);
    // _filtersLabel.SetPosition(FILTERS_LABEL_X, _position.y + FILTERS_LABEL_Y);
}

void DisplaySettingsBottomSheetView::Update()
{
    BottomSheetView::Update();
    UpdateLabels();
    auto selectedDisplayMode = _viewModel->GetRomBrowserDisplayMode();
    int x = 70;
    u32 idx = 0;
    for (auto& layoutOption : _layoutOptions)
    {
        layoutOption->SetPosition(x, _position.y + 38);
        layoutOption->SetState(sRomBrowserDisplayModes[idx] == selectedDisplayMode
            ? IconButtonView::State::ToggleSelected
            : IconButtonView::State::ToggleUnselected);
        x += 32;
        idx++;
    }
    auto selectedSortMode = _viewModel->GetRomBrowserSortMode();
    x = 70;
    idx = 0;
    for (auto& sortOption : _sortOptions)
    {
        sortOption->SetPosition(x, _position.y + 70);
        sortOption->SetState(sRomBrowserSortModes[idx] == selectedSortMode
            ? IconButtonView::State::ToggleSelected
            : IconButtonView::State::ToggleUnselected);
        x += 32;
        idx++;
    }
    // x = 70;
    // for (auto& filterOption : _filterOptions)
    // {
    //     filterOption.SetPosition(x, _position.y + 102);
    //     x += 32;
    // }
}

void DisplaySettingsBottomSheetView::Draw(GraphicsContext& graphicsContext)
{
    graphicsContext.SetClipArea(GetBounds());
    u32 oldPrio = graphicsContext.SetPriority(1);
    {
        _titleLabel->SetBackgroundColor(_materialColorScheme->GetColor(md::sys::color::surfaceContainerLow));
        _titleLabel->SetForegroundColor(_materialColorScheme->onSurface);
        _layoutLabel->SetBackgroundColor(_materialColorScheme->GetColor(md::sys::color::surfaceContainerLow));
        _layoutLabel->SetForegroundColor(_materialColorScheme->onSurfaceVariant);
        _sortingLabel->SetBackgroundColor(_materialColorScheme->GetColor(md::sys::color::surfaceContainerLow));
        _sortingLabel->SetForegroundColor(_materialColorScheme->onSurfaceVariant);
        // _filtersLabel.SetBackgroundColor(_materialColorScheme->GetColor(md::sys::color::surfaceContainerLow));
        // _filtersLabel.SetForegroundColor(_materialColorScheme->onSurfaceVariant);
        BottomSheetView::Draw(graphicsContext);
    }
    graphicsContext.SetPriority(oldPrio);
    graphicsContext.ResetClipArea();
}

bool DisplaySettingsBottomSheetView::HandleInput(
    const InputProvider& inputProvider, FocusManager& focusManager)
{
    if (inputProvider.Triggered(InputKey::B))
    {
        _viewModel->Close();
        return true;
    }
    return false;
}

SharedPtr<View> DisplaySettingsBottomSheetView::MoveFocus(const SharedPtr<View>& currentFocus,
    FocusMoveDirection direction, View* source)
{
    int idx = 0;
    for (auto& layoutOption : _layoutOptions)
    {
        if (currentFocus.GetPointer() == layoutOption.GetPointer())
        {
            if (direction == FocusMoveDirection::Left)
            {
                if (--idx < 0)
                    idx += _layoutOptions.size();
                return _layoutOptions[idx];
            }
            else if (direction == FocusMoveDirection::Right)
            {
                if (++idx >= (int)_layoutOptions.size())
                    idx = 0;
                return _layoutOptions[idx];
            }
            // else if (direction == FocusMoveDirection::Up)
            // {
            //     if (idx >= (int)_filterOptions.size())
            //         idx = _filterOptions.size() - 1;
            //     return &_filterOptions[idx];
            // }
            else //if (direction == FocusMoveDirection::Down)
            {
                if (idx >= (int)_sortOptions.size())
                    idx = _sortOptions.size() - 1;
                return _sortOptions[idx];
            }
        }
        idx++;
    }
    idx = 0;
    for (auto& sortOption : _sortOptions)
    {
        if (currentFocus.GetPointer() == sortOption.GetPointer())
        {
            if (direction == FocusMoveDirection::Left)
            {
                if (--idx < 0)
                    idx += _sortOptions.size();
                return _sortOptions[idx];
            }
            else if (direction == FocusMoveDirection::Right)
            {
                if (++idx >= (int)_sortOptions.size())
                    idx = 0;
                return _sortOptions[idx];
            }
            else //if (direction == FocusMoveDirection::Up)
            {
                if (idx >= (int)_layoutOptions.size())
                    idx = _layoutOptions.size() - 1;
                return _layoutOptions[idx];
            }
            // else //if (direction == FocusMoveDirection::Down)
            // {
            //     if (idx >= (int)_filterOptions.size())
            //         idx = _filterOptions.size() - 1;
            //     return &_filterOptions[idx];
            // }
        }
        idx++;
    }
    // idx = 0;
    // for (auto& filterOption : _filterOptions)
    // {
    //     if (currentFocus == &filterOption)
    //     {
    //         if (direction == FocusMoveDirection::Left)
    //         {
    //             if (--idx < 0)
    //                 idx += _filterOptions.size();
    //             return &_filterOptions[idx];
    //         }
    //         else if (direction == FocusMoveDirection::Right)
    //         {
    //             if (++idx >= (int)_filterOptions.size())
    //                 idx = 0;
    //             return &_filterOptions[idx];
    //         }
    //         else if (direction == FocusMoveDirection::Up)
    //         {
    //             if (idx >= (int)_sortOptions.size())
    //                 idx = _sortOptions.size() - 1;
    //             return &_sortOptions[idx];
    //         }
    //         else //if (direction == FocusMoveDirection::Down)
    //         {
    //             if (idx >= (int)_layoutOptions.size())
    //                 idx = _layoutOptions.size() - 1;
    //             return &_layoutOptions[idx];
    //         }
    //     }
    //     idx++;
    // }
    return nullptr;
}

void DisplaySettingsBottomSheetView::SetGraphics(
    const IconButton2DView::VramToken& iconButtonVramToken)
{
    for (auto& layoutOption : _layoutOptions)
    {
        layoutOption->SetGraphics(iconButtonVramToken);
    }
    for (auto& sortOption : _sortOptions)
    {
        sortOption->SetGraphics(iconButtonVramToken);
    }
    // for (auto& filterOption : _filterOptions)
    //     filterOption.SetGraphics(iconButtonVramToken);
}

void DisplaySettingsBottomSheetView::Close()
{
    _viewModel->Close();
}

u32 DisplaySettingsBottomSheetView::LoadIcon(IVramManager& vramManager,
    const unsigned int* tiles, u32 tilesLength) const
{
    u32 vramOffset = vramManager.Alloc(tilesLength);
    dma_ntrCopy32(3, tiles, vramManager.GetVramAddress(vramOffset), tilesLength);
    return vramOffset;
}