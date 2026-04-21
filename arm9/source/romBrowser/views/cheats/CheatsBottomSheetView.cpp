#include "common.h"
#include "gui/GraphicsContext.h"
#include "themes/material/MaterialColorScheme.h"
#include "themes/IFontRepository.h"
#include "gui/input/InputProvider.h"
#include "gui/VramContext.h"
#include "gui/palette/GradientPalette.h"
#include "gui/OamBuilder.h"
#include "folderIcon.h"
#include "upIcon.h"
#include "checkboxChecked.h"
#include "checkboxUnchecked.h"
#include "cheatSelector.h"
#include "gui/DescendingStackVramManager.h"
#include "CheatsBottomSheetView.h"

#define TITLE_LABEL_X               20
#define TITLE_LABEL_Y               16

#define CATEGORY_NAME_LABEL_X       (TITLE_LABEL_X + 43)
#define CATEGORY_NAME_LABEL_Y       (TITLE_LABEL_Y + 1)

#define NO_CHEATS_FOUND_LABEL_X     20
#define NO_CHEATS_FOUND_LABEL_Y     36

#define DESCRIPTION_LABEL_X         16
#define DESCRIPTION_LABEL_Y         147

#define UP_BUTTON_X                 212
#define UP_BUTTON_Y                 (TITLE_LABEL_Y - 7)

#define LIST_X                      16
#define LIST_Y                      36
#define LIST_WIDTH                  224
#define LIST_HEIGHT                 108

CheatsBottomSheetView::CheatsBottomSheetView(SharedPtr<CheatsViewModel> viewModel,
    const MaterialColorScheme* materialColorScheme, const IFontRepository* fontRepository,
    FocusManager* focusManager)
    : _viewModel(std::move(viewModel))
    , _titleLabel(Label2DView::CreateShared(64, 16, 25, fontRepository->GetFont(FontType::Medium11)))
    , _secondaryLabel(Label2DView::CreateShared(153, 16, 64, fontRepository->GetFont(FontType::Regular10)))
    , _descriptionLabel(Label2DView::CreateShared(224, 16, 256, fontRepository->GetFont(FontType::Medium7_5)))
    , _cheatListRecycler(RecyclerView::CreateShared(
        LIST_X, LIST_Y, LIST_WIDTH, LIST_HEIGHT, RecyclerView::Mode::VerticalList))
    , _upButton(IconButton2DView::CreateShared(
        IconButtonView::Type::Standard,
        IconButtonView::State::NoToggle,
        md::sys::color::inverseOnSurface,
        materialColorScheme))
    , _materialColorScheme(materialColorScheme)
    , _fontRepository(fontRepository)
    , _focusManager(focusManager)
{
    _titleLabel->SetText(u"Cheats");
    _secondaryLabel->SetText(u"No cheats found.");
    _secondaryLabel->SetEllipsisStyle(LabelView::EllipsisStyle::Ellipsis);
    _descriptionLabel->SetEllipsisStyle(LabelView::EllipsisStyle::Marquee);
    _descriptionLabel->SetText(u"");
    AddChildTail(_titleLabel.GetPointer());
    AddChildTail(_secondaryLabel.GetPointer());
    AddChildTail(_descriptionLabel.GetPointer());
    AddChildTail(_cheatListRecycler.GetPointer());
    _upButton->SetAction([] (IconButtonView*, void* arg)
    {
        ((CheatsBottomSheetView*)arg)->_viewModel->NavigateUp();
    }, this);
}

void CheatsBottomSheetView::InitVram(const VramContext& vramContext)
{
    BottomSheetView::InitVram(vramContext);

    const auto objVramManager = vramContext.GetObjVramManager();
    if (objVramManager)
    {
        _vramOffsets.folderIconVramOffset
            = LoadSprite(*objVramManager, folderIconTiles, folderIconTilesLen);
        _vramOffsets.checkboxUncheckedIconVramOffset
            = LoadSprite(*objVramManager, checkboxUncheckedTiles, checkboxUncheckedTilesLen);
        _vramOffsets.checkboxCheckedIconVramOffset
            = LoadSprite(*objVramManager, checkboxCheckedTiles, checkboxCheckedTilesLen);
        _vramOffsets.cheatSelectorVramOffset
            = LoadSprite(*objVramManager, cheatSelectorTiles, cheatSelectorTilesLen);
        auto iconButtonVramToken = IconButton2DView::UploadGraphics(*objVramManager);
        _upButton->SetGraphics(iconButtonVramToken);
        _upButton->SetIconVramOffset(LoadSprite(*objVramManager, upIconTiles, upIconTilesLen));
    }

    _objVramManager = vramContext.GetObjVramManager();
}

void CheatsBottomSheetView::Update()
{
    if (_upButton->GetParent() == nullptr && _viewModel->IsInSubCategory())
    {
        AddChildTail(_upButton.GetPointer());
    }
    else if (_upButton->GetParent() != nullptr && !_viewModel->IsInSubCategory())
    {
        RemoveChild(_upButton.GetPointer());
    }

    _titleLabel->SetPosition(TITLE_LABEL_X, _position.y + TITLE_LABEL_Y);
    if (_viewModel->GetState() == CheatsViewModel::State::DisplayCheats)
    {
        _secondaryLabel->SetPosition(CATEGORY_NAME_LABEL_X, _position.y + CATEGORY_NAME_LABEL_Y);
    }
    else
    {
        _secondaryLabel->SetPosition(NO_CHEATS_FOUND_LABEL_X, _position.y + NO_CHEATS_FOUND_LABEL_Y);
    }
    _descriptionLabel->SetPosition(DESCRIPTION_LABEL_X, _position.y + DESCRIPTION_LABEL_Y);
    _cheatListRecycler->SetPosition(LIST_X, _position.y + LIST_Y);
    _upButton->SetPosition(UP_BUTTON_X, _position.y + UP_BUTTON_Y);
    if (_viewModel->GetState() == CheatsViewModel::State::DisplayCheats)
    {
        if (!_cheatsAdapter && _objVramManager != nullptr)
        {
            _currentCheatCategory = _viewModel->GetCurrentCheatCategory();
            _cheatsAdapter = SharedPtr<CheatsAdapter>::MakeShared(
                _currentCheatCategory, _viewModel, _materialColorScheme, _fontRepository, _vramOffsets);
            _cheatListRecycler->SetAdapter(_cheatsAdapter);

            // Ugly hack
            _savedVramState = ((DescendingStackVramManager*)_objVramManager)->GetState();

            _cheatListRecycler->InitVram(VramContext(nullptr, _objVramManager, nullptr, nullptr));
            _cheatListRecycler->Focus(*_focusManager);
        }
        else if (_currentCheatCategory != _viewModel->GetCurrentCheatCategory()
            && _viewModel->GetCurrentCheatCategory() != nullptr)
        {
            _secondaryLabel->SetText(_viewModel->GetCurrentCheatCategory()->GetName());
            UpdateCheatList();
        }
    }
    BottomSheetView::Update();
    int selectedItem = _cheatListRecycler->GetSelectedItem();
    if (selectedItem != _viewModel->GetSelectedItem())
    {
        _viewModel->SetSelectedItem(selectedItem);
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

        _titleLabel->SetBackgroundColor(backColor);
        _titleLabel->SetForegroundColor(_materialColorScheme->onSurface);
        _titleLabel->Draw(graphicsContext);

        if (_viewModel->GetState() == CheatsViewModel::State::NoCheats ||
            _viewModel->IsInSubCategory())
        {
            _secondaryLabel->SetBackgroundColor(backColor);
            _secondaryLabel->SetForegroundColor(_materialColorScheme->onSurfaceVariant);
            _secondaryLabel->Draw(graphicsContext);
        }

        _descriptionLabel->SetBackgroundColor(backColor);
        _descriptionLabel->SetForegroundColor(_materialColorScheme->onSurfaceVariant);
        _descriptionLabel->Draw(graphicsContext);

        if (_viewModel->IsInSubCategory())
        {
            _upButton->Draw(graphicsContext);
        }
    }
    graphicsContext.SetPriority(oldPrio);
    graphicsContext.ResetClipArea();
}

SharedPtr<View> CheatsBottomSheetView::MoveFocus(const SharedPtr<View>& currentFocus, FocusMoveDirection direction, View* source)
{
    if (!currentFocus)
    {
        return nullptr;
    }

    if (source == _cheatListRecycler.GetPointer() && direction == FocusMoveDirection::Up && _viewModel->IsInSubCategory())
    {
        return _upButton;
    }
    else if (source == _upButton.GetPointer() && direction == FocusMoveDirection::Down)
    {
        return _cheatListRecycler->MoveFocus(currentFocus, direction, this);
    }

    return nullptr;
}

bool CheatsBottomSheetView::HandleInput(const InputProvider& inputProvider, FocusManager& focusManager)
{
    if (inputProvider.Triggered(InputKey::B))
    {
        _viewModel->NavigateUp();
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

void CheatsBottomSheetView::Close()
{
    _viewModel->Close();
}

void CheatsBottomSheetView::UpdateCheatList()
{
    _currentCheatCategory = _viewModel->GetCurrentCheatCategory();
    _cheatsAdapter = SharedPtr<CheatsAdapter>::MakeShared(
        _currentCheatCategory, _viewModel, _materialColorScheme, _fontRepository, _vramOffsets);
    _cheatListRecycler->SetAdapter(_cheatsAdapter, _viewModel->GetSelectedItem());

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
        _descriptionLabel->SetText("");
    }
    else
    {
        auto cheatCategory = _viewModel->GetCurrentCheatCategory();
        u32 numberOfSubEntries = 0;
        auto subEntries = cheatCategory->GetSubEntries(numberOfSubEntries);
        _descriptionLabel->SetText(subEntries[selectedItem].GetDescription());
    }
}

u32 CheatsBottomSheetView::LoadSprite(IVramManager& vramManager, const unsigned int* tiles, u32 tilesLength) const
{
    u32 vramOffset = vramManager.Alloc(tilesLength);
    dma_ntrCopy32(3, tiles, vramManager.GetVramAddress(vramOffset), tilesLength);
    return vramOffset;
}
