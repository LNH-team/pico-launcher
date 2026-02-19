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

namespace {
    const char* SkipSpaces(const char* text)
    {
        while (text && *text == ' ')
            ++text;
        return text;
    }
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
{
    if (gameCode) {
        int i = 0;
        while (gameCode[i] && i < 4) {
            _gameCode[i] = gameCode[i];
            ++i;
        }
        _gameCode[i] = 0;
    } else {
        _gameCode[0] = 0;
    }

    for (int i = 0; i < 2; ++i) {
        _titleLabels[i] = std::make_unique<Label2DView>(
            230, 16, 50, fontRepository->GetFont(FontType::Medium11)
        );
        _titleLabels[i]->SetBackgroundColor(materialColorScheme->GetColor(md::sys::color::surfaceContainerLow));
        _titleLabels[i]->SetForegroundColor(materialColorScheme->GetColor(md::sys::color::onSurface));
        AddChildTail(_titleLabels[i].get());
    }

    int titleLineCount = 0;
    if (cheatName && cheatName[0]) {
        const nft2_header_t* font = fontRepository->GetFont(FontType::Medium11);
        const char* p = cheatName;

        while (p && *p && titleLineCount < 2) {
            char16_t line[64];
            p = WrapNextLine(font, p, 230, line, 64);
            if (line[0] == 0) break;

            _titleLabels[titleLineCount]->SetText(line);
            ++titleLineCount;
        }
    }

    if (titleLineCount == 0) {
        const char16_t* localizedTitle = Localization::Translate("cheats");
        if (localizedTitle && localizedTitle[0] != 0)
            _titleLabels[0]->SetText(localizedTitle);
        titleLineCount = 1;
    }

    _titleLineCount = titleLineCount;

    for (int i = 0; i < MAX_DESC_LINES; ++i) {
        _descriptionLabels[i] = std::make_unique<Label2DView>(
            kDescWidth, 16, 100, fontRepository->GetFont(FontType::Regular10)
        );
        _descriptionLabels[i]->SetBackgroundColor(materialColorScheme->GetColor(md::sys::color::surfaceContainerLow));
        _descriptionLabels[i]->SetForegroundColor(materialColorScheme->GetColor(md::sys::color::onSurface));
        AddChildTail(_descriptionLabels[i].get());
    }

    SetDescription(description);
}

const char* CheatDescriptionBottomSheetView::WrapNextLine(
    const nft2_header_t* font,
    const char* text,
    u32 maxWidth,
    char16_t* out,
    int outMax)
{
    const char* p = SkipSpaces(text);
    if (!p || !*p) {
        out[0] = 0;
        return p;
    }

    int lineLen = 0;
    int lastFitLen = 0;
    const char* lastFitPtr = p;
    bool anyFit = false;

    while (*p) {
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

        if (width <= maxWidth) {
            lastFitLen = lineLen;
            lastFitPtr = p;
            anyFit = true;
        } else {
            lineLen = prevLen;

            if (!anyFit) {
                const char* cp = wordStart;
                lineLen = 0;

                while (*cp && lineLen < outMax - 1) {
                    out[lineLen++] = (char16_t)(unsigned char)*cp;
                    out[lineLen] = 0;

                    nft2_measureString(font, out, width, height);
                    if (width > maxWidth) {
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
        if (!*p) {
            lastFitLen = lineLen;
            lastFitPtr = p;
            break;
        }
    }

    out[lastFitLen] = 0;
    return SkipSpaces(lastFitPtr);
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

    for (int i = 0; i < _titleLineCount; ++i)
        _titleLabels[i]->SetPosition(kDescX, baseY + kTitleY + i * 16);

    for (int i = 0; i < _descriptionLineCount; ++i)
        _descriptionLabels[i]->SetPosition(kDescX, baseY + kDescStartY + i * kDescSpacing);
}

void CheatDescriptionBottomSheetView::Draw(GraphicsContext& graphicsContext)
{
    graphicsContext.SetClipArea(GetBounds());
    u32 oldPrio = graphicsContext.SetPriority(0);

    BottomSheetView::Draw(graphicsContext);

    graphicsContext.SetPriority(oldPrio);
    graphicsContext.ResetClipArea();
}

void CheatDescriptionBottomSheetView::Focus(FocusManager& focusManager)
{
    if (_titleLineCount > 0)
        focusManager.Focus(_titleLabels[0].get());
}

bool CheatDescriptionBottomSheetView::HandleInput(const InputProvider& inputProvider, FocusManager& focusManager)
{
    if (inputProvider.Triggered(InputKey::B)) {
        _romBrowserController->HideCheatDescription();
        return true;
    }
    return false;
}

void CheatDescriptionBottomSheetView::SetDescription(const char* description)
{
    _descriptionLineCount = 0;

    for (int i = 0; i < MAX_DESC_LINES; ++i)
        _descriptionLabels[i]->SetText(u"");

    if (!description || !*description) {
        const char16_t* localized = Localization::Translate("cheats_no_description_available");
        if (localized && localized[0] != 0)
            _descriptionLabels[0]->SetText(localized);

        _descriptionLineCount = 1;
        return;
    }

    const nft2_header_t* font = _fontRepository->GetFont(FontType::Regular10);
    const char* p = description;

    while (p && *p && _descriptionLineCount < MAX_DESC_LINES) {
        char16_t line[128];
        p = WrapNextLine(font, p, kDescWidth, line, 128);

        if (line[0] == 0)
            break;

        _descriptionLabels[_descriptionLineCount]->SetText(line);
        _descriptionLineCount++;
    }
}
