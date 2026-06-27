#include "common.h"
#include <array>
#include "gui/GraphicsContext.h"
#include "themes/material/MaterialColorScheme.h"
#include "themes/IFontRepository.h"
#include "gui/input/InputProvider.h"
#include "SearchBottomSheetView.h"

#define TITLE_LABEL_X               20
#define TITLE_LABEL_Y               12
#define SEARCH_LABEL_X              20
#define SEARCH_LABEL_Y              34
#define STATUS_LABEL_X              20
#define STATUS_LABEL_Y              54
#define KEYBOARD_X                  16
#define KEYBOARD_Y                  74
#define KEY_WIDTH                   28
#define KEY_HEIGHT                  16
#define KEY_GAP_X                   2
#define KEY_GAP_Y                   2

const std::array<SearchBottomSheetView::KeySpec, SearchBottomSheetView::KEY_COUNT>
    SearchBottomSheetView::sKeySpecs =
{
    SearchBottomSheetView::KeySpec { u"A", SearchBottomSheetView::KeyType::Character, 'A' },
    SearchBottomSheetView::KeySpec { u"B", SearchBottomSheetView::KeyType::Character, 'B' },
    SearchBottomSheetView::KeySpec { u"C", SearchBottomSheetView::KeyType::Character, 'C' },
    SearchBottomSheetView::KeySpec { u"D", SearchBottomSheetView::KeyType::Character, 'D' },
    SearchBottomSheetView::KeySpec { u"E", SearchBottomSheetView::KeyType::Character, 'E' },
    SearchBottomSheetView::KeySpec { u"F", SearchBottomSheetView::KeyType::Character, 'F' },
    SearchBottomSheetView::KeySpec { u"G", SearchBottomSheetView::KeyType::Character, 'G' },
    SearchBottomSheetView::KeySpec { u"H", SearchBottomSheetView::KeyType::Character, 'H' },
    SearchBottomSheetView::KeySpec { u"I", SearchBottomSheetView::KeyType::Character, 'I' },
    SearchBottomSheetView::KeySpec { u"J", SearchBottomSheetView::KeyType::Character, 'J' },
    SearchBottomSheetView::KeySpec { u"K", SearchBottomSheetView::KeyType::Character, 'K' },
    SearchBottomSheetView::KeySpec { u"L", SearchBottomSheetView::KeyType::Character, 'L' },
    SearchBottomSheetView::KeySpec { u"M", SearchBottomSheetView::KeyType::Character, 'M' },
    SearchBottomSheetView::KeySpec { u"N", SearchBottomSheetView::KeyType::Character, 'N' },
    SearchBottomSheetView::KeySpec { u"O", SearchBottomSheetView::KeyType::Character, 'O' },
    SearchBottomSheetView::KeySpec { u"P", SearchBottomSheetView::KeyType::Character, 'P' },
    SearchBottomSheetView::KeySpec { u"Q", SearchBottomSheetView::KeyType::Character, 'Q' },
    SearchBottomSheetView::KeySpec { u"R", SearchBottomSheetView::KeyType::Character, 'R' },
    SearchBottomSheetView::KeySpec { u"S", SearchBottomSheetView::KeyType::Character, 'S' },
    SearchBottomSheetView::KeySpec { u"T", SearchBottomSheetView::KeyType::Character, 'T' },
    SearchBottomSheetView::KeySpec { u"U", SearchBottomSheetView::KeyType::Character, 'U' },
    SearchBottomSheetView::KeySpec { u"V", SearchBottomSheetView::KeyType::Character, 'V' },
    SearchBottomSheetView::KeySpec { u"W", SearchBottomSheetView::KeyType::Character, 'W' },
    SearchBottomSheetView::KeySpec { u"X", SearchBottomSheetView::KeyType::Character, 'X' },
    SearchBottomSheetView::KeySpec { u"Y", SearchBottomSheetView::KeyType::Character, 'Y' },
    SearchBottomSheetView::KeySpec { u"Z", SearchBottomSheetView::KeyType::Character, 'Z' },
    SearchBottomSheetView::KeySpec { u"0", SearchBottomSheetView::KeyType::Character, '0' },
    SearchBottomSheetView::KeySpec { u"1", SearchBottomSheetView::KeyType::Character, '1' },
    SearchBottomSheetView::KeySpec { u"2", SearchBottomSheetView::KeyType::Character, '2' },
    SearchBottomSheetView::KeySpec { u"3", SearchBottomSheetView::KeyType::Character, '3' },
    SearchBottomSheetView::KeySpec { u"4", SearchBottomSheetView::KeyType::Character, '4' },
    SearchBottomSheetView::KeySpec { u"5", SearchBottomSheetView::KeyType::Character, '5' },
    SearchBottomSheetView::KeySpec { u"6", SearchBottomSheetView::KeyType::Character, '6' },
    SearchBottomSheetView::KeySpec { u"7", SearchBottomSheetView::KeyType::Character, '7' },
    SearchBottomSheetView::KeySpec { u"8", SearchBottomSheetView::KeyType::Character, '8' },
    SearchBottomSheetView::KeySpec { u"9", SearchBottomSheetView::KeyType::Character, '9' },
    SearchBottomSheetView::KeySpec { u"SP", SearchBottomSheetView::KeyType::Space, ' ' },
    SearchBottomSheetView::KeySpec { u"DEL", SearchBottomSheetView::KeyType::Backspace, 0 },
    SearchBottomSheetView::KeySpec { u"CLR", SearchBottomSheetView::KeyType::Clear, 0 },
    SearchBottomSheetView::KeySpec { u"OK", SearchBottomSheetView::KeyType::Close, 0 }
};

SearchBottomSheetView::SearchBottomSheetView(std::unique_ptr<SearchViewModel> viewModel,
    const MaterialColorScheme* materialColorScheme, const IFontRepository* fontRepository,
    FocusManager* focusManager)
    : _viewModel(std::move(viewModel))
    , _titleLabel(Label2DView::CreateShared(160, 16, 32, fontRepository->GetFont(FontType::Medium11)))
    , _searchLabel(Label2DView::CreateShared(216, 16, SearchViewModel::QUERY_MAX_LENGTH + 2,
        fontRepository->GetFont(FontType::Regular10)))
    , _secondaryLabel(Label2DView::CreateShared(220, 16, 64, fontRepository->GetFont(FontType::Regular10)))
    , _materialColorScheme(materialColorScheme)
    , _focusManager(focusManager)
{
    _titleLabel->SetText(u"Search");
    _searchLabel->SetText(u"> ");
    _secondaryLabel->SetText(u"Enter text and press OK to close");
    _secondaryLabel->SetEllipsisStyle(LabelView::EllipsisStyle::Ellipsis);
    AddChildTail(_titleLabel.GetPointer());
    AddChildTail(_searchLabel.GetPointer());
    AddChildTail(_secondaryLabel.GetPointer());

    for (int i = 0; i < KEY_COUNT; i++)
    {
        _keyLabels[i] = SharedPtr<Label2DView>::MakeShared(KEY_WIDTH, KEY_HEIGHT, 4,
            fontRepository->GetFont(FontType::Medium10));
        _keyLabels[i]->SetText(sKeySpecs[i].label);
        _keyLabels[i]->SetHorizontalAlignment(Alignment::Center);
        AddChildTail(_keyLabels[i].GetPointer());
    }

    UpdateQueryLabel();
    UpdateStatusLabel();
}

void SearchBottomSheetView::Close()
{
    _viewModel->Close();
}

void SearchBottomSheetView::InitVram(const VramContext& vramContext)
{
    BottomSheetView::InitVram(vramContext);
}

void SearchBottomSheetView::Update()
{
    _titleLabel->SetPosition(TITLE_LABEL_X, _position.y + TITLE_LABEL_Y);
    _searchLabel->SetPosition(SEARCH_LABEL_X, _position.y + SEARCH_LABEL_Y);
    _secondaryLabel->SetPosition(STATUS_LABEL_X, _position.y + STATUS_LABEL_Y);

    for (int i = 0; i < KEY_COUNT; i++)
    {
        const int col = i % KEYBOARD_COLS;
        const int row = i / KEYBOARD_COLS;
        _keyLabels[i]->SetPosition(
            KEYBOARD_X + col * (KEY_WIDTH + KEY_GAP_X),
            _position.y + KEYBOARD_Y + row * (KEY_HEIGHT + KEY_GAP_Y));
    }

    BottomSheetView::Update();
}

void SearchBottomSheetView::Draw(GraphicsContext& graphicsContext)
{
    const auto backColor = _materialColorScheme->GetColor(md::sys::color::surfaceContainerHighest);

    graphicsContext.SetClipArea(GetBounds());
    u32 oldPrio = graphicsContext.SetPriority(1);

    _titleLabel->SetBackgroundColor(backColor);
    _titleLabel->SetForegroundColor(_materialColorScheme->onSurface);

    _searchLabel->SetBackgroundColor(_materialColorScheme->GetColor(md::sys::color::surfaceBright));
    _searchLabel->SetForegroundColor(_materialColorScheme->onSurface);

    _secondaryLabel->SetBackgroundColor(backColor);
    _secondaryLabel->SetForegroundColor(_materialColorScheme->onSurfaceVariant);

    for (int i = 0; i < KEY_COUNT; i++)
    {
        const bool selected = i == _selectedKey;
        _keyLabels[i]->SetBackgroundColor(selected
            ? _materialColorScheme->GetColor(md::sys::color::primary)
            : _materialColorScheme->GetColor(md::sys::color::surfaceBright));
        _keyLabels[i]->SetForegroundColor(selected
            ? _materialColorScheme->onPrimary
            : _materialColorScheme->onSurface);
    }

    BottomSheetView::Draw(graphicsContext);
    graphicsContext.SetPriority(oldPrio);
    graphicsContext.ResetClipArea();
}

bool SearchBottomSheetView::HandleInput(const InputProvider& inputProvider, FocusManager& focusManager)
{
    if (inputProvider.Triggered(InputKey::A))
    {
        HandleSelectedKey();
        UpdateQueryLabel();
        UpdateStatusLabel();
        return true;
    }

    if (inputProvider.Triggered(InputKey::X))
    {
        _viewModel->ClearQuery();
        UpdateQueryLabel();
        UpdateStatusLabel();
        return true;
    }

    if (inputProvider.Triggered(InputKey::B) || inputProvider.Triggered(InputKey::Y))
    {
        _viewModel->Close();
        return true;
    }

    return false;
}

SharedPtr<View> SearchBottomSheetView::MoveFocus(const SharedPtr<View>& currentFocus,
    FocusMoveDirection direction, View* source)
{
    int keyIndex = GetKeyIndexFromView(currentFocus);
    if (keyIndex < 0)
    {
        return View::MoveFocus(currentFocus, direction, source);
    }

    int col = keyIndex % KEYBOARD_COLS;
    int row = keyIndex / KEYBOARD_COLS;

    if (direction == FocusMoveDirection::Left)
    {
        col = (col + KEYBOARD_COLS - 1) % KEYBOARD_COLS;
    }
    else if (direction == FocusMoveDirection::Right)
    {
        col = (col + 1) % KEYBOARD_COLS;
    }
    else if (direction == FocusMoveDirection::Up)
    {
        row = (row + KEYBOARD_ROWS - 1) % KEYBOARD_ROWS;
    }
    else if (direction == FocusMoveDirection::Down)
    {
        row = (row + 1) % KEYBOARD_ROWS;
    }

    _selectedKey = row * KEYBOARD_COLS + col;
    return _keyLabels[_selectedKey];
}

void SearchBottomSheetView::UpdateQueryLabel()
{
    const char* query = _viewModel->GetQuery();
    int queryLength = _viewModel->GetQueryLength();
    _queryBuffer.fill(0);
    _queryBuffer[0] = u'>';
    _queryBuffer[1] = u' ';
    for (int i = 0; i < queryLength && i < SearchViewModel::QUERY_MAX_LENGTH; i++)
    {
        _queryBuffer[i + 2] = static_cast<char16_t>(query[i]);
    }
    _searchLabel->SetText(_queryBuffer.data());
}

void SearchBottomSheetView::UpdateStatusLabel()
{
    if (_viewModel->GetQueryLength() == 0)
    {
        _secondaryLabel->SetText(u"Type to search");
    } else
    {
        _secondaryLabel->SetText(u"Press OK to apply search");
    }

}

int SearchBottomSheetView::GetKeyIndexFromView(const SharedPtr<View>& view) const
{
    for (int i = 0; i < KEY_COUNT; i++)
    {
        if (_keyLabels[i].GetPointer() == view.GetPointer())
        {
            return i;
        }
    }
    return -1;
}

void SearchBottomSheetView::HandleSelectedKey()
{
    const auto& key = sKeySpecs[_selectedKey];
    switch (key.type)
    {
        case KeyType::Character:
            _viewModel->AppendCharacter(key.character);
            break;
        case KeyType::Space:
            _viewModel->AppendCharacter(' ');
            break;
        case KeyType::Backspace:
            _viewModel->BackspaceCharacter();
            break;
        case KeyType::Clear:
            _viewModel->ClearQuery();
            break;
        case KeyType::Close:
            _viewModel->ApplySearchAndClose();
            break;
    }
}
