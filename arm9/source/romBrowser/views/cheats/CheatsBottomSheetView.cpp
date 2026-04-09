#include "common.h"
#include "gui/GraphicsContext.h"
#include "themes/material/MaterialColorScheme.h"
#include "themes/IFontRepository.h"
#include "gui/input/InputProvider.h"
#include "gui/VramContext.h"
#include "gui/palette/GradientPalette.h"
#include "gui/OamBuilder.h"
#include "folderIcon.h"
#include "checkboxChecked.h"
#include "checkboxUnchecked.h"
#include "cheatSelector.h"
#include "gui/DescendingStackVramManager.h"
#include "CheatsBottomSheetView.h"

namespace
{
RecyclerView::State ToRecyclerViewState(const CheatsViewModel::ListState& listState)
{
    return { listState.selectedItem, listState.scrollOffset };
}

CheatsViewModel::ListState ToCheatsListState(const RecyclerView::State& state)
{
    return { state.selectedItem, state.scrollOffset };
}
}

#define TITLE_LABEL_X               20
#define TITLE_LABEL_Y               16

#define CATEGORY_NAME_LABEL_X       (TITLE_LABEL_X + 43)
#define CATEGORY_NAME_LABEL_Y       (TITLE_LABEL_Y + 1)

#define NO_CHEATS_FOUND_LABEL_X     20
#define NO_CHEATS_FOUND_LABEL_Y     36

#define DESCRIPTION_LABEL_X         16
#define DESCRIPTION_LABEL_Y         147

#define LIST_X                      16
#define LIST_Y                      36
#define LIST_WIDTH                  224
#define LIST_HEIGHT                 108

CheatsBottomSheetView::CheatsBottomSheetView(std::unique_ptr<CheatsViewModel> viewModel,
    const MaterialColorScheme* materialColorScheme, const IFontRepository* fontRepository,
    FocusManager* focusManager)
    : _viewModel(std::move(viewModel))
    , _titleLabel(64, 16, 25, fontRepository->GetFont(FontType::Medium11))
    , _secondaryLabel(177, 16, 64, fontRepository->GetFont(FontType::Regular10))
    , _descriptionLabel(224, 16, 256, fontRepository->GetFont(FontType::Medium7_5))
    , _cheatListRecycler(RecyclerView::CreateShared(
        LIST_X, LIST_Y, LIST_WIDTH, LIST_HEIGHT, RecyclerView::Mode::VerticalList))
    , _materialColorScheme(materialColorScheme)
    , _fontRepository(fontRepository)
    , _focusManager(focusManager)
{
    _titleLabel.SetText(u"Cheats");
    _secondaryLabel.SetText(u"No cheats found.");
    _secondaryLabel.SetEllipsisStyle(LabelView::EllipsisStyle::Ellipsis);
    _descriptionLabel.SetEllipsisStyle(LabelView::EllipsisStyle::Marquee);
    _descriptionLabel.SetText("");
    AddChildTail(&_titleLabel);
    AddChildTail(&_secondaryLabel);
    AddChildTail(&_descriptionLabel);
    AddChildTail(_cheatListRecycler.GetPointer());
}

void CheatsBottomSheetView::InitVram(const VramContext& vramContext)
{
    BottomSheetView::InitVram(vramContext);

    const auto objVramManager = vramContext.GetObjVramManager();
    if (objVramManager)
    {
        _vramOffsets.folderIconVramOffset = objVramManager->Alloc(folderIconTilesLen);
        dma_ntrCopy32(3, folderIconTiles,
            objVramManager->GetVramAddress(_vramOffsets.folderIconVramOffset), folderIconTilesLen);

        _vramOffsets.checkboxUncheckedIconVramOffset = objVramManager->Alloc(checkboxUncheckedTilesLen);
        dma_ntrCopy32(3, checkboxUncheckedTiles,
            objVramManager->GetVramAddress(_vramOffsets.checkboxUncheckedIconVramOffset), checkboxUncheckedTilesLen);

        _vramOffsets.checkboxCheckedIconVramOffset = objVramManager->Alloc(checkboxCheckedTilesLen);
        dma_ntrCopy32(3, checkboxCheckedTiles,
            objVramManager->GetVramAddress(_vramOffsets.checkboxCheckedIconVramOffset), checkboxCheckedTilesLen);

        _vramOffsets.cheatSelectorVramOffset = objVramManager->Alloc(cheatSelectorTilesLen);
        dma_ntrCopy32(3, cheatSelectorTiles,
            objVramManager->GetVramAddress(_vramOffsets.cheatSelectorVramOffset), cheatSelectorTilesLen);
    }

    _objVramManager = vramContext.GetObjVramManager();
}

void CheatsBottomSheetView::Update()
{
    _titleLabel.SetPosition(TITLE_LABEL_X, _position.y + TITLE_LABEL_Y);
    if (_viewModel->GetState() == CheatsViewModel::State::DisplayCheats)
    {
        _secondaryLabel.SetPosition(CATEGORY_NAME_LABEL_X, _position.y + CATEGORY_NAME_LABEL_Y);
    }
    else
    {
        _secondaryLabel.SetPosition(NO_CHEATS_FOUND_LABEL_X, _position.y + NO_CHEATS_FOUND_LABEL_Y);
    }
    _descriptionLabel.SetPosition(DESCRIPTION_LABEL_X, _position.y + DESCRIPTION_LABEL_Y);
    _cheatListRecycler->SetPosition(LIST_X, _position.y + LIST_Y);
    if (_viewModel->GetState() == CheatsViewModel::State::DisplayCheats)
    {
        if (!_cheatsAdapter && _objVramManager != nullptr)
        {
            _cheatsAdapter = SharedPtr<CheatsAdapter>::MakeShared(
                _viewModel->GetCurrentCheatCategory(), _materialColorScheme, _fontRepository, _vramOffsets);
            _cheatListRecycler->SetAdapter(_cheatsAdapter, _viewModel->GetSelectedItem());
            _cheatListRecycler->RestoreState(ToRecyclerViewState(_viewModel->GetCurrentListState()));

            // Ugly hack
            _savedVramState = ((DescendingStackVramManager*)_objVramManager)->GetState();

            _cheatListRecycler->InitVram(VramContext(nullptr, _objVramManager, nullptr, nullptr));
            _cheatListRecycler->Focus(*_focusManager);
        }
    }
    BottomSheetView::Update();
    int previousSelectedItem = _viewModel->GetSelectedItem();
    SyncViewModelListState();
    if (_viewModel->GetSelectedItem() != previousSelectedItem)
    {
        UpdateDescriptionText();
    }
}

void CheatsBottomSheetView::Draw(GraphicsContext& graphicsContext)
{
    graphicsContext.SetClipArea(GetBounds());
    u32 oldPrio = graphicsContext.SetPriority(1);
    {
        graphicsContext.SetClipArea(_cheatListRecycler->GetBounds());
        _cheatListRecycler->Draw(graphicsContext);

        graphicsContext.SetClipArea(GetBounds());

        auto backColor = _materialColorScheme->GetColor(md::sys::color::surfaceContainerLow);
        auto maskOam = graphicsContext.GetOamManager().AllocOams(8);
        // Top
        u32 maskPaletteRow = graphicsContext.GetPaletteManager().AllocRow(
            GradientPalette(backColor, backColor),
            _position.y + LIST_Y - 24, _position.y + LIST_Y);
        OamBuilder::OamWithSize<64, 32>(LIST_X, _position.y + LIST_Y - 24, _vramOffsets.cheatSelectorVramOffset >> 7)
            .WithPalette16(maskPaletteRow)
            .WithPriority(graphicsContext.GetPriority())
            .Build(maskOam[0]);
        OamBuilder::OamWithSize<64, 32>(LIST_X + 64, _position.y + LIST_Y - 24, _vramOffsets.cheatSelectorVramOffset >> 7)
            .WithPalette16(maskPaletteRow)
            .WithPriority(graphicsContext.GetPriority())
            .Build(maskOam[1]);
        OamBuilder::OamWithSize<64, 32>(LIST_X + 2 * 64, _position.y + LIST_Y - 24, _vramOffsets.cheatSelectorVramOffset >> 7)
            .WithPalette16(maskPaletteRow)
            .WithPriority(graphicsContext.GetPriority())
            .Build(maskOam[2]);
        OamBuilder::OamWithSize<64, 32>(LIST_X + 2 * 64 + 32, _position.y + LIST_Y - 24, _vramOffsets.cheatSelectorVramOffset >> 7)
            .WithPalette16(maskPaletteRow)
            .WithPriority(graphicsContext.GetPriority())
            .Build(maskOam[3]);

        // Bottom
        if (graphicsContext.IsVisible(Rectangle(LIST_X, _position.y + LIST_Y + LIST_HEIGHT, 224, 24)))
        {
            maskPaletteRow = graphicsContext.GetPaletteManager().AllocRow(
                GradientPalette(backColor, backColor),
                _position.y + LIST_Y + LIST_HEIGHT, 192);
            OamBuilder::OamWithSize<64, 32>(LIST_X, _position.y + LIST_Y + LIST_HEIGHT, _vramOffsets.cheatSelectorVramOffset >> 7)
                .WithPalette16(maskPaletteRow)
                .WithPriority(graphicsContext.GetPriority())
                .Build(maskOam[4]);
            OamBuilder::OamWithSize<64, 32>(LIST_X + 64, _position.y + LIST_Y + LIST_HEIGHT, _vramOffsets.cheatSelectorVramOffset >> 7)
                .WithPalette16(maskPaletteRow)
                .WithPriority(graphicsContext.GetPriority())
                .Build(maskOam[5]);
            OamBuilder::OamWithSize<64, 32>(LIST_X + 2 * 64, _position.y + LIST_Y + LIST_HEIGHT, _vramOffsets.cheatSelectorVramOffset >> 7)
                .WithPalette16(maskPaletteRow)
                .WithPriority(graphicsContext.GetPriority())
                .Build(maskOam[6]);
            OamBuilder::OamWithSize<64, 32>(LIST_X + 2 * 64 + 32, _position.y + LIST_Y + LIST_HEIGHT, _vramOffsets.cheatSelectorVramOffset >> 7)
                .WithPalette16(maskPaletteRow)
                .WithPriority(graphicsContext.GetPriority())
                .Build(maskOam[7]);
        }

        _titleLabel.SetBackgroundColor(backColor);
        _titleLabel.SetForegroundColor(_materialColorScheme->onSurface);
        _titleLabel.Draw(graphicsContext);

        if (_viewModel->GetState() == CheatsViewModel::State::NoCheats ||
            _viewModel->ShouldShowCategoryName())
        {
            _secondaryLabel.SetBackgroundColor(backColor);
            _secondaryLabel.SetForegroundColor(_materialColorScheme->onSurfaceVariant);
            _secondaryLabel.Draw(graphicsContext);
        }

        _descriptionLabel.SetBackgroundColor(backColor);
        _descriptionLabel.SetForegroundColor(_materialColorScheme->onSurfaceVariant);
        _descriptionLabel.Draw(graphicsContext);
    }
    graphicsContext.SetPriority(oldPrio);
    graphicsContext.ResetClipArea();
}

bool CheatsBottomSheetView::HandleInput(const InputProvider& inputProvider, FocusManager& focusManager)
{
    if (inputProvider.Triggered(InputKey::A))
    {
        if (focusManager.IsFocusInside(_cheatListRecycler.GetPointer()))
        {
            SyncViewModelListState();
            auto oldCategory = _viewModel->GetCurrentCheatCategory();
            _viewModel->ActivateSelectedItem();
            if (oldCategory != _viewModel->GetCurrentCheatCategory())
            {
                _secondaryLabel.SetText(_viewModel->GetCurrentCheatCategory()->GetName());
                UpdateCheatList();
            }

            return true;
        }
    }
    else if (inputProvider.Triggered(InputKey::B))
    {
        SyncViewModelListState();
        auto oldCategory = _viewModel->GetCurrentCheatCategory();
        if (_viewModel->NavigateUp() &&
            oldCategory != _viewModel->GetCurrentCheatCategory())
        {
            UpdateCheatList();
        }
        return true;
    }
    else if (inputProvider.Triggered(InputKey::Y))
    {
        _viewModel->Close();
        return true;
    }
    else if (inputProvider.Triggered(InputKey::X))
    {
        _viewModel->DisableAllCheats();
        return true;
    }
    return false;
}

void CheatsBottomSheetView::SyncViewModelListState()
{
    _viewModel->SetCurrentListState(ToCheatsListState(_cheatListRecycler->GetState()));
}

void CheatsBottomSheetView::UpdateCheatList()
{
    // Need to unfocus first, otherwise the focus manager still contains a pointer to a view that is going to be destroyed
    _focusManager->Unfocus();

    _cheatsAdapter = SharedPtr<CheatsAdapter>::MakeShared(
        _viewModel->GetCurrentCheatCategory(), _materialColorScheme, _fontRepository, _vramOffsets);
    _cheatListRecycler->SetAdapter(_cheatsAdapter, _viewModel->GetSelectedItem());
    _cheatListRecycler->RestoreState(ToRecyclerViewState(_viewModel->GetCurrentListState()));

    // Ugly hack
    ((DescendingStackVramManager*)_objVramManager)->SetState(_savedVramState);

    _cheatListRecycler->InitVram(VramContext(nullptr, _objVramManager, nullptr, nullptr));
    _cheatListRecycler->Focus(*_focusManager);
    UpdateDescriptionText();
}

void CheatsBottomSheetView::UpdateDescriptionText()
{
    int selectedItem = _viewModel->GetSelectedItem();
    if (selectedItem < 0)
    {
        _descriptionLabel.SetText("");
    }
    else
    {
        auto cheatCategory = _viewModel->GetCurrentCheatCategory();
        u32 numberOfSubEntries = 0;
        auto subEntries = cheatCategory->GetSubEntries(numberOfSubEntries);
        _descriptionLabel.SetText(subEntries[selectedItem].GetDescription());
    }
}
