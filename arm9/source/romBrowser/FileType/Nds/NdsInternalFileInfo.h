#pragma once
#include "../NdsBannerBase.h"
#include "ndsBanner.h"
#include "fat/FastFileRef.h"

/// @brief Internal file info for nds roms.
class alignas(32) NdsInternalFileInfo : public NdsBannerBase
{
public:
    explicit NdsInternalFileInfo(const FastFileRef& fastFileRef);

    const nds_banner_t& GetBanner() const { return _banner; }
};
