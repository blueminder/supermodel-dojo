#include "Dojo.h"

std::string Dojo::Frame::Create(uint32_t frame_num, int player, uint32_t delay, uint32_t digital)
{
  uint8_t new_frame[FRAME_SIZE] = { 0 };
  new_frame[0] = player;
  new_frame[1] = delay;

  // enter current frame count in next 4 bytes
  memcpy(new_frame + 2, (uint8_t*)&frame_num, sizeof(uint32_t));

  if (digital != 0)
    memcpy(new_frame + 6, (uint8_t*)&digital, sizeof(uint32_t));

  uint8_t ret_frame[FRAME_SIZE] = { 0 };
  memcpy((void*)ret_frame, new_frame, FRAME_SIZE);

  std::string frame_str(ret_frame, ret_frame + FRAME_SIZE);

  return frame_str;
}

uint32_t Dojo::Frame::GetPlayer(uint8_t* data)
{
  return (uint32_t)data[0];
}

uint32_t Dojo::Frame::GetDelay(uint8_t* data)
{
  return (uint32_t)data[1];
}

uint32_t Dojo::Frame::GetFrameNumber(uint8_t* data)
{
  return (*(uint32_t*)(data + 2));
}

uint32_t Dojo::Frame::GetEffectiveFrameNumber(uint8_t* data)
{
  return GetFrameNumber(data) + GetDelay(data);
}

uint32_t Dojo::Frame::GetDigital(uint8_t* data)
{
  return (*(uint32_t*)(data + 6));
}

std::string Dojo::Frame::Str(uint8_t* data)
{
  std::ostringstream OutStream;
  auto player = GetPlayer(data);
  auto delay = GetDelay(data);
  auto frame_num = GetFrameNumber(data);
  auto effective_frame_num = GetEffectiveFrameNumber(data);
  auto digital = GetDigital(data);
  auto digital_bits = std::bitset<32>(digital);

  OutStream << effective_frame_num << ": P" << player << " F" << frame_num << " D" << delay << " " << digital_bits.to_string();
  return OutStream.str();
}
