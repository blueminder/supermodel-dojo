namespace Dojo::Netplay {
    void ServerThread();
    void ClientThread();

    void Launch(bool hosting);

    inline bool started = false;
    inline uint16_t port;

    inline std::queue<std::string> frames_to_send;
}
