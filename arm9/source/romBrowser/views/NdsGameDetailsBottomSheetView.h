#include "../FileInfo.h"
#pragma once
#include "BottomSheetView.h"
#include "ChipView.h"
#include "gui/FocusManager.h"
#include "../FileType/Nds/NdsFileType.h"
#include "gui/views/Label2DView.h"
#include "themes/material/MaterialColorScheme.h"
#include "themes/IFontRepository.h"

class IRomBrowserController;
class IFontRepository;

class NdsGameDetailsBottomSheetView : public BottomSheetView
{
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

private:
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

    void InitLaunchCountLabel(const MaterialColorScheme* materialColorScheme);
    Label2DView _countLaunchLabel;
    Label2DView _countLaunchValueLabel;

};