#pragma once
#include <memory>
#include "SettingsController.h"
#include "animation/Animator.h"
#include "core/task/TaskQueue.h"
#include "gui/AdvancedPaletteManager.h"
#include "gui/AscendingStackVramManager.h"
#include "gui/OamManager.h"
#include "gui/Rgb6Palette.h"
#include "gui/SimplePaletteManager.h"
#include "gui/VBlankTextureLoader.h"
#include "gui/VramContext.h"
#include "gui/input/InputRepeater.h"
#include "gui/input/PadInputSource.h"
#include "gui/input/SampledInputProvider.h"
#include "gui/input/TouchInputSource.h"
#include "romBrowser/views/IconButton2DView.h"
#include "settings/views/ThemeListTopView.h"
#include "settings/views/ThemeListBottomView.h"
#include "themes/ITheme.h"
#include "services/process/IProcess.h"
#include "services/settings/IAppSettingsService.h"

class SettingsController;

class alignas(32) SettingsProcess : public IProcess
{
public:
    SettingsProcess(IAppSettingsService& appSettingsService);

    void Run() override;
    void Exit() override;

private:
    AdvancedPaletteManager<64> _mainObjPltt;
    OamManager _mainOam;
    AscendingStackVramManager _mainObjVram;
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

    std::unique_ptr<ITheme> _theme;
    std::unique_ptr<IThemeBackground> _topBackground;
    std::unique_ptr<IThemeBackground> _bottomBackground;

    IAppSettingsService& _appSettingsService;
    volatile bool _exit = false;

    PadInputSource _keyInputSource;
    TouchInputSource _touchInputSource;
    SampledInputProvider _inputProvider;
    InputRepeater _inputRepeater;

    FocusManager _focusManager;

    IconButton2DView::VramToken _iconButtonViewVram;

    bool _vcountIrqStarted = false;

    Point _lastTouchPoint = Point(0, 0);

    std::unique_ptr<SettingsController> _settingsController;
    SharedPtr<ThemeListTopView> _themeListTopView;
    SharedPtr<ThemeListBottomView> _themeListBottomView;

    void InitVramMapping() const;
    void LoadTheme();
    void MainLoop();
    void Update();
    void HandleInput();
    void Draw();
    void VBlank();
    void VCountIrq();
};
