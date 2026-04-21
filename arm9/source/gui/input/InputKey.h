#pragma once

enum class InputKey : u16
{
    None      = 0,
    A         = 1 << 0,
    B         = 1 << 1,
    Select    = 1 << 2,
    Start     = 1 << 3,
    DpadRight = 1 << 4,
    DpadLeft  = 1 << 5,
    DpadUp    = 1 << 6,
    DpadDown  = 1 << 7,
    R         = 1 << 8,
    L         = 1 << 9,
    X         = 1 << 10,
    Y         = 1 << 11,
    Debug     = 1 << 13,
    Touch     = 1 << 14,
    Lid       = 1 << 15
};

inline InputKey operator&(InputKey lhs, InputKey rhs)
{
    return static_cast<InputKey>(static_cast<u16>(lhs) & static_cast<u16>(rhs));
}

inline InputKey operator|(InputKey lhs, InputKey rhs)
{
    return static_cast<InputKey>(static_cast<u16>(lhs) | static_cast<u16>(rhs));
}

inline InputKey operator~(InputKey lhs)
{
    return static_cast<InputKey>(~static_cast<u16>(lhs));
}

inline InputKey operator^(InputKey lhs, InputKey rhs)
{
    return static_cast<InputKey>(static_cast<u16>(lhs) ^ static_cast<u16>(rhs));
}

inline InputKey operator|=(InputKey& lhs, InputKey rhs)
{
    lhs = lhs | rhs; // don't use |= to prevent recursion
    return lhs;
}