namespace Dojo::Frame {
  // frame data extraction methods
  uint32_t GetPlayer(uint8_t* data);
  uint32_t GetDelay(uint8_t* data);
  uint32_t GetFrameNumber(uint8_t* data);
  uint32_t GetEffectiveFrameNumber(uint8_t* data);
  uint32_t GetDigital(uint8_t* data);

  std::string Create(uint32_t frame_num, int player, uint32_t delay, uint32_t digital);
} // namespace Dojo::Frame
