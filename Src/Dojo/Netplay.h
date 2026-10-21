#pragma once

#include <atomic>

namespace Dojo::Netplay {
    void ServerThread();
    void ClientThread();

    void Launch(bool hosting);

    inline std::atomic<bool> started = false;
    inline uint16_t port;

    inline std::queue<std::string> frames_to_send;
}
