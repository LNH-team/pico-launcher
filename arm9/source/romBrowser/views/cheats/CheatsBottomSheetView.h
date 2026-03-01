#pragma once
#include <memory>
#include "romBrowser/views/BottomSheetView.h"
#include "gui/views/Label2DView.h"
#include "gui/views/RecyclerView.h"
#include "romBrowser/viewModels/CheatsViewModel.h"
#include "CheatsAdapter.h"
#include "CheatListItemView.h"

class MaterialColorScheme;
class IFontRepository;
class IVramManager;

class CheatsBottomSheetView : public BottomSheetView
{
public:
    CheatsBottomSheetView(std::unique_ptr<CheatsViewModel> viewModel,
        const MaterialColorScheme* materialColorScheme, const IFontRepository* fontRepository,
        FocusManager* focusManager);

    ~CheatsBottomSheetView() override
    {
        _cheatListRecycler.reset();
        if (_cheatsAdapter != nullptr)
        {
            delete _cheatsAdapter;
        }
    }

    void InitVram(const VramContext& vramContext) override;
    void Update() override;
    void Draw(GraphicsContext& graphicsContext) override;
    bool HandleInput(const InputProvider& inputProvider, FocusManager& focusManager) override;

    void Focus(FocusManager& focusManager) override
    {
        _cheatListRecycler->Focus(focusManager);
    }

private:
    std::unique_ptr<CheatsViewModel> _viewModel;
    Label2DView _titleLabel;
    std::unique_ptr<RecyclerView> _cheatListRecycler;
    CheatsAdapter* _cheatsAdapter = nullptr;
    const MaterialColorScheme* _materialColorScheme;
    const IFontRepository* _fontRepository;
    IVramManager* _objVramManager = nullptr;
    FocusManager* _focusManager;
    CheatListItemView::VramOffsets _vramOffsets;
    u32 _savedVramState = 0;

    void UpdateTitle();
    void UpdateCheatList(int initialSelectedIndex = 0);
};
