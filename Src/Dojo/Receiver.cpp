#include "Dojo.h"

void Dojo::Receiver::ReceiverThread()
{
  if (SDL_Init(0) == -1) {
    printf("SDL_Init: %s\n", SDL_GetError());
    exit(1);
  }
  if (SDLNet_Init() == -1) {
    printf("SDLNet_Init: %s\n", SDLNet_GetError());
    exit(2);
  }

  bool done = false;

  if (Dojo::hosting)
  {
    printf("Starting server...\n");
    TCPsocket server, client;
    IPaddress ip;
    if (SDLNet_ResolveHost(&ip, NULL, target_port) == -1) {
      printf("SDLNet_ResolveHost: %s\n", SDLNet_GetError());
      exit(1);
    }
    server = SDLNet_TCP_Open(&ip);
    if (!server) {
      printf("SDLNet_TCP_Open: %s\n", SDLNet_GetError());
      exit(2);
    }

    started = true;
    while (!done) {
      client = SDLNet_TCP_Accept(server);
      if (!client) {
        //printf("SDLNet_TCP_Accept: %s\n", SDLNet_GetError());
        SDL_Delay(100);
        continue;
      }

      IPaddress *remoteip;
      remoteip = SDLNet_TCP_GetPeerAddress(client);
      if (!remoteip) {
        printf("SDLNet_TCP_GetPeerAddress: %s\n", SDLNet_GetError());
        continue;
      }

      Uint32 ipaddr;
      ipaddr = SDL_SwapBE32(remoteip->host);
      printf("Accepted a connection from %d.%d.%d.%d port %hu\n", ipaddr >> 24,
             (ipaddr >> 16) & 0xff, (ipaddr >> 8) & 0xff, ipaddr & 0xff,
             remoteip->port);

      while (1) {
        char header[HEADER_SIZE];
        int header_len = SDLNet_TCP_Recv(client, header, HEADER_SIZE);
        if (!header_len) {
          printf("SDLNet_TCP_Recv: %s\n", SDLNet_GetError());
          break;
        }

        uint32_t body_size = Message::GetSize((uint8_t *)header);
        uint32_t cmd = Message::GetCmd((uint8_t *)header);

        char body[2048];
        int len = SDLNet_TCP_Recv(client, body, body_size);
        if (!len) {
          printf("SDLNet_TCP_Recv: %s\n", SDLNet_GetError());
          break;
        }
        //printf("Received: %.*s\n", len, message);
        std::string received = std::string(body, body_size);

        int offset = 0;
        Message::ProcessBody(cmd, body_size, body, &offset);
      }
      SDLNet_TCP_Close(client);
    }
  }
  else if (!Dojo::hosting)
  {
    printf("Starting client...\n");
    IPaddress ip;
    TCPsocket tcpsock;

    if (SDLNet_ResolveHost(&ip, target_ip.data(), target_port) == -1) {
      printf("SDLNet_ResolveHost: %s\n", SDLNet_GetError());
      exit(1);
    }

    tcpsock = SDLNet_TCP_Open(&ip);
    if (!tcpsock) {
      printf("SDLNet_TCP_Open: %s\n", SDLNet_GetError());
      exit(2);
    }

    int result;

    Dojo::Message::Writer spectate_request;

    spectate_request.AppendHeader(1, SPECTATE_REQUEST);
    spectate_request.AppendString("Quark");
    spectate_request.AppendString("MatchCode");

    std::vector<uint8_t> msg = spectate_request.Msg();
    
    result = SDLNet_TCP_Send(tcpsock, &msg[0], spectate_request.GetSize()); /* add 1 for the NULL */
    if (result < spectate_request.GetSize())
    {
      printf("SDLNet_TCP_Send: %s\n", SDLNet_GetError());
    }

    while (1) {
        char header[HEADER_SIZE];
        int header_len = SDLNet_TCP_Recv(tcpsock, header, HEADER_SIZE);
        if (!header_len) {
          printf("SDLNet_TCP_Recv: %s\n", SDLNet_GetError());
          break;
        }

        uint32_t body_size = Message::GetSize((uint8_t *)header);
        uint32_t cmd = Message::GetCmd((uint8_t *)header);

        char body[2048];
        int len = SDLNet_TCP_Recv(tcpsock, body, body_size);
        if (!len) {
          printf("SDLNet_TCP_Recv: %s\n", SDLNet_GetError());
          break;
        }
        //printf("Received: %.*s\n", len, message);
        std::string received = std::string(body, body_size);

        int offset = 0;
        Message::ProcessBody(cmd, body_size, body, &offset);
    }

    SDLNet_TCP_Close(tcpsock);
  }

  SDLNet_Quit();
  SDL_Quit();
}


void Dojo::Receiver::Launch()
{
	if (!started)
	{
		std::thread t5(&Dojo::Receiver::ReceiverThread);
		t5.detach();

		started = true;
	}
}
