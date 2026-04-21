#pragma once
#include "../../views/AppBarView.h"

class CustomAppBarView : public AppBarView
{
    SHARED_ONLY(CustomAppBarView)

public:
    void Draw(GraphicsContext& graphicsContext) override;

private:
    u32 _scrimTexVramOffset = 0;
    u32 _scrimPlttVramOffset = 0;

    CustomAppBarView(int x, int y, Orientation orientation,
        int startButtonCount, int endButtonCount, const MaterialColorScheme* materialColorScheme,
        u32 scrimTexVramOffset, u32 scrimPlttVramOffset);
};
