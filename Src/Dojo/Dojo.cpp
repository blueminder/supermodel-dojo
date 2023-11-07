#include "Dojo.h"

void Dojo::Init(std::string game_name, bool record_session, bool train_session)
{
    index = 0;

    if (!Replay::file_path.empty())
    {
        record = false;
        playback = true;
        Dojo::Replay::p1_override = false;
        Dojo::Replay::p2_override = false;
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
        Replay::LoadFile(Replay::file_path);
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

uint32_t Dojo::WipePlayerInputs(int player, uint32_t digital)
{
  uint32_t input_mask = 0;
  if (player == 0)
    input_mask = 986965;
  else if (player == 1)
    input_mask = 15790250;

  return digital & ~input_mask;
}
