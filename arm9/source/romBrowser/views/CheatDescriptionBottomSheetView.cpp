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
    const char* SkipInlineSpaces(const char* text)
    {
        while (text && (*text == ' ' || *text == '\t' || *text == '\r'))
            ++text;
        return text;
    }

    bool IsBreakChar(char c)
    {
        return c == 0 || c == ' ' || c == '\t' || c == '\r' || c == '\n';
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

    for (int i = 0; i < kMaxTitleLines; ++i) {
        _titleLabels[i] = std::make_unique<Label2DView>(
            kDescWidth, 16, kMaxLineChars - 1, fontRepository->GetFont(FontType::Medium11)
        );
        _titleLabels[i]->SetBackgroundColor(materialColorScheme->GetColor(md::sys::color::surfaceContainerLow));
        _titleLabels[i]->SetForegroundColor(materialColorScheme->GetColor(md::sys::color::onSurface));
        AddChildTail(_titleLabels[i].get());
    }

    int titleLineCount = 0;
    if (cheatName && cheatName[0]) {
        const nft2_header_t* font = fontRepository->GetFont(FontType::Medium11);
        const char* p = cheatName;

        while (p && *p && titleLineCount < kMaxTitleLines) {
            char16_t line[kMaxLineChars];
            p = WrapNextLine(font, p, kDescWidth, line, kMaxLineChars);
            if (line[0] == 0) break;

            for (int j = 0; j < kMaxLineChars; ++j) {
                _titleLineBuffer[titleLineCount][j] = line[j];
                if (line[j] == 0) break;
            }
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

    for (int i = 0; i < kMaxVisibleDescriptionLines; ++i) {
        _descriptionLabels[i] = std::make_unique<Label2DView>(
            kDescWidth, 16, kMaxLineChars - 1, fontRepository->GetFont(FontType::Regular10)
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
    if (!text || outMax <= 0) 
    {
        return nullptr;
    }

    const char* p = text;
    while (*p == '\r')
        ++p;

    if (*p == '\n') {
        out[0] = 0;
        return p + 1;
    }

    p = SkipInlineSpaces(p);
    if (!p || !*p) {
        out[0] = 0;
        return p;
    }

    int lineLen = 0;
    int lastFitLen = -1;
    const char* lastFitPtr = p;

    while (*p) {
        if (*p == '\n') 
        {
            ++p;
            break;
        }

        const char* wordStart = p;
        while (*p && !IsBreakChar(*p))
            ++p;

        int wordLen = (int)(p - wordStart);
        if (wordLen <= 0) 
        {
            if (*p == '\n') {
                ++p;
                break;
            }
            p = SkipInlineSpaces(p);
            continue;
        }

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
        } else {
            lineLen = prevLen;

            if (lastFitLen < 0) {
                const char* cp = wordStart;
                lineLen = 0;

                while (*cp && lineLen < outMax - 1) {
                    if (IsBreakChar(*cp) && *cp != '-')
                        break;

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

        p = SkipInlineSpaces(p);
        if (*p == '\n') {
            ++p;
            break;
        }
        if (!*p) {
            lastFitLen = lineLen;
            lastFitPtr = p;
            break;
        }
    }

    if (lastFitLen < 0)
        lastFitLen = 0;
    out[lastFitLen] = 0;
    return lastFitPtr;
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
    static constexpr int kLineHeight = 16;

    for (int i = 0; i < _titleLineCount; ++i)
        _titleLabels[i]->SetPosition(kDescX, baseY + kTitleY + i * kLineHeight);

    int dynamicDescStartY = kTitleY + _titleLineCount * kLineHeight + kTitleDescGap;
    int availableHeight = 192 - (baseY + dynamicDescStartY) - kDescBottomPadding;
    if (availableHeight < kLineHeight)
        availableHeight = kLineHeight;

    _descriptionVisibleLineCount = availableHeight / kLineHeight;
    if (_descriptionVisibleLineCount < 1)
        _descriptionVisibleLineCount = 1;
    if (_descriptionVisibleLineCount > kMaxVisibleDescriptionLines)
        _descriptionVisibleLineCount = kMaxVisibleDescriptionLines;

    int maxScroll = _descriptionTotalLineCount - _descriptionVisibleLineCount;
    if (maxScroll < 0)
        maxScroll = 0;
    if (_descriptionScrollOffset > maxScroll)
        _descriptionScrollOffset = maxScroll;

    for (int i = 0; i < kMaxVisibleDescriptionLines; ++i)
    {
        _descriptionLabels[i]->SetPosition(kDescX, baseY + dynamicDescStartY + i * kDescSpacing);

        if (i < _descriptionVisibleLineCount)
        {
            int sourceIdx = _descriptionScrollOffset + i;
            if (sourceIdx >= 0 && sourceIdx < _descriptionTotalLineCount)
                _descriptionLabels[i]->SetText(_descriptionLineBuffer[sourceIdx].data());
            else
                _descriptionLabels[i]->SetText(u"");
        }
        else
        {
            _descriptionLabels[i]->SetText(u"");
        }
    }
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

    if (inputProvider.Triggered(InputKey::DpadDown)) {
        int maxScroll = _descriptionTotalLineCount - _descriptionVisibleLineCount;
        if (maxScroll < 0)
            maxScroll = 0;
        if (_descriptionScrollOffset < maxScroll) {
            ++_descriptionScrollOffset;
            return true;
        }
    }

    if (inputProvider.Triggered(InputKey::DpadUp)) {
        if (_descriptionScrollOffset > 0) {
            --_descriptionScrollOffset;
            return true;
        }
    }

    return false;
}

void CheatDescriptionBottomSheetView::SetDescription(const char* description)
{
    _descriptionTotalLineCount = 0;
    _descriptionScrollOffset = 0;

    for (int i = 0; i < kMaxDescriptionLines; ++i)
        _descriptionLineBuffer[i][0] = 0;

    for (int i = 0; i < kMaxVisibleDescriptionLines; ++i)
        _descriptionLabels[i]->SetText(u"");

    if (!description || !*description) {
        const char16_t* localized = Localization::Translate("cheats_no_description_available");
        if (localized && localized[0] != 0) {
            for (int j = 0; j < kMaxLineChars; ++j) {
                _descriptionLineBuffer[0][j] = localized[j];
                if (localized[j] == 0) break;
            }
        } else {
            _descriptionLineBuffer[0][0] = 0;
        }

        _descriptionTotalLineCount = 1;
        return;
    }

    const nft2_header_t* font = _fontRepository->GetFont(FontType::Regular10);
    const char* p = description;

    while (p && *p && _descriptionTotalLineCount < kMaxDescriptionLines) {
        char16_t line[kMaxLineChars];
        const char* next = WrapNextLine(font, p, kDescWidth, line, kMaxLineChars);

        if (!next || next == p) {
            break;
        }

        int idx = _descriptionTotalLineCount;
        for (int j = 0; j < kMaxLineChars; ++j) {
            _descriptionLineBuffer[idx][j] = line[j];
            if (line[j] == 0) break;
        }
        ++_descriptionTotalLineCount;
        p = next;

        while (*p == '\r')
            ++p;

        if (*p == 0)
            break;
    }

    if (_descriptionTotalLineCount == 0) {
        _descriptionLineBuffer[0][0] = 0;
        _descriptionTotalLineCount = 1;
    }
}
