#include "common.h"
#include <string.h>
#include "UnknownFileType.h"
#include "Nds/NdsFileType.h"
#include "Gba/GbaFileType.h"
#include "ExtensionFileTypeProvider.h"

static bool isNdsExtension(const char* extension)
{
    return !strcasecmp(extension, "nds")
        || !strcasecmp(extension, "srl")
        || !strcasecmp(extension, "dsi");
}

static bool isGbaExtension(const char* extension)
{
    return !strcasecmp(extension, "gba")
        || !strcasecmp(extension, "agb");
}

ExtensionFileTypeProvider::ExtensionFileTypeProvider(const AppSettings& appSettings)
    : _appSettings(appSettings)
{
    _customFileTypes = std::make_unique_for_overwrite<CustomFileType[]>(appSettings.numberOfFileAssociations);
    for (u32 i = 0; i < appSettings.numberOfFileAssociations; i++)
    {
        const FileType* baseFileType = nullptr;
        if (isGbaExtension(appSettings.fileAssociations[i].extension))
        {
            baseFileType = &GbaFileType::sInstance;
        }
        else if (isNdsExtension(appSettings.fileAssociations[i].extension))
        {
            baseFileType = &NdsFileType::sInstance;
        }
        _customFileTypes[i] = CustomFileType(&appSettings.fileAssociations[i], baseFileType);
    }
}

const FileType* ExtensionFileTypeProvider::GetFileType(const TCHAR* path) const
{
    const char* extension = strrchr(path, '.');
    if (extension)
    {
        extension++; // skip over dot

        for (u32 i = 0; i < _appSettings.numberOfFileAssociations; i++)
        {
            if (!strcasecmp(extension, _customFileTypes[i].GetShortName()))
            {
                return &_customFileTypes[i];
            }
        }

        if (isNdsExtension(extension))
        {
            return &NdsFileType::sInstance;
        }
    }

    return &UnknownFileType::sInstance;
}