#include "common.h"
#include "NdsFileType.h"
#include "fat/File.h"
#include <string.h>
#include "core/mini-printf.h"

const NdsFileType NdsFileType::sInstance;

bool NdsFileType::TrySetLaunchParameters(pload_params_t* launchParameters, const char* filePath) const
{
    StringUtil::Copy(launchParameters->romPath, filePath, sizeof(launchParameters->romPath));
    TryAddCheatFile(launchParameters, filePath);
    return true;
}

void NdsFileType::TryAddCheatFile(pload_params_t* launchParameters, const char* filePath) const
{

    char baseName[256];
    strncpy(baseName, filePath, sizeof(baseName) - 1);
    baseName[sizeof(baseName) - 1] = 0;
    
    char* dot = strrchr(baseName, '.');
    if (dot) *dot = 0;
    
    char* lastSlash = strrchr(baseName, '/');
    if (!lastSlash) lastSlash = strrchr(baseName, '\\');
    if (lastSlash) lastSlash++;
    else lastSlash = baseName;
    

    char cheatPath[256];
    const char* cheatsDir = "/_pico/extras/cheats";
    
    File testFile;
    
    mini_snprintf(cheatPath, sizeof(cheatPath), "%s/*_%s.cc", cheatsDir, lastSlash);

    mini_snprintf(cheatPath, sizeof(cheatPath), "%s/%s.cc", cheatsDir, lastSlash);
    
    if (testFile.Open(cheatPath, FA_READ) == FR_OK)
    {
        testFile.Close();
        mini_snprintf(launchParameters->arguments, sizeof(launchParameters->arguments),
            "cheats=%s", cheatPath);
        launchParameters->argumentsLength = strlen(launchParameters->arguments) + 1;
    }
}
