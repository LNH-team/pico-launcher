#pragma once
#include "../NdsBannerInternalFileInfo.h"
#include "fat/FastFileRef.h"

/// @brief Internal file info loaded from an external .bnr file.
class alignas(32) BnrInternalFileInfo : public NdsBannerInternalFileInfo
{
public:
    BnrInternalFileInfo(const FastFileRef& bnrFileRef, const char* gameCode);
};
