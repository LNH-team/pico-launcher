#pragma once
#include "common.h"
#include <memory>
#include "services/settings/IAppSettingsService.h"
#include "bgm/IBgmService.h"
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
#include "romBrowser/Theme/Material/MaterialThemeFileIconFactory.h"
#include "romBrowser/RomBrowserController.h"
#include "DialogPresenter.h"
#include "themes/ITheme.h"
#include "animation/Animator.h"

class alignas(32) App : public IProcess
{
public:
    App(IAppSettingsService& appSettingsService, IBgmService& bgmService);

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
    u32 _ioTaskThreadStack[2048 / 4];
    TaskQueue<32, sizeof(TaskBase) + 32> _bgTaskQueue;
    u32 _bgTaskThreadStack[2048 / 4];

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

    std::unique_ptr<MaterialThemeFileIconFactory> _materialThemeFileIconFactory;

    RomBrowserBottomScreenViewModel _romBrowserBottomScreenViewModel;

    DialogPresenter _dialogPresenter;

    VramState _vramStateBeforeMakeBottomScreenView;
    VramState _vramStateAfterMakeBottomScreenView;
    bool _changeDisplayMode = false;

    ChipView::VramToken _chipViewVram;
    IconButton2DView::VramToken _iconButtonViewVram;

    bool _vcountIrqStarted = false;

    Point _lastTouchPoint = Point(0, 0);

    void InitVramMapping() const;
    void DisplaySplashScreen() const;
    void LoadTheme();
    void VCountIrq();
    void HandleInput();
    void HandleTrigger(RomBrowserStateTrigger trigger, RomBrowserState newState);
    void HandleShowGameInfoTrigger();
    void HandleHideGameInfoTrigger();
    void HandleShowDisplaySettingsTrigger();
    void HandleHideDisplaySettingsTrigger();
    void HandleNavigateTrigger();
    void HandleFolderLoadDoneTrigger();
    void HandleChangeDisplayModeTrigger(RomBrowserState newState);

    bool IsRomBrowserVisible() const;

    void MainLoop();
    void Update();
    void Draw();
    void VBlank();

    void StoreVramState(VramState& vramState) const;
    void RestoreVramState(const VramState& vramState);
};