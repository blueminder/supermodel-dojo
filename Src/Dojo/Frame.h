namespace Dojo::Frame {
  // frame data extraction methods
  uint32_t GetPlayer(const uint8_t* data);
  uint32_t GetDelay(const uint8_t* data);
  uint32_t GetFrameNumber(const uint8_t* data);
  uint32_t GetEffectiveFrameNumber(const uint8_t* data);
  uint32_t GetDigital(const uint8_t* data);
  std::string Str(const uint8_t* data);

  std::string Create(uint32_t frame_num, int player, uint32_t delay, uint32_t digital);
} // namespace Dojo::Frame
