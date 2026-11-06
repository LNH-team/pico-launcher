#pragma once
#include "common.h"

/// @brief Helpers for the in-game exit combo.
///
/// The combo travels to the loader as an active-low KEY_* mask, whose bits are
/// the same as InputKey's. It is stored in settings.json as names, though
/// ("Select+L+R+Right"), because a number cannot be read in a diff.
namespace KeyCombo
{
/// @brief The combo the loader has always used: L + R + Right + Select.
constexpr u16 kDefault = 0x0314;

/// @brief The twelve real buttons a combo may use; touch, lid and debug are
///        not keys and never reach the loader.
constexpr u16 kValidKeys = 0x0FFF;

/// @brief Room for the longest combo text ("Select+L+R+Right") and its
///        terminator.
constexpr u32 kMaxTextLength = 32;

/// @brief Whether \p combo is 2 to 4 real keys and includes SELECT.
bool IsValid(u16 combo);

/// @brief Number of keys in \p combo.
u32 Count(u16 combo);

/// @brief Writes \p combo as names, e.g. "Select+L+R+Right". Zero means the
///        loader's default, and is written as that default.
void Format(u16 combo, char* text, u32 textLength);

/// @brief Parses a name list written by Format. Empty text parses to zero,
///        the loader's default. Unknown names, or a combination the loader
///        would reject, fail and leave \p combo untouched.
bool TryParse(const char* text, u16& combo);
}
