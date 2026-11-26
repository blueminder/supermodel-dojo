//   gcc -O1 -DBLAKE3_NO_SSE2 -DBLAKE3_NO_SSE41 -DBLAKE3_NO_AVX2 -DBLAKE3_NO_AVX512 -DBLAKE3_USE_NEON=0 \
//     -c -I Libraries/blake3 Libraries/blake3/blake3.c Libraries/blake3/blake3_dispatch.c Libraries/blake3/blake3_portable.c
//   g++ -std=c++17 -O1 -I Src -I Src/OSD/SDL $(sdl2-config --cflags) \
//     Scripts/test_replay_headers.cpp Src/Dojo/Message.cpp Src/Dojo/Frame.cpp Src/Dojo/Replay.cpp \
//     Src/Dojo/InputPoll.cpp Src/Inputs/InputTypes.cpp Src/Util/NewConfig.cpp blake3*.o -o /tmp/test_replay_headers
#include "../Src/Dojo/Dojo.h"
#include "../Src/Dojo/InputPoll.h"
#include "../Src/Inputs/InputTypes.h"
#include "../Src/Dojo/Replay.h"
#include <cassert>
#include <cstdio>
#include <filesystem>
#include <cstdarg>

// stubs: logger symbols referenced by Message.cpp, not under test
Result ErrorLog(const char*, ...) { return Result::FAIL; }
void InfoLog(const char*, ...) {}

// stub: not under test
namespace FileSystemPath {
  enum PathType { Analysis, Config, Log, NVRAM, Saves, Screenshots, Assets, Replays, ROMs };
  std::string GetPath(PathType) { return "/tmp/dojo_replay_test/"; }
}

// stubs for symbols referenced by Message.cpp / Dojo.cpp, not under test
// AddNetFrame records every padded frame so ProcessBody stays under test
std::vector<std::string> added_frames;
void Dojo::AddNetFrame(const char* data)
{
  added_frames.emplace_back(data, data + DOJO_FRAME_SIZE);
}
void Dojo::FillDelay() {}

// stubs: Dojo.cpp / Training.cpp symbols referenced by InputPoll.cpp, not under test
void Dojo::Training::TrainingFrameAction() {}
bool Dojo::PlayerInputsFilledNoLock(uint32_t) { return false; }
bool Dojo::PlayerFramesFilledNoLock(uint32_t) { return false; }
uint32_t Dojo::WipePlayerInputs(int, uint32_t v) { return v; }
uint32_t Dojo::WipeAnalogPlayerInputs(int, uint32_t v) { return v; }
uint64_t Dojo::WipeGunPlayerInputs(int, uint64_t v) { return v; }
bool Dojo::PlayerInputsFilled(uint32_t) { return false; }
void Dojo::TrimNetMapsNoLock() {}

// minimal CInput ctor/dtor: the real ones live in Input.cpp, which drags in the
// whole input-system stack; the capture test only reads id and value
CInput::CInput(const char *inputId, const char *inputLabel, unsigned inputFlags, unsigned inputGameFlags,
  const char *defaultMapping, UINT16 initValue)
  : m_defaultMapping(defaultMapping), m_system(), m_source(NULL),
    id(inputId), label(inputLabel), flags(inputFlags), gameFlags(inputGameFlags),
    value(initValue), prevValue(initValue)
{
  m_mapping[0] = '\0';
}
CInput::~CInput() {}

// stubs: pulled in via vtables by InputTypes.cpp, not under test
void CInput::LoadFromConfig(const Util::Config::Node&) {}
void CInput::StoreToConfig(Util::Config::Node*) {}
int CInputSource::Clamp(int val, int minVal, int maxVal)
{
  return val < minVal ? minVal : (val > maxVal ? maxVal : val);
}

static void AppendMessage(std::ofstream& fout, Dojo::Message::Writer& w)
{
  std::vector<uint8_t> message = w.Msg();   // updates the writer's size member first
  fout.write((const char*)message.data(), (std::streamsize)(w.GetSize() + (uint32_t)DOJO_HEADER_LEN));
}

static Dojo::Message::Writer StartMsg(const std::string& game, const std::string& p1, const std::string& p2,
  uint32_t version = 1, const std::string& rom_hash = "")
{
  Dojo::Message::Writer w;
  w.AppendHeader(1, SPECTATE_START);
  w.AppendInt(version);
  w.AppendString(game);
  w.AppendString(p1);
  w.AppendString(p2);
  w.AppendString("Quark");
  w.AppendString("MatchCode");
  if (version >= 5)
  {
    w.AppendInt(166);     // ppc frequency MHz
    w.AppendInt(57524);   // refresh rate milliHz
  }
  if (version >= 6)
    w.AppendString(rom_hash);
  return w;
}

// frameSize < DOJO_FRAME_SIZE simulates a legacy recording
static void AppendFrames(std::ofstream& fout, int firstFrame, int count, bool bothPlayers,
  uint32_t frameSize = DOJO_FRAME_SIZE, uint32_t gun1 = 0, uint32_t gun2 = 0)
{
  for (int i = 0; i < count; i++) {
    Dojo::Message::Writer w;
    w.AppendHeader(0, GAME_BUFFER);
    w.AppendInt(frameSize);
    std::string f1 = Dojo::Frame::Create(firstFrame + i, 0, 0, 0xABCD, 0, gun1, gun2);
    f1.resize(frameSize);                       // legacy batches carry short frames
    w.AppendContinuousData(f1.data(), frameSize);
    if (bothPlayers) {
      std::string f2 = Dojo::Frame::Create(firstFrame + i, 1, 0, 0x1234, 0, gun1, gun2);
      f2.resize(frameSize);
      w.AppendContinuousData(f2.data(), frameSize);
    }
    AppendMessage(fout, w);
  }
}

int main()
{
  std::filesystem::create_directories("/tmp/dojo_replay_test");
  const std::string dir = "/tmp/dojo_replay_test/";

  // --- frame gun word round-trip: edges and neutral
  {
    const uint32_t gun1 = 150 | (80u << 16);
    const uint32_t gun2 = 651 | (465u << 16);
    std::string f = Dojo::Frame::Create(7, 0, 0, 0, 0, gun1, gun2);
    assert(f.size() == DOJO_FRAME_SIZE);
    assert(Dojo::Frame::GetGun1((const uint8_t*)f.data()) == gun1);
    assert(Dojo::Frame::GetGun2((const uint8_t*)f.data()) == gun2);

    const uint32_t neutral = 400 | (272u << 16);
    std::string n = Dojo::Frame::Create(8, 1, 0, 0, 0, neutral, neutral);
    assert(Dojo::Frame::GetGun1((const uint8_t*)n.data()) == neutral);
    assert(Dojo::Frame::GetGun2((const uint8_t*)n.data()) == neutral);

    // defaults: gun words zero
    std::string z = Dojo::Frame::Create(9, 0, 0, 0, 0);
    assert(Dojo::Frame::GetGun1((const uint8_t*)z.data()) == 0);
    assert(Dojo::Frame::GetGun2((const uint8_t*)z.data()) == 0);

    assert(neutral == DOJO_SERIAL_GUN_REST);
    std::printf("gun words: edges %08x/%08x round-trip ok, neutral %08x ok\n", gun1, gun2, neutral);
  }

  // --- serial gun axis capture: all four axes survive the packed gun word
  // (regression: a bad clear-mask in AxisAction wiped GunY and GunX2, pinning
  //  P1 to the top edge and P2 to the left edge in replays and netplay)
  {
    auto mk = [](const char* id) {
      return std::make_shared<CAxisInput>(id, "test", Game::INPUT_GUN1, "NONE",
        std::shared_ptr<CAnalogInput>(), std::shared_ptr<CAnalogInput>(), 0, 0, 0xFFFF);
    };
    auto gx = mk("GunX"), gy = mk("GunY"), gx2 = mk("GunX2"), gy2 = mk("GunY2");
    gx->value = 291; gy->value = 102; gx2->value = 400; gy2->value = 272;

    added_frames.clear();
    Dojo::Poll::StartAction();          // resets the packed word
    Dojo::Poll::AxisAction(gx);         // m_inputs order: X, Y, X2, Y2
    Dojo::Poll::AxisAction(gy);
    Dojo::Poll::AxisAction(gx2);
    Dojo::Poll::AxisAction(gy2);
    Dojo::record = true;
    Dojo::Poll::EndAction();
    Dojo::record = false;

    assert(added_frames.size() == 1);
    assert(Dojo::Frame::GetGun1((const uint8_t*)added_frames[0].data()) == (291u | (102u << 16)));
    assert(Dojo::Frame::GetGun2((const uint8_t*)added_frames[0].data()) == (400u | (272u << 16)));
    std::printf("serial gun capture: x1=291 y1=102 x2=400 y2=272 all intact\n");
  }

  // --- solo replay: 120 game frames, only P1 stored per game frame
  {
    std::ofstream fout(dir + "solo.supr", std::ios::binary);
    auto s = StartMsg("vf3", "Alice", "Bob");
    AppendMessage(fout, s);
    AppendFrames(fout, 0, 120, false);
  }
  auto h = Dojo::Replay::ReadReplayHeader(dir + "solo.supr");
  assert(h.valid);
  assert(h.game_name == "vf3");
  assert(h.player1 == "Alice");
  assert(h.player2 == "Bob");
  assert(Dojo::Replay::GetReplayFrameCount(dir + "solo.supr") == 120);
  std::printf("solo: valid=%d game=%s p1=%s p2=%s frames=%u\n",
      h.valid, h.game_name.c_str(), h.player1.c_str(), h.player2.c_str(),
      Dojo::Replay::GetReplayFrameCount(dir + "solo.supr"));

  // --- netplay replay: 90 game frames, P1+P2 stored per game frame (180 raw frames)
  {
    std::ofstream fout(dir + "netplay.supr", std::ios::binary);
    auto s = StartMsg("daytona2", "P1", "P2");
    AppendMessage(fout, s);
    Dojo::Message::Writer pi;
    pi.AppendHeader(1, PLAYER_INFO);
    pi.AppendString("P1#undefined,0,XX");
    pi.AppendString("P2#undefined,0,XX");
    AppendMessage(fout, pi);        // body must be skipped by the frame counter
    AppendFrames(fout, 0, 90, true);
  }
  auto hn = Dojo::Replay::ReadReplayHeader(dir + "netplay.supr");
  assert(hn.valid && hn.game_name == "daytona2");
  assert(Dojo::Replay::GetReplayFrameCount(dir + "netplay.supr") == 90); // max per player, not 180
  std::printf("netplay: frames=%u (expected 90 game frames, 180 raw)\n",
      Dojo::Replay::GetReplayFrameCount(dir + "netplay.supr"));

  // --- version 5 replay: header carries session settings
  {
    std::ofstream fout(dir + "v5.supr", std::ios::binary);
    auto s = StartMsg("oceanhun", "Alice", "Bob", 5);
    AppendMessage(fout, s);
    AppendFrames(fout, 0, 10, false);
  }
  auto hv5 = Dojo::Replay::ReadReplayHeader(dir + "v5.supr");
  assert(hv5.valid);
  assert(hv5.game_name == "oceanhun");
  assert(hv5.ppc_frequency_mhz == 166);
  assert(hv5.refresh_rate_millihz == 57524);
  assert(Dojo::Replay::GetReplayFrameCount(dir + "v5.supr") == 10);
  std::printf("v5: valid=%d game=%s ppc=%u refresh_millihz=%u\n",
      hv5.valid, hv5.game_name.c_str(), hv5.ppc_frequency_mhz, hv5.refresh_rate_millihz);

  // --- version 6 replay: header carries the BLAKE3 of the recording ROM
  {
    std::ofstream fout(dir + "v6.supr", std::ios::binary);
    auto s = StartMsg("vf3", "Alice", "Bob", 6,
      "a1b2c3d4a1b2c3d4a1b2c3d4a1b2c3d4a1b2c3d4a1b2c3d4a1b2c3d4a1b2c3d4");
    AppendMessage(fout, s);
  }
  auto hv6 = Dojo::Replay::ReadReplayHeader(dir + "v6.supr");
  assert(hv6.valid);
  assert(hv6.game_name == "vf3");
  assert(hv6.ppc_frequency_mhz == 166);
  assert(hv6.rom_hash == "a1b2c3d4a1b2c3d4a1b2c3d4a1b2c3d4a1b2c3d4a1b2c3d4a1b2c3d4a1b2c3d4");
  std::printf("v6: valid=%d game=%s rom_hash=%s...\n",
      hv6.valid, hv6.game_name.c_str(), hv6.rom_hash.substr(0, 8).c_str());

  // --- future version: unknown version must read as invalid, not crash
  {
    std::ofstream fout(dir + "vfuture.supr", std::ios::binary);
    auto s = StartMsg("vf3", "Alice", "Bob", Dojo::Replay::header_version_current + 1);
    AppendMessage(fout, s);
  }
  auto hv7 = Dojo::Replay::ReadReplayHeader(dir + "vfuture.supr");
  assert(!hv7.valid);

  // --- empty file
  { std::ofstream fout(dir + "empty.supr", std::ios::binary); }
  auto he = Dojo::Replay::ReadReplayHeader(dir + "empty.supr");
  assert(!he.valid);
  assert(Dojo::Replay::GetReplayFrameCount(dir + "empty.supr") == 0);

  // --- garbage file (random bytes)
  {
    std::ofstream fout(dir + "garbage.supr", std::ios::binary);
    for (int i = 0; i < 500; i++) fout.put((char)(i * 37 + 11));
  }
  auto hg = Dojo::Replay::ReadReplayHeader(dir + "garbage.supr");
  assert(!hg.valid);
  assert(Dojo::Replay::GetReplayFrameCount(dir + "garbage.supr") <= 500); // no crash, bounded
  std::printf("garbage: valid=%d frames=%u\n", hg.valid, Dojo::Replay::GetReplayFrameCount(dir + "garbage.supr"));

  // --- truncated file: full header + partial body
  {
    std::ofstream fout(dir + "trunc.supr", std::ios::binary);
    auto s = StartMsg("vf3", "Alice", "Bob");
    auto msg = s.Msg();
    fout.write((const char*)msg.data(), (std::streamsize)msg.size() - 10); // cut the body
  }
  auto ht = Dojo::Replay::ReadReplayHeader(dir + "trunc.supr");
  assert(!ht.valid);
  assert(Dojo::Replay::GetReplayFrameCount(dir + "trunc.supr") == 0);

  // --- frames without a SPECTATE_START header
  {
    std::ofstream fout(dir + "nohdr.supr", std::ios::binary);
    AppendFrames(fout, 0, 50, false);
  }
  auto hh = Dojo::Replay::ReadReplayHeader(dir + "nohdr.supr");
  assert(!hh.valid);   // no SPECTATE_START: invalid header, but frames still count
  assert(Dojo::Replay::GetReplayFrameCount(dir + "nohdr.supr") == 50);

  // --- corrupt string length (huge len must not pass the bounds check)
  {
    std::ofstream fout(dir + "badlen.supr", std::ios::binary);
    Dojo::Message::Writer w;
    w.AppendHeader(1, SPECTATE_START);
    w.AppendInt(1);                       // version
    w.AppendInt(0xFFFFFFFF);              // bogus game name length
    for (int i = 0; i < 16; i++) w.AppendInt(0x41414141); // trailing bytes
    AppendMessage(fout, w);
  }
  auto hb = Dojo::Replay::ReadReplayHeader(dir + "badlen.supr");
  assert(!hb.valid);
  assert(hb.game_name.empty());

  // --- legacy 15-byte replay of a joystick game: loads, padded, gun words zero
  {
    std::ofstream fout(dir + "legacy.supr", std::ios::binary);
    auto s = StartMsg("vf3", "Alice", "Bob");
    AppendMessage(fout, s);
    AppendFrames(fout, 0, 10, false, 15);
  }
  assert(Dojo::Replay::GetReplayFrameCount(dir + "legacy.supr") == 10);
  added_frames.clear();
  Dojo::serial_gun_game = false;
  Dojo::replay_rejected = false;
  Dojo::Replay::LoadFile(dir + "legacy.supr");
  assert(!Dojo::replay_rejected);
  assert(added_frames.size() == 10);
  for (const auto& f : added_frames)
  {
    assert(f.size() == DOJO_FRAME_SIZE);                       // zero-padded to 22
    assert(Dojo::Frame::GetGun1((const uint8_t*)f.data()) == 0);
    assert(Dojo::Frame::GetGun2((const uint8_t*)f.data()) == 0);
    assert(Dojo::Frame::GetPlayer((const uint8_t*)f.data()) == 0);
    assert(Dojo::Frame::GetDigital((const uint8_t*)f.data()) == 0xABCD);
  }
  assert(Dojo::Frame::GetFrameNumber((const uint8_t*)added_frames[9].data()) == 9);
  std::printf("legacy joystick replay: %zu frames loaded, padded %u, gun words zero\n",
      added_frames.size(), DOJO_FRAME_SIZE);

  // --- legacy 15-byte replay of a serial gun game: rejected
  {
    std::ofstream fout(dir + "legacy_gun.supr", std::ios::binary);
    auto s = StartMsg("vcop", "Alice", "Bob");
    AppendMessage(fout, s);
    AppendFrames(fout, 0, 10, false, 15);
  }
  added_frames.clear();
  Dojo::serial_gun_game = true;
  Dojo::replay_rejected = false;
  Dojo::Replay::LoadFile(dir + "legacy_gun.supr");
  assert(Dojo::replay_rejected);
  assert(added_frames.empty());
  Dojo::serial_gun_game = false;
  std::printf("legacy gun replay: rejected=%d, no frames loaded\n", Dojo::replay_rejected);

  // --- new-format 22-byte replay of a serial gun game: gun words survive the round trip
  {
    const uint32_t gun1 = 150 | (80u << 16);
    const uint32_t gun2 = 651 | (465u << 16);
    std::ofstream fout(dir + "gun.supr", std::ios::binary);
    auto s = StartMsg("vcop", "Alice", "Bob");
    AppendMessage(fout, s);
    AppendFrames(fout, 0, 10, true, DOJO_FRAME_SIZE, gun1, gun2);
  }
  assert(Dojo::Replay::GetReplayFrameCount(dir + "gun.supr") == 10);
  added_frames.clear();
  Dojo::replay_rejected = false;
  Dojo::Replay::LoadFile(dir + "gun.supr");
  assert(!Dojo::replay_rejected);
  assert(added_frames.size() == 20);
  for (const auto& f : added_frames)
  {
    assert(Dojo::Frame::GetGun1((const uint8_t*)f.data()) == (150 | (80u << 16)));
    assert(Dojo::Frame::GetGun2((const uint8_t*)f.data()) == (651 | (465u << 16)));
  }
  std::printf("new-format gun replay: %zu frames loaded, gun words intact\n", added_frames.size());

  // --- duration arithmetic used by the GUI (same formula as FormatReplayDuration)
  auto secs = [](uint32_t frames, float fps) { return (uint32_t)(frames / fps); };
  uint32_t s60 = secs(3453, 60.0f);        assert(s60 == 57);   // 0:57
  uint32_t s575 = secs(3453, 57.524f);    assert(s575 == 60);  // 1:00
  uint32_t sLong = secs(2400000, 57.524f); // >= 1 hour: renders H:MM:SS
  std::printf("durations: 3453f@60=%us 3453f@57.524=%us long=%us (%u:%02u:%02u)\n",
      s60, s575, sLong, sLong / 3600, (sLong / 60) % 60, sLong % 60);
  assert(sLong >= 3600);

  std::printf("ALL TESTS PASSED\n");
  return 0;
}
