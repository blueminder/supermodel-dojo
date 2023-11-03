#define GAME_START 1
#define GAME_BUFFER 2
#define SPECTATE_START 3
#define SPECTATE_REQUEST 4
#define PLAYER_INFO 5
#define GAME_END 6

#define HEADER_SIZE 12
#define FRAME_BATCH 120

namespace Dojo::Message {
  uint32_t GetSize(const uint8_t* buffer);
  uint32_t GetSeq(const uint8_t* buffer);
  uint32_t GetCmd(const uint8_t* buffer);

  uint32_t ReadInt(const char* buffer, int* offset);
  std::string ReadString(const char* buffer, int* offset);
  std::string ReadContinuousData(const char* buffer, int* offset, uint32_t len);
  std::vector<std::string> ReadPlayerInfo(const char* buffer, int* offset);

  void ProcessBody(uint32_t cmd, uint32_t body_size, const char* buffer, int* offset);

  std::vector<std::string> SplitString(const std::string input, const char& delimiter);

  class Writer
  {
  private:
    std::vector<uint8_t> message;
    uint32_t size;

  public:
    Writer();

    void AppendHeader(uint32_t _sequence, uint32_t _command);

    uint32_t UpdateSize();
    uint32_t GetSize();

    void AppendInt(uint32_t value);
    void AppendString(std::string value);
    void AppendData(const char* value, uint32_t size);

    // append int by divisible data size after header before calling
    void AppendContinuousData(const char* value, uint32_t size);
    std::vector<uint8_t> Msg();
  }; // class Writer

}  // namespace Dojo::Message

