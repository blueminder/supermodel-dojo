#include "SDLIncludes.h"
#include <thread>

namespace Dojo::Receiver {
    void ReceiverThread();
    void Launch();

    inline bool started = false;
}