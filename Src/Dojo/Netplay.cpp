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
	host = enet_host_create(&addr, 1, 2, 0, 0);
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

			auto start_msg = Message::Writer();
			start_msg.AppendHeader(0, GAME_START);
			start_msg.AppendInt(Dojo::delay);

			std::vector<uint8_t> message = start_msg.Msg();

			ENetPacket* packet = enet_packet_create(message.data(), start_msg.GetSize() + (uint32_t)HEADER_LEN, ENET_PACKET_FLAG_RELIABLE);
			enet_peer_send(peer, 1, packet);

			FillDelay();

			std::cout << "Starting Netplay Session, P" << player + 1 << " D" << delay << std::endl;

		} break;

		case ENET_EVENT_TYPE_RECEIVE: {
			if (event.channelID == 0)
			{
				const char to_add[FRAME_SIZE] = { 0 };
				memcpy((void*)to_add, event.packet->data, FRAME_SIZE);
				AddNetFrame((const char *)to_add);
			}
			else if (event.channelID == 1)
			{
				std::string received((const char*)event.packet->data, event.packet->dataLength);

				uint32_t body_size = Message::GetSize((uint8_t*)received.data());
				uint32_t seq = Message::GetSeq((uint8_t*)received.data());
				uint32_t cmd = Message::GetCmd((uint8_t*)received.data());

				std::vector<uint8_t> body_buf;
				body_buf.resize(body_size);

				memcpy((void*)body_buf.data(), event.packet->data + HEADER_LEN, body_size);
				int offset = 0;

				Message::ProcessBody(cmd, body_size, (const char *)body_buf.data(), &offset);
			}
			enet_packet_destroy(event.packet);
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

	host = enet_host_create(nullptr, 1, 2, 0, 0);
	assert(host != nullptr);

	ENetAddress addr = { 0 };
	enet_address_set_host(&addr, target_ip.data());
	addr.port = target_port;
	peer = enet_host_connect(host, &addr, 2, 0);
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
			if (event.channelID == 0)
			{
				const char to_add[FRAME_SIZE] = { 0 };
				memcpy((void*)to_add, event.packet->data, FRAME_SIZE);
				AddNetFrame((const char *)to_add);
			}
			else if (event.channelID == 1)
			{
				std::string received((const char*)event.packet->data, event.packet->dataLength);

				uint32_t body_size = Message::GetSize((uint8_t*)received.data());
				uint32_t seq = Message::GetSeq((uint8_t*)received.data());
				uint32_t cmd = Message::GetCmd((uint8_t*)received.data());

				std::vector<uint8_t> body_buf;
				body_buf.resize(body_size);

				memcpy((void*)body_buf.data(), event.packet->data + HEADER_LEN, body_size);
				int offset = 0;

				Message::ProcessBody(cmd, body_size, (const char *)body_buf.data(), &offset);
			}
			enet_packet_destroy(event.packet);

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
