#pragma once
#include "common.h"
#include "core/task/TaskQueue.h"
#include "romBrowser/status/BatteryState.h"
#include "services/settings/AppSettings.h"

/// @brief Polls ARM7 status data and exposes clock and battery state for the top-screen overlay.
class StatusViewModel
{
public:
    StatusViewModel(TaskQueueBase& bgTaskQueue, ClockFormat clockFormat)
        : _bgTaskQueue(bgTaskQueue), _clockFormat(clockFormat) {}

    /// @brief Advances polling and blink state once per frame.
    void Update();

    /// @brief Returns the current clock display string.
    /// @details The visible ':' blinks by swapping to a same-width hidden glyph.
    const char* GetClockText() const { return _clockDisplay; }

    /// @brief Returns the current battery icon state.
    BatteryIcon GetBatteryIcon() const { return _batteryIcon; }

private:
    static constexpr u32 STATUS_POLL_FRAMES = 60;   ///< Roughly 1s at 59.8Hz.
    static constexpr u32 CLOCK_BLINK_FRAMES = 30;   ///< Toggle every 0.5s for a 1s blink cycle.
    static constexpr u32 CLOCK_TEXT_BUFFER_SIZE = 9;
    static constexpr u32 CLOCK_DISPLAY_BUFFER_SIZE = 12;

    TaskQueueBase& _bgTaskQueue;
    ClockFormat _clockFormat;
    u32 _frameCounter = STATUS_POLL_FRAMES;   ///< Starts primed so the first frame polls immediately.
    volatile bool _pollInFlight = false;

    char _clockText[CLOCK_TEXT_BUFFER_SIZE] = "";         ///< Canonical time string with a visible ':'.
    char _clockDisplay[CLOCK_DISPLAY_BUFFER_SIZE] = "";   ///< Display string; blink-off swaps ':' for U+E000.
    u8 _lastMinute = 0xFF;
    u32 _blinkCounter = 0;
    bool _showColon = true;
    BatteryIcon _batteryIcon = BatteryIcon::Full;

    StatusPayload _pendingPayload {};   ///< Raw payload handed off from the background thread.
    volatile bool _resultReady = false;

    void ApplyResult(const StatusPayload& s);
    void RebuildClockDisplay();
};
