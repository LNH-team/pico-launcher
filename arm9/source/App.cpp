#include "common.h"
#include <algorithm>
#include <ctype.h>
#include <libtwl/mem/memVram.h>
#include <libtwl/gfx/gfx.h>
#include <libtwl/gfx/gfxOam.h>
#include <libtwl/gfx/gfxPalette.h>
#include <libtwl/gfx/gfxBackground.h>
#include <libtwl/gfx/gfxStatus.h>
#include <libtwl/gfx/gfx3d.h>
#include <libtwl/gfx/gfx3dCmd.h>
#include <libtwl/sys/sysPower.h>
#include "animation/Animator.h"
#include "gui/materialDesign.h"
#include "themes/material/MaterialColorSchemeFactory.h"
#include "core/math/ColorConverter.h"
#include "core/mini-printf.h"
#include "themes/DefaultFontRepository.h"
#include "Version.h"
#include "core/math/RgbMixer.h"
#include "gui/GraphicsContext.h"
#include "romBrowser/views/ChipView.h"
#include "picoLoaderBootstrap.h"
#include "romBrowser/DisplayMode/RomBrowserDisplayModeFactory.h"
#include "romBrowser/Theme/Material/MaterialThemeFileIconFactory.h"
#include "romBrowser/views/NdsGameDetailsBottomSheetView.h"
#include "romBrowser/views/cheats/CheatsBottomSheetView.h"
#include "romBrowser/views/recents/RecentsBottomSheetView.h"
#include "romBrowser/views/statistics/StatisticsBottomSheetView.h"
#include "romBrowser/viewModels/DeleteConfirmViewModel.h"
#include "romBrowser/views/deleteconfirm/DeleteConfirmBottomSheetView.h"
#include "romBrowser/views/menu/MenuBottomSheetView.h"
#include "romBrowser/views/about/AboutBottomSheetView.h"
#include "romBrowser/views/exitcombo/ExitComboBottomSheetView.h"
#include "romBrowser/views/gameinfo/GameInfoEditBottomSheetView.h"
#include "romBrowser/views/DisplaySettingsBottomSheetView.h"
#include "bgm/AudioStreamPlayer.h"
#include "bgm/BgmService.h"
#include "themes/ThemeInfoFactory.h"
#include "themes/ThemeFactory.h"
#include "gui/Gx.h"
#include "splashTop.h"
#include "App.h"

#define SPLASH_FRAMES       44

// Display control for the browser, and for the boot page under it: the same
// mode with every background off, so only the version sprite shows over the
// white backdrop until the browser takes over.
#define BROWSER_DISPCNT         0x211F1B
#define SPLASH_BOTTOM_DISPCNT   (BROWSER_DISPCNT & ~(0x0F << 8))
#define SPLASH_VERSION_WIDTH    128

App::App(IAppSettingsService& appSettingsService, IBgmService& bgmService, IGameDataService& gameDataService)
    : _mainObjPltt(GFX_PLTT_OBJ_MAIN)
    , _mainObjVram(GFX_OBJ_MAIN)
    , _mainObjDialogVram(GFX_OBJ_MAIN, 128 * 1024)
    , _subObjVram(GFX_OBJ_SUB)
    , _textureVram((vu16*)0x06860000)
    , _texturePaletteVram((vu16*)0x6880000)
    , _mainVramContext(nullptr, &_mainObjVram, &_textureVram, &_texturePaletteVram)
    , _subVramContext(nullptr, &_subObjVram, nullptr, nullptr)
    , _screenshot(&_ioTaskQueue, &_mainObjVram, &_mainObjDialogVram, &_subObjVram)
    , _appSettingsService(appSettingsService)
    , _bgmService(bgmService)
    , _inputProvider(&_keyInputSource, &_touchInputSource)
    , _inputRepeater(&_inputProvider,
        InputKey::DpadLeft | InputKey::DpadRight | InputKey::DpadUp | InputKey::DpadDown | InputKey::L | InputKey::R,
        25, 8)
    , _romBrowserController(&appSettingsService, &gameDataService, &bgmService, &_ioTaskQueue, &_bgTaskQueue)
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

// The bottom screen is white from power-on until the browser fades in. This
// puts the version and build on it meanwhile, small, bottom-right, so a boot
// says which launcher it is - for the time the theme takes to load, and then
// for the frames the splash holds. Drawn once here by hand, straight to the
// hardware, because the main loop and the palette manager's scanline machinery
// are not running yet; and left untouched by the loop until EndSplashBottom,
// so the page looks the same for the whole of its time on screen.
void App::ShowSplashVersion()
{
    static DefaultFontRepository sFonts;

    char text[48];
    FormatLauncherVersion(text, sizeof(text), true);

    _splashVersionLabel = Label2DView::CreateShared(SPLASH_VERSION_WIDTH, 16, 32, sFonts.GetFont(FontType::Medium10));
    _splashVersionLabel->SetHorizontalAlignment(Alignment::End);
    _splashVersionLabel->SetPosition(256 - 8 - SPLASH_VERSION_WIDTH, 192 - 8 - 16);
    _splashVersionLabel->SetBackgroundColor(Rgb<8, 8, 8>(255, 255, 255));
    _splashVersionLabel->SetForegroundColor(Rgb<8, 8, 8>(110, 112, 120));
    _splashVersionLabel->InitVram(_mainVramContext);
    _splashVersionLabel->SetText(text);
    _splashVersionLabel->VBlank();

    // Static, not a local: Apply copies the rows to palette ram with dma, and
    // dma cannot read the stack, which lives in dtcm - a local here handed the
    // sprite a row of garbage and the text came out as a smear of dark pixels.
    // Row 15 only, since Apply writes every row from the offset up.
    static SimplePaletteManager sPalette;
    sPalette.Reset(15);
    GraphicsContext context
    {
        &_mainOam,
        &sPalette,
        &_rgb6Palette
    };
    _mainOam.Clear();
    _splashVersionLabel->Draw(context);
    VBlank::Wait();
    _mainOam.Apply(GFX_OAM_MAIN);
    sPalette.Apply(GFX_PLTT_OBJ_MAIN);
    GFX_PLTT_BG_MAIN[0] = ColorConverter::ToGBGR565(Rgb<8, 8, 8>(255, 255, 255));
    REG_DISPCNT = SPLASH_BOTTOM_DISPCNT;
    REG_MASTER_BRIGHT = 0;
}

// The boot page hands the bottom screen to the browser: backgrounds on, colour
// 0 the browser's, and master brightness at full white for the fade to start
// from - the same white the page was, so nothing is seen to change until the
// browser comes through it.
void App::EndSplashBottom()
{
    _splashBottom = false;
    GFX_PLTT_BG_MAIN[0] = ColorConverter::ToGBGR565(_theme->GetMaterialColorScheme().inverseOnSurface);
    REG_DISPCNT = BROWSER_DISPCNT;
    REG_MASTER_BRIGHT = 0x4010;
}

void App::LoadTheme()
{
    ThemeInfoFactory themeInfoFactory;
    auto themeInfo = themeInfoFactory.CreateFromThemeFolder(_appSettingsService.GetAppSettings().theme);
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
}

void App::VCountIrq()
{
    _mainObjPltt.VCount();
}

void App::Run()
{
    // The hardware OAM still holds whatever the firmware or the previous
    // process left in it, and the splash turns the screens on before the
    // first VBlank copy of our (all-disabled) shadow tables. Without this,
    // a chunk of stale sprite flashes at the top-left during boot and on
    // slow theme loads (likely one half of upstream #82).
    _mainOam.Apply(GFX_OAM_MAIN);
    _subOam.Apply(GFX_OAM_SUB);

    InitVramMapping();
    DisplaySplashScreen();
    ShowSplashVersion();
    gx_init();

    _chipViewVram = ChipView::UploadGraphics(_mainObjVram);
    _iconButtonViewVram = IconButton2DView::UploadGraphics(_mainObjVram);

    mem_setVramEMapping(MEM_VRAM_E_LCDC);
    _rgb6Palette.UploadGraphics(_mainVramContext);
    mem_setVramEMapping(MEM_VRAM_E_TEX_PLTT_SLOT_0123);

    _dialogPresenter.InitVram();

    LoadTheme();

    // Made before the vram states are stored, so the texture behind its text is
    // part of what a display mode change restores instead of being dropped by it.
    _toast = ToastView::CreateShared(&_theme->GetMaterialColorScheme(),
        _theme->GetFontRepository(), &_vblankTextureLoader);
    _toast->InitVram(_mainVramContext);

    _ioTaskQueue.StartThread(1, _ioTaskThreadStack, sizeof(_ioTaskThreadStack));
    _bgTaskQueue.StartThread(2, _bgTaskThreadStack, sizeof(_bgTaskThreadStack));

    StoreVramState(_vramStateBeforeMakeBottomScreenView);

    _romBrowserBottomScreenView = RomBrowserBottomScreenView::CreateShared(
        &_romBrowserBottomScreenViewModel,
        RomBrowserDisplayModeFactory().GetRomBrowserDisplayMode(
            _romBrowserController.GetRomBrowserDisplaySettings().layout),
        _theme->GetThemeFileIconFactory(),
        _theme->GetRomBrowserViewFactory(),
        &_vblankTextureLoader);
    _romBrowserBottomScreenView->InitVram(_mainVramContext);

    StoreVramState(_vramStateAfterMakeBottomScreenView);

    const auto& materialColorScheme = _theme->GetMaterialColorScheme();

    auto scrimBlendColor = Rgb<8, 8, 8>(
        materialColorScheme.inverseOnSurface.r + (materialColorScheme.scrim.r - materialColorScheme.inverseOnSurface.r) * 5 / 16,
        materialColorScheme.inverseOnSurface.g + (materialColorScheme.scrim.g - materialColorScheme.inverseOnSurface.g) * 5 / 16,
        materialColorScheme.inverseOnSurface.b + (materialColorScheme.scrim.b - materialColorScheme.inverseOnSurface.b) * 5 / 16);

    RgbMixer::MakeGradientPalette((u16*)GFX_PLTT_BG_MAIN, scrimBlendColor, materialColorScheme.GetColor(md::sys::color::surfaceContainerLow));

    // Colour 0 stays the boot page's white for now, and the backgrounds stay
    // off: EndSplashBottom gives both to the browser when the page hands over.
    GFX_PLTT_BG_MAIN[0] = ColorConverter::ToGBGR565(Rgb<8, 8, 8>(255, 255, 255));
    GFX_PLTT_BG_MAIN[31] = ColorConverter::ToGBGR565(materialColorScheme.scrim);
    REG_DISPCNT = SPLASH_BOTTOM_DISPCNT;
    REG_BG0HOFS = 0;
    REG_BG0VOFS = 0;
    REG_BG0CNT = 3;

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
        _bgmService.StartBgmFromConfig();
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
        else if (_fadeIn)
        {
            if (fadeWaitFrames)
            {
                fadeWaitFrames--;
                REG_BLDALPHA_SUB = 16;
                // The bottom screen is the boot page at full brightness; the
                // white the fade will start from is the page's own.
                REG_MASTER_BRIGHT = 0;
            }
            else
            {
                if (_splashBottom)
                    EndSplashBottom();
                bool fadeComplete = _fadeAnimator.Update();
                if (fadeComplete)
                {
                    _fadeIn = false;
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
        case RomBrowserStateTrigger::GotoSettingsScreen:
        {
            break;
        }
        case RomBrowserStateTrigger::LaunchRefused:
        {
            // the browser is back where it was, so say why the game did not start
            if (_toast)
                _toast->Show("Needs a DSi or 3DS");
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
        case RomBrowserStateTrigger::ShowRecents:
        {
            HandleShowRecentsTrigger();
            break;
        }
        case RomBrowserStateTrigger::HideRecents:
        {
            HandleHideRecentsTrigger();
            break;
        }
        case RomBrowserStateTrigger::ShowFavorites:
        {
            HandleShowFavoritesTrigger();
            break;
        }
        case RomBrowserStateTrigger::HideFavorites:
        {
            HandleHideFavoritesTrigger();
            break;
        }
        case RomBrowserStateTrigger::ShowStatistics:
        {
            HandleShowStatisticsTrigger();
            break;
        }
        case RomBrowserStateTrigger::HideStatistics:
        {
            HandleHideStatisticsTrigger();
            break;
        }
        case RomBrowserStateTrigger::ShowDeleteConfirm:
        {
            HandleShowDeleteConfirmTrigger();
            break;
        }
        case RomBrowserStateTrigger::HideDeleteConfirm:
        {
            HandleHideDeleteConfirmTrigger();
            break;
        }
        case RomBrowserStateTrigger::ShowMenu:
        {
            HandleShowMenuTrigger();
            break;
        }
        case RomBrowserStateTrigger::HideMenu:
        {
            HandleHideMenuTrigger();
            break;
        }
        case RomBrowserStateTrigger::ShowAbout:
        {
            HandleShowAboutTrigger();
            break;
        }
        case RomBrowserStateTrigger::HideAbout:
        {
            HandleHideAboutTrigger();
            break;
        }
        case RomBrowserStateTrigger::ShowExitCombo:
        {
            HandleShowExitComboTrigger();
            break;
        }
        case RomBrowserStateTrigger::HideExitCombo:
        {
            HandleHideExitComboTrigger();
            break;
        }
        case RomBrowserStateTrigger::ShowGameInfoEdit:
        {
            HandleShowGameInfoEditTrigger();
            break;
        }
        case RomBrowserStateTrigger::HideGameInfoEdit:
        {
            HandleHideGameInfoEditTrigger();
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
            // A filter toggled from the menu, or game info saved: the browser
            // rebuilds and the sheet goes, in that order (see Update).
            if (_romBrowserController.GetStateMachine().GetPreviousState() == RomBrowserState::GameInfoEdit)
                _dialogPresenter.CloseDialog();
            else
                CloseSheetIfLeavingMenu();
            _changeDisplayMode = true;
            break;
        }
    }
}

void App::HandleShowGameInfoTrigger()
{
    // auto gameInfoDialog = std::make_unique<NdsGameDetailsBottomSheetView>(
    //     &_romBrowserController, &_theme->GetMaterialColorScheme(), _theme->GetFontRepository());
    // gameInfoDialog->SetGraphics(_chipViewVram);
    // _dialogPresenter.ShowDialog(std::move(gameInfoDialog));

    auto cheatsViewModel = SharedPtr<CheatsViewModel>::MakeShared(_romBrowserController.GetTriggerFileInfo(), &_romBrowserController);
    auto cheatsDialog = CheatsBottomSheetView::CreateShared(
        std::move(cheatsViewModel), &_theme->GetMaterialColorScheme(), _theme->GetFontRepository(), &_focusManager);
    _dialogPresenter.ShowDialog(std::move(cheatsDialog));
}

void App::HandleHideGameInfoTrigger()
{
    _dialogPresenter.CloseDialog();
    if (!_dialogPresenter.GetOldFocus())
        _romBrowserBottomScreenView->Focus(_focusManager);
}

void App::HandleShowDisplaySettingsTrigger()
{
    auto displaySettingsDialog = DisplaySettingsBottomSheetView::CreateShared(
        &_displaySettingsBottomSheetViewModel, &_theme->GetMaterialColorScheme(), _theme->GetFontRepository());
    displaySettingsDialog->SetGraphics(_iconButtonViewVram);
    _dialogPresenter.ShowDialog(std::move(displaySettingsDialog));
}

void App::HandleHideDisplaySettingsTrigger()
{
    _dialogPresenter.CloseDialog();
    if (!_dialogPresenter.GetOldFocus())
        _romBrowserBottomScreenView->Focus(_focusManager);
}

void App::HandleShowRecentsTrigger()
{
    CloseSheetIfLeavingMenu();
    auto recentsViewModel = SharedPtr<RecentsViewModel>::MakeShared(
        &_romBrowserController, GameListKind::Recents);
    auto recentsDialog = RecentsBottomSheetView::CreateShared(
        std::move(recentsViewModel), &_theme->GetMaterialColorScheme(), _theme->GetFontRepository(), &_focusManager);
    _dialogPresenter.ShowDialog(std::move(recentsDialog));
}

void App::HandleHideRecentsTrigger()
{
    _dialogPresenter.CloseDialog();
    if (!_dialogPresenter.GetOldFocus())
        _romBrowserBottomScreenView->Focus(_focusManager);
}

void App::HandleShowFavoritesTrigger()
{
    CloseSheetIfLeavingMenu();
    auto favoritesViewModel = SharedPtr<RecentsViewModel>::MakeShared(
        &_romBrowserController, GameListKind::Favorites);
    auto favoritesDialog = RecentsBottomSheetView::CreateShared(
        std::move(favoritesViewModel), &_theme->GetMaterialColorScheme(), _theme->GetFontRepository(), &_focusManager);
    _dialogPresenter.ShowDialog(std::move(favoritesDialog));
}

void App::HandleHideFavoritesTrigger()
{
    _dialogPresenter.CloseDialog();
    if (!_dialogPresenter.GetOldFocus())
        _romBrowserBottomScreenView->Focus(_focusManager);
}

void App::HandleShowStatisticsTrigger()
{
    CloseSheetIfLeavingMenu();
    auto statisticsViewModel = SharedPtr<StatisticsViewModel>::MakeShared(&_romBrowserController);
    auto statisticsDialog = StatisticsBottomSheetView::CreateShared(
        std::move(statisticsViewModel), &_theme->GetMaterialColorScheme(), _theme->GetFontRepository());
    _dialogPresenter.ShowDialog(std::move(statisticsDialog));
}

void App::HandleHideStatisticsTrigger()
{
    _dialogPresenter.CloseDialog();
    if (!_dialogPresenter.GetOldFocus())
        _romBrowserBottomScreenView->Focus(_focusManager);
}

void App::HandleShowDeleteConfirmTrigger()
{
    CloseSheetIfLeavingMenu();
    auto deleteConfirmViewModel = SharedPtr<DeleteConfirmViewModel>::MakeShared(&_romBrowserController);
    auto deleteConfirmDialog = DeleteConfirmBottomSheetView::CreateShared(
        std::move(deleteConfirmViewModel), &_theme->GetMaterialColorScheme(), _theme->GetFontRepository());
    _dialogPresenter.ShowDialog(std::move(deleteConfirmDialog));
}

void App::HandleHideDeleteConfirmTrigger()
{
    _dialogPresenter.CloseDialog();
    if (!_dialogPresenter.GetOldFocus())
        _romBrowserBottomScreenView->Focus(_focusManager);
}

void App::HandleShowMenuTrigger()
{
    auto menuViewModel = SharedPtr<MenuViewModel>::MakeShared(&_romBrowserController);
    auto menuDialog = MenuBottomSheetView::CreateShared(
        std::move(menuViewModel), &_theme->GetMaterialColorScheme(), _theme->GetFontRepository());
    menuDialog->SetGraphics(_iconButtonViewVram);
    _dialogPresenter.ShowDialog(std::move(menuDialog));
}

void App::HandleShowAboutTrigger()
{
    CloseSheetIfLeavingMenu();
    auto aboutViewModel = SharedPtr<AboutViewModel>::MakeShared(&_romBrowserController);
    auto aboutDialog = AboutBottomSheetView::CreateShared(
        std::move(aboutViewModel), &_theme->GetMaterialColorScheme(), _theme->GetFontRepository());
    _dialogPresenter.ShowDialog(std::move(aboutDialog));
}

void App::HandleHideAboutTrigger()
{
    _dialogPresenter.CloseDialog();
    if (!_dialogPresenter.GetOldFocus())
        _romBrowserBottomScreenView->Focus(_focusManager);
}

void App::HandleShowExitComboTrigger()
{
    CloseSheetIfLeavingMenu();
    auto exitComboViewModel = SharedPtr<ExitComboViewModel>::MakeShared(&_romBrowserController);
    auto exitComboDialog = ExitComboBottomSheetView::CreateShared(
        std::move(exitComboViewModel), &_theme->GetMaterialColorScheme(), _theme->GetFontRepository());
    _dialogPresenter.ShowDialog(std::move(exitComboDialog));
}

void App::HandleHideExitComboTrigger()
{
    _dialogPresenter.CloseDialog();
    if (!_dialogPresenter.GetOldFocus())
        _romBrowserBottomScreenView->Focus(_focusManager);
}

void App::HandleShowGameInfoEditTrigger()
{
    CloseSheetIfLeavingMenu();
    auto gameInfoViewModel = SharedPtr<GameInfoEditViewModel>::MakeShared(&_romBrowserController);
    auto gameInfoDialog = GameInfoEditBottomSheetView::CreateShared(
        std::move(gameInfoViewModel), &_theme->GetMaterialColorScheme(), _theme->GetFontRepository());
    _dialogPresenter.ShowDialog(std::move(gameInfoDialog));
}

void App::HandleHideGameInfoEditTrigger()
{
    _dialogPresenter.CloseDialog();
    if (!_dialogPresenter.GetOldFocus())
        _romBrowserBottomScreenView->Focus(_focusManager);
}

void App::HandleHideMenuTrigger()
{
    _dialogPresenter.CloseDialog();
    if (!_dialogPresenter.GetOldFocus())
        _romBrowserBottomScreenView->Focus(_focusManager);
}

// An entry picked from the menu takes the menu's sheet with it. Closing it and
// showing the next sheet in the same frame is fine: the presenter keeps the
// new one waiting until the old one has slid out, and Update holds input off
// meanwhile, so nothing can navigate away under a sheet that is still queued.
void App::CloseSheetIfLeavingMenu()
{
    if (_romBrowserController.GetStateMachine().GetPreviousState() == RomBrowserState::Menu)
        _dialogPresenter.CloseDialog();
}

void App::HandleNavigateTrigger()
{
    // navigation can also start from inside the recents/favorites/delete sheets
    auto previousState = _romBrowserController.GetStateMachine().GetPreviousState();
    if (previousState == RomBrowserState::Recents || previousState == RomBrowserState::Favorites ||
        previousState == RomBrowserState::DeleteConfirm)
    {
        _dialogPresenter.CloseDialog();
        // Focus stays on the app-bar button while the sheet closes (the presenter
        // restores it) - never null, so input is never lost. Once the folder is
        // loaded we move focus to the game instead, so the next A launches it and
        // does not reopen the panel. Nulling focus here (across the close+load
        // window) is what froze the launcher, so we flag it for later instead.
        _focusListAfterFolderLoad = true;
    }
    if (!_romBrowserBottomScreenView->IsAppBarFocused(_focusManager))
    {
        _focusManager.Unfocus();
    }
    else
    {
        // The app bar already holds focus, so the clear above is skipped and the
        // handoff below would never fire: FolderLoadDone only takes over when
        // focus is null. That is how stepping out of an EMPTY folder used to
        // strand the highlight on the back button - an empty listing has no row
        // to focus, so Focus() had fallen back to the app bar on the way in.
        // Flag it instead of nulling here, for the same reason as the panels.
        _focusListAfterFolderLoad = true;
    }
}

void App::HandleFolderLoadDoneTrigger()
{
    _romBrowserTopScreenView.Reset();
    RestoreVramState(_vramStateAfterMakeBottomScreenView);
    auto displayMode = RomBrowserDisplayModeFactory().GetRomBrowserDisplayMode(
        _romBrowserController.GetRomBrowserDisplaySettings().layout);
    _romBrowserTopScreenView = RomBrowserTopScreenView::CreateShared(
        _romBrowserController.GetRomBrowserViewModel(),
        displayMode,
        _theme->GetThemeFileIconFactory(),
        _theme->GetRomBrowserViewFactory(),
        _theme->GetFontRepository(),
        &_theme->GetMaterialColorScheme());
    _romBrowserTopScreenView->InitVram(_subVramContext);
    _romBrowserBottomScreenView->RomBrowserViewModelInvalidated(_mainVramContext);
    // Normally focus is null here (the navigated-from list row was destroyed) and
    // this puts it on the newly loaded folder. When the navigation started with
    // the app bar focused - from a panel, from the back arrow, or from a folder
    // whose listing was empty - focus is still on that button, so the flag makes
    // us move it onto the entry instead: a straight app-bar -> list handoff,
    // never through null. Focus() falls back to the app bar on its own if the
    // folder that just loaded has nothing to put the highlight on.
    if (_focusListAfterFolderLoad || !_focusManager.GetCurrentFocus())
        _romBrowserBottomScreenView->Focus(_focusManager);
    _focusListAfterFolderLoad = false;
}

void App::HandleChangeDisplayModeTrigger(RomBrowserState newState)
{
    _dialogPresenter.ClearOldFocus();
    RestoreVramState(_vramStateBeforeMakeBottomScreenView);
    auto displayMode = RomBrowserDisplayModeFactory().GetRomBrowserDisplayMode(
        _romBrowserController.GetRomBrowserDisplaySettings().layout);
    _romBrowserBottomScreenView = RomBrowserBottomScreenView::CreateShared(
        &_romBrowserBottomScreenViewModel,
        displayMode,
        _theme->GetThemeFileIconFactory(),
        _theme->GetRomBrowserViewFactory(),
        &_vblankTextureLoader);
    _romBrowserBottomScreenView->InitVram(_mainVramContext);
    StoreVramState(_vramStateAfterMakeBottomScreenView);
    _romBrowserTopScreenView = RomBrowserTopScreenView::CreateShared(
        _romBrowserController.GetRomBrowserViewModel(),
        displayMode,
        _theme->GetThemeFileIconFactory(),
        _theme->GetRomBrowserViewFactory(),
        _theme->GetFontRepository(),
        &_theme->GetMaterialColorScheme());
    _romBrowserTopScreenView->InitVram(_subVramContext);
    _romBrowserBottomScreenView->RomBrowserViewModelInvalidated(_mainVramContext);
    if (newState == RomBrowserState::Browser)
        _romBrowserBottomScreenView->Focus(_focusManager);
}

void App::Update()
{
    // Copying the captured frame out of vram is far too long for vblank, so it
    // happens here, in the visible period.
    _screenshot.Update();

    // The io thread is what finishes a capture, so the answer turns up a few
    // frames after the shutter. Saying so only once it is on the card means the
    // message is true, and it also puts it safely outside the picture.
    if (_toast)
    {
        // An L/R jump crosses the list by whole initials, which is easy to lose
        // your place in, so the letter landed on is said for a moment. The flag
        // is drained every frame whether or not there is anything to show, so a
        // jump is never kept to be announced later. It is read here, before
        // this frame's input, so the jump it sees is last frame's - and by the
        // end of that frame the browser view had already written the new
        // selection back into the view model, which is where the name is read.
        //
        // Before the screenshot below on purpose: only one message shows at a
        // time and the newest wins, and of the two the letter is the one that
        // will be along again.
        if (_romBrowserController.ConsumeBigStepJump())
        {
            const auto& viewModel = _romBrowserController.GetRomBrowserViewModel();
            int selectedItem = viewModel->GetSelectedItem();
            if (selectedItem >= 0)
            {
                char letter[2] = { (char)toupper((unsigned char)
                    viewModel->GetFileInfoManager().GetItem(selectedItem).GetFileName()[0]), 0 };
                _toast->Show(letter);
            }
        }

        switch (Screenshot::TakeResult())
        {
            case Screenshot::Result::Saved:
                _toast->Show("Screenshot saved");
                break;
            case Screenshot::Result::Failed:
                _toast->Show("Couldn't save the screenshot");
                break;
            case Screenshot::Result::Busy:
                _toast->Show("Still saving the last one");
                break;
            case Screenshot::Result::None:
                break;
        }
        _toast->Update();
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

    bool isRomBrowserVisible = _romBrowserBottomScreenViewModel.IsRomBrowserVisible();
    if (isRomBrowserVisible && !_exit &&
        curState != RomBrowserState::Launching &&
        curState != RomBrowserState::GoingToSettingsScreen &&
        !_dialogPresenter.IsSwitchingDialog())
    {
        HandleInput();
    }

    if (_topBackground)
        _topBackground->Update();
    if (_bottomBackground)
        _bottomBackground->Update();

    _dialogPresenter.Update();

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
    if (!_changeDisplayMode && _romBrowserBottomScreenViewModel.IsRomBrowserVisible())
    {
        _romBrowserTopScreenView->Draw(subGraphicsContext);
    }

    if (_splashBottom)
    {
        // The boot page is what ShowSplashVersion put in the hardware oam and
        // palette, and it stays exactly that until EndSplashBottom: nothing is
        // drawn on this engine and VBlank leaves its oam and palette alone, so
        // the page cannot change its look between the theme loading and the
        // splash holding. The browser is not drawn either: with the
        // backgrounds off, its sprites would be the only part of it to show.
    }
    else
    {
        if (_bottomBackground)
            _bottomBackground->Draw(mainGraphicsContext);

        _dialogPresenter.ApplyClipArea(mainGraphicsContext);
        if (!_changeDisplayMode)
        {
            _romBrowserBottomScreenView->Draw(mainGraphicsContext);
        }
        mainGraphicsContext.ResetClipArea();

        _dialogPresenter.Draw(mainGraphicsContext);
    }

    // Last, so nothing the browser draws afterwards can land on top of it. And
    // not at all while a screenshot has this engine: on the frame it is actually
    // mirrored, anything of ours drawn here would end up in that picture instead
    // of on this one.
    //
    // The test is deliberately wider than that one frame - it is true from the
    // moment the top half is queued until the mirror is handed back. Being too
    // wide costs a frame or two of a message that is inside the capture's flash
    // anyway; being too narrow puts the message inside a saved screenshot.
    if (_toast)
    {
        if (_screenshot.IsMirroringMainEngine())
            _toast->Suppress();
        else
            _toast->Draw(mainGraphicsContext);
    }

    _mainObjPltt.EndOfFrame();

    Gx::SwapBuffers(GX_XLU_SORT_MANUAL, GX_DEPTH_MODE_Z);
}

void App::VBlank()
{
    dma_ntrStopDirect(0); // stop hblank dma
    // Arms a pending bottom screen capture for the frame whose sprites go up a
    // few lines down, so the saved image is the one the player asked for.
    _screenshot.VBlankBegin();
    _inputProvider.Sample();
    _inputRepeater.Update();
    // The main engine keeps the boot page as drawn until the fade begins.
    if (!_splashBottom)
        _mainOam.Apply(GFX_OAM_MAIN);
    _subOam.Apply(GFX_OAM_SUB);
    _subObjPltt.Apply(GFX_PLTT_OBJ_SUB);

    if (!_vcountIrqStarted)
    {
        rtos_ackIrqMask(RTOS_IRQ_VCOUNT);
        rtos_enableIrqMask(RTOS_IRQ_VCOUNT);
        _vcountIrqStarted = true;
    }
    // Left alone from the moment the top half is queued until the mirror is
    // handed back, which is wider than the frame the palette is actually the
    // other screen's - on the first of those frames nothing has been mirrored
    // yet, since that happens further down this same function. Wider on
    // purpose, for the reason in Draw: too narrow writes into a saved image.
    // Runs during the boot page too: its scheme is empty then, so it writes no
    // palette row and leaves the page's own, but it is what arms the scanline
    // walk the vcount irq performs - which fires from the first frame when the
    // theme selector left the match line enabled, and would otherwise walk a
    // null scheme and hang.
    if (!_screenshot.IsMirroringMainEngine())
        _mainObjPltt.VBlank();

    if (_topBackground)
        _topBackground->VBlank();
    if (_bottomBackground)
        _bottomBackground->VBlank();

    _dialogPresenter.VBlank();

    // Above the mirroring below, and it has to stay there: this writes the main
    // engine's window and display control registers, which from the next
    // statement on belong to the capture.
    if (_toast)
        _toast->VBlank();

    // While a screenshot borrows sub background vram, the top screen view has
    // to sit out: it uploads the selected cover there and marks it done, so an
    // upload during those frames would be lost for good. Skipping means it
    // simply retries once the block is back.
    if (_romBrowserBottomScreenViewModel.IsRomBrowserVisible() && !_screenshot.IsBusy())
    {
        _romBrowserTopScreenView->VBlank();
    }
    _romBrowserBottomScreenView->VBlank();

    _vblankTextureLoader.VBlank();

    // Mirroring the sub engine onto the main one has to be the last word on the
    // frame: it needs this frame's sub sprites uploaded, and every register it
    // copies would otherwise be written over again below. The cover's matrix and
    // window cannot be read back off the sub engine, so the view that computes
    // them restates them in between.
    if (_screenshot.MirrorSubEngineIfPending())
    {
        if (_romBrowserTopScreenView)
            _romBrowserTopScreenView->MirrorToMainEngine();
        _screenshot.ArmMirroredCapture();
    }
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

void App::HandleInput()
{
    // Hold START to save both screens. One key rather than one per screen, and
    // not SELECT: the dsi changes its brightness with SELECT and the volume
    // buttons, and those are not readable in ds mode, so a player adjusting
    // brightness would be taking screenshots without asking for any. START does
    // nothing on any model of the family while software is running.
    //
    // The hold has to begin here: counting only once the key has been seen up
    // means a hold that started before the launcher did cannot be read as a
    // request, whatever it is held for. Counting frames of Current() rather
    // than using Triggered() keeps one hold to one shot.
    //
    // What named this case was the 3ds, where holding START from the home menu
    // is how a ds game is started at its native resolution. Whether the key can
    // still be down by the time this runs is left open on purpose: it was not
    // reproducible on the console it was tried on, which restarts rather than
    // reaching the launcher if START is never released, and one console is not
    // evidence about the rest. The guard is cheap and it also covers the cases
    // that do not need that question answered - a key that sticks, a thumb
    // resting on it, arriving here from another launcher.
    //
    // And no frames are counted while the opening fade is running, because that
    // fade writes master brightness every frame, after this - the same register
    // the capture blacks the bottom screen out with. A capture that started in
    // there would have its blackout overwritten every frame, and worse, would
    // save a half faded brightness and put it back once the fade had stopped
    // writing: the bottom screen would then stay dimmed for the rest of the
    // session, with nothing left to correct it.
    //
    // The test sits after the armed one on purpose. Suppressing the count is
    // not the same as arming: doing it in the branch above would mark a key
    // that is still held as armed, which is exactly what that branch exists to
    // prevent.
    if (!_inputRepeater.Current(InputKey::Start))
    {
        _screenshotHoldFrames = 0;
        _screenshotHoldArmed = true;
    }
    else if (_dialogPresenter.IsCapturingInput())
    {
        // The in-game exit sheet is capturing keys, and START may be part of
        // the combination. Drop the count and stay disarmed until START is seen
        // up, so finishing a combo with START held cannot save a screenshot.
        _screenshotHoldFrames = 0;
        _screenshotHoldArmed = false;
    }
    else if (_screenshotHoldArmed && !_fadeIn &&
        ++_screenshotHoldFrames == kScreenshotHoldFrames)
    {
        _screenshot.RequestBothScreens();
    }

    if (!_dialogPresenter.IsBottomSheetVisible() &&
        _inputRepeater.Triggered(InputKey::A) && _inputRepeater.Current(InputKey::Select))
    {
        // SELECT + A launches a random game. The chord is claimed here, before
        // the focused view sees the frame, because dispatch is leaf first and
        // the highlighted game or app bar button would take the A press
        // otherwise. It is a chord rather than a bare SELECT because on the
        // DSi SELECT + volume is the brightness shortcut, so SELECT on its own
        // must not launch anything.
        _romBrowserBottomScreenViewModel.LaunchRandomGame();
    }
    else
    {
        _focusManager.Update(_inputRepeater);
    }
    Point touchPoint;
    if (_inputRepeater.Triggered(InputKey::Touch) &&
        _inputRepeater.GetCurrentTouchPoint(touchPoint))
    {
        // pen down
        if (_dialogPresenter.IsBottomSheetVisible())
        {
            _dialogPresenter.HandlePenDown(touchPoint, _focusManager);
        }
        else
        {
            _romBrowserBottomScreenView->HandlePenDown(touchPoint, _focusManager);
        }
        _lastTouchPoint = touchPoint;
    }
    else if (_inputRepeater.Released(InputKey::Touch))
    {
        // pen up
        if (_dialogPresenter.IsBottomSheetVisible())
        {
            _dialogPresenter.HandlePenUp(_lastTouchPoint, _focusManager);
        }
        else
        {
            _romBrowserBottomScreenView->HandlePenUp(_lastTouchPoint, _focusManager);
        }
    }
    else if (_inputRepeater.Current(InputKey::Touch)
        && _inputRepeater.GetCurrentTouchPoint(touchPoint))
    {
        // pen move
        if (_dialogPresenter.IsBottomSheetVisible())
        {
            _dialogPresenter.HandlePenMove(touchPoint, _focusManager);
        }
        else
        {
            _romBrowserBottomScreenView->HandlePenMove(touchPoint, _focusManager);
        }
        _lastTouchPoint = touchPoint;
    }
}
