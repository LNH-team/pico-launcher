#pragma once
#include "core/SharedPtr.h"
#include "core/math/Rgb.h"
#include "romBrowser/views/BottomSheetView.h"
#include "gui/views/Label2DView.h"
#include "romBrowser/views/IconButton2DView.h"
#include "romBrowser/viewModels/MenuViewModel.h"

class MaterialColorScheme;
class IFontRepository;
class IVramManager;
class MenuBottomSheetView;

/// @brief One entry of the menu: an icon, its name and, for a filter, whether
///        it is on. A tap or A runs it; the sheet decides what that means.
class MenuItemView : public ViewContainer
{
    SHARED_ONLY(MenuItemView)

public:
    static constexpr int HEIGHT = 24;

    void Update() override;
    void Draw(GraphicsContext& graphicsContext) override;
    bool HandleInput(const InputProvider& inputProvider, FocusManager& focusManager) override;
    void HandlePenDown(const Point& touchPoint, FocusManager& focusManager) override;
    void HandlePenMove(const Point& touchPoint, FocusManager& focusManager) override;
    void HandlePenUp(const Point& lastTouchPoint, FocusManager& focusManager) override;

    Rectangle GetBounds() const override
    {
        return Rectangle(_position.x, _position.y, _width, HEIGHT);
    }

    void SetIconVramOffset(u32 vramOffset) { _iconVramOffset = vramOffset; }
    void SetSelectorVramOffset(u32 vramOffset) { _selectorVramOffset = vramOffset; }

    /// @brief A filter that is on draws in its colour and says so at the end.
    void SetActive(bool active)
    {
        if (_stateLabel && active != _active)
            _stateLabel->SetText(active ? "on" : "off");
        _active = active;
    }

    /// @brief Off is faded and inert, like the delete button was on a folder.
    void SetEnabled(bool enabled) { _enabled = enabled; }

private:
    MenuBottomSheetView* _sheet;
    int _index;
    int _width;
    Rgb<8, 8, 8> _activeColor;
    const MaterialColorScheme* _materialColorScheme;
    SharedPtr<Label2DView> _nameLabel;
    /// "on" or "off"; only the filters have one.
    SharedPtr<Label2DView> _stateLabel;
    u32 _iconVramOffset = 0;
    u32 _selectorVramOffset = 0;
    bool _active = false;
    bool _enabled = true;
    bool _penDown = false;

    /// @param width A cell of the two-column part or a full row (see the .cpp).
    /// @param hasState Whether this is a filter, with "on" or "off" at the end.
    MenuItemView(MenuBottomSheetView* sheet, int index, int width, const char* name,
        bool hasState, const Rgb<8, 8, 8>& activeColor,
        const MaterialColorScheme* materialColorScheme, const IFontRepository* fontRepository);
};

/// @brief The sheet behind the app bar's "more" button: what the bar used to
///        hold as icons, and behind long presses, written out.
///
/// Eight entries in two columns: recently played and favorites over statistics
/// and delete, game info beside the in-game exit combo, then the two filters,
/// their colour saying whether they are on. Picking a panel replaces this
/// sheet with it; a filter closes the sheet through the display mode change it
/// triggers; delete and game info are faded while a folder is highlighted or
/// parental controls take them away. The about sheet keeps its small button on
/// the title row.
class MenuBottomSheetView : public BottomSheetView
{
    SHARED_ONLY(MenuBottomSheetView)

public:
    void InitVram(const VramContext& vramContext) override;
    void Update() override;
    void Draw(GraphicsContext& graphicsContext) override;
    bool HandleInput(const InputProvider& inputProvider, FocusManager& focusManager) override;
    SharedPtr<View> MoveFocus(const SharedPtr<View>& currentFocus,
        FocusMoveDirection direction, View* source) override;
    void Focus(FocusManager& focusManager) override;

    /// @brief Runs the entry at index; the items call this on A or a tap.
    void Activate(int index);

    void SetGraphics(const IconButton2DView::VramToken& iconButtonVramToken);

protected:
    void Close() override;

private:
    enum Item
    {
        ITEM_RECENTS = 0,
        ITEM_FAVORITES,
        ITEM_STATISTICS,
        ITEM_DELETE,
        ITEM_GAME_INFO,
        ITEM_EXIT_COMBO,
        ITEM_FAVORITES_FILTER,
        ITEM_COMPLETED_FILTER,
        ITEM_COUNT
    };

    SharedPtr<MenuViewModel> _viewModel;
    const MaterialColorScheme* _materialColorScheme;
    SharedPtr<Label2DView> _titleLabel;
    SharedPtr<IconButton2DView> _aboutButton;
    SharedPtr<MenuItemView> _items[ITEM_COUNT];

    MenuBottomSheetView(SharedPtr<MenuViewModel> viewModel,
        const MaterialColorScheme* materialColorScheme, const IFontRepository* fontRepository);

    u32 LoadSprite(IVramManager& vramManager, const unsigned int* tiles, u32 tilesLength) const;
};
