#include "common.h"
#include <bit>
#include "gui/OamManager.h"
#include "gui/OamBuilder.h"
#include "gui/IVramManager.h"
#include "gui/VramContext.h"
#include "gui/GraphicsContext.h"
#include "core/StringUtil.h"
#include "gui/palette/GradientPalette.h"
#include "LabelView.h"

#define MARQUEE_START_FRAMES    60
#define MARQUEE_STEP_FRAMES     3
#define MARQUEE_END_FRAMES      90

LabelView::LabelView(u32 width, u32 height, u32 maxStringLength, const nft2_header_t* font, bool a5i3)
    : _width(width), _height(height)
    , _maxStringLength(maxStringLength), _font(font)
    , _hAlign(Alignment::Start)
    , _stringWidth(0), _newStringWidth(0), _a5i3(a5i3)
{
    _textBuffer = std::make_unique_for_overwrite<char16_t[]>(_maxStringLength + 1);
    _textBuffer[0] = 0;
    if (_a5i3)
    {
        _actualWidth = std::bit_ceil(width);
        _actualHeight = height;
        _tileBufferSize = _actualWidth * _actualHeight;
    }
    else
    {
        _actualWidth = (width + 31) & ~31;
        _actualHeight = (height + 15) & ~15;
        _tileBufferSize = (_actualWidth * _actualHeight) >> 1;
    }

    _tileBuffer = std::make_unique_for_overwrite<u8[]>(_tileBufferSize);
    SetText(u"");
}

void LabelView::Update()
{
    if (_ellipsisStyleChanged)
    {
        RestartMarquee();
        UpdateTileBuffer();
        _ellipsisStyleChanged = false;
    }
    else if (_ellipsisStyle == EllipsisStyle::Marquee)
    {
        UpdateMarquee();
    }
}

void LabelView::SetTextBuffer(const char* text)
{
    StringUtil::Copy(_textBuffer.get(), text, _maxStringLength + 1);
    RestartMarquee();
}

void LabelView::SetTextBuffer(const char16_t* text)
{
    StringUtil::Copy(_textBuffer.get(), text, _maxStringLength + 1);
    RestartMarquee();
}

void LabelView::SetTextBuffer(const char16_t* text, u32 length)
{
    u32 copyLength = std::min(length, _maxStringLength);
    if (copyLength > 0)
        memcpy(_textBuffer.get(), text, copyLength * 2);
    _textBuffer[copyLength] = 0;
    RestartMarquee();
}

void LabelView::UpdateTileBuffer()
{
    memset(_tileBuffer.get(), 0, _tileBufferSize);
    if (_textBuffer[0] != 0)
    {
        nft2_string_render_params_t renderParams;
        renderParams.x = 0;
        renderParams.y = 0;
        renderParams.width = _width;
        renderParams.height = _height;
        renderParams.a5i3 = _a5i3;
        if (_ellipsisStyle == EllipsisStyle::Ellipsis)
        {
            nft2_renderStringEllipsis(_font, _textBuffer.get(), _tileBuffer.get(), _actualWidth, &renderParams, u" ... ");
        }
        else
        {
            if (_ellipsisStyle == EllipsisStyle::Marquee)
            {
                renderParams.x = -_marqueeOffset;
                renderParams.onlyRenderWholeGlyphs = false;
            }
            nft2_renderString(_font, _textBuffer.get(), _tileBuffer.get(), _actualWidth, &renderParams);
        }
        _newStringWidth = renderParams.textWidth;
    }
    else
    {
        _newStringWidth = 0;
    }

    // The buffer now reflects the current ellipsis style, so there is no more
    // pending work for Update() to do. Clearing this here (rather than only in
    // Update(), after the fact) matters because SetEllipsisStyle()+SetText()/
    // SetTextAsync() are called back-to-back from the banner list's background
    // IO task while binding a row. If we left the flag set, the next frame's
    // Update() (main thread) would redundantly call UpdateTileBuffer() again on
    // this same LabelView - and by then the row may already have been recycled
    // and be mid-bind for a *different* item on the IO thread, so the two
    // unsynchronized UpdateTileBuffer() calls race on the same _textBuffer/
    // _tileBuffer, producing blank or wrong-looking list rows. Every caller of
    // SetEllipsisStyle() in this codebase immediately follows it with a
    // SetText()/SetTextAsync() call, so the render this function just did is
    // already up to date - there is nothing left for Update() to redo.
    _ellipsisStyleChanged = false;
}

QueueTask<void> LabelView::UpdateTileBufferAsync(TaskQueueBase* taskQueue)
{
    return taskQueue->Enqueue([this] (const vu8& canceled)
    {
        UpdateTileBuffer();
        return TaskResult<void>::Completed();
    });
}

void LabelView::SetText(const char* text)
{
    SetTextBuffer(text);
    UpdateTileBuffer();
}

void LabelView::SetText(const char16_t* text)
{
    SetTextBuffer(text);
    UpdateTileBuffer();
}

void LabelView::SetText(const char16_t* text, u32 length)
{
    SetTextBuffer(text, length);
    UpdateTileBuffer();
}

QueueTask<void> LabelView::SetTextAsync(TaskQueueBase* taskQueue, const char* text)
{
    SetTextBuffer(text);
    return UpdateTileBufferAsync(taskQueue);
}

QueueTask<void> LabelView::SetTextAsync(TaskQueueBase* taskQueue, const char16_t* text)
{
    SetTextBuffer(text);
    return UpdateTileBufferAsync(taskQueue);
}

QueueTask<void> LabelView::SetTextAsync(TaskQueueBase* taskQueue, const char16_t* text, u32 length)
{
    SetTextBuffer(text, length);
    return UpdateTileBufferAsync(taskQueue);
}

void LabelView::RestartMarquee()
{
    _marqueeOffset = 0;
    _marqueeCounter = MARQUEE_START_FRAMES;
    _marqueeState = MarqueeState::StartWait;
}

void LabelView::UpdateMarquee()
{
    UpdateTileBuffer();
    if (_newStringWidth <= _width)
    {
        _marqueeOffset = 0;
    }
    else
    {
        switch (_marqueeState)
        {
            case MarqueeState::StartWait:
            {
                if (--_marqueeCounter <= 0)
                {
                    _marqueeState = MarqueeState::Moving;
                    _marqueeOffset = 0;
                    _marqueeCounter = MARQUEE_STEP_FRAMES;
                }
                break;
            }
            case MarqueeState::Moving:
            {
                if (--_marqueeCounter == 0)
                {
                    _marqueeCounter = MARQUEE_STEP_FRAMES;
                    _marqueeOffset++;
                    if (_newStringWidth - _marqueeOffset < _width)
                    {
                        _marqueeState = MarqueeState::EndWait;
                        _marqueeCounter = MARQUEE_END_FRAMES;
                    }
                }
                break;
            }
            case MarqueeState::EndWait:
            {
                if (--_marqueeCounter <= 0)
                {
                    _marqueeState = MarqueeState::StartWait;
                    _marqueeCounter = MARQUEE_START_FRAMES;
                    _marqueeOffset = 0;
                }
                break;
            }
        }
    }
}
