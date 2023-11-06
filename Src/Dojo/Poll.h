#include <bitset>
#include "Input.h"

namespace Dojo::Poll {
    void StartAction();
    void ButtonAction(CInput* btn);
    void EndAction();

    inline int poll_idx = 0;
    inline std::bitset<32> poll_frame;
    inline std::bitset<32> poll_final;
}
