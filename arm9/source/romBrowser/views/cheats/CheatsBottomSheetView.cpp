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
#include "core/mini-printf.h"
#include "gui/DescendingStackVramManager.h"
#include "CheatsBottomSheetView.h"

#define TITLE_LABEL_X       20
#define TITLE_LABEL_Y       16

#define LIST_X              16
#define LIST_Y              36

CheatsBottomSheetView::CheatsBottomSheetView(std::unique_ptr<CheatsViewModel> viewModel,
    const MaterialColorScheme* materialColorScheme, const IFontRepository* fontRepository,
    FocusManager* focusManager)
    : _viewModel(std::move(viewModel))
    , _titleLabel(220, 16, 64, fontRepository->GetFont(FontType::Medium11))
    , _cheatListRecycler(std::make_unique<RecyclerView>(LIST_X, LIST_Y, 224, 124, RecyclerView::Mode::VerticalList))
    , _materialColorScheme(materialColorScheme)
    , _fontRepository(fontRepository)
    , _focusManager(focusManager)
{
    _titleLabel.SetEllipsis(true);
    UpdateTitle();
    AddChildTail(&_titleLabel);
    AddChildTail(_cheatListRecycler.get());
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
    _cheatListRecycler->SetPosition(LIST_X, _position.y + LIST_Y);
    if (_viewModel->GetState() == CheatsViewModel::State::DisplayCheats)
    {
        if (_cheatsAdapter == nullptr && _objVramManager != nullptr)
        {
            if (_viewModel->GetIsSelectedOnlyMode())
            {
                u32 numberOfSelectedCheats = 0;
                auto selectedCheats = _viewModel->GetSelectedCheats(numberOfSelectedCheats);
                _cheatsAdapter = new CheatsAdapter(selectedCheats, numberOfSelectedCheats,
                    _materialColorScheme, _fontRepository, _vramOffsets);
            }
            else
            {
                _cheatsAdapter = new CheatsAdapter(
                    _viewModel->GetCurrentCheatCategory(), _materialColorScheme, _fontRepository, _vramOffsets);
            }
            _cheatListRecycler->SetAdapter(_cheatsAdapter);

            // Ugly hack
            _savedVramState = ((DescendingStackVramManager*)_objVramManager)->GetState();

            _cheatListRecycler->InitVram(VramContext(nullptr, _objVramManager, nullptr, nullptr));
            _cheatListRecycler->Focus(*_focusManager);
        }
    }
    BottomSheetView::Update();
    _viewModel->SetSelectedItem(_cheatListRecycler->GetSelectedItem());
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
        u32 maskPaletteRow = graphicsContext.GetPaletteManager().AllocRow(
            GradientPalette(backColor, backColor),
            _position.y + LIST_Y - 24, _position.y + LIST_Y);
        auto maskOam = graphicsContext.GetOamManager().AllocOams(4);
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

        _titleLabel.SetBackgroundColor(backColor);
        _titleLabel.SetForegroundColor(_materialColorScheme->onSurface);
        _titleLabel.Draw(graphicsContext);
    }
    graphicsContext.SetPriority(oldPrio);
    graphicsContext.ResetClipArea();
}

bool CheatsBottomSheetView::HandleInput(const InputProvider& inputProvider, FocusManager& focusManager)
{
    if (inputProvider.Triggered(InputKey::A))
    {
        if (focusManager.IsFocusInside(_cheatListRecycler.get()))
        {
            int selectedIdx = _cheatListRecycler->GetSelectedItem();
            _lastFocusedFolderIndex = selectedIdx;
            bool categoryChanged = _viewModel->ItemActivated();
            if (categoryChanged)
            {
                UpdateCheatList(0);
            }
            return true;
        }
    }
    else if (inputProvider.Triggered(InputKey::B))
    {
        if (_viewModel->GetIsSelectedOnlyMode())
        {
            _viewModel->SetSelectedOnlyMode(false);
            UpdateCheatList(_selectedModeReturnIndex);
            return true;
        }

        auto oldCategory = _viewModel->GetCurrentCheatCategory();
        _viewModel->Back();
        if (oldCategory != _viewModel->GetCurrentCheatCategory())
        {
            UpdateCheatList(_lastFocusedFolderIndex);
        }
        return true;
    }
    else if (inputProvider.Triggered(InputKey::Select))
    {
        if (_viewModel->GetIsSelectedOnlyMode())
        {
            _viewModel->SetSelectedOnlyMode(false);
            UpdateCheatList(_selectedModeReturnIndex);
        }
        else
        {
            _selectedModeReturnIndex = _cheatListRecycler->GetSelectedItem();
            _viewModel->SetSelectedOnlyMode(true);
            UpdateCheatList(0);
        }
        return true;
    }
    else if (inputProvider.Triggered(InputKey::Y))
    {
        _viewModel->Close();
        return true;
    }
    return false;
}

void CheatsBottomSheetView::UpdateCheatList(int initialSelectedIndex)
{
    UpdateTitle();

    auto oldAdapter = _cheatsAdapter;
    if (_viewModel->GetIsSelectedOnlyMode())
    {
        u32 numberOfSelectedCheats = 0;
        auto selectedCheats = _viewModel->GetSelectedCheats(numberOfSelectedCheats);
        _cheatsAdapter = new CheatsAdapter(selectedCheats, numberOfSelectedCheats, _materialColorScheme, _fontRepository, _vramOffsets);
    }
    else
    {
        _cheatsAdapter = new CheatsAdapter(_viewModel->GetCurrentCheatCategory(), _materialColorScheme, _fontRepository, _vramOffsets);
    }
    _cheatListRecycler->SetAdapter(_cheatsAdapter, initialSelectedIndex);
    delete oldAdapter;

    // Ugly hack
    ((DescendingStackVramManager*)_objVramManager)->SetState(_savedVramState);

    _cheatListRecycler->InitVram(VramContext(nullptr, _objVramManager, nullptr, nullptr));
    _cheatListRecycler->Focus(*_focusManager);
}

void CheatsBottomSheetView::UpdateTitle()
{
    auto folderName = _viewModel->GetCurrentFolderName();
    if (folderName != nullptr && folderName[0] != '\0')
    {
        char titleBuffer[64];
        mini_snprintf(titleBuffer, sizeof(titleBuffer), "Cheats / %s", folderName);
        _titleLabel.SetText(titleBuffer);
    }
    else
    {
        _titleLabel.SetText(u"Cheats");
    }
}
