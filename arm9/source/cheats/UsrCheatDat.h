#pragma once

struct usr_cheat_index_entry_t
{
    u32 gameCode;
    u32 headerCrc32;
    u32 offset;
    u32 padding;
};

static_assert(sizeof(usr_cheat_index_entry_t) == 16);
