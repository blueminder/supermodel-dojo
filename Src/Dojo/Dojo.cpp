#include "Dojo.h"

void Dojo::Init(std::string game_name, bool record_session, bool train_session, bool receiving, bool hosting, bool netplay, std::string state_path)
{
    index = 0;
    Dojo::hosting = hosting;

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

    if (receiving)
    {
      Dojo::receiving = true;
      paused = true;
      playback = true;
      Receiver::Launch();
    }

    if (netplay)
    {
      if (!hosting)
      {
        player = 1;
        Training::TogglePlayerSwap();
      }
      if (delay == 0)
        delay = 1;
      Dojo::netplay = true;
      Netplay::Launch(hosting);
      std::cout << "Starting Netplay Session, P" << player + 1 << " D" << delay << std::endl;
    }

    Dojo::hosting = hosting;

    if (record)
    {
        std::cout << "Recording Session" << std::endl;
        std::string replay_fn = Replay::CreateReplayFile(game_name, state_path);
        std::cout << replay_fn << std::endl;
    }
    else if (playback)
    {
        std::cout << "Playing Replay" << std::endl;
        Replay::LoadFile(Replay::file_path);
        if (net_inputs[1].size() > 0)
          net_replay = true;
    }

    // add buffer frames for delay
    for (int d = 0; d < delay; d++)
    {
      auto p1_frame = Dojo::Frame::Create(d, 0, 0, 0);
      auto p2_frame = Dojo::Frame::Create(d, 1, 0, 0);

      AddNetFrame(p1_frame.data());
      AddNetFrame(p2_frame.data());
    }

    if (hosting)
    {
      for (int d = 0; d < (delay * 2) + 1; d++)
      {
        auto buffer_frame = Dojo::Frame::Create(d, 1, 0, 0);
        AddNetFrame(buffer_frame.data());
      }
    }
}

void Dojo::AdvanceFrame()
{
    // replays: save previous frame to file
    if (record && index > 1)
    {
      if (netplay && PlayerFramesFilled(index - 1))
      {
        Replay::AppendFrameToFile(net_frames[0].at(index - 1));
        Replay::AppendFrameToFile(net_frames[1].at(index - 1));
      }
      else
      {
        Replay::AppendFrameToFile(net_frames[0].at(index - 1));
      }
    }

    index++;
}

void Dojo::AddNetFrame(const char* received_data)
{
  const char data[FRAME_SIZE] = { 0 };
  memcpy((void*)data, received_data, FRAME_SIZE);

  uint32_t effective_frame_num = Dojo::Frame::GetEffectiveFrameNumber((uint8_t*)data);
  //if (effective_frame_num == 0)
    //return;

  uint32_t frame_player = (uint8_t)data[0];

  if (net_frames[frame_player].count(effective_frame_num))
    return;

  std::string data_to_queue(data, data + FRAME_SIZE);

  //std::cout << frame_player << " " << effective_frame_num << " " << Frame::GetDigital((uint8_t*)data) << std::endl;

  if (net_frames[frame_player].count(effective_frame_num) == 0 ||
    effective_frame_num > lccf)
  {
    net_frames[frame_player].emplace(effective_frame_num, data_to_queue);
    net_inputs[frame_player].emplace(effective_frame_num, Frame::GetDigital((uint8_t*)data));

    if (netplay && (effective_frame_num == lccf + 1) && PlayerInputsFilled(effective_frame_num))
    {
      lccf++;
      //std::cout << "last consecutive common frame " << lccf << std::endl;
    }

    if (netplay && frame_player == player)
      Netplay::frames_to_send.push(data_to_queue);
  }

  //std::cout << Frame::Str((uint8_t*)data_to_queue.data()) << std::endl;
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

bool Dojo::PlayerInputsFilled(uint32_t i = index)
{
  bool condition = net_inputs[0].count(i) && net_inputs[1].count(i);
  return condition;
}

bool Dojo::PlayerFramesFilled(uint32_t i = index)
{
  bool condition = net_frames[0].count(i) && net_frames[1].count(i);
  return condition;
}
