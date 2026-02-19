#include "common.h"
#include "gui/GraphicsContext.h"
#include "gui/VramContext.h"
#include "gui/IVramManager.h"
#include "gui/input/InputProvider.h"
#include "../IRomBrowserController.h"
#include "CheatDescriptionBottomSheetView.h"
#include "themes/material/MaterialColorScheme.h"
#include "services/localization/Localization.h"
#include "gui/font/nitroFont2.h"

namespace
{
    const char* SkipSpaces(const char* text)
    {
        while (text && *text == ' ')
            ++text;
        return text;
    }
}

const char* CheatDescriptionBottomSheetView::WrapNextLine(const nft2_header_t* font, const char* text, u32 maxWidth,
    char16_t* out, int outMax)
{
    const char* p = SkipSpaces(text);
    if (!p || !*p)
    {
        out[0] = 0;
        return p;
    }

    int lineLen = 0;
    int lastFitLen = 0;
    const char* lastFitPtr = p;
    bool anyFit = false;

    while (*p)
    {
        const char* wordStart = p;
        while (*p && *p != ' ')
            ++p;
        int wordLen = (int)(p - wordStart);

        int prevLen = lineLen;
        if (lineLen > 0 && lineLen < outMax - 1)
            out[lineLen++] = u' ';
        for (int i = 0; i < wordLen && lineLen < outMax - 1; ++i)
            out[lineLen++] = (char16_t)(unsigned char)wordStart[i];
        out[lineLen] = 0;

        u32 width = 0, height = 0;
        nft2_measureString(font, out, width, height);

        if (width <= maxWidth)
        {
            lastFitLen = lineLen;
            lastFitPtr = p;
            anyFit = true;
        }
        else
        {
            lineLen = prevLen;
            if (!anyFit)
            {
                // Fallback: split inside the word
                const char* cp = wordStart;
                lineLen = 0;
                while (*cp && lineLen < outMax - 1)
                {
                    out[lineLen++] = (char16_t)(unsigned char)*cp;
                    out[lineLen] = 0;
                    nft2_measureString(font, out, width, height);
                    if (width > maxWidth)
                    {
                        lineLen--;
                        out[lineLen] = 0;
                        break;
                    }
                    ++cp;
                }
                lastFitLen = lineLen;
                lastFitPtr = cp;
            }
            break;
        }

        p = SkipSpaces(p);
        if (!*p)
        {
            lastFitLen = lineLen;
            lastFitPtr = p;
            break;
        }
    }

    out[lastFitLen] = 0;
    return SkipSpaces(lastFitPtr);
}

CheatDescriptionBottomSheetView::CheatDescriptionBottomSheetView(
    IRomBrowserController* romBrowserController,
    const MaterialColorScheme* materialColorScheme,
    const IFontRepository* fontRepository,
    const char* cheatName,
    const char* description,
    const char* gameCode,
    u32 crc)
    : _romBrowserController(romBrowserController)
    , _materialColorScheme(materialColorScheme)
    , _fontRepository(fontRepository)
    , _crc(crc)
    , _titleLabel(230, 16, 50, fontRepository->GetFont(FontType::Medium11))
{
    // Save game code
    if (gameCode)
    {
        int i = 0;
        while (gameCode[i] && i < 4)
        {
            _gameCode[i] = gameCode[i];
            ++i;
        }
        _gameCode[i] = 0;
    }
    else
    {
        _gameCode[0] = 0;
    }
    
    // Set title to cheat name
    if (cheatName && cheatName[0])
    {
        char16_t titleText[64];
        int i = 0;
        while (cheatName[i] && i < 63)
        {
            titleText[i] = (char16_t)(unsigned char)cheatName[i];
            ++i;
        }
        titleText[i] = 0;
        _titleLabel.SetText(titleText);
    }
    else
    {
        const char16_t* localizedTitle = Localization::Translate("cheats");
        if (localizedTitle && localizedTitle[0] != 0)
            _titleLabel.SetText(localizedTitle);
    }
    
    _titleLabel.SetBackgroundColor(materialColorScheme->GetColor(md::sys::color::surfaceContainerLow));
    _titleLabel.SetForegroundColor(materialColorScheme->GetColor(md::sys::color::onSurface));
    AddChildTail(&_titleLabel);
    
    // Create description labels
    for (int i = 0; i < MAX_DESC_LINES; ++i)
    {
        _descriptionLabels[i] = std::make_unique<Label2DView>(kDescWidth, 16, 100, fontRepository->GetFont(FontType::Regular10));
        _descriptionLabels[i]->SetBackgroundColor(materialColorScheme->GetColor(md::sys::color::surfaceContainerLow));
        _descriptionLabels[i]->SetForegroundColor(materialColorScheme->onSurface);
        AddChildTail(_descriptionLabels[i].get());
    }
    
    SetDescription(description);
}

void CheatDescriptionBottomSheetView::InitVram(const VramContext& vramContext)
{
    BottomSheetView::InitVram(vramContext);
}

void CheatDescriptionBottomSheetView::VBlank()
{
    BottomSheetView::VBlank();
}

void CheatDescriptionBottomSheetView::Update()
{
    BottomSheetView::Update();
    
    int baseY = _position.y;
    
    _titleLabel.SetPosition(kDescX, baseY + kTitleY);
    
    // Position description labels
    for (int i = 0; i < _descriptionLineCount; ++i)
    {
        _descriptionLabels[i]->SetPosition(kDescX, baseY + kDescStartY + i * kDescSpacing);
    }
}

void CheatDescriptionBottomSheetView::Draw(GraphicsContext& graphicsContext)
{
    graphicsContext.SetClipArea(GetBounds());
    u32 oldPrio = graphicsContext.SetPriority(0);
    {
        BottomSheetView::Draw(graphicsContext);
    }
    graphicsContext.SetPriority(oldPrio);
    graphicsContext.ResetClipArea();
}

void CheatDescriptionBottomSheetView::Focus(FocusManager& focusManager)
{
    focusManager.Focus(&_titleLabel);
}

bool CheatDescriptionBottomSheetView::HandleInput(const InputProvider& inputProvider, FocusManager& focusManager)
{
    if (inputProvider.Triggered(InputKey::B))
    {
        _romBrowserController->HideCheatDescription();
        return true;
    }
    return false;
}

void CheatDescriptionBottomSheetView::SetDescription(const char* description)
{
    _descriptionLineCount = 0;

    // Clear all labels first
    for (int i = 0; i < MAX_DESC_LINES; ++i)
    {
        _descriptionLabels[i]->SetText(u"");
    }

    if (!description || !*description)
    {
        const char16_t* localized = Localization::Translate("cheats_no_description_available");
        if (localized && localized[0] != 0)
        {
            _descriptionLabels[0]->SetText(localized);
        }
        _descriptionLineCount = 1;
        return;
    }

    const nft2_header_t* font = _fontRepository->GetFont(FontType::Regular10);
    const char* p = description;

    while (p && *p && _descriptionLineCount < MAX_DESC_LINES)
    {
        char16_t line[128];
        p = WrapNextLine(font, p, kDescWidth, line, (int)(sizeof(line) / sizeof(line[0])));

        if (line[0] == 0)
            break;

        _descriptionLabels[_descriptionLineCount]->SetText(line);
        _descriptionLabels[_descriptionLineCount]->SetForegroundColor(_materialColorScheme->onSurface);
        _descriptionLabels[_descriptionLineCount]->SetBackgroundColor(_materialColorScheme->GetColor(md::sys::color::surfaceContainerLow));
        _descriptionLineCount++;
    }
}
