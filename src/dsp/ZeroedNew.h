#pragma once

#include <cstring>
#include <memory>
#include <new>

#include "airwin_consolidated_base.h"

namespace dlaw
{
/** Creates an Airwindows effect in memory that is cleared to zero first.

    A few original effects leave part of their state uninitialised (for example PunchyDeluxe sets
    angSL[17][14] of an [18][16] array). Their first output samples then depend on whatever was in
    memory, so two identical instances could sound slightly different. Clearing the memory first
    makes every instance start from silence, exactly like a freshly started host process usually
    does, and keeps every result repeatable. Effects that initialise everything are unaffected. */
template <typename T>
std::unique_ptr<AirwinConsolidatedBase> makeZeroed()
{
    static_assert (alignof (T) <= __STDCPP_DEFAULT_NEW_ALIGNMENT__, "over-aligned effect class");
    void* memory = ::operator new (sizeof (T));
    std::memset (memory, 0, sizeof (T));
    try
    {
        return std::unique_ptr<AirwinConsolidatedBase> (new (memory) T (0));
    }
    catch (...)
    {
        ::operator delete (memory);
        throw;
    }
}
} // namespace dlaw
