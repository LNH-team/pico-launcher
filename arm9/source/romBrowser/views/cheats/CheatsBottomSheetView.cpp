#include "common.h"
#include "gui/GraphicsContext.h"
#include "themes/material/MaterialColorScheme.h"
#include "themes/IFontRepository.h"
#include "gui/input/InputProvider.h"
#include "gui/input/TouchEvent.h"
#include "gui/VramContext.h"
#include "gui/palette/GradientPalette.h"
#include "gui/OamBuilder.h"
#include "folderIcon.h"
#include "checkboxChecked.h"
#include "checkboxUnchecked.h"
#include "cheatSelector.h"
#include "core/mini-printf.h"
#include "core/StringUtil.h"
#include "cheats/CheatEntry.h"
#include "gui/DescendingStackVramManager.h"
#include "services/Localization/Localization.h"
#include "CheatsBottomSheetView.h"

#define TOTALC_LABEL_X      20
#define TOTALC_LABEL_Y      8
#define TITLE_LABEL_X       20
#define TITLE_LABEL_Y       20

#define LIST_X              16
#define LIST_Y              36

#define TITLE_SCROLL_SEPARATOR          "   "
#define TITLE_SCROLL_START_PAUSE_FRAMES 120
#define TITLE_SCROLL_CYCLE_PAUSE_FRAMES 120
#define TITLE_SCROLL_SPEED_Q8           96

CheatsBottomSheetView::CheatsBottomSheetView(std::unique_ptr<CheatsViewModel> viewModel,
    const MaterialColorScheme* materialColorScheme, const IFontRepository* fontRepository,
    FocusManager* focusManager)
    : _viewModel(std::move(viewModel))
    , _titlePrefixLabel(120, 16, 64, fontRepository->GetFont(FontType::Medium11))
    , _titleLabel(220, 16, 128, fontRepository->GetFont(FontType::Medium11))
    , _totalCLabel(220, 16, 64, fontRepository->GetFont(FontType::Medium7_5))
    , _statusLabel(220, 16, 64, fontRepository->GetFont(FontType::Medium10))
    , _descriptionLabel(220, 104, 512, fontRepository->GetFont(FontType::Regular10))
    , _cheatListRecycler(std::make_unique<RecyclerView>(LIST_X, LIST_Y, 224, 124, RecyclerView::Mode::VerticalList))
    , _materialColorScheme(materialColorScheme)
    , _fontRepository(fontRepository)
    , _focusManager(focusManager)
{
    _cheatListRecycler->SetShoulderPagingEnabled(false);

    _cheatListRecycler->SetTouchTapCallback([](int itemIdx, void* arg)
    {
        auto* self = static_cast<CheatsBottomSheetView*>(arg);
        self->_viewModel->SetSelectedItem(itemIdx);
        bool categoryChanged = self->_viewModel->ItemActivated();
        if (categoryChanged)
        {
            if (itemIdx >= 0)
            {
                self->_lastFocusedFolderIndex = itemIdx;
            }
            self->UpdateCheatList(0);
        }
        else
        {
            self->UpdateTotalC();
        }
    }, this);

    _cheatListRecycler->SetTouchLongPressCallback([](int itemIdx, void* arg)
    {
        auto* self = static_cast<CheatsBottomSheetView*>(arg);
        self->_viewModel->SetSelectedItem(itemIdx);
        self->EnterDescriptionMode(*self->_focusManager);
    }, this);

    _titlePrefixLabel.SetEllipsis(false);
    _titleLabel.SetEllipsis(false);
    _totalCLabel.SetHorizontalAlignment(Alignment::Start);
    _totalCLabel.SetEllipsis(true);
    _statusLabel.SetEllipsis(true);
    _statusLabel.SetHorizontalAlignment(Alignment::Start);
    _descriptionLabel.SetHorizontalAlignment(Alignment::Start);
    _descriptionLabel.SetEllipsis(false);
    _descriptionLabel.SetText(u"");
    _descriptionLabel.SetParent(this);
    UpdateTitle();
    UpdateTotalC();
    AddChildTail(&_titlePrefixLabel);
    AddChildTail(&_totalCLabel); 
    AddChildTail(&_titleLabel);
    AddChildTail(&_statusLabel);
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
    const bool showNoCheatsMessage = _viewModel->GetState() == CheatsViewModel::State::NoCheats;

    _totalCLabel.SetPosition(TOTALC_LABEL_X, _position.y + TOTALC_LABEL_Y);
    _titlePrefixLabel.SetPosition(TITLE_LABEL_X, _position.y + TITLE_LABEL_Y);

    int titleTextX = TITLE_LABEL_X;
    if (_titleHasPrefix)
    {
        titleTextX += _titlePrefixPixelWidth + 4;
    }
    _titleLabel.SetPosition(titleTextX, _position.y + TITLE_LABEL_Y);

    int titleAvailableWidth = 220 - (titleTextX - TITLE_LABEL_X);
    if (titleAvailableWidth < 32)
    {
        titleAvailableWidth = 32;
    }
    UpdateTitleScroll(titleAvailableWidth);

    _statusLabel.SetPosition(TITLE_LABEL_X, _position.y + LIST_Y + 12);
    _descriptionLabel.SetPosition(TITLE_LABEL_X, _position.y + LIST_Y);
    if (showNoCheatsMessage)
    {
        const char16_t* missing = Localization::Translate("cheats_dat_missing");
        const char16_t* notFound = Localization::Translate("cheats_not_found");
        _statusLabel.SetText(_viewModel->GetIsUsrCheatDatMissing()
            ? missing
            : notFound);
    }
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
            UpdateTitle();
            UpdateTotalC();
            _cheatListRecycler->SetAdapter(_cheatsAdapter);

            // Ugly hack
            _savedVramState = ((DescendingStackVramManager*)_objVramManager)->GetState();

            _cheatListRecycler->InitVram(VramContext(nullptr, _objVramManager, nullptr, nullptr));
            _cheatListRecycler->Focus(*_focusManager);
        }
    }
    BottomSheetView::Update();
    if (_isDescriptionMode)
    {
        _descriptionLabel.Update();
    }
    _viewModel->SetSelectedItem(_cheatListRecycler->GetSelectedItem());
}

void CheatsBottomSheetView::Draw(GraphicsContext& graphicsContext)
{
    const bool showCheatList = _viewModel->GetState() == CheatsViewModel::State::DisplayCheats && !_isDescriptionMode;
    const bool showNoCheatsMessage = _viewModel->GetState() == CheatsViewModel::State::NoCheats;
    const bool showDescriptionMode = _isDescriptionMode;

    graphicsContext.SetClipArea(GetBounds());
    u32 oldPrio = graphicsContext.SetPriority(1);
    {
        if (showCheatList)
        {
            graphicsContext.SetClipArea(_cheatListRecycler->GetBounds());
            _cheatListRecycler->Draw(graphicsContext);
        }

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

        _totalCLabel.SetBackgroundColor(backColor);
        _totalCLabel.SetForegroundColor(_materialColorScheme->onSurface);
        _titlePrefixLabel.SetBackgroundColor(backColor);
        _titlePrefixLabel.SetForegroundColor(_materialColorScheme->onSurface);
        _titleLabel.SetBackgroundColor(backColor);
        _titleLabel.SetForegroundColor(_materialColorScheme->onSurface);
        _statusLabel.SetBackgroundColor(backColor);
        _statusLabel.SetForegroundColor(_materialColorScheme->onSurface);
        _descriptionLabel.SetBackgroundColor(backColor);
        _descriptionLabel.SetForegroundColor(_materialColorScheme->onSurface);
        if (showCheatList)
        {
            _totalCLabel.Draw(graphicsContext);
        }
        if (_titleHasPrefix)
        {
            _titlePrefixLabel.Draw(graphicsContext);
        }

        int titleTextX = TITLE_LABEL_X;
        if (_titleHasPrefix)
        {
            titleTextX += _titlePrefixPixelWidth + 4;
        }
        int titleAvailableWidth = 220 - (titleTextX - TITLE_LABEL_X);
        if (titleAvailableWidth < 1)
        {
            titleAvailableWidth = 1;
        }
        graphicsContext.SetClipArea(Rectangle(titleTextX, _position.y + TITLE_LABEL_Y, titleAvailableWidth, 16));
        _titleLabel.Draw(graphicsContext);
        graphicsContext.SetClipArea(GetBounds());
        if (showNoCheatsMessage)
        {
            _statusLabel.Draw(graphicsContext);
        }
        if (showDescriptionMode)
        {
            _descriptionLabel.Draw(graphicsContext);
        }
    }
    graphicsContext.SetPriority(oldPrio);
    graphicsContext.ResetClipArea();
}

void CheatsBottomSheetView::VBlank()
{
    BottomSheetView::VBlank();
    if (_isDescriptionMode)
    {
        _descriptionLabel.VBlank();
    }
}

View* CheatsBottomSheetView::MoveFocus(View* currentFocus, FocusMoveDirection direction, View* source)
{
    if (source == _cheatListRecycler.get())
    {
        if (direction == FocusMoveDirection::Left)
        {
            _cheatListRecycler->PageByShoulderButtons(true, *_focusManager);
            return nullptr;
        }
        if (direction == FocusMoveDirection::Right)
        {
            _cheatListRecycler->PageByShoulderButtons(false, *_focusManager);
            return nullptr;
        }
    }

    return View::MoveFocus(currentFocus, direction, source);
}

bool CheatsBottomSheetView::HandleInput(const InputProvider& inputProvider, FocusManager& focusManager)
{
    if (_isDescriptionMode &&
        (inputProvider.Triggered(InputKey::Y) || inputProvider.Triggered(InputKey::B)))
    {
        ExitDescriptionMode(focusManager);
        return true;
    }

    if (_isDescriptionMode)
    {
        if (inputProvider.GetTriggeredKeys() != InputKey::None)
        {
            return true;
        }

        return false;
    }

    // Gestione input quando non ci sono cheats
    if (_viewModel->GetState() == CheatsViewModel::State::NoCheats) {
        if (inputProvider.Triggered(InputKey::A)) {
            // Ignora A se non ci sono cheats
            return true;
        }
        if (inputProvider.Triggered(InputKey::B)) {
            _viewModel->Close();
            return true;
        }
        return false;
    }

    CheatListItemView::SetFastScrollEnabled(inputProvider.Current(InputKey::R));
    if (inputProvider.Current(InputKey::R))
    {
        CheatListItemView::RequestScrollStartNow();
    }

    if (inputProvider.Triggered(InputKey::A))
    {
        if (focusManager.IsFocusInside(_cheatListRecycler.get()))
        {
            int selectedIdx = _cheatListRecycler->GetSelectedItem();
            bool categoryChanged = _viewModel->ItemActivated();
            if (categoryChanged)
            {
                if (selectedIdx >= 0)
                {
                    _lastFocusedFolderIndex = selectedIdx;
                }
                UpdateCheatList(0);
            }
            else
            {
                UpdateTotalC();
            }
            return true;
        }
    }
    else if (inputProvider.Triggered(InputKey::L))
    {
        if (focusManager.IsFocusInside(_cheatListRecycler.get()))
        {
            u32 romActive = 0;
            u32 romTotal = 0;
            _viewModel->GetRomCheatStats(romActive, romTotal);
            if (romActive == 0)
            {
                return true;
            }

            int selectedIdx = _cheatListRecycler->GetSelectedItem();
            _viewModel->DisableAllCheats();

            if (_viewModel->GetIsSelectedOnlyMode())
            {
                UpdateCheatList(0);
            }
            else
            {
                if (selectedIdx < 0)
                {
                    selectedIdx = 0;
                }
                UpdateCheatList(selectedIdx);
            }
            return true;
        }
    }
    else if (inputProvider.Triggered(InputKey::R))
    {
        return true;
    }
    else if (inputProvider.Triggered(InputKey::Y))
    {
        EnterDescriptionMode(focusManager);
        return true;
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
    return false;
}

void CheatsBottomSheetView::OnDismissed()
{
    _viewModel->Close();
}

bool CheatsBottomSheetView::HandleTouch(const TouchEvent& event, FocusManager& focusManager)
{
    if (_isDescriptionMode)
    {
        if (event.type == TouchEventType::Down)
        {
            _descriptionTouchDownReceived = true;
        }
        else if (event.type == TouchEventType::Up && _descriptionTouchDownReceived)
        {
            ExitDescriptionMode(focusManager);
        }
        return true;
    }

    if (event.type == TouchEventType::Up)
    {
        int deltaX = event.position.x - event.startPosition.x;
        int deltaY = event.position.y - event.startPosition.y;
        int absDeltaY = deltaY < 0 ? -deltaY : deltaY;
        if (deltaX > 30 && deltaX > absDeltaY * 2)
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
        if (-deltaX > 30 && -deltaX > absDeltaY * 2 &&
            _viewModel->GetState() == CheatsViewModel::State::DisplayCheats)
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
    }

    return _cheatListRecycler->HandleTouch(event, focusManager);
}

bool CheatsBottomSheetView::TryGetSelectedItemNameAndDescription(const char*& selectedName, const char*& selectedDescription) const
{
    selectedName = nullptr;
    selectedDescription = nullptr;

    if (_viewModel->GetState() != CheatsViewModel::State::DisplayCheats)
    {
        return false;
    }

    const int selectedIndex = _cheatListRecycler->GetSelectedItem();
    if (selectedIndex < 0)
    {
        return false;
    }

    if (_viewModel->GetIsSelectedOnlyMode())
    {
        u32 numberOfSelectedCheats = 0;
        auto selectedCheats = _viewModel->GetSelectedCheats(numberOfSelectedCheats);
        if ((u32)selectedIndex >= numberOfSelectedCheats)
        {
            return false;
        }

        selectedName = selectedCheats[selectedIndex]->GetName();
        selectedDescription = selectedCheats[selectedIndex]->GetDescription();
        return selectedDescription != nullptr && selectedDescription[0] != '\0';
    }

    auto cheatCategory = _viewModel->GetCurrentCheatCategory();
    if (cheatCategory == nullptr)
    {
        return false;
    }

    u32 numberOfSubEntries = 0;
    auto subEntries = cheatCategory->GetSubEntries(numberOfSubEntries);
    if ((u32)selectedIndex >= numberOfSubEntries)
    {
        return false;
    }

    selectedName = subEntries[selectedIndex].GetName();
    selectedDescription = subEntries[selectedIndex].GetDescription();
    return selectedDescription != nullptr && selectedDescription[0] != '\0';
}

bool CheatsBottomSheetView::EnterDescriptionMode(FocusManager& focusManager)
{
    const char* selectedName = nullptr;
    const char* selectedDescription = nullptr;
    if (!TryGetSelectedItemNameAndDescription(selectedName, selectedDescription))
    {
        return false;
    }

    _descriptionModeSelectedIndex = _cheatListRecycler->GetSelectedItem();
    _isDescriptionMode = true;
    _descriptionTouchDownReceived = false;
    StringUtil::Copy(_descriptionModeTitle, selectedName, sizeof(_descriptionModeTitle));

    auto emptyAdapter = new CheatsAdapter(nullptr, 0, _materialColorScheme, _fontRepository, _vramOffsets);
    _cheatListRecycler->SetAdapter(emptyAdapter, 0);
    delete _cheatsAdapter;
    _cheatsAdapter = emptyAdapter;

    ((DescendingStackVramManager*)_objVramManager)->SetState(_savedVramState);
    _descriptionLabel.InitVram(VramContext(nullptr, _objVramManager, nullptr, nullptr));

    UpdateTitle();
    BuildWrappedDescriptionText(selectedDescription);
    _descriptionLabel.SetText(_wrappedDescriptionBuffer);

    focusManager.Unfocus();
    focusManager.Focus(&_descriptionLabel);
    return true;
}

void CheatsBottomSheetView::BuildWrappedDescriptionText(const char* description)
{
    _wrappedDescriptionBuffer[0] = 0;
    if (description == nullptr || description[0] == '\0')
    {
        return;
    }

    constexpr u32 maxChars = sizeof(_wrappedDescriptionBuffer) / sizeof(_wrappedDescriptionBuffer[0]);
    char16_t inputBuffer[maxChars];
    StringUtil::Copy(inputBuffer, description, maxChars);

    char16_t normalizedBuffer[maxChars];
    u32 normalizedLen = 0;
    bool previousWasSpace = true;
    for (u32 i = 0; inputBuffer[i] != 0 && normalizedLen + 1 < maxChars; i++)
    {
        char16_t c = inputBuffer[i];
        bool isWhitespace = c == '\r' || c == '\n' || c == '\t' || c == ' ';
        if (isWhitespace)
        {
            if (!previousWasSpace)
            {
                normalizedBuffer[normalizedLen++] = ' ';
                previousWasSpace = true;
            }
            continue;
        }

        normalizedBuffer[normalizedLen++] = c;
        previousWasSpace = false;
    }

    while (normalizedLen > 0 && normalizedBuffer[normalizedLen - 1] == ' ')
    {
        normalizedLen--;
    }
    normalizedBuffer[normalizedLen] = 0;

    if (normalizedLen == 0)
    {
        return;
    }

    const auto font = _fontRepository->GetFont(FontType::Regular10);
    constexpr int maxLineWidth = 220;
    u32 outLen = 0;

    auto measureWidth = [font](const char16_t* text) -> u32
    {
        u32 width = 0;
        u32 height = 0;
        nft2_measureString(font, text, width, height);
        return width;
    };

    auto appendLine = [this, &outLen](const char16_t* line, u32 lineLen)
    {
        if (lineLen == 0)
        {
            return;
        }

        if (outLen > 0 && outLen + 1 < maxChars)
        {
            _wrappedDescriptionBuffer[outLen++] = '\n';
        }

        for (u32 i = 0; i < lineLen && outLen + 1 < maxChars; i++)
        {
            _wrappedDescriptionBuffer[outLen++] = line[i];
        }

        _wrappedDescriptionBuffer[outLen] = 0;
    };

    char16_t lineBuffer[maxChars];
    u32 lineLen = 0;
    lineBuffer[0] = 0;

    for (u32 tokenStart = 0; normalizedBuffer[tokenStart] != 0 && outLen + 1 < maxChars;)
    {
        u32 tokenEnd = tokenStart;
        while (normalizedBuffer[tokenEnd] != 0 && normalizedBuffer[tokenEnd] != ' ')
        {
            tokenEnd++;
        }
        u32 tokenLen = tokenEnd - tokenStart;

        char16_t tokenBuffer[maxChars];
        u32 tokenCursor = 0;
        for (; tokenCursor < tokenLen && tokenCursor + 1 < maxChars; tokenCursor++)
        {
            tokenBuffer[tokenCursor] = normalizedBuffer[tokenStart + tokenCursor];
        }
        tokenBuffer[tokenCursor] = 0;

        char16_t candidateBuffer[maxChars];
        u32 candidateLen = 0;
        for (u32 i = 0; i < lineLen && candidateLen + 1 < maxChars; i++)
        {
            candidateBuffer[candidateLen++] = lineBuffer[i];
        }
        if (lineLen > 0 && candidateLen + 1 < maxChars)
        {
            candidateBuffer[candidateLen++] = ' ';
        }
        for (u32 i = 0; i < tokenLen && candidateLen + 1 < maxChars; i++)
        {
            candidateBuffer[candidateLen++] = tokenBuffer[i];
        }
        candidateBuffer[candidateLen] = 0;

        if (lineLen > 0 && measureWidth(candidateBuffer) > maxLineWidth)
        {
            appendLine(lineBuffer, lineLen);
            lineLen = 0;
            lineBuffer[0] = 0;
            continue;
        }

        if (lineLen == 0 && measureWidth(tokenBuffer) > maxLineWidth)
        {
            char16_t splitBuffer[maxChars];
            u32 splitLen = 0;
            for (u32 i = 0; i < tokenLen && outLen + 1 < maxChars; i++)
            {
                splitBuffer[splitLen++] = tokenBuffer[i];
                splitBuffer[splitLen] = 0;
                if (measureWidth(splitBuffer) > maxLineWidth)
                {
                    splitLen--;
                    if (splitLen > 0)
                    {
                        appendLine(splitBuffer, splitLen);
                    }
                    splitLen = 0;
                    splitBuffer[splitLen++] = tokenBuffer[i];
                    splitBuffer[splitLen] = 0;
                }
            }

            if (splitLen > 0)
            {
                for (u32 i = 0; i < splitLen && i + 1 < maxChars; i++)
                {
                    lineBuffer[i] = splitBuffer[i];
                }
                lineLen = splitLen;
                lineBuffer[lineLen] = 0;
            }
        }
        else
        {
            for (u32 i = 0; i < candidateLen && i + 1 < maxChars; i++)
            {
                lineBuffer[i] = candidateBuffer[i];
            }
            lineLen = candidateLen;
            lineBuffer[lineLen] = 0;
        }

        tokenStart = tokenEnd;
        if (normalizedBuffer[tokenStart] == ' ')
        {
            tokenStart++;
        }
    }

    if (lineLen > 0)
    {
        appendLine(lineBuffer, lineLen);
    }

    _wrappedDescriptionBuffer[outLen] = 0;
}

void CheatsBottomSheetView::ExitDescriptionMode(FocusManager& focusManager)
{
    _isDescriptionMode = false;
    _descriptionLabel.SetText(u"");
    UpdateTitle();

    focusManager.Unfocus();
    UpdateCheatList(_descriptionModeSelectedIndex);
}

void CheatsBottomSheetView::UpdateCheatList(int initialSelectedIndex)
{
    // Need to unfocus first, otherwise the focus manager still contains a pointer to a view that is going to be destroyed
    _focusManager->Unfocus();
    UpdateTitle();
    UpdateTotalC();

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

void CheatsBottomSheetView::UpdateTotalC()
{
    u32 romActive = 0;
    u32 romTotal = 0;
    _viewModel->GetRomCheatStats(romActive, romTotal);

    u32 currentActive = 0;
    u32 currentTotal = 0;
    _viewModel->GetCurrentScopeCheatStats(currentActive, currentTotal);

    char totalCBuffer[40];
    auto folderName = _viewModel->GetCurrentFolderName();
    if ((folderName == nullptr || folderName[0] == '\0') || _viewModel->GetIsSelectedOnlyMode()) {
        mini_snprintf(totalCBuffer, sizeof(totalCBuffer), "%lu/%lu", romActive, romTotal);
    } else {
        mini_snprintf(totalCBuffer, sizeof(totalCBuffer), "%lu/%lu             %lu/%lu", romActive, romTotal, currentActive, currentTotal);
    }
    _totalCLabel.SetText(totalCBuffer);
}

void CheatsBottomSheetView::ResetTitleScroll()
{
    _titleScrollPrepared = false;
    _titleScrollCycleQ8 = 0;
    _titleScrollOffsetQ8 = 0;
    _titleScrollPauseFrames = TITLE_SCROLL_START_PAUSE_FRAMES;
    _titleScrollPhase = TitleScrollPhase::PauseAtStart;
    _titleLabel.SetTextOffsetX(0);
}

void CheatsBottomSheetView::PrepareTitleScrollIfNeeded(int availableWidth)
{
    if (_titleScrollPrepared)
    {
        return;
    }

    _titleScrollPrepared = true;
    _titleScrollCycleQ8 = 0;
    _titleLabel.SetText(_titleBaseText);

    char16_t baseText16[128];
    StringUtil::Copy(baseText16, _titleBaseText, sizeof(baseText16) / sizeof(baseText16[0]));

    u32 baseWidth = 0;
    u32 baseHeight = 0;
    nft2_measureString(_fontRepository->GetFont(FontType::Medium11), baseText16, baseWidth, baseHeight);
    if ((int)baseWidth <= availableWidth)
    {
        return;
    }

    char16_t separator16[8];
    StringUtil::Copy(separator16, TITLE_SCROLL_SEPARATOR, sizeof(separator16) / sizeof(separator16[0]));
    u32 separatorWidth = 0;
    u32 separatorHeight = 0;
    nft2_measureString(_fontRepository->GetFont(FontType::Medium11), separator16, separatorWidth, separatorHeight);

    char scrollerText[320];
    scrollerText[0] = 0;
    strlcat(scrollerText, _titleBaseText, sizeof(scrollerText));
    strlcat(scrollerText, TITLE_SCROLL_SEPARATOR, sizeof(scrollerText));
    strlcat(scrollerText, _titleBaseText, sizeof(scrollerText));
    strlcat(scrollerText, TITLE_SCROLL_SEPARATOR, sizeof(scrollerText));
    strlcat(scrollerText, _titleBaseText, sizeof(scrollerText));
    _titleLabel.SetText(scrollerText);

    _titleScrollCycleQ8 = ((int)baseWidth + (int)separatorWidth) << 8;
}

void CheatsBottomSheetView::UpdateTitleScroll(int availableWidth)
{
    PrepareTitleScrollIfNeeded(availableWidth);
    if (_titleScrollCycleQ8 <= 0)
    {
        if (_titleScrollOffsetQ8 != 0)
        {
            _titleLabel.SetTextOffsetX(0);
            _titleScrollOffsetQ8 = 0;
        }
        return;
    }

    if (_titleScrollPhase == TitleScrollPhase::PauseAtStart)
    {
        if (_titleScrollPauseFrames > 0)
        {
            _titleScrollPauseFrames--;
            return;
        }

        _titleScrollPhase = TitleScrollPhase::Scrolling;
    }

    _titleScrollOffsetQ8 += TITLE_SCROLL_SPEED_Q8;
    if (_titleScrollOffsetQ8 >= _titleScrollCycleQ8)
    {
        _titleScrollOffsetQ8 = 0;
        _titleLabel.SetTextOffsetX(0);
        _titleScrollPhase = TitleScrollPhase::PauseAtStart;
        _titleScrollPauseFrames = TITLE_SCROLL_CYCLE_PAUSE_FRAMES;
        return;
    }

    _titleLabel.SetTextOffsetX(-(_titleScrollOffsetQ8 >> 8));
}

void CheatsBottomSheetView::UpdateTitle()
{
    const char16_t* cheatsText = Localization::Translate("cheats");
    const char16_t* selectedCheatsText = Localization::Translate("selected_cheats");
    const char16_t* selectedCheatsFallback = Localization::Translate("Selected_Cheats");

    if (_isDescriptionMode)
    {
        _titleHasPrefix = false;
        _titlePrefixPixelWidth = 0;
        _titlePrefixLabel.SetText(u"");
        StringUtil::Copy(_titleBaseText, _descriptionModeTitle, sizeof(_titleBaseText));
        ResetTitleScroll();
        return;
    }

    if (_viewModel->GetIsSelectedOnlyMode())
    {
        _titleHasPrefix = false;
        _titlePrefixPixelWidth = 0;
        _titlePrefixLabel.SetText(u"");
        if (selectedCheatsText && selectedCheatsText[0] != 0)
        {
            _titleLabel.SetText(selectedCheatsText);
        }
        else if (selectedCheatsFallback && selectedCheatsFallback[0] != 0)
        {
            _titleLabel.SetText(selectedCheatsFallback);
        }
        else
        {
            _titleLabel.SetText("Selected Cheats");
        }
        _titleBaseText[0] = 0;
        ResetTitleScroll();
        _titleScrollPrepared = true;
        return;
    }

    auto folderName = _viewModel->GetCurrentFolderName();
    if (folderName != nullptr && folderName[0] != '\0')
    {
        _titleHasPrefix = false;
        _titlePrefixPixelWidth = 0;
        _titlePrefixLabel.SetText(u"");
        StringUtil::Copy(_titleBaseText, folderName, sizeof(_titleBaseText));
    }
    else
    {
        _titleHasPrefix = false;
        _titlePrefixPixelWidth = 0;
        _titlePrefixLabel.SetText(u"");
        if (cheatsText && cheatsText[0] != 0)
        {
            _titleLabel.SetText(cheatsText);
        }
        else
        {
            _titleLabel.SetText("Cheats");
        }
        _titleBaseText[0] = 0;
        ResetTitleScroll();
        _titleScrollPrepared = true;
        return;
    }

    ResetTitleScroll();
}
