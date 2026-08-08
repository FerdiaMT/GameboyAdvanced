#pragma once

#include <cstdint>
#include <vector>

namespace tdmi7::legacy
{
struct Transaction
{
    uint32_t kind;
    uint32_t size;
    uint32_t addr;
    uint32_t data;
    uint32_t cycle;
    uint32_t access;
};

extern std::vector<Transaction> transactions;
extern bool singleStepTestActive;
extern uint32_t testBaseAddress;
extern uint16_t testThumbOpcode;
}
