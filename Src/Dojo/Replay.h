#include <fstream>
#include <iomanip>
#include <cstdio>
#include <cstring>
#include <filesystem>

namespace Dojo::Replay {
  static uint32_t replay_frame_count;
  static Message::Writer replay_msg;

  std::string currentISO8601TimeUTC();

  std::string CreateReplayFile(const std::string& game_name, const std::string& state_path);
  void AppendHeaderToFile(const std::string& game_name);
  void AppendPlayerInfoToFile();
  void AppendFrameToFile(const std::string& frame);
  void LoadFile(const std::string& path);
  std::string GetStatePath();

  std::string Takeover(int player);
  inline bool p1_override = false;
  inline bool p2_override = false;

  inline std::string file_path;
  inline std::string clip_state;
} // namespace Dojo::Replay
