#include <fstream>
#include <iomanip>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <filesystem>

namespace Dojo::Replay {
  static uint32_t replay_frame_count;
  static Message::Writer replay_msg;

  std::string currentISO8601TimeUTC();

  std::string CreateReplayFile(std::string game_name, std::string state_path);
  void AppendHeaderToFile(std::string game_name);
  void AppendPlayerInfoToFile();
  void AppendFrameToFile(std::string frame);
  void LoadFile(std::string path);
  std::string GetStatePath();

  std::string Takeover(int player);
  inline bool p1_override = false;
  inline bool p2_override = false;

  inline std::string file_path;
  inline std::string clip_state;
} // namespace Dojo::Replay
