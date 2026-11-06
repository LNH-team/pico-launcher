#pragma once
#include "core/math/Rgb.h"
#include "core/SharedPtr.h"
#include "gui/views/Label2DView.h"
#include "gui/views/ViewContainer.h"

/// @brief A d-pad on-screen keyboard for reading a string from the player,
///        shared by every screen that needs text entry instead of each one
///        carrying its own copy of the grid.
///
/// It edits one buffer in place, so the owner keeps whichever field is being
/// edited and its own save and cancel rules: point it at the field with
/// SetBuffer and it appends, deletes and clears there, then reports the change
/// through the callback so the owner can refresh its own labels.
///
/// A types the focused key, B deletes the last character, X types a space and Y
/// swaps the letters between capitals and lower case, all within reach of the
/// thumb, so a name is typed without leaving the key grid. The d-pad moves over
/// a compact grid of three rows of ten cells, which leaves each key room for one
/// character at a readable size on the small screen: the letters sit where a
/// keyboard puts them (ten, nine, then seven), and the last three cells of the
/// bottom row hold space, delete and the page switch (capitals, lower case,
/// digits with the usual punctuation, then the rest of the symbols). Start and
/// Select are the owner's, since what they do differs per screen (the game info
/// sheet, for one, uses them to leave the keyboard).
///
/// The view lays its keys out from its own position, so a sheet only places it
/// once, in Update. Colours come from the owner's theme through SetColors. The
/// focused key is drawn over a rounded bar in the focused background colour, so
/// the selected character is marked the way a picked item is elsewhere in the
/// launcher: a label cannot paint its own background (its empty pixels are the
/// transparent palette entry), so the bar is a sprite (see KeyboardView::Draw).
class KeyboardView : public ViewContainer
{
    SHARED_ONLY(KeyboardView)

public:
    struct Colors
    {
        Rgb<8, 8, 8> background;
        Rgb<8, 8, 8> text;
        Rgb<8, 8, 8> specialText;
        Rgb<8, 8, 8> focusedBackground;
        Rgb<8, 8, 8> focusedText;
    };

    /// @brief Called after the buffer the keyboard edits changed.
    using TextChangedCallback = void (*)(void* arg);

    static constexpr int WIDTH = 240;
    static constexpr int HEIGHT = 50;
    static constexpr int kKeyRows = 3;
    static constexpr int kKeyColumns = 10;
    static constexpr int kKeyCount = kKeyRows * kKeyColumns;
    /// Space, delete and the page switch, in the last cells of the bottom row.
    static constexpr int kSpecialCount = 3;
    static constexpr int kPageCount = 4;

    void InitVram(const VramContext& vramContext) override;
    void Update() override;
    void Draw(GraphicsContext& graphicsContext) override;
    bool HandleInput(const InputProvider& inputProvider, FocusManager& focusManager) override;
    SharedPtr<View> MoveFocus(const SharedPtr<View>& currentFocus,
        FocusMoveDirection direction, View* source) override;

    Rectangle GetBounds() const override
    {
        return Rectangle(_position.x, _position.y, WIDTH, HEIGHT);
    }

    /// @brief Points the keyboard at the field it edits.
    /// @param buffer The owner's storage, edited in place.
    /// @param capacity The longest string it may hold, terminator excluded.
    void SetBuffer(char* buffer, u32 capacity)
    {
        _buffer = buffer;
        _capacity = capacity;
    }

    void SetTextChangedCallback(TextChangedCallback callback, void* arg)
    {
        _textChanged = callback;
        _textChangedArg = arg;
    }

    void SetColors(const Colors& colors) { _colors = colors; }

    /// @brief Hidden while the owner is not editing, so the keys leave its sheet.
    void SetVisible(bool visible) { _visible = visible; }

    /// @brief Puts the focus on the first key, where the d-pad takes over.
    void Focus(FocusManager& focusManager);

private:
    const nft2_header_t* _font;
    SharedPtr<Label2DView> _keyLabels[kKeyRows][kKeyColumns];
    Colors _colors = {};
    char* _buffer = nullptr;
    u32 _capacity = 0;
    TextChangedCallback _textChanged = nullptr;
    void* _textChangedArg = nullptr;
    int _page = 0;
    bool _visible = true;
    /// @brief The highlight bar behind the focused key, 0xFFFFFFFF until uploaded.
    u32 _highlightVramOffset = 0xFFFFFFFF;

    explicit KeyboardView(const nft2_header_t* font);

    int FocusIndex(const SharedPtr<View>& currentFocus) const;
    bool IsSpecialCell(int row, int col) const;
    bool IsUsableCell(int row, int col) const;
    void ActivateKey(int index);
    void Append(char c);
    void Backspace();
    void RefreshKeys();
    void NotifyTextChanged();
};
