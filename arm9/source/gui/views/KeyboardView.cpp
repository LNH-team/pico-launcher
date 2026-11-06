#include "common.h"
#include <string.h>
#include "gui/GraphicsContext.h"
#include "gui/OamManager.h"
#include "gui/OamBuilder.h"
#include "gui/IVramManager.h"
#include "gui/VramContext.h"
#include "gui/palette/GradientPalette.h"
#include "gui/input/InputProvider.h"
#include "gui/FocusManager.h"
#include "KeyboardView.h"

// Three rows of ten cells, 22 px apart and 17 px down, all placed from the
// view's own position. A key is one character wide at the size the small
// screen can still show a glyph cleanly.
#define KEY_X               10
#define KEY_STEP            22
#define KEY_ROW_STEP        17
#define KEY_WIDTH           21
#define KEY_HEIGHT          16

// The keyboard's own highlight, behind the focused key: a rounded bar filled
// with the focused background colour the owner gives it, in the theme's accent.
// A label's empty pixels are the transparent palette entry, so a key cannot
// paint its own background and the bar has to be a sprite. It is an oam wide
// enough to hold a key cell, and the columns past the key stay transparent, so
// it never covers the key beside it (cells are 22 px apart).
#define HIGHLIGHT_WIDTH     32
#define HIGHLIGHT_HEIGHT    16
#define HIGHLIGHT_CORNER    2

// One page per mode: the letters as a keyboard lays them out, then the digits
// with the punctuation a name uses, then the rest of the symbols. Columns past
// the end of a row are left empty and the d-pad steps over them.
static const char* const kPages[KeyboardView::kPageCount][KeyboardView::kKeyRows] =
{
    { "QWERTYUIOP", "ASDFGHJKL", "ZXCVBNM" },
    { "qwertyuiop", "asdfghjkl", "zxcvbnm" },
    { "1234567890", "-_'\".:;!", "()[]{}?" },
    { "&/@#%*+=<>", "$^|~", "" }
};
static const char* const kPageNames[KeyboardView::kPageCount] = { "Abc", "abc", "123", "#@" };

/// @brief The character a cell holds on a page, or 0 where the page leaves it empty.
static char CellChar(int page, int row, int col)
{
    const char* line = kPages[page][row];
    return (col < (int)strlen(line)) ? line[col] : 0;
}

/// @brief Writes the highlight bar's tiles: the key cell filled with palette
///        index 1, a 2 px rounding at each corner and the rest left transparent.
static void BuildHighlightTiles(u32* tiles)
{
    memset(tiles, 0, (HIGHLIGHT_WIDTH * HIGHLIGHT_HEIGHT) >> 1);
    u8* pixels = (u8*)tiles;
    for (int y = 0; y < HIGHLIGHT_HEIGHT; y++)
    {
        for (int x = 0; x < KEY_WIDTH; x++)
        {
            // how far into a corner the pixel sits, both ways
            int cutX = x < HIGHLIGHT_CORNER ? HIGHLIGHT_CORNER - x
                : (x >= KEY_WIDTH - HIGHLIGHT_CORNER ? x - (KEY_WIDTH - HIGHLIGHT_CORNER - 1) : 0);
            int cutY = y < HIGHLIGHT_CORNER ? HIGHLIGHT_CORNER - y
                : (y >= HIGHLIGHT_HEIGHT - HIGHLIGHT_CORNER ? y - (HIGHLIGHT_HEIGHT - HIGHLIGHT_CORNER - 1) : 0);
            if (cutX != 0 && cutY != 0 && cutX + cutY > HIGHLIGHT_CORNER)
                continue;
            // 4bpp obj tiles are 8x8 blocks: tile index first, then the pixel
            // inside it (four bytes a row, two pixels a byte)
            u32 offset = (y >> 3) * (HIGHLIGHT_WIDTH >> 3) * 32 + (x >> 3) * 32
                + (y & 7) * 4 + ((x & 7) >> 1);
            pixels[offset] |= (x & 1) ? 0x10 : 0x01;
        }
    }
}

KeyboardView::KeyboardView(const nft2_header_t* font)
    : _font(font)
{
    for (int r = 0; r < kKeyRows; r++)
    {
        for (int c = 0; c < kKeyColumns; c++)
        {
            _keyLabels[r][c] = Label2DView::CreateShared(KEY_WIDTH, KEY_HEIGHT, 4, _font);
            _keyLabels[r][c]->SetHorizontalAlignment(Alignment::Start);
            _keyLabels[r][c]->SetEllipsisStyle(LabelView::EllipsisStyle::Ellipsis);
            AddChildTail(_keyLabels[r][c].GetPointer());
        }
    }
    RefreshKeys();
}

void KeyboardView::InitVram(const VramContext& vramContext)
{
    const auto objVramManager = vramContext.GetObjVramManager();
    if (objVramManager && _highlightVramOffset == 0xFFFFFFFF)
    {
        constexpr u32 tilesSize = (HIGHLIGHT_WIDTH * HIGHLIGHT_HEIGHT) >> 1;
        static u32 tiles[64];
        static bool tilesBuilt = false;
        if (!tilesBuilt)
        {
            BuildHighlightTiles(tiles);
            tilesBuilt = true;
        }
        _highlightVramOffset = objVramManager->Alloc(tilesSize);
        // copied from static storage with memcpy: this runs in the vblank, and a
        // dma from a buffer that goes out of scope can land after the frame is gone
        memcpy((void*)objVramManager->GetVramAddress(_highlightVramOffset), tiles, tilesSize);
    }
    ViewContainer::InitVram(vramContext);
}

void KeyboardView::Update()
{
    for (int r = 0; r < kKeyRows; r++)
    {
        for (int c = 0; c < kKeyColumns; c++)
            _keyLabels[r][c]->SetPosition(_position.x + KEY_X + c * KEY_STEP, _position.y + r * KEY_ROW_STEP);
    }
    ViewContainer::Update();
}

bool KeyboardView::IsSpecialCell(int row, int col) const
{
    return row == kKeyRows - 1 && col >= kKeyColumns - kSpecialCount;
}

bool KeyboardView::IsUsableCell(int row, int col) const
{
    return IsSpecialCell(row, col) || CellChar(_page, row, col) != 0;
}

void KeyboardView::Draw(GraphicsContext& graphicsContext)
{
    if (!_visible)
        return;

    int focusedRow = -1;
    int focusedCol = -1;
    for (int r = 0; r < kKeyRows; r++)
    {
        for (int c = 0; c < kKeyColumns; c++)
        {
            auto& label = _keyLabels[r][c];
            bool focused = label->IsFocused();
            if (focused)
            {
                focusedRow = r;
                focusedCol = c;
            }
            // The background is only what a glyph's edge pixels blend towards:
            // the focused key takes the highlight colour there as well, so its
            // character sits on the bar without a rim of the sheet's colour.
            label->SetBackgroundColor(focused ? _colors.focusedBackground : _colors.background);
            label->SetForegroundColor(focused
                ? _colors.focusedText
                : (IsSpecialCell(r, c) ? _colors.specialText : _colors.text));
        }
    }

    // The bar is drawn before the keys, so the character sits on top of it.
    if (focusedRow >= 0 && _highlightVramOffset != 0xFFFFFFFF)
    {
        int x = _position.x + KEY_X + focusedCol * KEY_STEP;
        int y = _position.y + focusedRow * KEY_ROW_STEP;
        if (graphicsContext.IsVisible(Rectangle(x, y, KEY_WIDTH, KEY_HEIGHT)))
        {
            const auto& highlightColor = _colors.focusedBackground;
            u32 highlightRow = graphicsContext.GetPaletteManager().AllocRow(
                GradientPalette(highlightColor, highlightColor), y, y + KEY_HEIGHT);
            auto oams = graphicsContext.GetOamManager().AllocOams(1);
            OamBuilder::OamWithSize<HIGHLIGHT_WIDTH, HIGHLIGHT_HEIGHT>(
                    x, y, _highlightVramOffset >> 7)
                .WithPalette16(highlightRow)
                .WithPriority(graphicsContext.GetPriority())
                .Build(oams[0]);
        }
    }

    ViewContainer::Draw(graphicsContext);
}

bool KeyboardView::HandleInput(const InputProvider& inputProvider, FocusManager& focusManager)
{
    if (inputProvider.Triggered(InputKey::A))
    {
        int index = FocusIndex(focusManager.GetCurrentFocus());
        if (index >= 0)
            ActivateKey(index);
        return true;
    }
    if (inputProvider.Triggered(InputKey::B))
    {
        Backspace();
        return true;
    }
    if (inputProvider.Triggered(InputKey::X))
    {
        Append(' ');
        return true;
    }
    if (inputProvider.Triggered(InputKey::Y))
    {
        // the case swap, so a name needs no trip to the page key; from the
        // symbol pages it lands on lower case, the one after capitals
        _page = (_page == 1) ? 0 : 1;
        RefreshKeys();
        return true;
    }
    // Start and Select belong to the screen the keyboard sits on, so they keep
    // bubbling up the way they did before the keyboard existed.
    return View::HandleInput(inputProvider, focusManager);
}

SharedPtr<View> KeyboardView::MoveFocus(
    const SharedPtr<View>& currentFocus, FocusMoveDirection direction, View* source)
{
    int index = FocusIndex(currentFocus);
    if (index < 0)
        return nullptr;

    int row = index / kKeyColumns;
    int col = index % kKeyColumns;
    int rowStep = 0;
    int colStep = 0;
    switch (direction)
    {
        case FocusMoveDirection::Left:
            colStep = -1;
            break;
        case FocusMoveDirection::Right:
            colStep = 1;
            break;
        case FocusMoveDirection::Up:
            rowStep = -1;
            break;
        case FocusMoveDirection::Down:
            rowStep = 1;
            break;
        default:
            return nullptr;
    }

    // Step over the cells this page leaves empty, so the focus never lands on
    // a key that does nothing.
    row += rowStep;
    col += colStep;
    while (row >= 0 && row < kKeyRows && col >= 0 && col < kKeyColumns && !IsUsableCell(row, col))
    {
        row += rowStep;
        col += colStep;
    }
    if (row < 0 || row >= kKeyRows || col < 0 || col >= kKeyColumns)
        return nullptr;
    return _keyLabels[row][col];
}

void KeyboardView::Focus(FocusManager& focusManager)
{
    // a key, so the d-pad moves around the grid and A bubbles up here
    focusManager.Focus(_keyLabels[0][0]);
}

int KeyboardView::FocusIndex(const SharedPtr<View>& currentFocus) const
{
    auto* focus = currentFocus.GetPointer();
    for (int r = 0; r < kKeyRows; r++)
    {
        for (int c = 0; c < kKeyColumns; c++)
        {
            if (focus == _keyLabels[r][c].GetPointer())
                return r * kKeyColumns + c;
        }
    }
    return -1;
}

void KeyboardView::ActivateKey(int index)
{
    int row = index / kKeyColumns;
    int col = index % kKeyColumns;
    if (IsSpecialCell(row, col))
    {
        switch (col - (kKeyColumns - kSpecialCount))
        {
            case 0:
                Append(' ');
                break;
            case 1:
                Backspace();
                break;
            case 2:
                _page = (_page + 1) % kPageCount;
                RefreshKeys();
                break;
        }
        return;
    }

    char c = CellChar(_page, row, col);
    if (c != 0)
        Append(c);
}

void KeyboardView::Append(char c)
{
    if (!_buffer || _capacity == 0)
        return;
    u32 length = strlen(_buffer);
    if (length >= _capacity)
        return;
    _buffer[length] = c;
    _buffer[length + 1] = 0;
    NotifyTextChanged();
}

void KeyboardView::Backspace()
{
    if (!_buffer)
        return;
    u32 length = strlen(_buffer);
    if (length == 0)
        return;
    // step over continuation bytes, so deleting a character the keyboard did
    // not type (an accented letter from the file name) removes all of it
    length--;
    while (length > 0 && (static_cast<u8>(_buffer[length]) & 0xC0) == 0x80)
        length--;
    _buffer[length] = 0;
    NotifyTextChanged();
}

void KeyboardView::RefreshKeys()
{
    for (int r = 0; r < kKeyRows; r++)
    {
        for (int c = 0; c < kKeyColumns; c++)
        {
            const char* text = nullptr;
            char charText[2] = { 0, 0 };
            if (IsSpecialCell(r, c))
            {
                switch (c - (kKeyColumns - kSpecialCount))
                {
                    case 0:
                        text = "Spc";
                        break;
                    case 1:
                        text = "Del";
                        break;
                    default:
                        text = kPageNames[_page];
                        break;
                }
            }
            else
            {
                charText[0] = CellChar(_page, r, c);
                // A cell this page leaves empty still paints its background as a
                // space, so the focus colour of a neighbour stops at the key edge
                // instead of running on under the empty cell.
                text = (charText[0] != 0) ? charText : " ";
            }
            _keyLabels[r][c]->SetText(text);
        }
    }
}

void KeyboardView::NotifyTextChanged()
{
    if (_textChanged)
        _textChanged(_textChangedArg);
}
