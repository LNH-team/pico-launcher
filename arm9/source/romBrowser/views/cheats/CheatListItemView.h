#pragma once
#include "gui/views/ViewContainer.h"
#include "gui/views/Label2DView.h"
#include "cheats/CheatEntry.h"

class MaterialColorScheme;
class IFontRepository;

class CheatListItemView : public ViewContainer
{
public:
    struct VramOffsets
    {
        u32 folderIconVramOffset = 0;
        u32 checkboxUncheckedIconVramOffset = 0;
        u32 checkboxCheckedIconVramOffset = 0;
        u32 cheatSelectorVramOffset = 0;
    };

    CheatListItemView(const VramOffsets& vramOffsets, const MaterialColorScheme* materialColorScheme, const IFontRepository* fontRepository);

    static void SetFastScrollEnabled(bool enabled)
    {
        sFastScrollEnabled = enabled;
    }

    static void RequestScrollStartNow()
    {
        sForceScrollStartRequestId++;
    }

    void Update() override;
    void Draw(GraphicsContext& graphicsContext) override;

    Rectangle GetBounds() const override
    {
        return Rectangle(_position.x, _position.y, 224, 24);
    }

    void SetName(const char* name)
    {
        SetBaseName(name);
    }

    void SetEntry(const CheatEntry* cheatEntry)
    {
        _cheatEntry = cheatEntry;
        SetBaseName(cheatEntry->GetName());
        _iconVramOffset = cheatEntry->IsCheatCategory()
            ? _vramOffsets.folderIconVramOffset
            : _vramOffsets.checkboxUncheckedIconVramOffset;
    }

private:
    enum class NameScrollPhase
    {
        PauseAtStart,
        Scrolling
    };

    static bool sFastScrollEnabled;
    static u32 sForceScrollStartRequestId;

    const nft2_header_t* _nameFont;
    Label2DView _nameLabel;
    VramOffsets _vramOffsets;
    const MaterialColorScheme* _materialColorScheme;
    u32 _iconVramOffset = 0;
    const CheatEntry* _cheatEntry = nullptr;
    NameScrollPhase _nameScrollPhase = NameScrollPhase::PauseAtStart;
    int _nameScrollPauseFrames = 0;
    int _nameScrollOffsetQ8 = 0;
    int _nameScrollCycleQ8 = 0;
    bool _nameScrollPrepared = false;
    u32 _handledForceScrollStartRequestId = 0;
    char _baseName[96] = { 0 };

    void ResetNameScroll();
    void SetBaseName(const char* name);
    void PrepareNameScrollIfNeeded();
    void UpdateNameScroll();
};
