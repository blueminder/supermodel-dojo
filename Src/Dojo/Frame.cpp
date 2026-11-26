#include "Dojo.h"

std::string Dojo::Frame::Create(uint32_t frame_num, int player, uint32_t delay, uint32_t digital, uint32_t analog, uint32_t gun1, uint32_t gun2)
{
  uint8_t new_frame[DOJO_FRAME_SIZE] = { 0 };
  new_frame[0] = player;
  new_frame[1] = delay;

  // enter current frame count in next 4 bytes
  memcpy(new_frame + 2, (uint8_t*)&frame_num, sizeof(uint32_t));

  if (digital != 0)
    memcpy(new_frame + 6, (uint8_t*)&digital, sizeof(uint32_t));

  // bytes 10-13: packed analog gun axes, P1 X | P1 Y << 8 | P2 X << 16 | P2 Y << 24
  if (analog != 0)
    memcpy(new_frame + 10, (uint8_t*)&analog, sizeof(uint32_t));

  // bytes 14-17: P1 serial gun word, X | Y << 16 (raw axis values)
  if (gun1 != 0)
    memcpy(new_frame + 14, (uint8_t*)&gun1, sizeof(uint32_t));

  // bytes 18-21: P2 serial gun word, X | Y << 16 (raw axis values)
  if (gun2 != 0)
    memcpy(new_frame + 18, (uint8_t*)&gun2, sizeof(uint32_t));

  uint8_t ret_frame[DOJO_FRAME_SIZE] = { 0 };
  memcpy((void*)ret_frame, new_frame, DOJO_FRAME_SIZE);

  std::string frame_str(ret_frame, ret_frame + DOJO_FRAME_SIZE);

  return frame_str;
}

uint32_t Dojo::Frame::GetPlayer(const uint8_t* data)
{
  return (uint32_t)data[0];
}

uint32_t Dojo::Frame::GetDelay(const uint8_t* data)
{
  return (uint32_t)data[1];
}

uint32_t Dojo::Frame::GetFrameNumber(const uint8_t* data)
{
  return (*(uint32_t*)(data + 2));
}

uint32_t Dojo::Frame::GetEffectiveFrameNumber(const uint8_t* data)
{
  return GetFrameNumber(data) + GetDelay(data);
}

uint32_t Dojo::Frame::GetDigital(const uint8_t* data)
{
  return (*(uint32_t*)(data + 6));
}

uint32_t Dojo::Frame::GetAnalog(const uint8_t* data)
{
  return (*(uint32_t*)(data + 10));
}

uint32_t Dojo::Frame::GetGun1(const uint8_t* data)
{
  return (*(uint32_t*)(data + 14));
}

uint32_t Dojo::Frame::GetGun2(const uint8_t* data)
{
  return (*(uint32_t*)(data + 18));
}

std::string Dojo::Frame::Str(const uint8_t* data)
{
  std::ostringstream OutStream;
  auto player = GetPlayer(data);
  auto delay = GetDelay(data);
  auto frame_num = GetFrameNumber(data);
  auto effective_frame_num = GetEffectiveFrameNumber(data);
  auto digital = GetDigital(data);
  auto digital_bits = std::bitset<32>(digital);
  auto analog = GetAnalog(data);
  auto gun1 = GetGun1(data);
  auto gun2 = GetGun2(data);

  OutStream << effective_frame_num << ": P" << player << " F" << frame_num << " D" << delay << ' ' << digital_bits.to_string()
    << " A" << std::hex << analog << " G1 " << gun1 << " G2 " << gun2;
  return OutStream.str();
}
