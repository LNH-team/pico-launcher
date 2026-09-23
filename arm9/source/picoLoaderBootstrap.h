#pragma once
#include "picoLoader7.h"

pload_params_t* pload_getLoadParams();
void pload_setBootDrive(PicoLoaderBootDrive bootDrive);
void pload_setLauncherPath(const char* launcherPath);
void pload_setCheatData(const pload_cheats_t* cheatData);

/// @brief Sets the language games see, see \see PicoLoaderGameLanguage.
///        Only used by Pico Loader versions with API version 4 or higher.
/// @param gameLanguage The game language, or \see PLOAD_GAME_LANGUAGE_AUTO to let Pico Loader choose.
void pload_setGameLanguage(u16 gameLanguage);

void pload_start();
