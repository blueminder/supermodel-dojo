#ifndef _GUI_H_
#define _GUI_H_

#include <string>
#include <vector>

namespace Util
{
  namespace Config
  {
    class Node;
  }
}

// Runs the settings GUI. fileConfig is the complete contents of the config
// file; only its global section is edited, all other sections are preserved
// when the file is written back out. launchTraining starts game in Training mode.
// replayFile receives the path of the replay to launch, if any was selected
std::vector<std::string> RunGUI(const std::string& configPath, const Util::Config::Node& fileConfig, bool& launchTraining, std::string& replayFile);

#endif // !_GUI_H_
