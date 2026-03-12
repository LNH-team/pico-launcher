#pragma once
#include "common.h"
#include <memory>
#include "services/settings/IAppSettingsService.h"
#include "services/State/AppStateBin.h"
#include "services/State/AppStateBinSerializer.h"
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
#include "gui/input/SampledInputProvider.h"
#include "gui/input/InputRepeater.h"
#include "gui/input/TouchProvider.h"
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
#include "romBrowser/layout/LayoutService.h"
#include "DialogPresenter.h"
#include "themes/ITheme.h"
#include "core/SharedPtr.h"
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

    TaskQueue<32, 32> _ioTaskQueue;
    u32 _ioTaskThreadStack[2048 / 4];
    TaskQueue<32, 32> _bgTaskQueue;
    u32 _bgTaskThreadStack[2048 / 4];

    std::unique_ptr<ITheme> _theme;
    std::unique_ptr<IThemeBackground> _topBackground;
    std::unique_ptr<IThemeBackground> _bottomBackground;

    IAppSettingsService& _appSettingsService;
    IBgmService& _bgmService;
    volatile bool _exit = false;

    PadInputSource _inputSource;
    SampledInputProvider _inputProvider;
    InputRepeater _inputRepeater;
    TouchProvider _touchProvider;
    View* _touchCaptureTarget = nullptr;
    bool _touchCapturedByDialog = false;

    std::unique_ptr<RomBrowserBottomScreenView> _romBrowserBottomScreenView;
    std::unique_ptr<RomBrowserTopScreenView> _romBrowserTopScreenView;

    RomBrowserController _romBrowserController;

    DisplaySettingsViewModel _displaySettingsBottomSheetViewModel;
    LayoutService _layoutService;

    FocusManager _focusManager;

    std::unique_ptr<MaterialThemeFileIconFactory> _materialThemeFileIconFactory;

    RomBrowserBottomScreenViewModel _romBrowserBottomScreenViewModel;

    DialogPresenter _dialogPresenter;

    VramState _vramStateBeforeThemeLoad;
    VramState _vramStateBeforeMakeBottomScreenView;
    VramState _vramStateAfterMakeBottomScreenView;
    bool _changeDisplayMode = false;
    bool _pendingAppRestart = false;
    bool _sleepModeWasActive = false;

    ChipView::VramToken _chipViewVram;
    IconButton2DView::VramToken _iconButtonViewVram;

    bool _vcountIrqStarted = false;

    void InitVramMapping() const;
    void DisplaySplashScreen() const;
    void LoadTheme();
    void ApplyThemeColors();
    void VCountIrq();
    void HandleTrigger(RomBrowserStateTrigger trigger, RomBrowserState newState);
    void HandleShowGameInfoTrigger();
    void HandleHideGameInfoTrigger();
    void HandleShowCheatsTrigger();
    void HandleHideCheatsTrigger();
    void HandleShowCheatDescriptionTrigger();
    void HandleHideCheatDescriptionTrigger();
    void HandleShowDisplaySettingsTrigger();
    void HandleHideDisplaySettingsTrigger();
    void HandleShowDisplayInfoTrigger();
    void HandleHideDisplayInfoTrigger();
    void HandleShowLayoutEditorTrigger();
    void HandleHideLayoutEditorTrigger();
    void HandleNavigateTrigger();
    void HandleFolderLoadDoneTrigger();
    void HandleRomBrowserViewModelInvalidated();
    void HandleChangeDisplayModeTrigger(RomBrowserState newState);
    void DrainTaskQueues();

    bool IsRomBrowserVisible() const;

    void MainLoop();
    void Update();
    void Draw();
    void VBlank();
    void DispatchTouch(const TouchEvent& event);

    void StoreVramState(VramState& vramState) const;
    void RestoreVramState(const VramState& vramState);

    static constexpr int kMaxThemeCount = 64;
    std::array<String<char, 64>, kMaxThemeCount> _themeNames;
    int _themeCount = 0;

    String<char, 64> _effectiveThemeName;

    u8 _loadedPrimaryColorR = 0xFF;
    u8 _loadedPrimaryColorG = 0xFF;
    u8 _loadedPrimaryColorB = 0xFF;
    bool _loadedDarkTheme = false;

    static constexpr const char* kStateBinPath = "/_pico/extras/state.bin";

    AppStateBinSerializer _stateBinSerializer;

    void LoadAppStateBin();
    void SaveAppStateBinAsync();
};
