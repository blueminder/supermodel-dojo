#pragma once

#include <bitset>
#include "Inputs/Input.h"

namespace Dojo::Poll {
    void StartAction();
    void ButtonAction(std::shared_ptr<CInput> btn);
    void AxisAction(std::shared_ptr<CInput> axis);
    void EndAction();

    inline int idx = 0;
    inline std::bitset<32> incoming;
    inline std::bitset<32> outgoing;
    inline std::bitset<32> current;

    // packed analog gun axes: P1 X | P1 Y << 8 | P2 X << 16 | P2 Y << 24
    inline uint32_t analog = 0;
    inline uint32_t incoming_analog = 0;
    inline bool has_incoming_analog = false;

    // packed serial gun axes, raw values: P1 X | P1 Y << 16 | P2 X << 32 | P2 Y << 48
    inline uint64_t gun = 0;
    inline uint64_t incoming_gun = 0;
    inline bool has_incoming_gun = false;
}
