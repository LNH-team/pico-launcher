#include "common.h"
#include <algorithm>
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
#include "bgm/AudioStreamPlayer.h"
#include "bgm/BgmService.h"
#include "themes/ThemeInfoFactory.h"
#include "themes/ThemeFactory.h"
#include "core/StringUtil.h"
#include "gui/Gx.h"
#include "splashTop.h"
#include "App.h"
#include "fat/Directory.h"
#include "services/localization/Localization.h"

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
    , _dialogPresenter(&_focusManager, &_mainObjDialogVram) { }

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

    StoreVramState(_vramStateBeforeThemeLoad);
    LoadTheme();

    _ioTaskQueue.StartThread(1, _ioTaskThreadStack, sizeof(_ioTaskThreadStack));
    _bgTaskQueue.StartThread(2, _bgTaskThreadStack, sizeof(_bgTaskThreadStack));

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
        case RomBrowserStateTrigger::ChangeDisplayMode:
        {
            _changeDisplayMode = true;
            break;
        }
    }
}

void App::HandleShowGameInfoTrigger()
{
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
    _dialogPresenter.CloseDialog();

    auto cheatsViewModel = std::make_unique<CheatsViewModel>(_romBrowserController.GetTriggerFileInfo(), &_romBrowserController);
    auto cheatsDialog = std::make_unique<CheatsBottomSheetView>(
        std::move(cheatsViewModel), &_theme->GetMaterialColorScheme(), _theme->GetFontRepository(), &_focusManager);
    _dialogPresenter.ShowDialog(std::move(cheatsDialog));
}

void App::HandleHideCheatsTrigger()
{
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
    auto displaySettingsDialog = std::make_unique<DisplaySettingsBottomSheetView>(
        &_displaySettingsBottomSheetViewModel, &_theme->GetMaterialColorScheme(), _theme->GetFontRepository(), &_appSettingsService);
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
        _romBrowserBottomScreenView->Focus(_focusManager);
}

void App::HandleNavigateTrigger()
{
    if (!_romBrowserBottomScreenView->IsAppBarFocused(_focusManager))
        _focusManager.Unfocus();
}

void App::HandleFolderLoadDoneTrigger()
{
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
        &_bgmService);
    _romBrowserTopScreenView->InitVram(_subVramContext);
    _romBrowserBottomScreenView->RomBrowserViewModelInvalidated(_mainVramContext);
    if (!_focusManager.GetCurrentFocus())
        _romBrowserBottomScreenView->Focus(_focusManager);
}

void App::HandleRomBrowserViewModelInvalidated()
{
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
        &_bgmService);
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
    bool wasAppBarFocused = _romBrowserBottomScreenView->IsAppBarFocused(_focusManager);
    auto focusedAppBarButton = wasAppBarFocused
        ? _romBrowserBottomScreenView->GetFocusedAppBarButton(_focusManager)
        : RomBrowserAppBarView::APP_BAR_BUTTON_BACK;

    _dialogPresenter.ClearOldFocus();
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
        &_bgmService);
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

bool App::IsRomBrowserVisible() const
{
    const auto& stateMachine = _romBrowserController.GetStateMachine();
    auto curState = stateMachine.GetCurrentState();
    return curState == RomBrowserState::Browser
        || curState == RomBrowserState::GameInfo
        || curState == RomBrowserState::Cheats
        || curState == RomBrowserState::CheatDescription
        || curState == RomBrowserState::DisplaySettings
        || curState == RomBrowserState::Launching;
}

void App::Update()
{
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
    if (!_changeDisplayMode && _romBrowserController.ConsumeViewModelInvalidated())
    {
        HandleRomBrowserViewModelInvalidated();
    }

    bool isRomBrowserVisible = IsRomBrowserVisible();
    if (isRomBrowserVisible && !_exit && curState != RomBrowserState::Launching)
    {
        _focusManager.Update(_inputRepeater);

        _touchProvider.Update();
        if (_touchProvider.HasEvent())
        {
            DispatchTouch(_touchProvider.GetEvent());
        }
    }

    if (_topBackground)
        _topBackground->Update();
    if (_bottomBackground)
        _bottomBackground->Update();

    _dialogPresenter.Update();

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

void App::DispatchTouch(const TouchEvent& event)
{
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
