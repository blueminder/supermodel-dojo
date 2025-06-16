#pragma once

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
#include <queue>
#include <mutex>

#include "Message.h"
#include "Frame.h"
#include "Replay.h"
#include "Training.h"

#include "InputPoll.h"
#include "Receiver.h"
#include "Netplay.h"

#include "OSD/Logger.h"

#define FRAME_SIZE 15
#define HEADER_LEN 12

namespace Dojo {
    void Init(const std::string& game_name, bool record_session, bool train_session, bool receiving, bool hosting, bool netplay, const std::string& state_path);
    void FillDelay();
    void AdvanceFrame();
    void AddNetFrame(const char* received_data);

    uint32_t WipePlayerInputs(int player_no, uint32_t digital);
    bool PlayerInputsFilled(uint32_t i);
    bool PlayerFramesFilled(uint32_t i);

    inline uint32_t index = 0;
    inline uint32_t delay = 0;
    inline std::map<uint32_t, std::string> net_frames[2];
    inline std::map<uint32_t, uint32_t> net_inputs[2];
    inline std::string current_frame;

    inline uint32_t lccf = 0;

    inline bool record = false;
    inline bool playback = false;
    inline bool training = false;
    inline bool receiving = false;
    inline bool hosting = false;
    inline bool netplay = false;

    inline int player = 0;

    inline bool players_swapped = false;
    inline bool paused = false;

    inline std::string target_ip;
    inline uint16_t target_port = 5000;
    inline uint16_t source_port = 0;

    inline bool net_replay = false;

    inline std::mutex inputs_mtx;
};

#endif  // INCLUDED_DOJO_H
