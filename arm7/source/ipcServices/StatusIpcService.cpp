#include "common.h"
#include <nds/system.h>
#include <libtwl/sio/sioRtc.h>
#include <libtwl/spi/spiPmic.h>
#include <libtwl/i2c/i2cMcu.h>
#include "statusPayload.h"
#include "StatusIpcService.h"

static u8 bcd2dec(u8 v) { return (u8)((v >> 4) * 10 + (v & 0x0F)); }

void StatusIpcService::HandleMessage(u32 data)
{
    auto* out = reinterpret_cast<StatusPayload*>(data << 2);

    rtc_datetime_t dt;
    rtc_readDateTime(&dt);
    dt.time.hour   = bcd2dec(dt.time.hour & 0x3F);
    dt.time.minute = bcd2dec(dt.time.minute);
    out->dateTime = dt;

    u8 flags = 0;
    if (isDSiMode())
    {
        /** @brief BPTWL register 0x20: low nibble is battery level, bit 7 is charger present. */
        u8 raw = mcu_readReg(0x20);
        out->batteryLevel = (u8)(raw & 0x0F);
        flags |= STATUS_FLAG_HAS_FINE_LEVEL;
        if (raw & 0x80)
        {
            flags |= STATUS_FLAG_CHARGING;
        }
    }
    else
    {
        out->batteryLevel = pmic_isBatteryLow() ? 1 : 0;
        if (pmic_isExternalPowerConnected())
        {
            flags |= STATUS_FLAG_CHARGING;
        }
    }
    out->flags = flags;

    SendResponseMessage(1);
}
