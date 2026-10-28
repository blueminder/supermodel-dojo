#include "Dojo.h"

void Dojo::Init(const std::string& game_name, bool record_session, bool train_session, bool receiving, bool hosting, bool netplay, const std::string& state_path)
{
    index = 0;
    Dojo::hosting = hosting;
    Dojo::game_name = game_name;
    replay_rejected = false;
    Replay::header_version = 0;

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
    }

    Dojo::hosting = hosting;

    if (record)
    {
        std::cout << "Recording Session" << std::endl;
        // session replacement: flush the previous recording first
        Replay::FinalizeRecording();
        std::string replay_fn = Replay::CreateReplayFile(game_name, state_path);
        std::cout << replay_fn << std::endl;
    }
    else if (playback)
    {
        std::cout << "Playing Replay" << std::endl;
        Replay::LoadFile(Replay::file_path);
        if (!net_inputs[1].empty())
          net_replay = true;
        Replay::NormalizeLegacyKeys();
        // recorded replays only: live spectate has no final frame
        if (!receiving)
          Replay::ValidateFinalFrame();
    }

    if (!netplay)
      FillDelay();
}

void Dojo::FillDelay()
{
  // add buffer frames for delay
  for (uint32_t d = 0; d < delay; d++)
  {
    auto p1_frame = Dojo::Frame::Create(d, 0, 0, 0);
    auto p2_frame = Dojo::Frame::Create(d, 1, 0, 0);

    AddNetFrame(p1_frame.data());
    AddNetFrame(p2_frame.data());
  }
}

void Dojo::AdvanceFrame()
{
    // replays: record the frame about to execute
    if (record)
    {
      std::lock_guard<std::mutex> lock(inputs_mtx);

      if (!netplay)
      {
        // solo: delay prefix and local input are already in the map
        Replay::AppendFrameToFile(net_frames[0].at(index));
      }
      else if (PlayerFramesFilledNoLock(index))
      {
        Replay::AppendFrameToFile(net_frames[0].at(index));
        Replay::AppendFrameToFile(net_frames[1].at(index));
      }
      else
      {
        // reject incomplete player pairs: stop the recording
        record = false;
        Replay::FinalizeRecording();
        std::cout << "Recording Stopped: Incomplete Player Pair At Frame " << index << std::endl;
      }
    }

    index++;
}

void Dojo::AddNetFrame(const char* received_data)
{
  char data[FRAME_SIZE];
  memcpy((void*)data, received_data, FRAME_SIZE);

  uint32_t effective_frame_num = Dojo::Frame::GetEffectiveFrameNumber((uint8_t*)data);
  //if (effective_frame_num == 0)
    //return;

  uint32_t frame_player = (uint8_t)data[0];

  std::string data_to_queue(data, data + FRAME_SIZE);

  std::lock_guard<std::mutex> lock(inputs_mtx);

  if (net_frames[frame_player].count(effective_frame_num))
    return;

  net_frames[frame_player].emplace(effective_frame_num, data_to_queue);
  net_inputs[frame_player].emplace(effective_frame_num, Frame::GetDigital((uint8_t*)data));

  // idempotent advance: a lost update cannot freeze the chain
  if (netplay)
  {
    while (PlayerInputsFilledNoLock(lccf + 1))
      lccf++;
    //std::cout << "last consecutive common frame " << lccf << std::endl;
  }

  if (netplay && frame_player == player)
    Netplay::frames_to_send.push(data_to_queue);

  //std::cout << Frame::Str((uint8_t*)data_to_queue.data()) << std::endl;
}

uint32_t Dojo::WipePlayerInputs(int player_no, uint32_t digital)
{
  uint32_t input_mask = 0;
  if (player_no == 0)
    input_mask = 986965;
  else if (player_no == 1)
    input_mask = 15790250;

  return digital & ~input_mask;
}

bool Dojo::PlayerInputsFilled(uint32_t i)
{
  std::lock_guard<std::mutex> lock(inputs_mtx);
  return PlayerInputsFilledNoLock(i);
}

bool Dojo::PlayerFramesFilled(uint32_t i)
{
  std::lock_guard<std::mutex> lock(inputs_mtx);
  return PlayerFramesFilledNoLock(i);
}

bool Dojo::PlayerInputsFilledNoLock(uint32_t i)
{
  return net_inputs[0].count(i) && net_inputs[1].count(i);
}

bool Dojo::PlayerFramesFilledNoLock(uint32_t i)
{
  return net_frames[0].count(i) && net_frames[1].count(i);
}

void Dojo::TrimNetMapsNoLock()
{
  // drop stale entries: bounds memory on long sessions
  // safety window matches receiving-buffer (600 frames)
  const uint32_t cutoff = (index > 600) ? index - 600 : 0;

  for (int p = 0; p < 2; p++)
  {
    net_frames[p].erase(net_frames[p].begin(), net_frames[p].lower_bound(cutoff));
    net_inputs[p].erase(net_inputs[p].begin(), net_inputs[p].lower_bound(cutoff));
  }
}
