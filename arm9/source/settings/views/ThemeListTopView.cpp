#include "common.h"
#include <libtwl/dma/dmaNitro.h>
#include <libtwl/gfx/gfx.h>
#include <libtwl/gfx/gfxBackground.h>
#include "settings/SettingsController.h"
#include "themes/IFontRepository.h"
#include "themes/material/MaterialColorScheme.h"
#include "ThemeListTopView.h"
#include "translate.h"

ThemeListTopView::ThemeListTopView(SharedPtr<ThemeListViewModel> viewModel, const MaterialColorScheme* materialColorScheme,
    const IFontRepository* fontRepository)
    : _viewModel(viewModel)
    , _noPreviewLabel(Label2DView::CreateShared(64, 16, 25, fontRepository->GetFont(FontType::Medium11)))
{
    _noPreviewLabel->SetHorizontalAlignment(Alignment::Center);
    _noPreviewLabel->SetText(_(u"No preview"));
    _noPreviewLabel->SetPosition(128 - 32, 96 - 8);
    _noPreviewLabel->SetBackgroundColor(materialColorScheme->inverseOnSurface);
    _noPreviewLabel->SetForegroundColor(materialColorScheme->onSurfaceVariant);
    AddChildTail(_noPreviewLabel.GetPointer());
}

void ThemeListTopView::VBlank()
{
    ViewContainer::VBlank();
    REG_DISPCNT_SUB = (REG_DISPCNT_SUB & ~0xF) | 5 | (4 << 8);
    REG_BG2CNT_SUB = 0x4084;
    REG_BG2HOFS_SUB = 0;
    REG_BG2VOFS_SUB = 0;
    gfx_setSubBg2Affine(256, 0, 0, 256, 0, 0);

    int selectedItem = _viewModel->GetSelectedItem();
    if (selectedItem != _lastSelectedItem)
    {
        auto extraThemeInfo = _viewModel->GetSettingsController()->GetThemeInfoManager().GetExtraThemeInfo(selectedItem);
        if (extraThemeInfo)
        {
            dma_ntrCopy32(3, extraThemeInfo->previewImage, GFX_BG_SUB, 256 * 192 * 2);
            _lastSelectedItem = selectedItem;
        }
    }
}
