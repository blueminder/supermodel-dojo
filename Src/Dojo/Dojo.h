#ifndef INCLUDED_DOJO_H
#define INCLUDED_DOJO_H

#include <cinttypes>
#include <iostream>
#include <sstream>
#include <vector>
#include <bitset>
#include <algorithm>
#include <set>
#include <map>

#include "Message.h"
#include "Frame.h"
#include "Replay.h"
#include "Training.h"

#include "Poll.h"

#define FRAME_SIZE 10
#define HEADER_LEN 12

namespace Dojo {
    void Init(std::string game_name, bool record_session, bool train_session);
    void AdvanceFrame();
    void AddNetFrame(const char* received_data);

    inline uint32_t index = 0;
    inline uint32_t delay = 0;
    inline std::string replay_filename;
    inline std::map<uint32_t, std::string> net_frames[2];
    inline std::map<uint32_t, uint32_t> net_inputs[2];
    inline std::string current_frame;

    inline bool record = false;
    inline bool playback = false;
    inline bool training = false;

    inline bool players_swapped = false;
};

#endif  // INCLUDED_DOJO_H
