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

void LabelView::SetTextBuffer(const char* text)
{
    StringUtil::Copy(_textBuffer.get(), text, _maxStringLength + 1);
}

void LabelView::SetTextBuffer(const char16_t* text)
{
    StringUtil::Copy(_textBuffer.get(), text, _maxStringLength + 1);
}

void LabelView::SetTextBuffer(const char16_t* text, u32 length)
{
    u32 copyLength = std::min(length, _maxStringLength);
    if (copyLength > 0)
        memcpy(_textBuffer.get(), text, copyLength * 2);
    _textBuffer[copyLength] = 0;
}

void LabelView::UpdateTileBuffer()
{
    memset(_tileBuffer.get(), 0, _tileBufferSize);
    if (_textBuffer[0] != 0)
    {
        nft2_string_render_params_t renderParams;
        renderParams.x = _textOffsetX;
        renderParams.y = 0;
        renderParams.width = _width;
        renderParams.height = _height;
        renderParams.a5i3 = _a5i3;
        if (_ellipsis)
            nft2_renderStringEllipsis(_font, _textBuffer.get(), _tileBuffer.get(), _actualWidth, &renderParams, u" ... ");
        else
            nft2_renderString(_font, _textBuffer.get(), _tileBuffer.get(), _actualWidth, &renderParams);
        _newStringWidth = renderParams.textWidth;
    }
    else
    {
        _newStringWidth = 0;
    }
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
