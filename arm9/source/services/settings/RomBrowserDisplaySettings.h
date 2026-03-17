#pragma once
#include "RomBrowserLayout.h"
#include "RomBrowserSortMode.h"

class RomBrowserDisplaySettings
{
public:
    RomBrowserLayout layout = RomBrowserLayout::HorizontalIconGrid;
    RomBrowserSortMode sortMode = RomBrowserSortMode::NameAscending;
    bool showHiddenFiles = false;
};
