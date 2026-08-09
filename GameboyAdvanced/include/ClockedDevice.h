#pragma once

#include <cstdint>

// Every system component advances from the same 16.78 MHz GBA master clock.
// The scheduler owns ordering; devices only consume the elapsed cycle delta.
class ClockedDevice
{
public:
    virtual ~ClockedDevice() = default;
    virtual void advance(uint32_t cycles) = 0;
};
