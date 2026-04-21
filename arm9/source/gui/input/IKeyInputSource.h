#pragma once
#include "InputKey.h"

/// @brief Interface for a source of key input.
class IKeyInputSource
{
public:
    virtual ~IKeyInputSource() { }

    virtual InputKey Sample() const = 0;
};
