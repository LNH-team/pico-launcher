#include "common.h"
#include <nds/input.h>
#include <nds/arm7/touch.h>

// See: https://github.com/blocksds/libnds/blob/master/source/arm7/input.c

// === Touchscreen filter configuration ===

// Replace Z1/Z2 values with X/Y noisiness measurements.
// #define TOUCH_DEBUG_NOISINESS

// The number of frames to debounce/hold pen presses for.
// Set to 0 to disable.
#define PEN_DOWN_DEBOUNCE 1
// The shift (1 << N) used for the IIR filter to average noisy samples across
// time. Set to 0 to disable.
#define TOUCH_MAX_NOISE_PEN_UP_IIR_SHIFT 5

// The maximum value of noisiness for pressing a pen down (measurement now valid).
#define TOUCH_MAX_NOISE_PEN_DOWN 38
// The minimum value of noisiness for lifting a pen up (measurement no longer valid).
#define TOUCH_MAX_NOISE_PEN_UP   50

// === Touchscreen filter ===

// IIR filter constants.
#define TOUCH_MAX_NOISE_PEN_UP_IIR_RATIO (1 << TOUCH_MAX_NOISE_PEN_UP_IIR_SHIFT)
#define TOUCH_MAX_NOISE_PEN_UP_IIR_MIN (TOUCH_MAX_NOISE_PEN_UP - TOUCH_MAX_NOISE_PEN_UP_IIR_RATIO)

typedef struct {
    u16 value;      // 1..4095, 0 if invalid
    u16 noisiness;  // 0..4095, ~15-16 = 1 pixel
} libnds_touchMeasurementFilterResult;

/**
 * @brief Perform filtering on the raw touch samples provided to return one
 * averaged sample and an estimate of how noisy it is, while skipping outliers.
 *
 * Internal. See touchFilter.c for more information.
 */
extern "C" libnds_touchMeasurementFilterResult libnds_touchMeasurementFilter(u16 values[5]);

void touch_init()
{
    touchInit();
}

bool touch_update(touchPosition& touchPos)
{
#if PEN_DOWN_DEBOUNCE > 0
    static touchPosition lastTouchPosition;
    static bool lastPenDown = false;
    static u8 penDownDebounce = 0;
#else
    touchPosition lastTouchPosition;
    bool lastPenDown = false;
#endif

    bool penDown = touchPenDown();
    if (penDown)
    {
        // Set penDown to false for later fallthroughs to noPenDown.
        // It will be set to true if all the touch filtering ensures the readout is valid.
        penDown = false;

        // Measure new touch position.
        touchRawArray data;
        if (!touchReadData(&data))
            goto noPenDown;

        libnds_touchMeasurementFilterResult rawXresult = libnds_touchMeasurementFilter(data.rawX);
        if (!rawXresult.value)
            goto noPenDown;
        libnds_touchMeasurementFilterResult rawYresult = libnds_touchMeasurementFilter(data.rawY);
        if (!rawYresult.value)
            goto noPenDown;

        // Valid sample read.
        u16 noisiness = rawXresult.noisiness > rawYresult.noisiness ? rawXresult.noisiness : rawYresult.noisiness;
        if (noisiness <= (lastPenDown ? TOUCH_MAX_NOISE_PEN_UP : TOUCH_MAX_NOISE_PEN_DOWN))
        {
            lastTouchPosition.z1 = libnds_touchMeasurementFilter(data.z1).value;
            lastTouchPosition.z2 = libnds_touchMeasurementFilter(data.z2).value;

#if TOUCH_MAX_NOISE_PEN_UP_IIR_SHIFT > 0
            // Apply an IIR filter on noisy X/Y samples.
            // Skip the IIR filter if the pen was just pressed.
            int n = (noisiness - TOUCH_MAX_NOISE_PEN_UP_IIR_MIN);
            if (noisiness <= 0 || !lastPenDown)
            {
                lastTouchPosition.rawx = rawXresult.value;
                lastTouchPosition.rawy = rawYresult.value;
            }
            else if (noisiness <= TOUCH_MAX_NOISE_PEN_UP_IIR_RATIO)
            {
                lastTouchPosition.rawx =
                    ((rawXresult.value * (TOUCH_MAX_NOISE_PEN_UP_IIR_RATIO - n))
                     + (lastTouchPosition.rawx * n)) >> TOUCH_MAX_NOISE_PEN_UP_IIR_SHIFT;
                lastTouchPosition.rawy =
                    ((rawYresult.value * (TOUCH_MAX_NOISE_PEN_UP_IIR_RATIO - n))
                     + (lastTouchPosition.rawy * n)) >> TOUCH_MAX_NOISE_PEN_UP_IIR_SHIFT;
            }
#else
            lastTouchPosition.rawx = rawXresult.value;
            lastTouchPosition.rawy = rawYresult.value;
#endif

            touchApplyCalibration(lastTouchPosition.rawx, lastTouchPosition.rawy, &lastTouchPosition.px, &lastTouchPosition.py);
            penDown = true;
        }

#ifdef TOUCH_DEBUG_NOISINESS
        lastTouchPosition.z1 = rawXresult.noisiness;
        lastTouchPosition.z2 = rawYresult.noisiness;
#endif
    }

noPenDown:
#if PEN_DOWN_DEBOUNCE > 0
    // Perform simple debouncing.
    // Hold new presses for PEN_DOWN_DEBOUNCE frames.
    if (!penDownDebounce)
    {
        if (lastPenDown != penDown)
        {
            lastPenDown = penDown;
            if (penDown)
                penDownDebounce = PEN_DOWN_DEBOUNCE;
        }
    }
    else
    {
        penDownDebounce--;
    }
#else
    lastPenDown = penDown;
#endif

    // Return the touch position and pen down.
    if (lastPenDown)
    {
        touchPos = lastTouchPosition;
    }

    return lastPenDown;
}
