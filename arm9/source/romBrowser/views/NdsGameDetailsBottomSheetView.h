#pragma once
#include "../FileInfo.h"
#include "BottomSheetView.h"
#include "ChipView.h"
#include "gui/FocusManager.h"
#include "gui/views/Label2DView.h"
#include "themes/material/MaterialColorScheme.h"
#include "themes/IFontRepository.h"

class IRomBrowserController;
class IFontRepository;
struct TouchEvent;

class NdsGameDetailsBottomSheetView : public BottomSheetView {
public:
    static constexpr int DialogTypeId = 0x4E444744;
    int GetDialogTypeId() const override { return DialogTypeId; }

public:
    NdsGameDetailsBottomSheetView(
        IRomBrowserController* romBrowserController,
        const MaterialColorScheme* materialColorScheme,
        const IFontRepository* fontRepository);

    void SetGraphics(const ChipView::VramToken& chipVramToken)
    {
        _cheatsChip.SetGraphics(chipVramToken);
        _favoriteChip.SetGraphics(chipVramToken);
    }

    void InitVram(const VramContext& vramContext) override;

    void Update() override;
    void Draw(GraphicsContext& graphicsContext) override;

    void Focus(FocusManager& focusManager) override
    {
        if (_hasCheatsChip) {
            focusManager.Focus(&_cheatsChip);
        } else {
            focusManager.Focus(&_favoriteChip);
        }
    }

    View* MoveFocus(View* currentFocus, FocusMoveDirection direction, View* source) override;

    bool HandleInput(const InputProvider& inputProvider, FocusManager& focusManager) override;

    void OnDismissed() override;
    bool HandleTouch(const TouchEvent& event, FocusManager& focusManager) override;

private:
    Label2DView _titleLabel;
    Label2DView _romIdentityLabel;
    bool _hasRomIdentity = false;
    bool _hasCheatsChip = false;
    IRomBrowserController* _romBrowserController;
    u32 _smallHeartIconVramOffset;
    u32 _smallHeartIconFilledVramOffset;
    ChipView _cheatsChip;
    ChipView _favoriteChip;
    bool _isFavorite = false;

    void UpdateFavoriteChipIcon()
    {
        _favoriteChip.SetSelected(_isFavorite);
        u32 iconOffset = _isFavorite ? _smallHeartIconFilledVramOffset : _smallHeartIconVramOffset;
        _favoriteChip.SetIcon(true, iconOffset);
    }

    void InitLaunchCountLabel();
    Label2DView _countLaunchLabel;
    Label2DView _countLaunchValueLabel;
    Label2DView _lastLaunchLabel;
    Label2DView _lastLaunchDateValueLabel;
    Label2DView _lastLaunchTimeValueLabel;
};
