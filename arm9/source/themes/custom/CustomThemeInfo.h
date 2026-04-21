#pragma once
#include "CustomBannerListTextElementInfo.h"
#include "CustomBottomIconInfo.h"
#include "CustomTopCoverInfo.h"
#include "CustomTopIconInfo.h"
#include "CustomTextElementInfo.h"

struct CustomThemeInfo
{
    CustomTopIconInfo topIconInfo;
    CustomTopTextElementInfo topBannerTextLine0Info;
    CustomTopTextElementInfo topBannerTextLine1Info;
    CustomTopTextElementInfo topBannerTextLine2Info;
    CustomTopTextElementInfo topFileNameTextInfo;
    CustomTopCoverInfo topCoverInfo;

    CustomBottomIconInfo gridIconInfo;

    CustomBottomIconInfo bannerListIconInfo;
    CustomBannerListTextElementInfo bannerListTextLine0Info;
    CustomBannerListTextElementInfo bannerListTextLine1Info;
    CustomBannerListTextElementInfo bannerListTextLine2Info;
};
