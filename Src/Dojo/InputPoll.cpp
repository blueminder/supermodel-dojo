#include "Dojo.h"
#include <cstring>

void Dojo::Poll::StartAction()
{
	idx = 0;
	incoming.reset();
	outgoing.reset();
	current.reset();
	analog = 0;
	incoming_analog = 0;
	has_incoming_analog = false;
	gun = 0;
	incoming_gun = 0;
	has_incoming_gun = false;

	// may enqueue frames (AddNetFrame takes the lock itself)
	if (Dojo::training)
		Dojo::Training::TrainingFrameAction();

	std::lock_guard<std::mutex> lock(Dojo::inputs_mtx);

	// netplay only: replays and spectate still need full history
	if (Dojo::netplay)
		Dojo::TrimNetMapsNoLock();

	if (Dojo::playback || Dojo::training)
	{
		auto it = Dojo::net_inputs[0].find(Dojo::index);
		if (it != Dojo::net_inputs[0].end())
		{
			uint32_t input_data = it->second;
			if (playback && net_replay && PlayerInputsFilledNoLock(Dojo::index))
				input_data |= Dojo::net_inputs[1].at(Dojo::index);
			if (Dojo::Replay::p1_override)
				input_data = Dojo::WipePlayerInputs(0, input_data);
			else if (Dojo::Replay::p2_override)
				input_data = Dojo::WipePlayerInputs(1, input_data);
			incoming = std::bitset<32>(input_data);
		}

		auto ita = Dojo::net_analog[0].find(Dojo::index);
		if (ita != Dojo::net_analog[0].end())
		{
			uint32_t analog_data = ita->second;
			if (playback && net_replay && PlayerInputsFilledNoLock(Dojo::index))
				analog_data = (analog_data & 0x0000FFFFu) | (Dojo::net_analog[1].at(Dojo::index) & 0xFFFF0000u);
			if (Dojo::Replay::p1_override)
				analog_data = Dojo::WipeAnalogPlayerInputs(0, analog_data);
			else if (Dojo::Replay::p2_override)
				analog_data = Dojo::WipeAnalogPlayerInputs(1, analog_data);
			incoming_analog = analog_data;
			has_incoming_analog = true;
		}

		auto itg = Dojo::net_gun[0].find(Dojo::index);
		if (itg != Dojo::net_gun[0].end())
		{
			uint64_t gun_data = itg->second;
			if (playback && net_replay && PlayerInputsFilledNoLock(Dojo::index))
				gun_data = (gun_data & 0xFFFFFFFFull) | (Dojo::net_gun[1].at(Dojo::index) & 0xFFFFFFFF00000000ull);
			if (Dojo::Replay::p1_override)
				gun_data = Dojo::WipeGunPlayerInputs(0, gun_data);
			else if (Dojo::Replay::p2_override)
				gun_data = Dojo::WipeGunPlayerInputs(1, gun_data);
			incoming_gun = gun_data;
			has_incoming_gun = true;
		}
	}

	if (Dojo::netplay && PlayerInputsFilledNoLock(Dojo::index))
	{
		uint32_t p1_input_data = Dojo::net_inputs[0].at(Dojo::index);
		uint32_t p2_input_data = Dojo::net_inputs[1].at(Dojo::index);

		uint32_t input_data = p1_input_data | p2_input_data;

		current = std::bitset<32>(input_data);

		incoming_analog = (Dojo::net_analog[0].at(Dojo::index) & 0x0000FFFFu) |
			(Dojo::net_analog[1].at(Dojo::index) & 0xFFFF0000u);
		has_incoming_analog = true;

		incoming_gun = (Dojo::net_gun[0].at(Dojo::index) & 0xFFFFFFFFull) |
			(Dojo::net_gun[1].at(Dojo::index) & 0xFFFFFFFF00000000ull);
		has_incoming_gun = true;

		if (Dojo::debug)
		{
			std::ostringstream LogStream;
			LogStream << Dojo::index << ": " << current.to_string();
			InfoLog(LogStream.str().data());
		}
	}
}

void Dojo::Poll::ButtonAction(std::shared_ptr<CInput> btn)
{
	if (Dojo::playback || Dojo::training)
	{
		if (incoming.test(idx))
		{
			btn->value = 1;
			outgoing.set(idx);
		}
	}

	if (Dojo::record || Dojo::training || Dojo::netplay)
	{
		if (btn->value)
			outgoing.set(idx);
	}

	if (Dojo::netplay)
	{
		if (PlayerInputsFilled(Dojo::index))
		{
			if (current.test(idx))
				btn->value = 1;
			else
				btn->value = 0;
		}
	}

	idx++;
}

void Dojo::Poll::AxisAction(std::shared_ptr<CInput> axis)
{
	if (strcmp(axis->id, "AnalogGunX") == 0)
		analog = (analog & ~0x000000FFu) | ((uint32_t)axis->value & 0xFFu);
	else if (strcmp(axis->id, "AnalogGunY") == 0)
		analog = (analog & ~0x0000FF00u) | (((uint32_t)axis->value & 0xFFu) << 8);
	else if (strcmp(axis->id, "AnalogGunX2") == 0)
		analog = (analog & ~0x00FF0000u) | (((uint32_t)axis->value & 0xFFu) << 16);
	else if (strcmp(axis->id, "AnalogGunY2") == 0)
		analog = (analog & ~0xFF000000u) | (((uint32_t)axis->value & 0xFFu) << 24);
	// serial gun: raw axis values, no scaling
	else if (strcmp(axis->id, "GunX") == 0)
		gun = (gun & ~0x000000000000FFFFull) | ((uint64_t)axis->value & 0xFFFFull);
	else if (strcmp(axis->id, "GunY") == 0)
		gun = (gun & ~0x00000000FFFF0000ull) | (((uint64_t)axis->value & 0xFFFFull) << 16);
	else if (strcmp(axis->id, "GunX2") == 0)
		gun = (gun & ~0x0000FFFF00000000ull) | (((uint64_t)axis->value & 0xFFFFull) << 32);
	else if (strcmp(axis->id, "GunY2") == 0)
		gun = (gun & ~0xFFFF000000000000ull) | (((uint64_t)axis->value & 0xFFFFull) << 48);
}

void Dojo::Poll::EndAction()
{
	uint32_t digital = (uint32_t)outgoing.to_ulong();
	if (Dojo::record || Dojo::training || Dojo::netplay)
	{
		std::string outgoing_frame = Dojo::Frame::Create(Dojo::index, Dojo::player, Dojo::delay, digital, analog,
			(uint32_t)(gun & 0xFFFFFFFFull), (uint32_t)(gun >> 32));

		Dojo::AddNetFrame(outgoing_frame.data());

		Dojo::current_frame = outgoing_frame;
	}
}
