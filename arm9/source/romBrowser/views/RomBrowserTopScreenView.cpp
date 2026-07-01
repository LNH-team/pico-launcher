#include "common.h"
#include <libtwl/mem/memVram.h>
#include <libtwl/gfx/gfx.h>
#include <libtwl/gfx/gfxBackground.h>
#include <libtwl/gfx/gfxPalette.h>
#include <libtwl/gfx/gfxWindow.h>
#include "../viewModels/RomBrowserViewModel.h"
#include "gui/GraphicsContext.h"
#include "gui/IVramManager.h"
#include "themes/material/MaterialColorScheme.h"
#include "../Theme/IRomBrowserViewFactory.h"
#include "RomBrowserTopScreenView.h"

RomBrowserTopScreenView::RomBrowserTopScreenView(
    SharedPtr<RomBrowserViewModel> viewModel,
    const RomBrowserDisplayMode* displayMode,
    const IThemeFileIconFactory* themeFileIconFactory,
    const IRomBrowserViewFactory* romBrowserViewFactory)
    : _viewModel(std::move(viewModel))
    , _themeFileIconFactory(themeFileIconFactory)
    , _fileInfoView(romBrowserViewFactory->CreateFileInfoView())
    , _showCover(displayMode->ShowCoverOnTopScreen())
    , _coverPosition(romBrowserViewFactory->GetTopCoverPosition())
{
    AddChildTail(_fileInfoView.GetPointer());
}

void RomBrowserTopScreenView::InitVram(const VramContext& vramContext)
{
    ViewContainer::InitVram(vramContext);
    int tileIndex = 0;
    vu16* mapPtr = (vu16*)((u8*)GFX_BG_SUB + 0x3800);
    for (int y = 0; y < 12; y++)
    {
        for (int x = 0; x < 14; x++)
        {
            *mapPtr++ = tileIndex;
            tileIndex++;
        }
        mapPtr += 2;
    }
}

void RomBrowserTopScreenView::Update()
{
    int selectedItem = _viewModel->GetSelectedItem();
    if (selectedItem != _lastSelectedItem && selectedItem >= 0)
    {
        auto& fileInfoManager = _viewModel->GetFileInfoManager();
        // GetInternalFileInfo() covers both game banners and custom icon overrides (the
        // latter apply to any file type, including folders), so check it directly instead
        // of branching on FileType::HasInternalFileInfo() - that's a static per-type property
        // and knows nothing about a per-item custom icon. IsFileInfoLoaded() distinguishes
        // "still loading" from "loaded, and there's legitimately nothing" so this waits for
        // the io thread instead of flashing the previous item's icon while undecided.
        if (fileInfoManager.IsFileInfoLoaded(selectedItem))
        {
            const auto& item = fileInfoManager.GetItem(selectedItem);
            auto info = fileInfoManager.GetInternalFileInfo(selectedItem);

            bool fileNameAsTitle = true;
            const char16_t* gameTitle = info ? info->GetGameTitle() : nullptr;
            if (gameTitle && gameTitle[0] != 0)
            {
                _fileInfoView->SetGameTitleAsync(_viewModel->GetBgTaskQueue(), gameTitle);
                fileNameAsTitle = false;
            }

            _selectedFileIcon = info ? info->CreateGameIcon() : nullptr;
            if (!_selectedFileIcon)
            {
                _selectedFileIcon = item.GetFileType()->CreateFileIcon("", _themeFileIconFactory);
            }
            if (_selectedFileIcon)
            {
                _selectedFileIcon->SetAnimFrame(_viewModel->GetIconFrameCounter());
                _iconGraphicsUploaded = false;
            }
            _fileInfoView->SetIcon(std::move(_selectedFileIcon));
            _fileInfoView->SetFileNameAsync(_viewModel->GetBgTaskQueue(), item.GetFileName(), fileNameAsTitle);

            _lastSelectedItem = selectedItem;

            auto cover = fileInfoManager.GetFileCover(selectedItem);
            if (cover.IsValid())
            {
                _selectedFileCover = std::move(cover);
                _coverGraphicsUploaded = false;
            }
        }
    }
    ViewContainer::Update();
}

void RomBrowserTopScreenView::VBlank()
{
    ViewContainer::VBlank();

    if (!_coverGraphicsUploaded && _selectedFileCover.IsValid())
    {
        if (_showCover && _selectedFileCover->IsActualCover())
        {
            _selectedFileCover->Upload2DCoverBitmap((u8*)GFX_BG_SUB + 0x4000);
            mem_setVramHMapping(MEM_VRAM_H_LCDC);
            _selectedFileCover->Upload2DCoverPalette((void*)0x0689E000);
            GFX_PLTT_BG_SUB[0] = *(vu16*)0x0689E000;
            mem_setVramHMapping(MEM_VRAM_H_SUB_BG_EXT_PLTT_SLOT_0123);
        }
        _coverGraphicsUploaded = true;
    }
    int x0 = std::clamp(_coverPosition.x, 0, 256);
    int x1 = std::clamp(_coverPosition.x + 106, 0, 256);
    int y0 = std::clamp(_coverPosition.y, 0, 192);
    int y1 = std::clamp(_coverPosition.y + 96, 0, 192);
    if (!_showCover || !_selectedFileCover.IsValid() || !_selectedFileCover->IsActualCover() ||
        x0 >= x1 || y0 >= y1)
    {
        // hide cover
        REG_DISPCNT_SUB &= ~(((1 << 3) | (1 << 5)) << 8);
    }
    else
    {
        // display cover
        REG_BG3PA_SUB = 0x100;
        REG_BG3PB_SUB = 0;
        REG_BG3PC_SUB = 0;
        REG_BG3PD_SUB = -0x100;
        REG_BG3X_SUB = (-_coverPosition.x) << 8;
        REG_BG3Y_SUB = (96 + _coverPosition.y - 1) << 8;
        REG_BG3CNT_SUB = 0x0705;
        REG_DISPCNT_SUB |= ((1 << 3) | (1 << 5)) << 8;
        gfx_setSubWindow0(x0, y0, x1, y1);
        REG_WININ_SUB = 0x002A;
        REG_WINOUT_SUB = ~(1 << 3);
    }
    if (!_iconGraphicsUploaded)
    {
        _fileInfoView->UploadIconGraphics();
        _iconGraphicsUploaded = true;
    }
}
