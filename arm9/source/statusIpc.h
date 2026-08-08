#pragma once
#include "statusPayload.h"

/// @brief Initializes the ARM9 status IPC client.
void status_init();

/// @brief Reads the latest status payload from ARM7.
/// @param[out] out Destination payload.
void status_read(StatusPayload* out);
