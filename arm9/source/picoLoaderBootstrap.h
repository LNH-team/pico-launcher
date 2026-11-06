#pragma once
#include "picoLoader7.h"

pload_params_t* pload_getLoadParams();
void pload_setBootDrive(PicoLoaderBootDrive bootDrive);
void pload_setLauncherPath(const char* launcherPath);
/// @brief Keys held to exit a game, as an active-low KEY_* mask. Only written
///        to a loader that reports API version 4 or newer; zero means its
///        default combo.
void pload_setExitCombo(u16 exitCombo);
void pload_setCheatData(const pload_cheats_t* cheatData);
void pload_start();
