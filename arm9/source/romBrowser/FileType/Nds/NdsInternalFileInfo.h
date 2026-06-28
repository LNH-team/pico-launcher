#pragma once
#include "../NdsBannerInternalFileInfo.h"
#include "ndsBanner.h"
#include "fat/FastFileRef.h"

/// @brief Internal file info for nds roms.
class alignas(32) NdsInternalFileInfo : public NdsBannerInternalFileInfo
{
public:
    explicit NdsInternalFileInfo(const FastFileRef& fastFileRef);

    const nds_banner_t& GetBanner() const { return _banner; }
};
