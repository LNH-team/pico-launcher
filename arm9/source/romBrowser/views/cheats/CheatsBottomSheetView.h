#pragma once
#include <memory>
#include "core/SharedPtr.h"
#include "romBrowser/views/BottomSheetView.h"
#include "gui/views/Label2DView.h"
#include "gui/views/RecyclerView.h"
#include "romBrowser/viewModels/CheatsViewModel.h"
#include "CheatsAdapter.h"
#include "CheatListItemView.h"
#include "romBrowser/views/IconButton2DView.h"

class MaterialColorScheme;
class IFontRepository;
class IVramManager;

/// @brief Bottom sheet for browsing and enabling/disabling cheats.
class CheatsBottomSheetView : public BottomSheetView
{
    SHARED_ONLY(CheatsBottomSheetView)

public:
    void InitVram(const VramContext& vramContext) override;
    void Update() override;
    void Draw(GraphicsContext& graphicsContext) override;
    SharedPtr<View> MoveFocus(const SharedPtr<View>& currentFocus, FocusMoveDirection direction, View* source) override;
    bool HandleInput(const InputProvider& inputProvider, FocusManager& focusManager) override;

    void Focus(FocusManager& focusManager) override;

protected:
    void Close() override;

private:
    SharedPtr<CheatsViewModel> _viewModel;
    SharedPtr<Label2DView> _titleLabel;
    SharedPtr<Label2DView> _secondaryLabel;
    SharedPtr<Label2DView> _descriptionLabel;
    SharedPtr<RecyclerView> _cheatListRecycler;
    SharedPtr<CheatsAdapter> _cheatsAdapter;
    SharedPtr<IconButton2DView> _upButton;
    const MaterialColorScheme* _materialColorScheme;
    const IFontRepository* _fontRepository;
    IVramManager* _objVramManager = nullptr;
    FocusManager* _focusManager;
    CheatListItemView::VramOffsets _vramOffsets;
    u32 _savedVramState = 0;
    const CheatEntry* _currentCheatCategory = nullptr;

    CheatsBottomSheetView(SharedPtr<CheatsViewModel> viewModel,
        const MaterialColorScheme* materialColorScheme, const IFontRepository* fontRepository,
        FocusManager* focusManager);

    void UpdateCheatList();
    void UpdateDescriptionText();
    u32 LoadSprite(IVramManager& vramManager, const unsigned int* tiles, u32 tilesLength) const;
};
