#include "common.h"
#include <string.h>
#include <libtwl/mem/memVram.h>
#include <libtwl/gfx/gfx.h>
#include <libtwl/gfx/gfxBackground.h>
#include <libtwl/gfx/gfxPalette.h>
#include <libtwl/gfx/gfxWindow.h>
#include "bgm/IBgmService.h"
#include "../viewModels/RomBrowserViewModel.h"
#include "gui/GraphicsContext.h"
#include "gui/IVramManager.h"
#include "../Theme/IRomBrowserViewFactory.h"
#include "RomBrowserTopScreenView.h"

RomBrowserTopScreenView::RomBrowserTopScreenView(
    const SharedPtr<RomBrowserViewModel>& viewModel,
    const RomBrowserDisplayMode* displayMode,
    const IThemeFileIconFactory* themeFileIconFactory,
    const IRomBrowserViewFactory* romBrowserViewFactory,
    const MaterialColorScheme* materialColorScheme,
    const IFontRepository* fontRepository,
    const IBgmService* bgmService)
    : _viewModel(viewModel)
    , _themeFileIconFactory(themeFileIconFactory)
    , _fileInfoView(romBrowserViewFactory->CreateFileInfoView())
    , _bgmNowPlayingView(std::make_unique<BgmNowPlayingView>(materialColorScheme, fontRepository))
    , _showCover(displayMode->ShowCoverOnTopScreen())
    , _bgmService(bgmService)
{
    AddChildTail(_fileInfoView.get());
    AddChildTail(_bgmNowPlayingView.get());
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
    if (_bgmService && _bgmNowPlayingView)
    {
        const char* name = _bgmService->GetCurrentBgmName();
        if (!name)
            name = "";
        if (strcmp(_lastBgmName.GetString(), name) != 0)
        {
            _bgmNowPlayingView->SetBgmName(name);
            _lastBgmName = name;
        }
    }

    int selectedItem = _viewModel->GetSelectedItem();
    if (selectedItem != _lastSelectedItem)
    {
        auto& fileInfoManager = _viewModel->GetFileInfoManager();
        const auto& item = fileInfoManager.GetItem(selectedItem);
        if (item.GetFileType()->HasInternalFileInfo())
        {
            auto info = fileInfoManager.GetInternalFileInfo(selectedItem);
            if (info)
            {
                bool fileNameAsTitle = true;
                const char16_t* gameTitle = info->GetGameTitle();
                if (gameTitle)
                {
                    _fileInfoView->SetGameTitleAsync(_viewModel->GetBgTaskQueue(), gameTitle);
                    fileNameAsTitle = false;
                }

                _selectedFileIcon = info->CreateGameIcon();
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
        else
        {
            auto cover = fileInfoManager.GetFileCover(selectedItem);
            if (cover.IsValid())
            {
                _selectedFileCover = std::move(cover);
                _coverGraphicsUploaded = false;

                _selectedFileIcon = item.GetFileType()->CreateFileIcon("", _themeFileIconFactory);
                if (_selectedFileIcon)
                {
                    _selectedFileIcon->SetAnimFrame(_viewModel->GetIconFrameCounter());
                    _iconGraphicsUploaded = false;
                }
                _fileInfoView->SetIcon(std::move(_selectedFileIcon));
                _fileInfoView->SetFileNameAsync(_viewModel->GetBgTaskQueue(), item.GetFileName(), true);

                _lastSelectedItem = selectedItem;
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
    if (!_showCover || !_selectedFileCover.IsValid() || !_selectedFileCover->IsActualCover())
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
        REG_BG3X_SUB = -75 << 8;
        REG_BG3Y_SUB = 113 << 8;
        REG_BG3CNT_SUB = 0x0705;
        REG_DISPCNT_SUB |= ((1 << 3) | (1 << 5)) << 8;
        gfx_setSubWindow0(75, 18, 75 + 106, 18 + 96);
        REG_WININ_SUB = 0x002A;
        REG_WINOUT_SUB = ~(1 << 3);
    }
    if (!_iconGraphicsUploaded)
    {
        _fileInfoView->UploadIconGraphics();
        _iconGraphicsUploaded = true;
    }
}
