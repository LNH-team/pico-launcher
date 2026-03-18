#include "common.h"
#include "NotoSansSC-Regular-10_nft2.h"
#include "NotoSansSC-Medium-7_5_nft2.h"
#include "NotoSansSC-Medium-10_nft2.h"
#include "NotoSansSC-Medium-11_nft2.h"
#include "DefaultFontRepository.h"

const nft2_header_t* DefaultFontRepository::GetFont(FontType fontType) const
{
    switch (fontType)
    {
        case FontType::Regular10:
        {
            return (const nft2_header_t*)NotoSansSC_Regular_10_nft2;
        }
        case FontType::Medium7_5:
        {
            return (const nft2_header_t*)NotoSansSC_Medium_7_5_nft2;
        }
        case FontType::Medium10:
        {
            return (const nft2_header_t*)NotoSansSC_Medium_10_nft2;
        }
        case FontType::Medium11:
        {
            return (const nft2_header_t*)NotoSansSC_Medium_11_nft2;
        }
        default:
        {
            return nullptr;
        }
    }
}
