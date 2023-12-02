#include "Dojo.h"

#define ENET_IMPLEMENTATION
#include "enet.h"
#include <cassert>

void Dojo::Netplay::ServerThread()
{
	uint32_t index = 0;
	int player = 0;
	int delay = Dojo::delay;

	ENetHost* host = nullptr;
	ENetPeer* peer = nullptr;

	ENetAddress addr = { ENET_HOST_ANY, target_port };
	host = enet_host_create(&addr, 1, 1, 0, 0);
	assert(host != nullptr);

	bool done = false;
	while (!done) { // server loop
		if (peer != nullptr)
		{
			if (frames_to_send.size() > 0)
			{
				std::string frame = frames_to_send.front();
				ENetPacket* packet = enet_packet_create(frame.data(), FRAME_SIZE, ENET_PACKET_FLAG_RELIABLE);
				enet_peer_send(peer, 0, packet);
				frames_to_send.pop();
				//std::cout << "[SERVER] SENT " << Frame::Str((uint8_t*)frame.data()) << std::endl;
			}
		}

		ENetEvent event;
		const int serviceRet = enet_host_service(host, &event, 1);
		if (serviceRet <= 0)
			continue;

		switch (event.type)
		{
		case ENET_EVENT_TYPE_CONNECT: {
			printf("[SERVER] Connected!\n");
			peer = event.peer;

			std::string frame = frames_to_send.front();
			ENetPacket* packet = enet_packet_create(frame.data(), FRAME_SIZE, ENET_PACKET_FLAG_RELIABLE);
			enet_peer_send(peer, 0, packet);
			frames_to_send.pop();
		} break;

		case ENET_EVENT_TYPE_RECEIVE: {
			//std::cout << "[SERVER] RECEIVED " << Frame::Str((uint8_t*)event.packet->data) << std::endl;
			AddNetFrame((const char*)event.packet->data);
			enet_packet_destroy(event.packet);

			//enet_host_flush(host); // since we are going to disconnect right after, we need to do this flush. Otherwise, the message won't get to the destination

			//printf("[SERVER] Now I want to disconnect\n");
			//enet_peer_disconnect(peer, 0);
		} break;

		case ENET_EVENT_TYPE_DISCONNECT: {
			printf("[SERVER] Client acknowledged disconnection\n");
			done = true;
		} break;
		}

	}
}

void Dojo::Netplay::ClientThread()
{
	uint32_t index = 0;
	int player = 1;
	int delay = Dojo::delay;

	ENetHost* host;
	ENetPeer* peer;

	host = enet_host_create(nullptr, 1, 1, 0, 0);
	assert(host != nullptr);

	ENetAddress addr = { 0 };
	enet_address_set_host(&addr, target_ip.data());
	addr.port = target_port;
	peer = enet_host_connect(host, &addr, 1, 0);
	printf("[CLIENT] Attempting to connect to the server...\n");

	bool done = false;
	while(!done) { // client loop
		if (peer != nullptr)
		{
			if (frames_to_send.size() > 0)
			{
				std::string frame = frames_to_send.front();
				ENetPacket* packet = enet_packet_create(frame.data(), FRAME_SIZE, ENET_PACKET_FLAG_RELIABLE);
				enet_peer_send(peer, 0, packet);
				frames_to_send.pop();
				//std::cout << "[CLIENT] SENT " << Frame::Str((uint8_t*)frame.data()) << std::endl;
			}
		}

		ENetEvent event;
		const int serviceRet = enet_host_service(host, &event, 1);
		if (serviceRet <= 0)
			continue;

		switch (event.type)
		{
		case ENET_EVENT_TYPE_CONNECT: {
			printf("[CLIENT] Connected!\n");
		} break;

		case ENET_EVENT_TYPE_RECEIVE: {
			//std::cout << "[CLIENT] RECEIVED " << Frame::Str((uint8_t*)event.packet->data) << std::endl;
			AddNetFrame((const char*)event.packet->data);
			enet_packet_destroy(event.packet);

			//enet_host_flush(host); // since we are going to disconnect right after, we need to do this flush. Otherwise, the message won't get to the destination

			//printf("[SERVER] Now I want to disconnect\n");
			//enet_peer_disconnect(peer, 0);
		} break;

		case ENET_EVENT_TYPE_DISCONNECT: {
			printf("[CLIENT] Looks like the server wants to disconnect\n");
			done = true;
		} break;
		}
	}
}

void Dojo::Netplay::Launch(bool hosting)
{
	if (!started)
	{
		enet_initialize();

		if (hosting)
		{
			std::thread t5(&Dojo::Netplay::ServerThread);
			t5.detach();
		}
		else
		{
			std::thread t5(&Dojo::Netplay::ClientThread);
			t5.detach();
		}

		started = true;
	}
}
