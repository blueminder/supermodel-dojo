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
#include <atomic>

#include "Message.h"
#include "Frame.h"
#include "Replay.h"
#include "Training.h"

#include "InputPoll.h"
#include "Receiver.h"
#include "Netplay.h"

#include "OSD/Logger.h"

#define DOJO_FRAME_SIZE 22
#define DOJO_HEADER_LEN 12
// analog gun rest position: 0x80 per axis, all four bytes
#define DOJO_ANALOG_GUN_REST 0x80808080
// serial gun rest position: X=400, Y=272 per player word
#define DOJO_SERIAL_GUN_REST ((uint32_t)400 | ((uint32_t)272 << 16))
// vehicle rest position: steering centered (0x80), pedals released
#define DOJO_VEHICLE_REST 0x00000080u

namespace Dojo {
    void Init(const std::string& game_name, bool record_session, bool train_session, bool receiving, bool hosting, bool netplay, const std::string& state_path);
    void FillDelay();
    void AdvanceFrame();
    void AddNetFrame(const char* received_data);

    uint32_t WipePlayerInputs(int player_no, uint32_t digital);
    uint32_t WipeAnalogPlayerInputs(int player_no, uint32_t analog);
    uint64_t WipeGunPlayerInputs(int player_no, uint64_t gun);
    // shared-seat racing: per byte, value farthest from rest wins, P1 wins ties
    uint32_t MergeVehicleAnalog(uint32_t p1_analog, uint32_t p2_analog);
    bool PlayerInputsFilled(uint32_t i);
    bool PlayerFramesFilled(uint32_t i);
    // no-lock variants: caller must already hold inputs_mtx
    bool PlayerInputsFilledNoLock(uint32_t i);
    bool PlayerFramesFilledNoLock(uint32_t i);
    void TrimNetMapsNoLock();

    inline uint32_t index = 0;
    // cross-thread flags: written and read from the net and main threads
    inline std::atomic<uint32_t> delay = 0;
    inline std::map<uint32_t, std::string> net_frames[2];
    inline std::map<uint32_t, uint32_t> net_inputs[2];
    inline std::map<uint32_t, uint32_t> net_analog[2];
    // serial gun words per player frame: P1 X | P1 Y << 16 | P2 X << 32 | P2 Y << 48
    inline std::map<uint32_t, uint64_t> net_gun[2];
    // vehicle digital bits: one seat, either player drives
    inline uint32_t shared_input_mask = 0;
    inline uint32_t player_input_masks[2] = { 0, 0 };
    inline std::string current_frame;

    inline std::string game_name;
    inline bool serial_gun_game = false;
    inline bool vehicle_game = false;
    inline uint32_t ppc_frequency = 0;
    // effective session refresh rate in milliHz; stored in replay headers
    inline uint32_t refresh_rate_millihz = 0;
    inline std::atomic<bool> mismatch = false;
    // set at replay load, consumed by the main loop before frame 0
    inline bool replay_rejected = false;
    // set by the net thread at handshake, consumed by the main loop before frame 0
    inline std::atomic<bool> clear_nvram = false;

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

    // per-frame input log switch (-dojo-debug)
    inline bool debug = false;

    inline std::mutex inputs_mtx;
};

#endif  // INCLUDED_DOJO_H
