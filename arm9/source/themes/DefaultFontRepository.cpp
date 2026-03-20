#include "common.h"
#include "WenQuanYi-Bitmap-12_nft2.h"
#include "DefaultFontRepository.h"

const nft2_header_t* DefaultFontRepository::GetFont(FontType fontType) const
{
    (void)fontType;
    return (const nft2_header_t*)WenQuanYi_Bitmap_12_nft2;
}
