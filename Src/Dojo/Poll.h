#include <bitset>
#include "Input.h"

namespace Dojo::Poll {
    void StartAction();
    void ButtonAction(CInput* btn);
    void EndAction();

    inline int idx = 0;
    inline std::bitset<32> frame;
    inline std::bitset<32> final;
}
