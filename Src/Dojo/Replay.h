#include <fstream>
#include <iomanip>
#include <chrono>
#include <cstdio>
#include <cstring>

namespace Dojo::Replay {
  static uint32_t replay_frame_count;
  static Message::Writer replay_msg;

  std::string currentISO8601TimeUTC();

  std::string CreateReplayFile(std::string game_name);
  void AppendHeaderToFile(std::string game_name);
  void AppendPlayerInfoToFile();
  void AppendFrameToFile(std::string frame);
  void LoadFile(std::string path);

  std::string Takeover(int player);
  inline bool p1_override = false;
  inline bool p2_override = false;
} // namespace Dojo::Replay
