#pragma once
#include "../../views/AppBarView.h"

class MaterialAppBarView : public AppBarView
{
    SHARED_ONLY(MaterialAppBarView)

public:
    void InitVram(const VramContext& vramContext) override;

private:
    MaterialAppBarView(int x, int y, Orientation orientation,
        int startButtonCount, int endButtonCount, const MaterialColorScheme* materialColorScheme);
};
