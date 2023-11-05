namespace Dojo::Training {
  static bool enabled = false;
  static bool recording;
  inline std::vector<std::string> record_slot[3];
  inline std::set<int> recorded_slots;

  void TrainingFrameAction();

  std::string ToggleRecording(int slot);
  std::string TogglePlayback(int slot);
  std::string ToggleRandomPlayback();
  std::string ToggleLoop();
  std::string TogglePlayerSwap();

  void PlayRecording(int slot);
  void ResetTraining();

  static bool playback_loop = false;
  static bool playing_input = false;

  static bool trigger_playback[3] = { false };
  static uint32_t next_playback_frame;

  static int record_player = 0;
  static int current_record_slot = 0;
} // namespace Dojo::Training
