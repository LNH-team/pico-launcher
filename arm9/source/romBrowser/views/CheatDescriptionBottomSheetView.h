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

    static constexpr int kMaxTitleLines = 6;
    static constexpr int kMaxDescriptionLines = 16;
    static constexpr int kMaxLineChars = 192;
    static constexpr int kMaxVisibleDescriptionLines = 8;

    std::unique_ptr<Label2DView> _titleLabels[kMaxTitleLines];
    int _titleLineCount = 0;

    std::array<std::unique_ptr<Label2DView>, kMaxVisibleDescriptionLines> _descriptionLabels;
    int _descriptionTotalLineCount = 0;
    int _descriptionScrollOffset = 0;
    int _descriptionVisibleLineCount = 0;

    std::array<std::array<char16_t, kMaxLineChars>, kMaxTitleLines> _titleLineBuffer;
    std::array<std::array<char16_t, kMaxLineChars>, kMaxDescriptionLines> _descriptionLineBuffer;
    
    static constexpr int kTitleY = 18;
    static constexpr int kDescStartY = 44;
    static constexpr int kDescSpacing = 16;
    static constexpr int kTitleDescGap = 6;
    static constexpr int kDescBottomPadding = 8;
    static constexpr int kDescX = 12;
    static constexpr int kDescWidth = 232;
    
    void SetDescription(const char* description);
    static const char* WrapNextLine(const nft2_header_t* font, const char* text, u32 maxWidth,
        char16_t* out, int outMax);
};
