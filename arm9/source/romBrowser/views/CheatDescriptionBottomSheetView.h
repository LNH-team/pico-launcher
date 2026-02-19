#pragma once
#include "BottomSheetView.h"
#include "gui/views/Label2DView.h"
#include "themes/material/MaterialColorScheme.h"
#include "themes/IFontRepository.h"
#include <array>
#include <memory>

class IRomBrowserController;

class CheatDescriptionBottomSheetView : public BottomSheetView
{
public:
    static constexpr int DialogTypeId = 0x43484454; // "CHDT" in hex
    
    CheatDescriptionBottomSheetView(
        IRomBrowserController* romBrowserController,
        const MaterialColorScheme* materialColorScheme,
        const IFontRepository* fontRepository,
        const char* cheatName,
        const char* description,
        const char* gameCode,
        u32 crc);

    int GetDialogTypeId() const override { return DialogTypeId; }
    
    const char* GetGameCode() const { return _gameCode; }
    u32 GetCrc() const { return _crc; }
    
    void InitVram(const VramContext& vramContext) override;
    void VBlank() override;
    void Update() override;
    void Draw(GraphicsContext& graphicsContext) override;
    
    void Focus(FocusManager& focusManager) override;
    bool HandleInput(const InputProvider& inputProvider, FocusManager& focusManager) override;

    Rectangle GetBounds() const override
    {
        return Rectangle(_position.x, _position.y, 256 - _position.x, 192 - _position.y);
    }

private:
    IRomBrowserController* _romBrowserController;
    const MaterialColorScheme* _materialColorScheme;
    const IFontRepository* _fontRepository;
    
    char _gameCode[5];  
    u32 _crc;
    
    Label2DView _titleLabel;
    
    // Labels for displaying description
    enum { MAX_DESC_LINES = 8 };
    std::array<std::unique_ptr<Label2DView>, MAX_DESC_LINES> _descriptionLabels;
    int _descriptionLineCount = 0;
    
    static constexpr int kTitleY = 12;
    static constexpr int kDescStartY = 44;
    static constexpr int kDescSpacing = 16;
    static constexpr int kDescX = 12;
    static constexpr int kDescWidth = 230;
    
    void SetDescription(const char* description);
    static const char* WrapNextLine(const nft2_header_t* font, const char* text, u32 maxWidth,
        char16_t* out, int outMax);
};
