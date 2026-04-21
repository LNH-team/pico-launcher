#include "common.h"
#include <string.h>
#include <algorithm>
#include <libtwl/mem/memVram.h>
#include "gui/Gx.h"
#include "gui/GraphicsContext.h"
#include "gui/materialDesign.h"
#include "gui/input/InputProvider.h"
#include "core/math/SinTable.h"
#include "romBrowser/FileType/FileCover.h"
#include "CoverFlowRecyclerView.h"

#define COVER_SPACING       6

void CoverFlowRecyclerView::Update()
{
    if (_itemCount == 0)
    {
        return;
    }

    _scrollAnimator.Update();

    int rangeStartIndex = _scrollAnimator.GetValue().Int() - 4;
    int rangeEndIndex = _scrollAnimator.GetValue().Int() + 1 + 4;
    rangeStartIndex = std::clamp(rangeStartIndex, 0, (int)_itemCount - 1);
    rangeEndIndex = std::clamp(rangeEndIndex, 0, (int)_itemCount);

    if (_curRangeStart != rangeStartIndex || _curRangeLength != rangeEndIndex - rangeStartIndex)
    {
        LOG_DEBUG("range: %d - %d\n", rangeStartIndex, rangeEndIndex - 1);
        if (_curRangeLength != 0)
        {
            if (_curRangeStart < rangeStartIndex)
                ReleaseRange(_curRangeStart, rangeStartIndex);

            if (rangeEndIndex < _curRangeStart + _curRangeLength)
                ReleaseRange(rangeEndIndex, _curRangeStart + _curRangeLength);
        }

        BindRange(rangeStartIndex, rangeEndIndex);

        _curRangeStart = rangeStartIndex;
        _curRangeLength = rangeEndIndex - rangeStartIndex;
    }

    for (u32 i = _viewPoolFreeCount; i < _viewPool.size(); i++)
    {
        UpdateItemPosition(i);
        _viewPool[i].view->SetPosition(_viewPoolEx[i].xPosition.Int(), 32 + 80);
        _viewPool[i].view->Update();
    }
}

void CoverFlowRecyclerView::Draw(GraphicsContext& graphicsContext)
{
    Gx::PolygonAttr(GX_LIGHTMASK_0, GX_POLYGON_MODE_MODULATE, GX_DISPLAY_MODE_FRONT,
        false, false, false, GX_DEPTH_FUNC_LESS, false, 31, 0);

    Gx::MtxMode(GX_MTX_MODE_PROJECTION);
    Gx::MtxPush();
    fix32<12> near = 2;
    fix32<12> far = 100;
    fix32<12> left = -1;
    fix32<12> right = 1;
    fix32<12> top = -1;
    fix32<12> bottom = 1;
    mtx44_t perspectiveMtx =
    {
        ((2 * near) / (right - left)).GetRawValue(), 0, 0, 0,
        0, ((2 * near) / (top - bottom)).GetRawValue(), 0, 0,
        ((right + left) / (right - left)).GetRawValue(), ((top + bottom) / (top - bottom)).GetRawValue(), ((far + near) / (near - far)).GetRawValue(), -FX32_ONE * 64,
        0, 0, ((2 * far * near) / (near - far)).GetRawValue(), 0
    };
    Gx::MtxLoad44(&perspectiveMtx);
    Gx::MtxMode(GX_MTX_MODE_POSITION_VECTOR);
    Gx::MtxIdentity();
    Gx::LightVector(0, 0, 0, -0x200);
    Gx::LightColor(0, 0x7FFF);
    Gx::DiffuseAmbient(Rgb<5, 5, 5>(16, 16, 16), false, Rgb<5, 5, 5>(20, 20, 20));
    Gx::SpecularEmission(0, false, 0);
    // Gx::MtxScale(2.0 / (128.0 / 64.0), 2.0 / (96.0 / 64.0), 1);
    Gx::MtxTranslate(-128, /*-96*/-128, fix32<12>(-4));

    for (u32 i = _viewPoolFreeCount; i < _viewPool.size(); i++)
    {
        graphicsContext.SetPolygonId(i);
        Gx::MtxPush();
        {
            fix32<12> x = _viewPoolEx[i].xPosition;
            u32 angle = _viewPoolEx[i].yAngle;
            fix32<12> z = _viewPoolEx[i].zPosition;
            auto sinCos = gSinTable.SinCos(angle);
            mtx43_t rotMtx =
            {
                fix32<18>(sinCos.cos).GetRawValue(), 0, -fix32<12>(sinCos.sin).GetRawValue(),
                0, FX32_ONE * 64, 0,
                fix32<18>(sinCos.sin).GetRawValue(), 0, fix32<12>(sinCos.cos).GetRawValue(),
                x.GetRawValue() + 2048, 148 * FX32_ONE + 2048, 0
            };
            Gx::MtxMult43(&rotMtx);
            Gx::MtxTranslate(0, 0, z / 64);
            Gx::MtxScale(1.0 / 8, /*1.0 / 8*/1.0/*/6*/, 1);
            _viewPool[i].view->Draw(graphicsContext);
        }
        Gx::MtxPop(1);
    }

    Gx::MtxMode(GX_MTX_MODE_PROJECTION);
    Gx::MtxPop(1);
    Gx::MtxMode(GX_MTX_MODE_POSITION_VECTOR);
}

bool CoverFlowRecyclerView::HandleInput(const InputProvider& inputProvider, FocusManager& focusManager)
{
    if (_itemCount != 0 && inputProvider.Triggered(InputKey::L | InputKey::R))
    {
        int direction = inputProvider.Triggered(InputKey::L) ? -1 : 1;
        int selected = std::clamp(_selectedItem->itemIdx + 10 * direction, 0, (int)_itemCount - 1);

        focusManager.Unfocus();
        SetSelectedItem(selected, false);
        focusManager.Focus(_selectedItem->view);
        return true;
    }

    return View::HandleInput(inputProvider, focusManager);
}

void CoverFlowRecyclerView::HandlePenDown(const Point& touchPoint, FocusManager& focusManager)
{
    if (GetBounds().Contains(touchPoint))
    {
        _penDown = true;
        _penDownPosition = touchPoint;
        _hasScrollStarted = false;
        _penDownScrollOffset = _scrollAnimator.GetValue();

        if (_itemCount > 0)
        {
            _selectedItem->view->HandlePenDown(touchPoint, focusManager);
        }
    }
}

void CoverFlowRecyclerView::HandlePenMove(const Point& touchPoint, FocusManager& focusManager)
{
    if (!_penDown)
    {
        return;
    }

    if (!_hasScrollStarted)
    {
        if (_itemCount > 0)
        {
            _selectedItem->view->HandlePenMove(touchPoint, focusManager);
        }

        int dx = touchPoint.x - _penDownPosition.x;
        int dy = touchPoint.y - _penDownPosition.y;
        if (dx * dx + dy * dy > 7 * 7)
        {
            bool shouldScrollStart = std::abs(touchPoint.x - _penDownPosition.x) > std::abs(touchPoint.y - _penDownPosition.y);
            if (shouldScrollStart)
            {
                _hasScrollStarted = true;
            }
            else
            {
                _penDown = false; //wrong direction drag, so cancel it
            }

            if (_itemCount > 0)
            {
                _selectedItem->view->HandlePenUp(Point(-1, -1), focusManager);
            }
        }
    }
    else
    {
        fix32<12> newScrollOffset = _penDownScrollOffset + fix32<12>(_penDownPosition.x - touchPoint.x) * (2.5 / COVER_WIDTH);
        if (newScrollOffset < 0)
        {
            newScrollOffset = 0;
            _penDownScrollOffset = 0;
            _penDownPosition.x = touchPoint.x;
        }
        else if (newScrollOffset > (int)_itemCount - 1)
        {
            newScrollOffset = (int)_itemCount - 1;
            _penDownScrollOffset = newScrollOffset;
            _penDownPosition.x = touchPoint.x;
        }

        _scrollAnimator = Animator<fix32<12>>(newScrollOffset);
    }
}

void CoverFlowRecyclerView::HandlePenUp(const Point& lastTouchPoint, FocusManager& focusManager)
{
    if (_hasScrollStarted)
    {
        SetSelectedItem((_scrollAnimator.GetValue() + 0.5).Int(), false);
    }

    if (_itemCount > 0)
    {
        _selectedItem->view->HandlePenUp(lastTouchPoint, focusManager);
        focusManager.Focus(_selectedItem->view);
    }

    _penDown = false;
}

void CoverFlowRecyclerView::SetSelectedItem(int itemIdx, bool initial)
{
    CoverFlowRecyclerViewBase::SetSelectedItem(itemIdx, initial);

    if (itemIdx < 0 || itemIdx >= (int)_itemCount)
    {
        return;
    }

    if (initial)
    {
        _scrollAnimator = Animator<fix32<12>>(itemIdx);
    }
    else
    {
        _scrollAnimator.Goto(itemIdx, md::sys::motion::duration::medium4, &md::sys::motion::easing::standard);
    }
}

void CoverFlowRecyclerView::UpdateItemPosition(int viewPoolIndex)
{
    auto& item = _viewPool[viewPoolIndex];
    auto& itemEx = _viewPoolEx[viewPoolIndex];
    int itemIndex = item.itemIdx;
    fix32<12> absOffsetFromCenter = (itemIndex - _scrollAnimator.GetValue()).Abs();
    fix32<12> initialOffsetFromCenter = absOffsetFromCenter.Clamp(0, 1);
    fix32<12> x = (COVER_HEIGHT / 4) * initialOffsetFromCenter + COVER_SPACING * absOffsetFromCenter;
    int angle = -(fix32<16>((absOffsetFromCenter * 10 + 45 * initialOffsetFromCenter).Clamp(-90, 90)) / 360).GetRawValue();
    fix32<12> z = -absOffsetFromCenter * 30 - 20 * initialOffsetFromCenter;
    if (itemIndex <= _scrollAnimator.GetValue().Int())
    {
        x = -x;
        angle = -angle;
    }

    itemEx.yAngle = angle;
    itemEx.xPosition = x + 256 / 2;
    itemEx.zPosition = z;
}
