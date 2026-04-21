#pragma once
#include "gui/views/ViewContainer.h"
#include "gui/views/Label2DView.h"
#include "cheats/CheatEntry.h"
#include "romBrowser/viewModels/CheatsViewModel.h"

class MaterialColorScheme;
class IFontRepository;

/// @brief List item view for the cheats panel, representing a single cheat or cheat category.
class CheatListItemView : public ViewContainer
{
    SHARED_ONLY(CheatListItemView)

public:
    struct VramOffsets
    {
        u32 folderIconVramOffset = 0;
        u32 checkboxUncheckedIconVramOffset = 0;
        u32 checkboxCheckedIconVramOffset = 0;
        u32 cheatSelectorVramOffset = 0;
    };

    void Update() override;
    void Draw(GraphicsContext& graphicsContext) override;
    bool HandleInput(const InputProvider& inputProvider, FocusManager& focusManager) override;
    void HandlePenDown(const Point& touchPoint, FocusManager& focusManager) override;
    void HandlePenMove(const Point& touchPoint, FocusManager& focusManager) override;
    void HandlePenUp(const Point& lastTouchPoint, FocusManager& focusManager) override;

    Rectangle GetBounds() const override
    {
        return Rectangle(_position.x, _position.y, 224, 24);
    }

    void SetName(const char* name)
    {
        _nameLabel->SetText(name);
    }

    void SetEntry(const CheatEntry* cheatEntry, int index)
    {
        _cheatEntry = cheatEntry;
        _index = index;
        _nameLabel->SetText(cheatEntry->GetName());
        if (cheatEntry->IsCheatCategory())
        {
            _iconVramOffset = _vramOffsets.folderIconVramOffset;
        }
    }

private:
    SharedPtr<CheatsViewModel> _viewModel;
    SharedPtr<Label2DView> _nameLabel;
    VramOffsets _vramOffsets;
    const MaterialColorScheme* _materialColorScheme;
    u32 _iconVramOffset = 0;
    const CheatEntry* _cheatEntry = nullptr;
    int _index = -1;
    bool _penDown = false;

    CheatListItemView(SharedPtr<CheatsViewModel> viewModel, const VramOffsets& vramOffsets,
        const MaterialColorScheme* materialColorScheme, const IFontRepository* fontRepository);
};
