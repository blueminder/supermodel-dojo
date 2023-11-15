#include <bitset>
#include "Input.h"

namespace Dojo::Poll {
    void StartAction();
    void ButtonAction(CInput* btn);
    void EndAction();

    inline int idx = 0;
    inline std::bitset<32> incoming;
    inline std::bitset<32> outgoing;
    inline std::bitset<32> current;
}
