#include "common.h"
#include "NotoSansJP-Regular-10_nft2.h"
#include "NotoSansJP-Medium-7_5_nft2.h"
#include "NotoSansJP-Medium-10_nft2.h"
#include "NotoSansJP-Medium-10-clock_nft2.h"
#include "NotoSansJP-Medium-11_nft2.h"
#include "DefaultFontRepository.h"

const nft2_header_t* DefaultFontRepository::GetFont(FontType fontType) const
{
    switch (fontType)
    {
        case FontType::Regular10:
        {
            return (const nft2_header_t*)NotoSansJP_Regular_10_nft2;
        }
        case FontType::Medium7_5:
        {
            return (const nft2_header_t*)NotoSansJP_Medium_7_5_nft2;
        }
        case FontType::Medium10:
        {
            return (const nft2_header_t*)NotoSansJP_Medium_10_nft2;
        }
        case FontType::Medium10Clock:
        {
            return (const nft2_header_t*)NotoSansJP_Medium_10_clock_nft2;
        }
        case FontType::Medium11:
        {
            return (const nft2_header_t*)NotoSansJP_Medium_11_nft2;
        }
        default:
        {
            return nullptr;
        }
    }
}
