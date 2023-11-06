#include "Dojo.h"

void Dojo::Poll::StartAction()
{
    idx = 0;
    frame.reset();
    final.reset();

    if (Dojo::playback || Dojo::training)
	{
		if (Dojo::training)
			Dojo::Training::TrainingFrameAction();

		if (Dojo::net_inputs[0].find(Dojo::index) != Dojo::net_inputs[0].end())
		{
			uint32_t input_data = Dojo::net_inputs[0].at(Dojo::index);
			if (Dojo::Replay::p1_override)
				input_data = Dojo::WipePlayerInputs(0, input_data);
			else if (Dojo::Replay::p2_override)
				input_data = Dojo::WipePlayerInputs(1, input_data);
			frame = std::bitset<32>(input_data);
		}
	}
}

void Dojo::Poll::ButtonAction(CInput* btn)
{
    if (Dojo::playback || Dojo::training)
	{
		if (frame.test(idx))
		{
			btn->value = 1;
			final.set(idx);
		}
	}

	if (Dojo::record || Dojo::playback || Dojo::training)
	{
		if (btn->value)
			final.set(idx);
	}

    idx++;
}

void Dojo::Poll::EndAction()
{
    uint32_t digital = (uint32_t)final.to_ulong();
	if (Dojo::record || Dojo::training)
	{
		std::string final_frame = Dojo::Frame::Create(Dojo::index, 0, 0, digital);

		if (Dojo::training)
			Dojo::AddNetFrame(final_frame.data());

		if (Dojo::record)
			Dojo::Replay::AppendFrameToFile(final_frame);

		Dojo::current_frame = final_frame;
	}

	//std::cout << Dojo::index << ":" << inputBits.to_string() << " " << digital << std::endl;
}
