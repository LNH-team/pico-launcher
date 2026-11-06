#include "common.h"
#include <string.h>
#include <strings.h>
#include "gui/input/InputKey.h"
#include "KeyCombo.h"

namespace
{
constexpr u16 Key(InputKey key)
{
    return static_cast<u16>(key);
}

struct KeyName
{
    const char* name;
    u16 mask;
};

// The order a combo reads in: the keys that make it deliberate first, then the
// d-pad, then the face buttons. The default renders "Select+L+R+Right".
constexpr KeyName kKeyNames[] =
{
    { "Select", Key(InputKey::Select) },
    { "Start",  Key(InputKey::Start) },
    { "L",      Key(InputKey::L) },
    { "R",      Key(InputKey::R) },
    { "Up",     Key(InputKey::DpadUp) },
    { "Down",   Key(InputKey::DpadDown) },
    { "Left",   Key(InputKey::DpadLeft) },
    { "Right",  Key(InputKey::DpadRight) },
    { "A",      Key(InputKey::A) },
    { "B",      Key(InputKey::B) },
    { "X",      Key(InputKey::X) },
    { "Y",      Key(InputKey::Y) },
};
}

u32 KeyCombo::Count(u16 combo)
{
    u32 count = 0;
    for (u32 mask = 1; mask <= kValidKeys; mask <<= 1)
    {
        if (combo & mask)
            count++;
    }
    return count;
}

bool KeyCombo::IsValid(u16 combo)
{
    if ((combo & static_cast<u16>(~kValidKeys)) != 0)
        return false;
    if ((combo & Key(InputKey::Select)) == 0)
        return false;
    u32 count = Count(combo);
    return count >= 2 && count <= 4;
}

void KeyCombo::Format(u16 combo, char* text, u32 textLength)
{
    if (textLength == 0)
        return;
    text[0] = 0;
    if (combo == 0)
        combo = kDefault;

    u32 length = 0;
    for (const auto& keyName : kKeyNames)
    {
        if ((combo & keyName.mask) == 0)
            continue;
        u32 nameLength = strlen(keyName.name);
        u32 needed = length + (length != 0 ? 1 : 0) + nameLength;
        if (needed + 1 > textLength)
            break;
        if (length != 0)
            text[length++] = '+';
        memcpy(&text[length], keyName.name, nameLength);
        length += nameLength;
        text[length] = 0;
    }
}

bool KeyCombo::TryParse(const char* text, u16& combo)
{
    if (!text || text[0] == 0)
    {
        combo = 0;
        return true;
    }

    u16 parsed = 0;
    const char* token = text;
    while (*token)
    {
        const char* end = strchr(token, '+');
        u32 tokenLength = end ? static_cast<u32>(end - token) : strlen(token);
        if (tokenLength == 0)
            return false;

        bool found = false;
        for (const auto& keyName : kKeyNames)
        {
            if (strlen(keyName.name) == tokenLength &&
                strncasecmp(token, keyName.name, tokenLength) == 0)
            {
                parsed |= keyName.mask;
                found = true;
                break;
            }
        }
        if (!found)
            return false;

        token = end ? end + 1 : token + tokenLength;
    }

    if (!IsValid(parsed))
        return false;
    combo = parsed;
    return true;
}
