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
}
