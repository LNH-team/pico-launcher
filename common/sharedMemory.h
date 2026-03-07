#pragma once

#define SHARED_KEY_XY           (*(vu16*)0x02FFFFA8)
#define SHARED_TOUCH_X          (*(vu16*)0x02FFFFAA)
#define SHARED_TOUCH_Y          (*(vu16*)0x02FFFFAC)
#define SHARED_SYSTEM_FLAGS     (*(vu16*)0x02FFFFAE)

#define SHARED_FLAG_SLEEP_MODE  (1 << 0)
