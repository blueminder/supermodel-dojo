namespace Dojo::Frame {
  // frame data extraction methods
  uint32_t GetPlayer(const uint8_t* data);
  uint32_t GetDelay(const uint8_t* data);
  uint32_t GetFrameNumber(const uint8_t* data);
  uint32_t GetEffectiveFrameNumber(const uint8_t* data);
  uint32_t GetDigital(const uint8_t* data);
  uint32_t GetAnalog(const uint8_t* data);
  uint32_t GetGun1(const uint8_t* data);
  uint32_t GetGun2(const uint8_t* data);
  std::string Str(const uint8_t* data);

  std::string Create(uint32_t frame_num, int player, uint32_t delay, uint32_t digital, uint32_t analog, uint32_t gun1 = 0, uint32_t gun2 = 0);
} // namespace Dojo::Frame
