#include "StatusViewModel.h"
#include "statusIpc.h"
#include "romBrowser/status/ClockFormatter.h"
#include <string.h>

namespace
{
    /// @brief Hidden UTF-8 glyph whose advance matches ':' in the dedicated clock font.
    constexpr char kHiddenClockColonUtf8[] = "\xEE\x80\x80";
}

void StatusViewModel::Update()
{
    if (_resultReady)
    {
        _resultReady = false;
        ApplyResult(_pendingPayload);
        _pollInFlight = false;
    }

    if (++_blinkCounter >= CLOCK_BLINK_FRAMES)
    {
        _blinkCounter = 0;
        _showColon = !_showColon;
        RebuildClockDisplay();
    }

    if (++_frameCounter >= STATUS_POLL_FRAMES && !_pollInFlight)
    {
        _frameCounter = 0;
        _pollInFlight = true;
        _bgTaskQueue.Enqueue([this](const vu8&) {
            StatusPayload s;
            status_read(&s);
            _pendingPayload = s;
            _resultReady = true;
            return TaskResult<void>::Completed();
        });
    }
}

void StatusViewModel::ApplyResult(const StatusPayload& s)
{
    if (s.dateTime.time.minute != _lastMinute)
    {
        _lastMinute = s.dateTime.time.minute;
        FormatClock(_clockText, s.dateTime.time.hour, s.dateTime.time.minute, _clockFormat);
        RebuildClockDisplay();
    }
    _batteryDisplay = ResolveBatteryDisplay(s);
}

void StatusViewModel::RebuildClockDisplay()
{
    if (_showColon)
    {
        strcpy(_clockDisplay, _clockText);
        return;
    }

    const char* colon = strchr(_clockText, ':');
    if (!colon)
    {
        strcpy(_clockDisplay, _clockText);
        return;
    }

    size_t prefixLength = colon - _clockText;
    size_t suffixLength = strlen(colon + 1);
    memcpy(_clockDisplay, _clockText, prefixLength);
    memcpy(_clockDisplay + prefixLength, kHiddenClockColonUtf8, sizeof(kHiddenClockColonUtf8) - 1);
    memcpy(_clockDisplay + prefixLength + sizeof(kHiddenClockColonUtf8) - 1, colon + 1, suffixLength + 1);
}
