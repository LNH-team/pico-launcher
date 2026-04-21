#include "common.h"
#include "gui/VramContext.h"
#include "romBrowser/views/IconButton2DView.h"
#include "MaterialAppBarView.h"

MaterialAppBarView::MaterialAppBarView(int x, int y, Orientation orientation,
    int startButtonCount, int endButtonCount, const MaterialColorScheme* materialColorScheme)
    : AppBarView(x, y, orientation, startButtonCount, endButtonCount, materialColorScheme)
{
    for (int i = 0; i < _startButtonCount + _endButtonCount; i++)
    {
        _buttons[i] = IconButton2DView::CreateShared(
            IconButtonView::Type::Standard,
            IconButtonView::State::NoToggle,
            md::sys::color::inverseOnSurface,
            materialColorScheme);
        AddChildTail(_buttons[i].GetPointer());
    }
}

void MaterialAppBarView::InitVram(const VramContext& vramContext)
{
    const auto objVramManager = vramContext.GetObjVramManager();
    if (objVramManager)
    {
        auto iconButtonVramToken = IconButton2DView::UploadGraphics(*objVramManager);
        for (int i = 0; i < _startButtonCount + _endButtonCount; i++)
        {
            static_cast<IconButton2DView*>(_buttons[i].GetPointer())->SetGraphics(iconButtonVramToken);
        }
    }
}
