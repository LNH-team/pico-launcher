#pragma once
#include <memory>
#include "../views/AppBarView.h"
#include "gui/views/RecyclerViewBase.h"
#include "../FileRecyclerAdapter.h"
#include "../viewModels/RomBrowserViewModel.h"
#include "../Theme/IRomBrowserViewFactory.h"

class VBlankTextureLoader;
class ICoverRepository;

class RomBrowserDisplayMode
{
public:
    virtual bool IsVertical() const = 0;
    virtual bool ShowCoverOnTopScreen() const { return true; }
    virtual SharedPtr<AppBarView> CreateAppBarView(const IRomBrowserViewFactory* romBrowserViewFactory,
        int startButtonCount, int endButtonCount) const = 0;
    virtual SharedPtr<RecyclerViewBase> CreateRecyclerView(const IRomBrowserViewFactory* romBrowserViewFactory) const = 0;
    virtual SharedPtr<FileRecyclerAdapter> CreateRecyclerAdapter(
        RomBrowserViewModel* viewModel, const IThemeFileIconFactory* themeFileIconFactory,
        const IRomBrowserViewFactory* romBrowserViewFactory, VBlankTextureLoader* vblankTextureLoader) const = 0;
};
