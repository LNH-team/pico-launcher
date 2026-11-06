#pragma once
#include "common.h"
#include <memory>
#include "services/settings/IAppSettingsService.h"
#include "bgm/IBgmService.h"
#include "services/gamedata/IGameDataService.h"
#include "services/process/IProcess.h"
#include "gui/SimplePaletteManager.h"
#include "gui/AdvancedPaletteManager.h"
#include "gui/OamManager.h"
#include "gui/VramContext.h"
#include "gui/AscendingStackVramManager.h"
#include "gui/DescendingStackVramManager.h"
#include "material/scheme/scheme.h"
#include "gui/input/PadInputSource.h"
#include "gui/input/TouchInputSource.h"
#include "gui/input/SampledInputProvider.h"
#include "gui/input/InputRepeater.h"
#include "gui/VBlankTextureLoader.h"
#include "gui/Rgb6Palette.h"
#include "core/task/TaskQueue.h"
#include "themes/material/MaterialColorScheme.h"
#include "romBrowser/viewModels/RomBrowserBottomScreenViewModel.h"
#include "romBrowser/viewModels/DisplaySettingsViewModel.h"
#include "romBrowser/views/RomBrowserBottomScreenView.h"
#include "romBrowser/views/RomBrowserTopScreenView.h"
#include "romBrowser/views/IconButton2DView.h"
#include "romBrowser/views/ChipView.h"
#include "romBrowser/RomBrowserController.h"
#include "DialogPresenter.h"
#include "themes/ITheme.h"
#include "animation/Animator.h"
#include "Screenshot.h"
#include "gui/views/ToastView.h"
#include "gui/views/Label2DView.h"

class alignas(32) App : public IProcess
{
public:
    App(IAppSettingsService& appSettingsService, IBgmService& bgmService, IGameDataService& gameDataService);

    void Run() override;
    void Exit() override;

private:
    struct VramState
    {
        u32 _subObjVramState;
        u32 _mainObjVramState;
        u32 _texVramState;
        u32 _texPlttVramState;
    };

    AdvancedPaletteManager<64> _mainObjPltt;
    OamManager _mainOam;
    AscendingStackVramManager _mainObjVram;
    DescendingStackVramManager _mainObjDialogVram;
    OamManager _subOam;
    SimplePaletteManager _subObjPltt;
    AscendingStackVramManager _subObjVram;
    AscendingStackVramManager _textureVram;
    AscendingStackVramManager _texturePaletteVram;
    VBlankTextureLoader _vblankTextureLoader;
    VramContext _mainVramContext;
    VramContext _subVramContext;
    Rgb6Palette _rgb6Palette;
    Animator<int> _fadeAnimator;

    TaskQueue<32, sizeof(TaskBase) + 32> _ioTaskQueue;
    // 4096, not 2048: this thread also runs SdFolderFactory's recursive
    // empty-folder probe (issue #6), and 2048 turned out to be tight enough
    // for that recursion to plausibly overrun it on real hardware - see
    // SdFolderFactory.cpp's own stack-usage comments for the accounting.
    u32 _ioTaskThreadStack[4096 / 4];
    TaskQueue<32, sizeof(TaskBase) + 32> _bgTaskQueue;
    u32 _bgTaskThreadStack[2048 / 4];

    /// Visible periods START has to be held before both screens are saved.
    static constexpr u32 kScreenshotHoldFrames = 30;

    /// Whether the fade the launcher opens with is still running. A member
    /// rather than a local of the loop because the screenshot shortcut has to
    /// know: that fade writes master brightness every frame, which is the same
    /// register a capture blacks the bottom screen out with.
    bool _fadeIn = true;

    Screenshot _screenshot;
    u32 _screenshotHoldFrames = 0;
    /// False until START has been seen up, so a hold that started before the
    /// launcher did cannot count as a request.
    bool _screenshotHoldArmed = false;
    SharedPtr<ToastView> _toast;

    /// The version and build hash on the bottom screen while the launcher
    /// boots: the only thing that screen shows until the browser fades in.
    SharedPtr<Label2DView> _splashVersionLabel;
    /// True from boot until the fade to the browser begins. While it is, the
    /// bottom screen is the boot page - white, with the version - and the
    /// browser is not drawn there.
    bool _splashBottom = true;

    std::unique_ptr<ITheme> _theme;
    std::unique_ptr<IThemeBackground> _topBackground;
    std::unique_ptr<IThemeBackground> _bottomBackground;

    IAppSettingsService& _appSettingsService;
    IBgmService& _bgmService;
    volatile bool _exit = false;

    PadInputSource _keyInputSource;
    TouchInputSource _touchInputSource;
    SampledInputProvider _inputProvider;
    InputRepeater _inputRepeater;

    SharedPtr<RomBrowserBottomScreenView> _romBrowserBottomScreenView;
    SharedPtr<RomBrowserTopScreenView> _romBrowserTopScreenView;

    RomBrowserController _romBrowserController;

    DisplaySettingsViewModel _displaySettingsBottomSheetViewModel;

    FocusManager _focusManager;

    RomBrowserBottomScreenViewModel _romBrowserBottomScreenViewModel;

    DialogPresenter _dialogPresenter;

    VramState _vramStateBeforeMakeBottomScreenView;
    VramState _vramStateAfterMakeBottomScreenView;
    bool _changeDisplayMode = false;
    // Set when a navigation started from a panel (recents/favorites/delete), so
    // the folder that loads takes focus onto its game instead of leaving it on
    // the app-bar button that opened the panel. Consumed at FolderLoadDone.
    bool _focusListAfterFolderLoad = false;

    ChipView::VramToken _chipViewVram;
    IconButton2DView::VramToken _iconButtonViewVram;

    bool _vcountIrqStarted = false;

    Point _lastTouchPoint = Point(0, 0);

    void InitVramMapping() const;
    void DisplaySplashScreen() const;
    void ShowSplashVersion();
    void EndSplashBottom();
    void LoadTheme();
    void VCountIrq();
    void HandleInput();
    void HandleTrigger(RomBrowserStateTrigger trigger, RomBrowserState newState);
    void HandleShowGameInfoTrigger();
    void HandleHideGameInfoTrigger();
    void HandleShowDisplaySettingsTrigger();
    void HandleHideDisplaySettingsTrigger();
    void HandleShowRecentsTrigger();
    void HandleHideRecentsTrigger();
    void HandleShowFavoritesTrigger();
    void HandleHideFavoritesTrigger();
    void HandleShowStatisticsTrigger();
    void HandleHideStatisticsTrigger();
    void HandleShowDeleteConfirmTrigger();
    void HandleHideDeleteConfirmTrigger();
    void HandleShowMenuTrigger();
    void HandleHideMenuTrigger();
    void HandleShowAboutTrigger();
    void HandleHideAboutTrigger();
    void HandleShowExitComboTrigger();
    void HandleHideExitComboTrigger();
    void HandleShowGameInfoEditTrigger();
    void HandleHideGameInfoEditTrigger();
    void CloseSheetIfLeavingMenu();
    void HandleNavigateTrigger();
    void HandleFolderLoadDoneTrigger();
    void HandleChangeDisplayModeTrigger(RomBrowserState newState);

    void MainLoop();
    void Update();
    void Draw();
    void VBlank();

    void StoreVramState(VramState& vramState) const;
    void RestoreVramState(const VramState& vramState);
};