#include "common.h"
#include <algorithm>
#include <cstring>
#include <libtwl/mem/memVram.h>
#include <libtwl/gfx/gfx.h>
#include <libtwl/gfx/gfxOam.h>
#include <libtwl/gfx/gfxPalette.h>
#include <libtwl/gfx/gfxBackground.h>
#include <libtwl/gfx/gfxStatus.h>
#include <libtwl/gfx/gfx3d.h>
#include <libtwl/gfx/gfx3dCmd.h>
#include <libtwl/sys/sysPower.h>
#include <libtwl/ipc/ipcFifoSystem.h>
#include <nds/arm9/cache.h>
#include "animation/Animator.h"
#include "gui/materialDesign.h"
#include "gui/input/TouchEvent.h"
#include "themes/material/MaterialColorSchemeFactory.h"
#include "core/math/ColorConverter.h"
#include "core/math/RgbMixer.h"
#include "gui/GraphicsContext.h"
#include "romBrowser/views/ChipView.h"
#include "picoLoaderBootstrap.h"
#include "PicoLoaderProcess.h"
#include "romBrowser/DisplayMode/RomBrowserDisplayModeFactory.h"
#include "romBrowser/Theme/Material/MaterialThemeFileIconFactory.h"
#include "romBrowser/views/NdsGameDetailsBottomSheetView.h"
#include "romBrowser/views/cheats/CheatsBottomSheetView.h"
#include "romBrowser/views/DisplaySettingsBottomSheetView.h"
#include "romBrowser/views/InfoBottomSheetView.h"
#include "romBrowser/views/LayoutEditorBottomSheetView.h"
#include "romBrowser/views/QuickMenuBottomSheetView.h"
#include "romBrowser/FileType/FileTypeClassification.h"
#include "romBrowser/FileType/Nds/NdsFileType.h"
#include "bgm/AudioStreamPlayer.h"
#include "bgm/BgmService.h"
#include "themes/ThemeInfoFactory.h"
#include "themes/ThemeFactory.h"
#include "core/StringUtil.h"
#include "gui/Gx.h"
#include "splashTop.h"
#include "App.h"
#include "fat/Directory.h"
#include "services/Localization/Localization.h"

#define SPLASH_FRAMES       44

static bool TryGetThemeReloadLauncherPath(const char*& outLauncherPath)
{
    FILINFO fileInfo;
    if (f_stat("/_picoboot.nds", &fileInfo) == FR_OK && (fileInfo.fattrib & AM_DIR) == 0)
    {
        outLauncherPath = "/_picoboot.nds";
        return true;
    }
    else if (f_stat("/LAUNCHER.nds", &fileInfo) == FR_OK && (fileInfo.fattrib & AM_DIR) == 0)
    {
        outLauncherPath = "/LAUNCHER.nds";
        return true;
    }

    outLauncherPath = nullptr;
    return false;
}

class MaskedInputProvider final : public InputProvider
{
public:
    MaskedInputProvider(const InputProvider& source, InputKey mask)
    {
        _currentKeys = source.GetCurrentKeys() & mask;
        _triggeredKeys = source.GetTriggeredKeys() & mask;
        _releasedKeys = source.GetReleasedKeys() & mask;
    }

    void Update() override { }
};

static bool IsViewInside(const View* view, const View* root)
{
    for (auto current = view; current; current = current->GetParent())
    {
        if (current == root)
            return true;
    }

    return false;
}

App::App(IAppSettingsService& appSettingsService, IBgmService& bgmService)
    : _mainObjPltt(GFX_PLTT_OBJ_MAIN)
    , _mainObjVram(GFX_OBJ_MAIN)
    , _mainObjDialogVram(GFX_OBJ_MAIN, 128 * 1024)
    , _subObjVram(GFX_OBJ_SUB)
    , _textureVram((vu16*)0x06860000)
    , _texturePaletteVram((vu16*)0x6880000)
    , _mainVramContext(nullptr, &_mainObjVram, &_textureVram, &_texturePaletteVram)
    , _subVramContext(nullptr, &_subObjVram, nullptr, nullptr)
    , _appSettingsService(appSettingsService)
    , _bgmService(bgmService)
    , _inputProvider(&_inputSource)
    , _inputRepeater(&_inputProvider,
        InputKey::DpadLeft | InputKey::DpadRight | InputKey::DpadUp | InputKey::DpadDown,
        25, 8)
    , _romBrowserController(&appSettingsService, &_ioTaskQueue, &_bgTaskQueue)
    , _displaySettingsBottomSheetViewModel(&_romBrowserController)
    , _romBrowserBottomScreenViewModel(&_romBrowserController)
    , _dialogPresenter(&_focusManager, &_mainObjDialogVram)
    , _quickMenuPresenter(&_focusManager, &_mainObjDialogVram) { }

void App::InitVramMapping() const
{
    mem_setVramAMapping(MEM_VRAM_AB_TEX_SLOT_1);
    mem_setVramBMapping(MEM_VRAM_AB_MAIN_OBJ_00000);
    mem_setVramCMapping(MEM_VRAM_C_SUB_BG_00000);
    mem_setVramDMapping(MEM_VRAM_D_TEX_SLOT_0);
    mem_setVramEMapping(MEM_VRAM_E_TEX_PLTT_SLOT_0123);
    mem_setVramFMapping(MEM_VRAM_FG_MAIN_BG_00000);
    mem_setVramGMapping(MEM_VRAM_FG_MAIN_BG_04000);
    mem_setVramHMapping(MEM_VRAM_H_SUB_BG_EXT_PLTT_SLOT_0123);
    mem_setVramIMapping(MEM_VRAM_I_SUB_OBJ_00000);
}

void App::DisplaySplashScreen() const
{
    dma_ntrCopy32(3, splashTopTiles, GFX_BG_SUB, splashTopTilesLen);
    dma_ntrCopy32(3, splashTopMap, (u8*)GFX_BG_SUB + 0x3000, splashTopMapLen);
    mem_setVramHMapping(MEM_VRAM_H_LCDC);
    dma_ntrCopy32(3, splashTopPal, (void*)0x0689A000, splashTopPalLen);
    mem_setVramHMapping(MEM_VRAM_H_SUB_BG_EXT_PLTT_SLOT_0123);

    VBlank::Wait();

    sys_setMainEngineToBottomScreen();
    REG_DISPCNT_SUB = 0x40211015;
    REG_BG1HOFS_SUB = 0;
    REG_BG1VOFS_SUB = 0;
    REG_BG1CNT_SUB = 0x0680;
    REG_DISPCNT_SUB |= 1 << 9;
    REG_BLDCNT_SUB = 0x3D42;
    REG_BLDALPHA_SUB = 0x10;
    REG_MASTER_BRIGHT_SUB = 0;
}

void App::LoadTheme()
{
    ThemeInfoFactory themeInfoFactory;
    std::unique_ptr<ThemeInfo> themeInfo;

    if (strcmp(_appSettingsService.GetAppSettings().theme.GetString(), "RANDOM") == 0) 
    {
        _themeCount = 0;
        Directory directory;
        if (directory.Open("/_pico/themes") == FR_OK) {
            FILINFO fileInfo;
            while (true) {
                if (directory.Read(&fileInfo) != FR_OK)
                    break;
                if (fileInfo.fname[0] == 0)
                    break;
                if (fileInfo.fname[0] == '.')
                    continue;
                if ((fileInfo.fattrib & AM_DIR) == 0)
                    continue;
                if (_themeCount >= kMaxThemeCount)
                    break;
                _themeNames[_themeCount++] = String<char, 64>(fileInfo.fname);
            }
        }
        if (_themeCount > 0) {
            uint32_t randIdx = gRandomGenerator->NextU32(_themeCount);
            _effectiveThemeName = _themeNames[randIdx];
            themeInfo = themeInfoFactory.CreateFromThemeFolder(_effectiveThemeName.GetString());
        } else {
            _effectiveThemeName = "";
            themeInfo = themeInfoFactory.CreateFallbackTheme();
        }
    } 
    else
    {
        _effectiveThemeName = _appSettingsService.GetAppSettings().theme;
        themeInfo = themeInfoFactory.CreateFromThemeFolder(_effectiveThemeName.GetString());
    } 

    if (!themeInfo)
    {
        LOG_DEBUG("Failed to load theme '%s'. Using fallback theme.\n", _appSettingsService.GetAppSettings().theme.GetString());
        themeInfo = themeInfoFactory.CreateFallbackTheme();
    }

    _loadedPrimaryColorR = themeInfo->GetPrimaryColor().r;
    _loadedPrimaryColorG = themeInfo->GetPrimaryColor().g;
    _loadedPrimaryColorB = themeInfo->GetPrimaryColor().b;
    _loadedDarkTheme     = themeInfo->GetIsDarkTheme();

    _theme = ThemeFactory().CreateFromThemeInfo(themeInfo.get());
    themeInfo.reset();
    _theme->LoadRomBrowserResources(_mainVramContext, _subVramContext);
    _topBackground = _theme->CreateRomBrowserTopBackground();
    _topBackground->LoadResources(*_theme, _subVramContext);
    _bottomBackground = _theme->CreateRomBrowserBottomBackground();
    _bottomBackground->LoadResources(*_theme, _mainVramContext);

    _materialThemeFileIconFactory = std::make_unique<MaterialThemeFileIconFactory>(
        &_theme->GetMaterialColorScheme(), _theme->GetFontRepository());
}

void App::ApplyThemeColors()
{
    const auto& materialColorScheme = _theme->GetMaterialColorScheme();

    auto scrimBlendColor = Rgb<8, 8, 8>(
        materialColorScheme.inverseOnSurface.r + (materialColorScheme.scrim.r - materialColorScheme.inverseOnSurface.r) * 5 / 16,
        materialColorScheme.inverseOnSurface.g + (materialColorScheme.scrim.g - materialColorScheme.inverseOnSurface.g) * 5 / 16,
        materialColorScheme.inverseOnSurface.b + (materialColorScheme.scrim.b - materialColorScheme.inverseOnSurface.b) * 5 / 16);

    RgbMixer::MakeGradientPalette((u16*)GFX_PLTT_BG_MAIN, scrimBlendColor, materialColorScheme.GetColor(md::sys::color::surfaceContainerLow));

    GFX_PLTT_BG_MAIN[0] = ColorConverter::ToGBGR565(materialColorScheme.inverseOnSurface);
    GFX_PLTT_BG_MAIN[31] = ColorConverter::ToGBGR565(materialColorScheme.scrim);
    REG_DISPCNT = 0x211F1B;
    REG_BG0HOFS = 0;
    REG_BG0VOFS = 0;
    REG_BG0CNT = 3;
}

void App::VCountIrq()
{
    _mainObjPltt.VCount();
}

void App::Run()
{
    InitVramMapping();
    DisplaySplashScreen();
    gx_init();

    _chipViewVram = ChipView::UploadGraphics(_mainObjVram);
    _iconButtonViewVram = IconButton2DView::UploadGraphics(_mainObjVram);

    mem_setVramEMapping(MEM_VRAM_E_LCDC);
    _rgb6Palette.UploadGraphics(_mainVramContext);
    mem_setVramEMapping(MEM_VRAM_E_TEX_PLTT_SLOT_0123);

    _dialogPresenter.InitVram();

    Localization::Initialize(&_appSettingsService);

    LoadAppStateBin();

    _layoutService.Initialize(_appSettingsService.GetAppSettings().layoutSlot);

    StoreVramState(_vramStateBeforeThemeLoad);
    LoadTheme();

    _ioTaskQueue.StartThread(1, _ioTaskThreadStack, sizeof(_ioTaskThreadStack));
    _bgTaskQueue.StartThread(2, _bgTaskThreadStack, sizeof(_bgTaskThreadStack));

    SaveAppStateBinAsync();

    StoreVramState(_vramStateBeforeMakeBottomScreenView);

    _romBrowserBottomScreenView = std::make_unique<RomBrowserBottomScreenView>(
        &_romBrowserBottomScreenViewModel,
        RomBrowserDisplayModeFactory().GetRomBrowserDisplayMode(
            _romBrowserController.GetRomBrowserDisplaySettings().layout),
        _materialThemeFileIconFactory.get(),
        _theme->GetRomBrowserViewFactory(),
        &_vblankTextureLoader);
    _romBrowserBottomScreenView->InitVram(_mainVramContext);

    StoreVramState(_vramStateAfterMakeBottomScreenView);

    ApplyThemeColors();

    Gx::MtxMode(GX_MTX_MODE_PROJECTION);
    mtx43_t orthoMtx =
    {
        2048, 0, 0,
        0, -21845, 0,
        0, 0, 4096 >> 5,
        -4096, 4096, 0
    };
    Gx::MtxLoad43(&orthoMtx);

    _vcountIrqStarted = false;
    rtos_disableIrqMask(RTOS_IRQ_VCOUNT);
    rtos_setIrqFunc(RTOS_IRQ_VCOUNT, [] (u32 mask) { ((App*)gProcessManager.GetRunningProcess())->VCountIrq(); });

    LOG_DEBUG("Amount of main obj vram used: %d\n", _mainObjVram.GetState());

    _ioTaskQueue.Enqueue([this] (const vu8& cancelRequested)
    {
        _bgmService.StartBgmFromConfig(_effectiveThemeName.GetString());
        return TaskResult<void>::Completed();
    });
    _fadeAnimator = Animator(16, 0, 16, &md::sys::motion::easing::linear);

    MainLoop();

    _bgmService.StopBgm();
    rtos_disableIrqMask(RTOS_IRQ_VCOUNT);
    rtos_setIrqFunc(RTOS_IRQ_VCOUNT, nullptr);
}

void App::MainLoop()
{
    bool fadeIn = true;
    int fadeWaitFrames = SPLASH_FRAMES;
    while (true)
    {
        Update();
        Draw();
        VBlank::Wait();
        VBlank();
        if (_exit)
        {
            bool fadeComplete = _fadeAnimator.Update();
            REG_MASTER_BRIGHT = 0x4000 | _fadeAnimator.GetValue();
            REG_MASTER_BRIGHT_SUB = 0x4000 | _fadeAnimator.GetValue();
            if (fadeComplete)
            {
                break;
            }
        }
        else if (fadeIn)
        {
            if (fadeWaitFrames)
            {
                fadeWaitFrames--;
                REG_BLDALPHA_SUB = 16;
                REG_MASTER_BRIGHT = 0x4010;
            }
            else
            {
                bool fadeComplete = _fadeAnimator.Update();
                if (fadeComplete)
                {
                    fadeIn = false;
                    REG_BLDCNT_SUB = 0;
                    REG_DISPCNT_SUB &= ~(1 << 9);
                    REG_MASTER_BRIGHT = 0;
                }
                else
                {
                    int fade = _fadeAnimator.GetValue();
                    REG_BLDALPHA_SUB = ((16 - fade) << 8) | fade;
                    REG_MASTER_BRIGHT = 0x4000 | fade;
                }
            }
        }
    }
}

void App::Exit()
{
    _fadeAnimator.Goto(16, 16, &md::sys::motion::easing::linear);
    _exit = true;
}

void App::HandleTrigger(RomBrowserStateTrigger trigger, RomBrowserState newState)
{
    switch (trigger)
    {
        case RomBrowserStateTrigger::None:
        case RomBrowserStateTrigger::Launch:
        {
            break;
        }
        case RomBrowserStateTrigger::ShowGameInfo:
        {
            HandleShowGameInfoTrigger();
            break;
        }
        case RomBrowserStateTrigger::HideGameInfo:
        {
            HandleHideGameInfoTrigger();
            break;
        }
        case RomBrowserStateTrigger::ShowDisplaySettings:
        {
            HandleShowDisplaySettingsTrigger();
            break;
        }
        case RomBrowserStateTrigger::HideDisplaySettings:
        {
            HandleHideDisplaySettingsTrigger();
            break;
        }
        case RomBrowserStateTrigger::ShowDisplayInfo:
        {
            HandleShowDisplayInfoTrigger();
            break;
        }
        case RomBrowserStateTrigger::HideDisplayInfo:
        {
            HandleHideDisplayInfoTrigger();
            break;
        }
        case RomBrowserStateTrigger::ShowLayoutEditor:
        {
            HandleShowLayoutEditorTrigger();
            break;
        }
        case RomBrowserStateTrigger::HideLayoutEditor:
        {
            HandleHideLayoutEditorTrigger();
            break;
        }
        case RomBrowserStateTrigger::ShowCheats:
        {
            HandleShowCheatsTrigger();
            break;
        }
        case RomBrowserStateTrigger::HideCheats:
        {
            HandleHideCheatsTrigger();
            break;
        }
        case RomBrowserStateTrigger::ShowCheatDescription:
        {
            HandleShowCheatDescriptionTrigger();
            break;
        }
        case RomBrowserStateTrigger::HideCheatDescription:
        {
            HandleHideCheatDescriptionTrigger();
            break;
        }
        case RomBrowserStateTrigger::Navigate:
        {
            HandleNavigateTrigger();
            break;
        }
        case RomBrowserStateTrigger::FolderLoadDone:
        {
            HandleFolderLoadDoneTrigger();
            break;
        }
        case RomBrowserStateTrigger::ShowQuickMenu:
        {
            HandleShowQuickMenuTrigger();
            break;
        }
        case RomBrowserStateTrigger::HideQuickMenu:
        {
            HandleHideQuickMenuTrigger();
            break;
        }
        case RomBrowserStateTrigger::ChangeDisplayMode:
        {
            _changeDisplayMode = true;
            break;
        }
    }
}

void App::HandleShowGameInfoTrigger()
{
    if (!_quickMenuPresenter.IsIdle())
    {
        _pendingDialog = PendingDialog::GameInfo;
        return;
    }

    auto gameInfoDialog = std::make_unique<NdsGameDetailsBottomSheetView>(
        &_romBrowserController, &_theme->GetMaterialColorScheme(), _theme->GetFontRepository());
    gameInfoDialog->SetGraphics(_chipViewVram);
    _dialogPresenter.ShowDialog(std::move(gameInfoDialog));
}

void App::HandleHideGameInfoTrigger()
{
    _dialogPresenter.CloseDialog();
    if (_romBrowserController.IsFavoritesViewActive())
    {
        _dialogPresenter.ClearOldFocus();

        // After game info dialog closes in favorites view, check if items remain
        auto viewModel = _romBrowserController.GetRomBrowserViewModel();
        bool hasItems = viewModel.IsValid()
            && viewModel->GetFileInfoManager().GetItemCount() > 0;

        if (hasItems)
        {
            // Focus the list — it will select the appropriate item
            _romBrowserBottomScreenView->Focus(_focusManager);
        }
        else
        {
            // No items left in favorites, focus the favorites button
            _romBrowserBottomScreenView->FocusAppBar(
                _focusManager, RomBrowserAppBarView::APP_BAR_BUTTON_FAVORITES);
        }
    }
    else if (!_dialogPresenter.GetOldFocus())
    {
        _romBrowserBottomScreenView->Focus(_focusManager);
    }
}

void App::HandleShowCheatsTrigger()
{
    if (!_quickMenuPresenter.IsIdle())
    {
        _pendingDialog = PendingDialog::Cheats;
        return;
    }

    _dialogPresenter.CloseDialog();

    auto cheatsViewModel = std::make_unique<CheatsViewModel>(_romBrowserController.GetTriggerFileInfo(), &_romBrowserController);
    auto cheatsDialog = std::make_unique<CheatsBottomSheetView>(
        std::move(cheatsViewModel), &_theme->GetMaterialColorScheme(), _theme->GetFontRepository(), &_focusManager);
    _dialogPresenter.ShowDialog(std::move(cheatsDialog));
}

void App::HandleHideCheatsTrigger()
{
    if (_romBrowserController.ConsumeDirectMenuAccess())
    {
        _dialogPresenter.CloseDialog();
        RestoreDirectMenuAccessFocus();
        if (_romBrowserController.GetStateMachine().GetCurrentState() == RomBrowserState::GameInfo)
            _romBrowserController.HideGameInfo();
        return;
    }

    _dialogPresenter.CloseDialog();

    auto gameInfoDialog = std::make_unique<NdsGameDetailsBottomSheetView>(
        &_romBrowserController, &_theme->GetMaterialColorScheme(), _theme->GetFontRepository());
    gameInfoDialog->SetGraphics(_chipViewVram);
    _dialogPresenter.ShowDialog(std::move(gameInfoDialog));
}

void App::HandleShowCheatDescriptionTrigger()
{
}

void App::HandleHideCheatDescriptionTrigger()
{
}

void App::HandleShowDisplaySettingsTrigger()
{
    if (!_quickMenuPresenter.IsIdle())
    {
        _pendingDialog = PendingDialog::DisplaySettings;
        return;
    }

    if (_dialogPresenter.IsIdle()
        && _romBrowserBottomScreenView->IsAppBarFocused(_focusManager))
    {
        _displaySettingsReturnToAppBar = true;
        _displaySettingsReturnAppBarButton =
            _romBrowserBottomScreenView->GetFocusedAppBarButton(_focusManager);
    }
    else if (_dialogPresenter.IsIdle())
    {
        _displaySettingsReturnToAppBar = false;
    }

    auto displaySettingsDialog = std::make_unique<DisplaySettingsBottomSheetView>(
        &_displaySettingsBottomSheetViewModel, &_theme->GetMaterialColorScheme(),
        _theme->GetFontRepository(), &_appSettingsService,
        _effectiveThemeName.GetString());
    displaySettingsDialog->SetGraphics(_iconButtonViewVram);
    _dialogPresenter.ShowDialog(std::move(displaySettingsDialog));
}

void App::HandleHideDisplaySettingsTrigger()
{
    _dialogPresenter.CloseDialog();

    // Check if theme reload was explicitly requested with A button
    if (_romBrowserController.ConsumeThemeReloadRequest())
    {
        _pendingAppRestart = true;
    }

    if (!_dialogPresenter.GetOldFocus())
    {
        if (_displaySettingsReturnToAppBar)
            _romBrowserBottomScreenView->FocusAppBar(_focusManager, _displaySettingsReturnAppBarButton);
        else
            _romBrowserBottomScreenView->Focus(_focusManager);
    }

    _displaySettingsReturnToAppBar = false;
}

void App::HandleShowDisplayInfoTrigger()
{
    if (!_quickMenuPresenter.IsIdle())
    {
        _pendingDialog = PendingDialog::DisplayInfo;
        return;
    }

    _dialogPresenter.CloseDialog();

    auto displayInfoDialog = std::make_unique<SettingsInfoBottomSheetView>(
        &_romBrowserController, &_theme->GetMaterialColorScheme(), _theme->GetFontRepository());
    _dialogPresenter.ShowDialog(std::move(displayInfoDialog));
}

void App::HandleHideDisplayInfoTrigger()
{
    _dialogPresenter.CloseDialog();

    if (_romBrowserController.ConsumeDirectMenuAccess())
    {
        RestoreDirectMenuAccessFocus();
        return;
    }

    if (!_dialogPresenter.GetOldFocus())
        _romBrowserBottomScreenView->Focus(_focusManager);
}

void App::HandleShowLayoutEditorTrigger()
{
    if (!_quickMenuPresenter.IsIdle())
    {
        _pendingDialog = PendingDialog::LayoutEditor;
        return;
    }

    _dialogPresenter.CloseDialog();

    auto layoutEditorDialog = std::make_unique<LayoutEditorBottomSheetView>(
        &_romBrowserController, &_layoutService,
        &_theme->GetMaterialColorScheme(), _theme->GetFontRepository(),
        &_appSettingsService, _effectiveThemeName.GetString(),
        _loadedPrimaryColorR, _loadedPrimaryColorG, _loadedPrimaryColorB, _loadedDarkTheme);
    _dialogPresenter.ShowDialog(std::move(layoutEditorDialog));
}

void App::HandleHideLayoutEditorTrigger()
{
    _appSettingsService.GetAppSettings().layoutSlot = _layoutService.GetCurrentSlot();
    SaveAppStateBinAsync();

    _dialogPresenter.CloseDialog();

    if (_romBrowserController.ConsumeDirectMenuAccess())
    {
        RestoreDirectMenuAccessFocus();
        return;
    }

    if (!_dialogPresenter.GetOldFocus())
        _romBrowserBottomScreenView->Focus(_focusManager);
}

void App::HandleShowQuickMenuTrigger()
{
    if (_quickMenuPresenter.IsIdle()
        && _romBrowserBottomScreenView->IsAppBarFocused(_focusManager))
    {
        _directMenuAccessReturnToAppBar = true;
        _directMenuAccessReturnAppBarButton =
            _romBrowserBottomScreenView->GetFocusedAppBarButton(_focusManager);
    }
    else if (_quickMenuPresenter.IsIdle())
    {
        _directMenuAccessReturnToAppBar = false;
    }

    const FileInfo* selectedFileInfo = nullptr;
    auto viewModel = _romBrowserController.GetRomBrowserViewModel();
    if (viewModel.IsValid())
    {
        int selectedIndex = viewModel->GetSelectedItem();
        if (selectedIndex >= 0 && selectedIndex < (int)viewModel->GetFileInfoManager().GetItemCount())
            selectedFileInfo = &viewModel->GetFileInfoManager().GetItem(selectedIndex);
    }

    bool hasSelectedRom = false;
    bool isNdsRom = false;
    FileInfo selectedFileInfoCopy;
    if (selectedFileInfo)
    {
        const FileType* fileType = selectedFileInfo->GetFileType();
        hasSelectedRom = fileType && fileType->GetClassification() != FileTypeClassification::Folder;
        if (hasSelectedRom)
        {
            selectedFileInfoCopy = FileInfo(selectedFileInfo->GetFileName(),
                selectedFileInfo->GetFileType(), selectedFileInfo->GetFastFileRef(),
                selectedFileInfo->GetFullPath());
            _romBrowserController.SetActiveFile(selectedFileInfoCopy);
        }
        const char* name = selectedFileInfo->GetFullPath();
        if (!name)
            name = selectedFileInfo->GetFileName();
        auto hasExt = [] (const char* path, const char* ext)
        {
            if (!path)
                return false;
            size_t len = strlen(path);
            size_t extLen = strlen(ext);
            if (len < extLen)
                return false;
            const char* tail = path + len - extLen;
            for (size_t i = 0; i < extLen; i++)
            {
                char a = tail[i];
                char b = ext[i];
                if (a >= 'A' && a <= 'Z') a = a - 'A' + 'a';
                if (b >= 'A' && b <= 'Z') b = b - 'A' + 'a';
                if (a != b)
                    return false;
            }
            return true;
        };
        isNdsRom = fileType == &NdsFileType::sInstance
            || hasExt(name, ".nds") || hasExt(name, ".dsi") || hasExt(name, ".srl");
    }

    auto quickMenuDialog = std::make_unique<QuickMenuBottomSheetView>(
        &_romBrowserController, &_theme->GetMaterialColorScheme(),
        _theme->GetFontRepository(), hasSelectedRom, isNdsRom);
    quickMenuDialog->SetGraphics(_chipViewVram);
    _quickMenuPresenter.Show(std::move(quickMenuDialog));
}

void App::HandleHideQuickMenuTrigger()
{
    auto action = _romBrowserController.ConsumeQuickMenuAction();
    const bool opensDialog = action != IRomBrowserController::QuickMenuAction::None;
    if (opensDialog)
    {
        View* transferredFocus = _quickMenuPresenter.DetachOldFocus();
        _quickMenuPresenter.DismissImmediately();
        if (transferredFocus)
            _dialogPresenter.SetOldFocus(transferredFocus);
    }
    else
    {
        _quickMenuPresenter.Close();
    }

    if (action != IRomBrowserController::QuickMenuAction::None)
    {
        switch (action)
        {
            case IRomBrowserController::QuickMenuAction::GameDetails:
                _directMenuAccessReturnToAppBar = false;
                _romBrowserController.ShowGameInfo(_romBrowserController.GetTriggerFileInfo());
                return;
            case IRomBrowserController::QuickMenuAction::Cheats:
                _romBrowserController.ShowCheats();
                return;
            case IRomBrowserController::QuickMenuAction::DisplaySettings:
                _directMenuAccessReturnToAppBar = false;
                _romBrowserController.ShowDisplaySettings();
                return;
            case IRomBrowserController::QuickMenuAction::LayoutEditor:
                _romBrowserController.ShowLayoutEditor();
                return;
            case IRomBrowserController::QuickMenuAction::Information:
                _romBrowserController.ShowDisplayInfo();
                return;
            default:
                _directMenuAccessReturnToAppBar = false;
                break;
        }
    }
    else
    {
        _directMenuAccessReturnToAppBar = false;
    }

    if (!_quickMenuPresenter.GetOldFocus())
        _romBrowserBottomScreenView->Focus(_focusManager);
}

void App::ShowPendingDialog()
{
    PendingDialog pending = _pendingDialog;
    _pendingDialog = PendingDialog::None;

    switch (pending)
    {
        case PendingDialog::GameInfo:
            HandleShowGameInfoTrigger();
            break;
        case PendingDialog::Cheats:
            HandleShowCheatsTrigger();
            break;
        case PendingDialog::DisplaySettings:
            HandleShowDisplaySettingsTrigger();
            break;
        case PendingDialog::LayoutEditor:
            HandleShowLayoutEditorTrigger();
            break;
        case PendingDialog::DisplayInfo:
            HandleShowDisplayInfoTrigger();
            break;
        case PendingDialog::None:
        default:
            break;
    }
}

void App::HandleNavigateTrigger()
{
    if (!_romBrowserBottomScreenView->IsAppBarFocused(_focusManager))
        _focusManager.Unfocus();
}

void App::HandleFolderLoadDoneTrigger()
{
    DrainTaskQueues();

    ClearRetainedRomBrowserFocus(false);

    if (_romBrowserBottomScreenView
        && _focusManager.IsFocusInside(_romBrowserBottomScreenView.get()))
    {
        _focusManager.Unfocus();
    }

    _romBrowserTopScreenView.reset();
    RestoreVramState(_vramStateAfterMakeBottomScreenView);
    auto displayMode = RomBrowserDisplayModeFactory().GetRomBrowserDisplayMode(
        _romBrowserController.GetRomBrowserDisplaySettings().layout);
    _romBrowserTopScreenView = std::make_unique<RomBrowserTopScreenView>(
        _romBrowserController.GetRomBrowserViewModel(),
        displayMode,
        _materialThemeFileIconFactory.get(),
        _theme->GetRomBrowserViewFactory(),
        &_theme->GetMaterialColorScheme(),
        _theme->GetFontRepository(),
        &_bgmService,
        &_layoutService);
    _romBrowserTopScreenView->InitVram(_subVramContext);
    _romBrowserBottomScreenView->RomBrowserViewModelInvalidated(_mainVramContext);
    if (!_focusManager.GetCurrentFocus())
        _romBrowserBottomScreenView->Focus(_focusManager);
}

void App::HandleRomBrowserViewModelInvalidated()
{
    DrainTaskQueues();

    ClearRetainedRomBrowserFocus(false);

    if (_romBrowserBottomScreenView
        && _focusManager.IsFocusInside(_romBrowserBottomScreenView.get()))
    {
        _focusManager.Unfocus();
    }

    bool wasFavoritesAppBarFocused = _romBrowserBottomScreenView->IsAppBarFocused(_focusManager)
        && _romBrowserBottomScreenView->GetFocusedAppBarButton(_focusManager)
            == RomBrowserAppBarView::APP_BAR_BUTTON_FAVORITES;

    _romBrowserTopScreenView.reset();
    RestoreVramState(_vramStateAfterMakeBottomScreenView);
    auto displayMode = RomBrowserDisplayModeFactory().GetRomBrowserDisplayMode(
        _romBrowserController.GetRomBrowserDisplaySettings().layout);
    _romBrowserTopScreenView = std::make_unique<RomBrowserTopScreenView>(
        _romBrowserController.GetRomBrowserViewModel(),
        displayMode,
        _materialThemeFileIconFactory.get(),
        _theme->GetRomBrowserViewFactory(),
        &_theme->GetMaterialColorScheme(),
        _theme->GetFontRepository(),
        &_bgmService,
        &_layoutService);
    _romBrowserTopScreenView->InitVram(_subVramContext);
    _romBrowserBottomScreenView->RomBrowserViewModelInvalidated(_mainVramContext);

    if (_romBrowserController.IsFavoritesViewActive())
    {
        if (wasFavoritesAppBarFocused)
        {
            _romBrowserBottomScreenView->FocusAppBar(
                _focusManager, RomBrowserAppBarView::APP_BAR_BUTTON_FAVORITES);
        }
        else
        {
            auto viewModel = _romBrowserController.GetRomBrowserViewModel();
            bool hasItems = viewModel.IsValid()
                && viewModel->GetFileInfoManager().GetItemCount() > 0;

            if (hasItems)
                _romBrowserBottomScreenView->Focus(_focusManager);
            else
                _romBrowserBottomScreenView->FocusAppBar(
                    _focusManager, RomBrowserAppBarView::APP_BAR_BUTTON_FAVORITES);
        }
    }
    else if (wasFavoritesAppBarFocused)
    {
        _romBrowserBottomScreenView->FocusAppBar(
            _focusManager, RomBrowserAppBarView::APP_BAR_BUTTON_FAVORITES);
    }
    else if (!_focusManager.GetCurrentFocus())
    {
        _romBrowserBottomScreenView->Focus(_focusManager);
    }
}

void App::HandleChangeDisplayModeTrigger(RomBrowserState newState)
{
    DrainTaskQueues();

    ClearRetainedRomBrowserFocus(true);

    if (_romBrowserBottomScreenView
        && _focusManager.IsFocusInside(_romBrowserBottomScreenView.get()))
    {
        _focusManager.Unfocus();
    }

    _touchCaptureTarget = nullptr;

    bool wasAppBarFocused = _romBrowserBottomScreenView->IsAppBarFocused(_focusManager);
    auto focusedAppBarButton = wasAppBarFocused
        ? _romBrowserBottomScreenView->GetFocusedAppBarButton(_focusManager)
        : RomBrowserAppBarView::APP_BAR_BUTTON_BACK;

    RestoreVramState(_vramStateBeforeMakeBottomScreenView);
    auto displayMode = RomBrowserDisplayModeFactory().GetRomBrowserDisplayMode(
        _romBrowserController.GetRomBrowserDisplaySettings().layout);
    _romBrowserBottomScreenView = std::make_unique<RomBrowserBottomScreenView>(
        &_romBrowserBottomScreenViewModel,
        displayMode,
        _materialThemeFileIconFactory.get(),
        _theme->GetRomBrowserViewFactory(),
        &_vblankTextureLoader);
    _romBrowserBottomScreenView->InitVram(_mainVramContext);
    StoreVramState(_vramStateAfterMakeBottomScreenView);
    _romBrowserTopScreenView = std::make_unique<RomBrowserTopScreenView>(
        _romBrowserController.GetRomBrowserViewModel(),
        displayMode,
        _materialThemeFileIconFactory.get(),
        _theme->GetRomBrowserViewFactory(),
        &_theme->GetMaterialColorScheme(),
        _theme->GetFontRepository(),
        &_bgmService,
        &_layoutService);
    _romBrowserTopScreenView->InitVram(_subVramContext);
    _romBrowserBottomScreenView->RomBrowserViewModelInvalidated(_mainVramContext);
    if (newState == RomBrowserState::Browser)
    {
        if (wasAppBarFocused)
            _romBrowserBottomScreenView->FocusAppBar(_focusManager, focusedAppBarButton);
        else
            _romBrowserBottomScreenView->Focus(_focusManager);
    }
}

void App::ClearRetainedRomBrowserFocus(bool includeAppBar)
{
    if (!_romBrowserBottomScreenView)
        return;

    View* dialogOldFocus = _dialogPresenter.GetOldFocus();
    if (dialogOldFocus
        && (includeAppBar
            ? IsViewInside(dialogOldFocus, _romBrowserBottomScreenView.get())
            : _romBrowserBottomScreenView->IsViewInsideRomBrowser(dialogOldFocus)))
        _dialogPresenter.ClearOldFocus();

    View* quickMenuOldFocus = _quickMenuPresenter.GetOldFocus();
    if (quickMenuOldFocus
        && (includeAppBar
            ? IsViewInside(quickMenuOldFocus, _romBrowserBottomScreenView.get())
            : _romBrowserBottomScreenView->IsViewInsideRomBrowser(quickMenuOldFocus)))
        _quickMenuPresenter.ClearOldFocus();
}

void App::RestoreDirectMenuAccessFocus()
{
    if (_directMenuAccessReturnToAppBar)
    {
        if (!_dialogPresenter.GetOldFocus())
        {
            _romBrowserBottomScreenView->FocusAppBar(
                _focusManager, _directMenuAccessReturnAppBarButton);
        }
    }
    else
    {
        _dialogPresenter.ClearOldFocus();
        _romBrowserBottomScreenView->Focus(_focusManager);
    }

    _directMenuAccessReturnToAppBar = false;
}

bool App::IsRomBrowserVisible() const
{
    const auto& stateMachine = _romBrowserController.GetStateMachine();
    auto curState = stateMachine.GetCurrentState();
    return curState == RomBrowserState::Browser
        || curState == RomBrowserState::GameInfo
        || curState == RomBrowserState::Cheats
        || curState == RomBrowserState::CheatDescription
        || curState == RomBrowserState::DisplaySettings
        || curState == RomBrowserState::DisplayInfo
        || curState == RomBrowserState::LayoutEditor
        || curState == RomBrowserState::QuickMenu
        || curState == RomBrowserState::Launching;
}

void App::Update()
{
    bool sleepModeActive = (SHARED_SYSTEM_FLAGS & SHARED_FLAG_SLEEP_MODE) != 0;
    if (sleepModeActive != _sleepModeWasActive)
    {
        _inputProvider.Reset();
        _inputRepeater.Reset();
        _touchProvider.Reset();
        _touchCaptureTarget = nullptr;
        _touchCapturedByDialog = false;
        _sleepModeWasActive = sleepModeActive;
    }

    const auto& stateMachine = _romBrowserController.GetStateMachine();
    _romBrowserController.Update();
    auto curState = stateMachine.GetCurrentState();
    if (_changeDisplayMode)
    {
        HandleChangeDisplayModeTrigger(curState);
        _changeDisplayMode = false;
    }
    if (stateMachine.HasStateChanged())
    {
        HandleTrigger(stateMachine.GetLastTrigger(), curState);
    }
    if (_romBrowserController.ConsumeStateDirty())
        SaveAppStateBinAsync();

    if (!_changeDisplayMode && _romBrowserController.ConsumeViewModelInvalidated())
    {
        HandleRomBrowserViewModelInvalidated();
    }

    bool isRomBrowserVisible = IsRomBrowserVisible();
    if (isRomBrowserVisible && !_exit && curState != RomBrowserState::Launching)
    {
        const bool quickMenuActive = !_quickMenuPresenter.IsIdle();
        const bool blockNonBInput = _dialogPresenter.IsTransitioning()
            || _quickMenuPresenter.IsTransitioning();
        auto* currentDialog = _dialogPresenter.GetCurrentDialog();
        const MaskedInputProvider bOnlyInput(_inputRepeater, InputKey::B);
        const InputProvider& activeInput = blockNonBInput
            ? static_cast<const InputProvider&>(bOnlyInput)
            : static_cast<const InputProvider&>(_inputRepeater);

        if (quickMenuActive && !_focusManager.GetCurrentFocus())
            _quickMenuPresenter.HandleInput(activeInput, _focusManager);
        else if (currentDialog && !_focusManager.GetCurrentFocus())
            currentDialog->HandleInput(activeInput, _focusManager);
        else if (!blockNonBInput)
            _focusManager.Update(_inputRepeater);

        if (!blockNonBInput)
        {
            _touchProvider.Update();
            if (_touchProvider.HasEvent())
            {
                DispatchTouch(_touchProvider.GetEvent());
            }
        }
    }

    if (_topBackground)
        _topBackground->Update();
    if (_bottomBackground)
        _bottomBackground->Update();

    _dialogPresenter.Update();
    _quickMenuPresenter.Update();

    if (_pendingDialog != PendingDialog::None && _quickMenuPresenter.IsIdle())
    {
        ShowPendingDialog();
    }

    if (_pendingAppRestart && _dialogPresenter.IsIdle())
    {
        _pendingAppRestart = false;

        const char* launcherPath = nullptr;
        if (!TryGetThemeReloadLauncherPath(launcherPath))
        {
            return;
        }

        auto loadParams = pload_getLoadParams();
        StringUtil::Copy(loadParams->romPath, launcherPath, sizeof(loadParams->romPath));
        loadParams->savePath[0] = 0;
        loadParams->arguments[0] = 0;
        loadParams->argumentsLength = 0;
        pload_setCheatData(nullptr);
        gProcessManager.Goto<PicoLoaderProcess>();
        return;
    }

    _romBrowserBottomScreenView->Update();
    if (isRomBrowserVisible)
    {
        _romBrowserTopScreenView->Update();
        _romBrowserController.GetRomBrowserViewModel()->SetIconFrameCounter(
            _romBrowserController.GetRomBrowserViewModel()->GetIconFrameCounter() + 1);
    }
}

void App::Draw()
{
    gx_reset();
    Gx::Viewport(0, 0, 255, 191);
    Gx::MtxMode(GX_MTX_MODE_POSITION_VECTOR);
    Gx::MtxIdentity();

    GraphicsContext mainGraphicsContext
    {
        &_mainOam,
        &_mainObjPltt,
        &_rgb6Palette
    };
    GraphicsContext subGraphicsContext
    {
        &_subOam,
        &_subObjPltt,
        nullptr
    };

    _mainOam.Clear();
    _subOam.Clear();
    _mainObjPltt.Reset();
    _subObjPltt.Reset();
    mainGraphicsContext.SetPriority(3);
    subGraphicsContext.SetPriority(2);

    if (_topBackground)
        _topBackground->Draw(subGraphicsContext);
    if (_bottomBackground)
        _bottomBackground->Draw(mainGraphicsContext);

    if (!_changeDisplayMode && IsRomBrowserVisible())
    {
        _romBrowserTopScreenView->Draw(subGraphicsContext);
    }

    _dialogPresenter.ApplyClipArea(mainGraphicsContext);
    if (!_changeDisplayMode)
    {
        _romBrowserBottomScreenView->Draw(mainGraphicsContext);
    }
    mainGraphicsContext.ResetClipArea();

    _dialogPresenter.Draw(mainGraphicsContext);
    _quickMenuPresenter.Draw(mainGraphicsContext);

    _mainObjPltt.EndOfFrame();

    Gx::SwapBuffers(GX_XLU_SORT_MANUAL, GX_DEPTH_MODE_Z);
}

void App::VBlank()
{
    dma_ntrStopDirect(0); // stop hblank dma
    _inputProvider.Sample();
    _touchProvider.Sample();
    _inputRepeater.Update();
    _mainOam.Apply(GFX_OAM_MAIN);
    _subOam.Apply(GFX_OAM_SUB);
    _subObjPltt.Apply(GFX_PLTT_OBJ_SUB);

    if (!_vcountIrqStarted)
    {
        rtos_ackIrqMask(RTOS_IRQ_VCOUNT);
        rtos_enableIrqMask(RTOS_IRQ_VCOUNT);
        _vcountIrqStarted = true;
    }
    _mainObjPltt.VBlank();

    if (_topBackground)
        _topBackground->VBlank();
    if (_bottomBackground)
        _bottomBackground->VBlank();

    _dialogPresenter.VBlank();
    _quickMenuPresenter.VBlank();

    if (IsRomBrowserVisible())
    {
        _romBrowserTopScreenView->VBlank();
    }
    _romBrowserBottomScreenView->VBlank();

    _vblankTextureLoader.VBlank();
}

void App::StoreVramState(VramState& vramState) const
{
    vramState._mainObjVramState = _mainObjVram.GetState();
    vramState._texVramState = _textureVram.GetState();
    vramState._texPlttVramState = _texturePaletteVram.GetState();
    vramState._subObjVramState = _subObjVram.GetState();
}

void App::RestoreVramState(const VramState& vramState)
{
    _mainObjVram.SetState(vramState._mainObjVramState);
    _textureVram.SetState(vramState._texVramState);
    _texturePaletteVram.SetState(vramState._texPlttVramState);
    _subObjVram.SetState(vramState._subObjVramState);
}

void App::DrainTaskQueues()
{
    _ioTaskQueue.StopThread();
    _bgTaskQueue.StopThread();
    _ioTaskQueue.StartThread(1, _ioTaskThreadStack, sizeof(_ioTaskThreadStack));
    _bgTaskQueue.StartThread(2, _bgTaskThreadStack, sizeof(_bgTaskThreadStack));
}

void App::LoadAppStateBin()
{
    AppStateBin state;
    if (_stateBinSerializer.Deserialize(&state, kStateBinPath))
    {
        auto& appSettings = _appSettingsService.GetAppSettings();
        appSettings.layoutSlot = state.layoutSlot;
        appSettings.favorites  = std::move(state.favorites);
        appSettings.numberOfFavorites = state.numberOfFavorites;
    }
}

void App::SaveAppStateBinAsync()
{
    const auto& appSettings = _appSettingsService.GetAppSettings();
    AppStateBin state;
    state.appliedThemeName  = _effectiveThemeName;
    state.primaryColorR     = _loadedPrimaryColorR;
    state.primaryColorG     = _loadedPrimaryColorG;
    state.primaryColorB     = _loadedPrimaryColorB;
    state.darkTheme         = _loadedDarkTheme ? 1u : 0u;
    state.layoutSlot        = appSettings.layoutSlot;

    if (appSettings.numberOfFavorites > 0)
    {
        state.favorites = std::make_unique_for_overwrite<String<char, 256>[]>(appSettings.numberOfFavorites);
        for (u32 i = 0; i < appSettings.numberOfFavorites; i++)
            state.favorites[i] = appSettings.favorites[i];
        state.numberOfFavorites = appSettings.numberOfFavorites;
    }

    u32 length = 0;
    auto buf = _stateBinSerializer.SerializeToBuffer(&state, length);
    auto shared = std::shared_ptr<u8[]>(std::move(buf));
    _ioTaskQueue.Enqueue([this, shared, len = length] (const vu8& cancelRequested)
    {
        _stateBinSerializer.WriteBufferToFile(shared.get(), len, kStateBinPath);
        return TaskResult<void>::Completed();
    });
}

void App::DispatchTouch(const TouchEvent& event)
{
    if (!_quickMenuPresenter.IsIdle())
    {
        _quickMenuPresenter.HandleTouch(event, _focusManager);
        return;
    }

    if (event.type == TouchEventType::Down)
    {
        _touchCapturedByDialog = false;
        _touchCaptureTarget = nullptr;

        if (_dialogPresenter.GetCurrentDialog())
        {
            if (_dialogPresenter.HandleTouch(event, _focusManager))
            {
                _touchCapturedByDialog = true;
                return;
            }
        }

        if (_romBrowserBottomScreenView &&
            _romBrowserBottomScreenView->HandleTouch(event, _focusManager))
        {
            _touchCaptureTarget = _romBrowserBottomScreenView.get();
            return;
        }
    }
    else
    {
        if (_touchCapturedByDialog)
        {
            _dialogPresenter.HandleTouch(event, _focusManager);
            if (event.type == TouchEventType::Up)
                _touchCapturedByDialog = false;
        }
        else if (_touchCaptureTarget)
        {
            _touchCaptureTarget->HandleTouch(event, _focusManager);
            if (event.type == TouchEventType::Up)
                _touchCaptureTarget = nullptr;
        }
    }
}
