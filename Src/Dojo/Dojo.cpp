#include "Dojo.h"

void Dojo::Init(std::string game_name, bool record_session, bool train_session)
{
    index = 0;

    if (!replay_filename.empty())
    {
        record = false;
        playback = true;
    }

    if (record_session)
    {
        record = true;
        playback = false;
    }

    if (train_session)
    {
      std::cout << "Training Session" << std::endl;
      training = true;
      playback = false;
    }

    if (record)
    {
        std::cout << "Recording Session" << std::endl;
        std::string replay_fn = Replay::CreateReplayFile(game_name);
        std::cout << replay_fn << std::endl;
    }
    else if (playback)
    {
        std::cout << "Playing Replay" << std::endl;
        Dojo::Replay::LoadFile(Dojo::replay_filename);
    }
}

void Dojo::AdvanceFrame()
{
    index++;
}

void Dojo::AddNetFrame(const char* received_data)
{
  const char data[FRAME_SIZE] = { 0 };
  memcpy((void*)data, received_data, FRAME_SIZE);

  uint32_t effective_frame_num = Dojo::Frame::GetEffectiveFrameNumber((uint8_t*)data);
  if (effective_frame_num == 0)
    return;

  uint32_t frame_player = (uint8_t)data[0];

  std::string data_to_queue(data, data + FRAME_SIZE);

  //if (net_inputs[frame_player].count(effective_frame_num) == 0 ||
  //  effective_frame_num >= last_consecutive_common_frame)
  {
    net_frames[frame_player].emplace(effective_frame_num, data_to_queue);
    net_inputs[frame_player].emplace(effective_frame_num, Frame::GetDigital((uint8_t*)data));
  }
}
