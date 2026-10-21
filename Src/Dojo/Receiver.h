#include "SDLIncludes.h"
#include <thread>
#include <atomic>

namespace Dojo::Receiver {
    void ReceiverThread();
    void Launch();

    inline std::atomic<bool> started = false;
}