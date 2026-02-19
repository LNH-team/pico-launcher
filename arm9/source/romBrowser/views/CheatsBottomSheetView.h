#pragma once
#include <array>
#include "BottomSheetView.h"
#include "ChipView.h"
#include "gui/FocusManager.h"
#include "gui/views/Label2DView.h"
#include "themes/material/MaterialColorScheme.h"
#include "themes/IFontRepository.h"
#include "cheats/CheatCodelist.h"

class IRomBrowserController;
class MaterialColorScheme;
class IFontRepository;

#define CHEATS_VIEW_VISIBLE_ITEMS 5

class CheatsBottomSheetView : public BottomSheetView
{
public:
    static constexpr int DialogTypeId = 0x43484554;
    
    CheatsBottomSheetView(
        IRomBrowserController* romBrowserController,
        const MaterialColorScheme* materialColorScheme,
        const IFontRepository* fontRepository,
        const char* gameCode,
        u32 crc);

    int GetDialogTypeId() const override { return DialogTypeId; }

    void SetGraphics(const ChipView::VramToken& chipVramToken);
    void InitVram(const VramContext& vramContext) override;
    void VBlank() override;
    void Update() override;
    void Draw(GraphicsContext& graphicsContext) override;

    void Focus(FocusManager& focusManager) override;
    void SetInitialFocusState(int scrollOffset, int cursorIndex,
        int folderIndex = -1, int rootScrollOffset = 0, int rootCursorIndex = 0,
        bool enabledOnlyMode = false, int savedViewScrollOffset = 0,
        int savedViewCursorIndex = 0, int savedViewFolderIndex = -1);

    View* MoveFocus(View* currentFocus,
        FocusMoveDirection direction, View* source) override;

    bool HandleInput(const InputProvider& inputProvider, FocusManager& focusManager) override;

private:


    void UpdateLabels();
    void UpdateStatusLabel();
    void ScrollDown();
    void ScrollUp();
    void EnsureCursorVisible();
    void SetAsciiLabelText(Label2DView& label, const char* text);
    void BuildFolderNameLines(const char* folderName, char16_t* line1, int line1Max,
        char16_t* line2, int line2Max) const;
    void SaveSelectionsAndClose();
    int GetSelectedCursorVisibleIndex() const;

    static constexpr int kTitleY = 12;
    static constexpr int kItemX = 12;
    static constexpr int kItemStartY = 44;
    static constexpr int kItemSpacing = 16;
    static constexpr int kFolderTitleExtraHeight = 10;
    static constexpr int kStatusX = 12;
    static constexpr int kItemWidth = 230;
    static constexpr int kItemHeight = 16;

    IRomBrowserController* _romBrowserController;
    const MaterialColorScheme* _materialColorScheme;
    const IFontRepository* _fontRepository;

    Label2DView _titleLabel;
    Label2DView _gameCodeLabel;
    Label2DView _crcLabel;
    Label2DView _folderTitleLine1Label;
    Label2DView _folderTitleLine2Label;
    Label2DView _statusLabel;
    std::array<Label2DView, CHEATS_VIEW_VISIBLE_ITEMS> _itemLabels;


    CheatCodelist _cheatList;
    CheatParseResult _parseResult = CheatParseResult::NoCheatsFound;
    bool _hasCheats = false;
    bool _selection_dirty = false;

    char _gameCode[5] = {0};
    char _romFileName[256] = {0};

    int _currentFolderIndex = -1;
    int _scroll_offset = 0;
    int _cursor_index = 0;
    int _focusedVisibleIndex = 0;
    int _savedRootScrollOffset = 0;
    int _savedRootCursorIndex = 0;
    int _savedViewScrollOffset = 0;
    int _savedViewCursorIndex = 0;
    int _savedViewFolderIndex = -1;
    int _lastFocusedFolderIndex = -1;
    int _descriptionIndex = -1;
    u32 _crc = 0;
    int _lastFocusedCheatIndex = 0;
    u32 _frameCounter = 0;
    int _marqueeStep = 0;
    bool _marqueeActive = false;
};
