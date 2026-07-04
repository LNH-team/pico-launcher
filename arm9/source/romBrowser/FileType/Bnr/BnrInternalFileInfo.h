#pragma once
#include <memory>
#include "../NdsBannerInternalFileInfo.h"
#include "fat/FastFileRef.h"

class File;

/// @brief Internal file info loaded from an external .bnr file patch.
class alignas(32) BnrInternalFileInfo : public NdsBannerInternalFileInfo
{
public:
    BnrInternalFileInfo(const FastFileRef& bnrFileRef, const char* gameCode);

private:
    void Load(std::unique_ptr<File> file, const char* gameCode);
};
