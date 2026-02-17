/*
    CheatsBottomSheetView.cpp
    Bottom sheet dialog for displaying and selecting cheats.
*/

#include "common.h"
#include "gui/GraphicsContext.h"
#include "gui/VramContext.h"
#include "gui/IVramManager.h"
#include "gui/input/InputProvider.h"
#include "../IRomBrowserController.h"
#include "../FileInfo.h"
#include "../viewModels/RomBrowserViewModel.h"
#include "CheatsBottomSheetView.h"
#include "themes/material/MaterialColorScheme.h"
#include "services/localization/Localization.h"
#include "core/mini-printf.h"
#include "cheats/CheatSaveManager.h"
#include "gui/font/nitroFont2.h"

namespace
{
const char* SkipSpaces(const char* text)
{
    while (text && *text == ' ')
        ++text;
    return text;
}

const char* WrapNextLine(const nft2_header_t* font, const char* text, u32 maxWidth,
    char16_t* out, int outMax)
{
    const char* p = SkipSpaces(text);
    if (!p || !*p)
    {
        out[0] = 0;
        return p;
    }

    int lineLen = 0;
    int lastFitLen = 0;
    const char* lastFitPtr = p;
    bool anyFit = false;

    while (*p)
    {
        const char* wordStart = p;
        while (*p && *p != ' ')
            ++p;
        int wordLen = (int)(p - wordStart);

        int prevLen = lineLen;
        if (lineLen > 0 && lineLen < outMax - 1)
            out[lineLen++] = u' ';
        for (int i = 0; i < wordLen && lineLen < outMax - 1; ++i)
            out[lineLen++] = (char16_t)(unsigned char)wordStart[i];
        out[lineLen] = 0;

        u32 width = 0, height = 0;
        nft2_measureString(font, out, width, height);

        if (width <= maxWidth)
        {
            lastFitLen = lineLen;
            lastFitPtr = p;
            anyFit = true;
        }
        else
        {
            lineLen = prevLen;
            if (!anyFit)
            {
                // Fallback: split inside the word
                const char* cp = wordStart;
                lineLen = 0;
                while (*cp && lineLen < outMax - 1)
                {
                    out[lineLen++] = (char16_t)(unsigned char)*cp;
                    out[lineLen] = 0;
                    nft2_measureString(font, out, width, height);
                    if (width > maxWidth)
                    {
                        lineLen--;
                        out[lineLen] = 0;
                        break;
                    }
                    ++cp;
                }
                lastFitLen = lineLen;
                lastFitPtr = cp;
            }
            break;
        }

        p = SkipSpaces(p);
        if (!*p)
        {
            lastFitLen = lineLen;
            lastFitPtr = p;
            break;
        }
    }

    out[lastFitLen] = 0;
    return SkipSpaces(lastFitPtr);
}
} // namespace

CheatsBottomSheetView::CheatsBottomSheetView(
    IRomBrowserController* romBrowserController,
    const MaterialColorScheme* materialColorScheme,
    const IFontRepository* fontRepository,
    const char* gameCode,
    u32 crc)
    : _romBrowserController(romBrowserController)
    , _materialColorScheme(materialColorScheme)
    , _fontRepository(fontRepository)
    , _titleLabel(128, 16, 25, fontRepository->GetFont(FontType::Medium11))
    , _gameCodeLabel(40, 14, 20, fontRepository->GetFont(FontType::Medium7_5))
    , _crcLabel(60, 14, 20, fontRepository->GetFont(FontType::Medium7_5))
    , _statusLabel(200, 16, 48, fontRepository->GetFont(FontType::Regular10))
    , _itemLabels{
        Label2DView(kItemWidth, kItemHeight, 50, fontRepository->GetFont(FontType::Regular10)),
        Label2DView(kItemWidth, kItemHeight, 50, fontRepository->GetFont(FontType::Regular10)),
        Label2DView(kItemWidth, kItemHeight, 50, fontRepository->GetFont(FontType::Regular10)),
        Label2DView(kItemWidth, kItemHeight, 50, fontRepository->GetFont(FontType::Regular10)),
        Label2DView(kItemWidth, kItemHeight, 50, fontRepository->GetFont(FontType::Regular10))
    }
    , _debugLabels{
        Label2DView(kItemWidth, kItemHeight, 48, fontRepository->GetFont(FontType::Regular10)),
        Label2DView(kItemWidth, kItemHeight, 48, fontRepository->GetFont(FontType::Regular10)),
        Label2DView(kItemWidth, kItemHeight, 48, fontRepository->GetFont(FontType::Regular10)),
        Label2DView(kItemWidth, kItemHeight, 48, fontRepository->GetFont(FontType::Regular10)),
        Label2DView(kItemWidth, kItemHeight, 48, fontRepository->GetFont(FontType::Regular10)),
        Label2DView(kItemWidth, kItemHeight, 48, fontRepository->GetFont(FontType::Regular10)),
        Label2DView(kItemWidth, kItemHeight, 48, fontRepository->GetFont(FontType::Regular10))
    }

    , _descriptionIndex(-1)
    , _crc(crc)
{
    _parseResult = CheatParseResult::NoCheatsFound;

    if (_romBrowserController)
    {
        const auto& viewModel = _romBrowserController->GetRomBrowserViewModel();
        if (viewModel.IsValid())
        {
            int selected = viewModel->GetSelectedItem();
            if (selected >= 0)
            {
                const auto& fileInfo = viewModel->GetFileInfoManager().GetItem(selected);
                const TCHAR* name = fileInfo.GetFileName();
                if (name)
                {
                    // Convert TCHAR to ASCII char for file operations
                    for (int j = 0; name[j] && j < (int)sizeof(_romFileName) - 1; ++j)
                    {
                        _romFileName[j] = (char)name[j];
                    }
                    _romFileName[sizeof(_romFileName) - 1] = 0;
                }

                _parseResult = _cheatList.Parse(fileInfo.GetFastFileRef());
                _hasCheats = (_parseResult == CheatParseResult::Success);

                // Usa gameCode e crc passati se forniti
                if (gameCode && gameCode[0]) {
                    memcpy(_gameCode, gameCode, 4);
                    _gameCode[4] = 0;
                } else if (_hasCheats) {
                    memcpy(_gameCode, _cheatList.GetGameCode(), 4);
                    _gameCode[4] = 0;
                }

                // Se serve, puoi salvare anche il CRC passato in una variabile membro

                if (_hasCheats)
                {
                    CheatSaveManager::LoadSelections(_cheatList, _gameCode, _romFileName);
                }
            }
        }
    }

    _showDebug = !_hasCheats;

    _titleLabel.SetText((const char16_t*)L"Cheats");
    const char16_t* localizedTitle = Localization::Translate("cheats");
    if (localizedTitle && localizedTitle[0] != 0)
        _titleLabel.SetText(localizedTitle);
    
    _titleLabel.SetBackgroundColor(materialColorScheme->GetColor(md::sys::color::surfaceContainerLow));
    _titleLabel.SetForegroundColor(materialColorScheme->GetColor(md::sys::color::onSurface));
    AddChildTail(&_titleLabel);

    _gameCodeLabel.SetBackgroundColor(materialColorScheme->GetColor(md::sys::color::surfaceContainerLow));
    _gameCodeLabel.SetForegroundColor(materialColorScheme->onSurfaceVariant);
    AddChildTail(&_gameCodeLabel);

    _crcLabel.SetBackgroundColor(materialColorScheme->GetColor(md::sys::color::surfaceContainerLow));
    _crcLabel.SetForegroundColor(materialColorScheme->onSurfaceVariant);
    AddChildTail(&_crcLabel);

    // Convert ASCII GameCode to UTF-16
    char16_t wc[16];
    int i = 0;
    for (; _gameCode[i]; ++i) wc[i] = (char16_t)(unsigned char)_gameCode[i];
    wc[i] = 0;
    _gameCodeLabel.SetText(wc);

    // Convert CRC
    char buf[16];
    u32 crcVal = _crc;
    mini_snprintf(buf, sizeof(buf), "%08X", crcVal);
    for (i = 0; buf[i]; ++i) wc[i] = (char16_t)(unsigned char)buf[i];
    wc[i] = 0;
    _crcLabel.SetText(wc);

    _statusLabel.SetBackgroundColor(materialColorScheme->GetColor(md::sys::color::surfaceContainerLow));
    AddChildTail(&_statusLabel);

    if (_hasCheats)
    {
        for (int i = 0; i < CHEATS_VIEW_VISIBLE_ITEMS; ++i)
        {
            _itemLabels[i].SetBackgroundColor(materialColorScheme->GetColor(md::sys::color::surfaceContainerLow));
            _itemLabels[i].SetForegroundColor(materialColorScheme->onSurfaceVariant);
            _itemLabels[i].SetEllipsis(true);
            AddChildTail(&_itemLabels[i]);
        }
    }

    UpdateLabels();
    UpdateStatusLabel();
}

void CheatsBottomSheetView::InitVram(const VramContext& vramContext)
{
    BottomSheetView::InitVram(vramContext);
}

void CheatsBottomSheetView::VBlank()
{
    BottomSheetView::VBlank();
}

void CheatsBottomSheetView::SetGraphics(const ChipView::VramToken& chipVramToken)
{
    // Reserved for future use (e.g., checkbox icons)
}





void CheatsBottomSheetView::Update()
{
    BottomSheetView::Update();

    int baseY = _position.y;

    _titleLabel.SetPosition(12, _position.y + 12);
    
    // Position code & crc
    int codeX = 140; 
    _gameCodeLabel.SetPosition(codeX, baseY + kTitleY + 2);
    int codeW = _gameCodeLabel.GetStringWidth();
    _crcLabel.SetPosition(codeX + codeW + 8, baseY + kTitleY + 2);

    if (_hasCheats)
    {
        // Position item labels
        for (int i = 0; i < CHEATS_VIEW_VISIBLE_ITEMS; ++i)
        {
            _itemLabels[i].SetPosition(kItemX, baseY + kItemStartY + i * kItemSpacing);
        }

        // Status below items
        int statusY = baseY + kItemStartY + CHEATS_VIEW_VISIBLE_ITEMS * kItemSpacing + 4;
        _statusLabel.SetPosition(kStatusX, statusY + 10);
    }
    else
    {
        _statusLabel.SetPosition(kStatusX, baseY + kItemStartY + 8);
    }
}

void CheatsBottomSheetView::Draw(GraphicsContext& graphicsContext)
{
    graphicsContext.SetClipArea(GetBounds());
    u32 oldPrio = graphicsContext.SetPriority(1);
    {
        BottomSheetView::Draw(graphicsContext);
    }
    graphicsContext.SetPriority(oldPrio);
    graphicsContext.ResetClipArea();
}



void CheatsBottomSheetView::Focus(FocusManager& focusManager)
{
    if (_hasCheats && _cheatList.GetVisibleCount() > 0)
    {
        focusManager.Focus(&_itemLabels[0]);
    }
    else
    {
        // Focus the status label so HandleInput still receives B presses
        focusManager.Focus(&_statusLabel);
    }
}

View* CheatsBottomSheetView::MoveFocus(View* currentFocus,
    FocusMoveDirection direction, View* source)
{
    // Find which item label is currently focused
    int focusedIdx = -1;
    for (int i = 0; i < CHEATS_VIEW_VISIBLE_ITEMS; ++i)
    {
        if (currentFocus == &_itemLabels[i])
        {
            focusedIdx = i;
            break;
        }
    }

    if (focusedIdx < 0) return nullptr;

    int visibleCount = _cheatList.GetVisibleCount();
    if (visibleCount <= 0) return nullptr;

    if (direction == FocusMoveDirection::Down)
    {
        int newVisIdx = _scroll_offset + focusedIdx + 1;
        if (newVisIdx < visibleCount)
        {
            if (focusedIdx + 1 < CHEATS_VIEW_VISIBLE_ITEMS)
            {
                _cursor_index = focusedIdx + 1;
                _descriptionIndex = -1;
                UpdateLabels();
                return &_itemLabels[_cursor_index];
            }
            else
            {
                ScrollDown();
                _descriptionIndex = -1;
                UpdateLabels();
                return &_itemLabels[_cursor_index];
            }
        }
    }
    else if (direction == FocusMoveDirection::Up)
    {
        int newVisIdx = _scroll_offset + focusedIdx - 1;
        if (newVisIdx >= 0)
        {
            if (focusedIdx - 1 >= 0)
            {
                _cursor_index = focusedIdx - 1;
                _descriptionIndex = -1;
                UpdateLabels();
                return &_itemLabels[_cursor_index];
            }
            else
            {
                ScrollUp();
                _descriptionIndex = -1;
                UpdateLabels();
                return &_itemLabels[_cursor_index];
            }
        }
    }

    return nullptr;
}

bool CheatsBottomSheetView::HandleInput(const InputProvider& inputProvider, FocusManager& focusManager)
{
    if (!_hasCheats)
    {
        // No cheats - only allow closing
        if (inputProvider.Triggered(InputKey::B))
        {
            _romBrowserController->HideCheats();
            return true;
        }
        return false;
    }

    // Gestione descrizione inline
    if (inputProvider.Triggered(InputKey::Y)) {
        int visIdx = GetSelectedCursorVisibleIndex();
        if (visIdx >= 0 && visIdx < _cheatList.GetVisibleCount()) {
            if (_descriptionIndex == visIdx) {
                // Se gia visibile, la nascondo
                _descriptionIndex = -1;
            } else {
                _descriptionIndex = visIdx;

                // Se il cursore e sull'ultima riga visibile, crea spazio per la descrizione
                if (_cursor_index >= CHEATS_VIEW_VISIBLE_ITEMS - 1
                    && _scroll_offset + _cursor_index + 1 < _cheatList.GetVisibleCount()) {
                    _scroll_offset++;
                    _cursor_index--;
                    focusManager.Focus(&_itemLabels[_cursor_index]);
                }
            }
            UpdateLabels();
            return true;
        }
    }

    if (inputProvider.Triggered(InputKey::A))
    {
        int visIdx = GetSelectedCursorVisibleIndex();
        if (visIdx >= 0 && visIdx < _cheatList.GetVisibleCount())
        {
            auto& item = _cheatList.GetVisibleItem(visIdx);
            if (item.IsFolder())
            {
                _descriptionIndex = -1;
                // Enter folder view
                int realIdx = _cheatList.GetVisibleIndices()[visIdx];
                _currentFolderIndex = realIdx;

                // Salva l'indice della cartella per il focus al ritorno (indice relativo alla vista corrente)
                _lastFocusedFolderIndex = _cursor_index;

                // Save state before entering
                _savedRootScrollOffset = _scroll_offset;
                _savedRootCursorIndex = _cursor_index;

                // When entering folder, we must rebuild the visible list to show ONLY folder contents
                _cheatList.BuildVisibleListForFolder(_currentFolderIndex);

                // Focus sempre sul primo elemento della cartella
                _scroll_offset = 0;
                _cursor_index = 0;
                _focusedVisibleIndex = 0;
                
                // IMPORTANTE: Resetta il focus del FocusManager sul primo elemento
                focusManager.Focus(&_itemLabels[0]);

                EnsureCursorVisible();
                UpdateLabels();
                UpdateStatusLabel();
            }
            else
            {
                // Se il cheat è di tipo EOne (selezione singola), deseleziona tutti gli altri e toggla questo
                if (item.flags & CheatItem::EOne) {
                    auto& items = _cheatList.GetItems();
                    int realIdx = _cheatList.GetVisibleIndices()[visIdx];
                    // Trova la cartella che contiene questo cheat
                    int folderStart = realIdx - 1;
                    while (folderStart >= 0 && !(items[folderStart].flags & CheatItem::EFolder))
                        folderStart--;
                    if (folderStart >= 0) {
                        for (int i = folderStart + 1; i < items.size(); ++i) {
                            if (!(items[i].flags & CheatItem::EInFolder)) break;
                            if (&items[i] != &item) items[i].SetSelected(false);
                        }
                    }
                    // Toggle anche se già selezionato (così puoi deselezionare tutto)
                    item.ToggleSelected();
                } else {
                    item.ToggleSelected();
                }
                _selection_dirty = true;

                if (false) {}

                UpdateLabels();
                UpdateStatusLabel();
            }
        }
        return true;
    }
    
    if (inputProvider.Triggered(InputKey::L))
    {
        // Deselect all cheats
        auto& items = _cheatList.GetItems();
        for (int i = 0; i < items.size(); ++i)
        {
            items[i].SetSelected(false);
        }
        _selection_dirty = true;
        if (false) {}
        UpdateLabels();
        UpdateStatusLabel();
        return true;
    }
    
    // Select button: View Enabled Cheats (Filter view)
    if (inputProvider.Triggered(InputKey::Select))
    {
        _savedViewScrollOffset = _scroll_offset;
        _savedViewCursorIndex = _cursor_index;

        _descriptionIndex = -1;

        _cheatList.BuildVisibleListEnabledOnly();
        _scroll_offset = 0;
        _cursor_index = 0;
        _focusedVisibleIndex = 0;
        if (_cheatList.GetVisibleCount() > 0)
            focusManager.Focus(&_itemLabels[0]);
        else
            focusManager.Focus(&_statusLabel);
        UpdateLabels();
        UpdateStatusLabel(); 
        // We are "reusing" the main list UI but with a filtered backend list.
        return true;
    }

    if (inputProvider.Triggered(InputKey::B))
    {
        // Remove overlay mode for enabled list
        if (_cheatList.IsEnabledListMode())
        {
            _descriptionIndex = -1;
            _cheatList.BuildVisibleListForFolder(_currentFolderIndex);
            _scroll_offset = _savedViewScrollOffset;
            _cursor_index = _savedViewCursorIndex;
            EnsureCursorVisible();
            UpdateLabels();
            UpdateStatusLabel();
            if (_cheatList.GetVisibleCount() > 0)
                focusManager.Focus(&_itemLabels[_cursor_index]);
            else
                focusManager.Focus(&_statusLabel);
            return true;
        }

        if (_currentFolderIndex != -1)
        {
            // Exit folder view
            int restoreCursorIdx = _lastFocusedFolderIndex;
            _currentFolderIndex = -1;
            _cheatList.BuildVisibleListForFolder(-1);
            _scroll_offset = _savedRootScrollOffset;
            _cursor_index = _savedRootCursorIndex;
            _descriptionIndex = -1;

            // Ripristina il focus sulla cartella da cui si è usciti
            if (restoreCursorIdx >= 0 && restoreCursorIdx < CHEATS_VIEW_VISIBLE_ITEMS)
            {
                _cursor_index = restoreCursorIdx;
                _focusedVisibleIndex = restoreCursorIdx;
                
                // Resetta il focus del FocusManager sull'elemento che era selezionato
                focusManager.Focus(&_itemLabels[_cursor_index]);
            }

            EnsureCursorVisible();
            UpdateLabels();
            UpdateStatusLabel();
        }
        else
        {
            SaveSelectionsAndClose();
        }
        return true;
    }

    return false;
}

void CheatsBottomSheetView::UpdateLabels()
{
    int visibleCount = _cheatList.GetVisibleCount();
    int descIdx = _descriptionIndex;
    int cheatIdx = _scroll_offset;
    for (int i = 0; i < CHEATS_VIEW_VISIBLE_ITEMS; ++i)
    {
        _itemLabels[i].SetText(u"");
        _itemLabels[i].SetForegroundColor(_materialColorScheme->onSurfaceVariant);
        _itemLabels[i].SetBackgroundColor(_materialColorScheme->GetColor(md::sys::color::surfaceContainerLow));

        if (cheatIdx >= visibleCount)
            continue;

        const auto& item = _cheatList.GetVisibleItem(cheatIdx);
        char16_t displayText[64];
        int pos = 0;
        bool isFocused = (cheatIdx == GetSelectedCursorVisibleIndex());
        if (isFocused)
        {
            displayText[pos++] = u' ';
            displayText[pos++] = u' ';
        }
        if (item.IsFolder())
        {
            displayText[pos++] = u'[';
        }
        else
        {
            if (item.IsSelected())
            {
                displayText[pos++] = u'[';
                displayText[pos++] = u'x';
                displayText[pos++] = u']';
                displayText[pos++] = u' ';
            }
            else
            {
                displayText[pos++] = u'[';
                displayText[pos++] = u' ';
                displayText[pos++] = u']';
                displayText[pos++] = u' ';
            }
        }
        const char* name = item.name;
        while (*name && pos < 58)
        {
            displayText[pos++] = (char16_t)(unsigned char)*name++;
        }
        if (item.IsFolder())
        {
            displayText[pos++] = u']';
        }
        displayText[pos] = 0;
        _itemLabels[i].SetText(displayText, (u32)pos);

        if (isFocused)
        {
            _itemLabels[i].SetForegroundColor(_materialColorScheme->GetColor(md::sys::color::primary));
            _itemLabels[i].SetBackgroundColor(_materialColorScheme->GetColor(md::sys::color::surfaceContainerHighest));
        }
        else if (item.IsSelected())
        {
            _itemLabels[i].SetForegroundColor(_materialColorScheme->GetColor(md::sys::color::onSecondaryContainer));
            _itemLabels[i].SetBackgroundColor(_materialColorScheme->GetColor(md::sys::color::surfaceContainerLow));
        }
        else
        {
            _itemLabels[i].SetForegroundColor(_materialColorScheme->onSurfaceVariant);
            _itemLabels[i].SetBackgroundColor(_materialColorScheme->GetColor(md::sys::color::surfaceContainerLow));
        }

        bool showDesc = (descIdx == cheatIdx);
        cheatIdx++;

        if (showDesc && i + 1 < CHEATS_VIEW_VISIBLE_ITEMS)
        {
            const char* note = item.note;
            const char* descText = (note && note[0]) ? note : "No description available.";
            const nft2_header_t* font = _fontRepository->GetFont(FontType::Regular10);
            const char* p = descText;
            while (p && *p && i + 1 < CHEATS_VIEW_VISIBLE_ITEMS)
            {
                ++i;
                char16_t desc[128];
                p = WrapNextLine(font, p, kItemWidth, desc, (int)(sizeof(desc) / sizeof(desc[0])));
                if (desc[0] == 0)
                    break;
                _itemLabels[i].SetText(desc);
                _itemLabels[i].SetForegroundColor(_materialColorScheme->onSurfaceVariant);
                _itemLabels[i].SetBackgroundColor(_materialColorScheme->GetColor(md::sys::color::surfaceContainerHighest));
            }
        }
    }
}

void CheatsBottomSheetView::UpdateStatusLabel()
{
    if (!_hasCheats)
    {
        const char16_t* msg;
        switch (_parseResult)
        {
            case CheatParseResult::DatFileNotFound:
                msg = Localization::Translate("cheats_dat_missing");
                break;
            default:
                msg = Localization::Translate("cheats_not_found");
                break;
        }
        _statusLabel.SetText(msg);
        return;
    }

    u32 total = 0;
    u32 selected = 0;
    const auto& items = _cheatList.GetItems();
    for (int i = 0; i < items.size(); ++i)
    {
        const auto& item = items[i];
        if (!(item.flags & CheatItem::EFolder))
        {
            total++;
            if (item.flags & CheatItem::ESelected)
                selected++;
        }
    }
    
    char buf[64];
    mini_snprintf(buf, sizeof(buf), "%u/%u", selected, total);

    char16_t u16buf[64];
    int len = 0;
    for (; buf[len]; ++len)
        u16buf[len] = (char16_t)(unsigned char)buf[len];
    u16buf[len] = 0;
    
    _statusLabel.SetText(u16buf, (u32)len);
    _statusLabel.SetForegroundColor(_materialColorScheme->onSurfaceVariant);

    // Folder name
    if (_currentFolderIndex != -1)
    {
        const auto& items = _cheatList.GetItems();
        if (_currentFolderIndex >= 0 && _currentFolderIndex < items.size())
        {
            // Just show folder name as request #7
            const char* name = items[_currentFolderIndex].name;
            mini_snprintf(buf, sizeof(buf), "[%s]", name);
            
            len = 0;
            for (; buf[len]; ++len)
                u16buf[len] = (char16_t)(unsigned char)buf[len];
            u16buf[len] = 0;
            
        }
    }
}


void CheatsBottomSheetView::ScrollDown()
{
    int visibleCount = _cheatList.GetVisibleCount();
    if (visibleCount == 0) return; // Cannot scroll empty list

    int maxScroll = visibleCount - CHEATS_VIEW_VISIBLE_ITEMS;
    if (maxScroll < 0) maxScroll = 0;

    int totalVisIdx = _scroll_offset + _cursor_index;
    
    // Check if we are at the last item
    if (totalVisIdx >= visibleCount - 1)
    {
        // Wrap to top
        _scroll_offset = 0;
        _cursor_index = 0;
    }
    else
    {
        // Normal scroll down
        if (_cursor_index < CHEATS_VIEW_VISIBLE_ITEMS - 1 && _cursor_index < visibleCount - 1)
        {
             _cursor_index++;
        }
        else
        {
             if (_scroll_offset < maxScroll)
                 _scroll_offset++;
        }
    }
}

void CheatsBottomSheetView::ScrollUp()
{
    int visibleCount = _cheatList.GetVisibleCount();
    if (visibleCount == 0) return;

    int totalVisIdx = _scroll_offset + _cursor_index;
    
    // Check if we are at the first item
    if (totalVisIdx <= 0)
    {
        // Wrap to bottom
        int lastIdx = visibleCount - 1;
         // Set scroll offset so last item is at bottom
         int maxScroll = visibleCount - CHEATS_VIEW_VISIBLE_ITEMS;
         if (maxScroll < 0) maxScroll = 0;
         
         _scroll_offset = maxScroll;
         _cursor_index = lastIdx - _scroll_offset;
    }
    else
    {
        // Normal scroll up
        if (_cursor_index > 0)
        {
            _cursor_index--;
        }
        else
        {
            if (_scroll_offset > 0)
                _scroll_offset--;
        }
    }
}

void CheatsBottomSheetView::EnsureCursorVisible()
{
    int visibleCount = _cheatList.GetVisibleCount();
    if (visibleCount <= 0)
    {
        _scroll_offset = 0;
        _cursor_index = 0;
        return;
    }

    // Make sure scroll offset + cursor is within range
    int totalVisIdx = _scroll_offset + _cursor_index;
    if (totalVisIdx >= visibleCount)
    {
        totalVisIdx = visibleCount - 1;
        _scroll_offset = totalVisIdx - CHEATS_VIEW_VISIBLE_ITEMS + 1;
        if (_scroll_offset < 0) _scroll_offset = 0;
        _cursor_index = totalVisIdx - _scroll_offset;
    }
}

int CheatsBottomSheetView::GetSelectedCursorVisibleIndex() const
{
    return _scroll_offset + _cursor_index;
}

void CheatsBottomSheetView::SaveSelectionsAndClose()
{
    if (_selection_dirty && _hasCheats)
    {
        CheatSaveManager::SaveSelections(_cheatList, _gameCode, _romFileName);
        _cheatList.UpdateUsrCheatDat("/_pico/extras/usrcheat.dat");
    }
    _romBrowserController->HideCheats();
}

