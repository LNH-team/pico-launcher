#pragma once
#include "../NdsBannerBase.h"
#include "fat/FastFileRef.h"

class File;

/// @brief Internal file info loaded from an external .bnr file patch.
class alignas(32) BnrInternalFileInfo : public NdsBannerBase
{
public:
    BnrInternalFileInfo(const FastFileRef& bnrFileRef, const char* originalGameCode = nullptr);
    BnrInternalFileInfo(const TCHAR* path, const char* originalGameCode = nullptr);

private:
    void Load(File& file, const char* originalGameCode);
};
