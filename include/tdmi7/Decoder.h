#pragma once
#include "tdmi7/CPUTypes.h"

namespace tdmi7
{
class Decoder
{

public:
    Decoder();
    CPUTypes::thumbInstr decodeThumb(uint16_t instr);
    CPUTypes::armInstr decodeArm(uint32_t const instr);
};
}
