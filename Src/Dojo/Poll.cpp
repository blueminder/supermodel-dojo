#include "Dojo.h"

void Dojo::Poll::StartAction()
{
    poll_idx = 0;
    poll_final.reset();

    std::bitset<32> frame_input;

    if (Dojo::playback || Dojo::training)
	{
	    if (Dojo::training)
        Dojo::Training::TrainingFrameAction();

		if (Dojo::net_inputs[0].find(Dojo::index) != Dojo::net_inputs[0].end())
		{
			uint32_t input_data = Dojo::net_inputs[0].at(Dojo::index);
			frame_input = std::bitset<32>(input_data);
		}
	}

  poll_frame = frame_input;
}

void Dojo::Poll::ButtonAction(CInput* btn)
{
    if (Dojo::playback || Dojo::training)
	{
		if (poll_frame.test(poll_idx))
		{
			btn->value = 1;
			poll_final.set(poll_idx);
		}
	}

	if (Dojo::record || Dojo::playback || Dojo::training)
	{
		if (btn->value)
			poll_final.set(poll_idx);
	}

    poll_idx++;
}

void Dojo::Poll::EndAction()
{
    uint32_t digital = (uint32_t)poll_final.to_ulong();
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
