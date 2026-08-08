#pragma once
#include "common.h"
#include "core/SharedPtr.h"
#include "gui/views/ViewContainer.h"
#include "gui/views/Label2DView.h"
#include "romBrowser/status/BatteryState.h"

class StatusViewModel;
class MaterialColorScheme;
class IFontRepository;

/// @brief Material top-screen status overlay.
/// @details Renders a clock at top-left and a battery icon at top-right above the file info card.
class MaterialStatusView : public ViewContainer
{
    SHARED_ONLY(MaterialStatusView)

public:
    void InitVram(const VramContext& vramContext) override;
    void Update() override;
    void Draw(GraphicsContext& graphicsContext) override;

    Rectangle GetBounds() const override
    {
        return Rectangle(0, 0, 256, 18);
    }

private:
    static constexpr int BatteryIconCount = 6;
    static constexpr int ClockTextBufferSize = 12;

    SharedPtr<Label2DView> _clock;
    StatusViewModel* _viewModel;
    const MaterialColorScheme* _materialColorScheme;
    u32 _batteryVramOffsets[BatteryIconCount] = {};
    BatteryIcon _batteryIcon = BatteryIcon::Full;
    char _lastClockText[ClockTextBufferSize] = "";
    bool _darkTheme;

    MaterialStatusView(StatusViewModel* viewModel, const MaterialColorScheme* materialColorScheme,
        const IFontRepository* fontRepository);
};
