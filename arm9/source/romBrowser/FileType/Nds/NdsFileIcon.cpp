#include "common.h"
#include <libtwl/dma/dmaNitro.h>
#include "gui/PaletteManager.h"
#include "gui/OamManager.h"
#include "gui/OamBuilder.h"
#include "gui/GraphicsContext.h"
#include "gui/palette/DirectPalette.h"
#include "NdsFileIcon.h"

NdsFileIcon::NdsFileIcon(const nds_banner_t* banner)
    : _banner(banner), _animTokenIdx(0), _lastAnimToken(0)
    , _animLength(0), _loop(false)
{
    _animated = _banner->header.version >= NDS_BANNER_VERSION_103
             && _banner->animation.animTokens[0].duration != 0;
    if (_animated)
    {
        _lastAnimToken = NDS_BANNER_ANIM_TOKEN_COUNT - 1;
        int length = 0;
        _loop = true;
        for (int i = 0; i < NDS_BANNER_ANIM_TOKEN_COUNT; i++)
        {
            _tokenStartTimes[i] = length;
            const auto& token = _banner->animation.animTokens[i];
            if (token.duration == NDS_BANNER_ANIM_DURATION_CONTROL_FRAME)
            {
                _loop = token.control != NDS_BANNER_ANIM_CONTROL_STOP;
                _lastAnimToken = i - 1;
                break;
            }
            else
            {
                length += token.duration;
            }
        }
        _animLength = length;
        _tokenStartTimes[NDS_BANNER_ANIM_TOKEN_COUNT] = _animLength;
    }
}

void NdsFileIcon::UploadGraphics(vu16* vram){
    _vramAddress = vram;
    _currentVramSlot = 0;
    _currentGfxIdx = -1;
    if (!_animated)
    {
        dma_ntrCopy32(3, _banner->iconGfx, vram, sizeof(_banner->iconGfx));
    }
}

void NdsFileIcon::Update()
{
    if (!_animated)
    {
        return;
    }

    _frame %= _animLength;

    if ((!_loop && _frame >= _animLength) || _lastAnimToken == 0)
    {
        _animTokenIdx = _lastAnimToken;
    }
    else
    {
        u32 start = 0;
        u32 end = _lastAnimToken;

        while (start <= end)
        {
            const u32 mid = (start + end) >> 1;
            u32 midTime = _tokenStartTimes[mid];
            if (midTime <= _frame && _frame < _tokenStartTimes[mid + 1])
            {
                start = mid;
                break;
            }
            else if (_frame < midTime)
            {
                end = mid - 1;
            }
            else
            {
                start = mid + 1;
            }
        }

        _animTokenIdx = start;
    }

    if (++_frame == _animLength)
        _frame = 0;
}

void NdsFileIcon::Draw(GraphicsContext& graphicsContext, const Rgb<8, 8, 8>& backgroundColor)
{
    if (!graphicsContext.IsVisible(Rectangle(_position, 32, 32)) ||
        _vramAddress == nullptr)    {
        return;
    }

    const u16* palette = _animated
        ? _banner->animation.iconPltt[_banner->animation.animTokens[_animTokenIdx].plttIdx]
        : _banner->iconPltt;

    u32 paletteRowIdx = graphicsContext.GetPaletteManager().AllocRow(
        DirectPalette(palette), _position.y, _position.y + 32);

    u32 vramOffset = _vramOffset;
    
    if (_animated)
    {
        int gfxIdx = _banner->animation.animTokens[_animTokenIdx].gfxIdx;
        if (gfxIdx != _currentGfxIdx)
        {
            _currentVramSlot = 1 - _currentVramSlot;
            _currentGfxIdx = gfxIdx;
            dma_ntrCopy32(3, &_banner->animation.iconGfx[gfxIdx][0], (u8*)_vramAddress + (_currentVramSlot * NDS_BANNER_ICON_SIZE), NDS_BANNER_ICON_SIZE);
        }

        vramOffset += _currentVramSlot * NDS_BANNER_ICON_SIZE;
    }

    auto builder = OamBuilder::OamWithSize<32, 32>(
            _position, vramOffset >> 7)
        .WithPalette16(paletteRowIdx)
        .WithPriority(graphicsContext.GetPriority());

    if (_animated)
    {
        builder
            .WithHFlip(_banner->animation.animTokens[_animTokenIdx].hFlip)
            .WithVFlip(_banner->animation.animTokens[_animTokenIdx].vFlip);
    }

    gfx_oam_entry_t* oam = graphicsContext.GetOamManager().AllocOams(1);
    builder.Build(oam[0]);
}