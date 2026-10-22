#pragma once

#include <fstream>
#include <iomanip>
#include <cstdio>
#include <cstring>
#include <filesystem>

namespace Dojo::Replay {
  struct ReplayHeader
  {
    bool valid = false;
    std::string game_name;
    std::string player1;
    std::string player2;
    uint32_t frame_count = 0;
  };

  std::string currentISO8601TimeUTC();

  std::string CreateReplayFile(const std::string& game_name, const std::string& state_path);
  void AppendHeaderToFile(const std::string& game_name);
  void AppendPlayerInfoToFile();
  void AppendFrameToFile(const std::string& frame);
  void FinalizeRecording();
  void LoadFile(const std::string& path);

  ReplayHeader ReadReplayHeader(const std::string& path);
  uint32_t GetReplayFrameCount(const std::string& path);
  std::string GetStatePath();

  std::string Takeover(int player);
  inline bool p1_override = false;
  inline bool p2_override = false;

  inline std::string file_path;
  inline std::string clip_state;
} // namespace Dojo::Replay
