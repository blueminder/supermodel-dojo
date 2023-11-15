#include "Dojo.h"

void Dojo::Poll::StartAction()
{
	idx = 0;
	incoming.reset();
	outgoing.reset();
	current.reset();

    if (Dojo::playback || Dojo::training)
	{
		if (Dojo::training)
			Dojo::Training::TrainingFrameAction();

		if (Dojo::net_inputs[0].find(Dojo::index) != Dojo::net_inputs[0].end())
		{
			uint32_t input_data = Dojo::net_inputs[0].at(Dojo::index);
			if (playback && net_replay && PlayerInputsFilled(Dojo::index))
				input_data |= Dojo::net_inputs[1].at(Dojo::index);
			if (Dojo::Replay::p1_override)
				input_data = Dojo::WipePlayerInputs(0, input_data);
			else if (Dojo::Replay::p2_override)
				input_data = Dojo::WipePlayerInputs(1, input_data);
			incoming = std::bitset<32>(input_data);
		}
	}

	if (Dojo::netplay && PlayerInputsFilled(Dojo::index))
	{
		if (Dojo::net_inputs[1].find(Dojo::index) != Dojo::net_inputs[1].end())
		{
			uint32_t p1_input_data = Dojo::net_inputs[0].at(Dojo::index);
			uint32_t p2_input_data = Dojo::net_inputs[1].at(Dojo::index);

			uint32_t input_data = p1_input_data | p2_input_data;

			current = std::bitset<32>(input_data);
		}
	}
}

void Dojo::Poll::ButtonAction(CInput* btn)
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

void Dojo::Poll::EndAction()
{
    uint32_t digital = (uint32_t)outgoing.to_ulong();
	if (Dojo::record || Dojo::training || Dojo::netplay)
	{
		std::string outgoing_frame = Dojo::Frame::Create(Dojo::index, Dojo::player, Dojo::delay, digital);

		Dojo::AddNetFrame(outgoing_frame.data());

		Dojo::current_frame = outgoing_frame;
	}
}
