#include "../Src/Dojo/Dojo.h"
#include "../Src/Dojo/Replay.h"
#include <cassert>
#include <cstdio>
#include <filesystem>

// stub: not under test
namespace FileSystemPath {
  enum PathType { Analysis, Config, Log, NVRAM, Saves, Screenshots, Assets, Replays, ROMs };
  std::string GetPath(PathType) { return "/tmp/dojo_replay_test/"; }
}

// stubs for symbols referenced by Message.cpp / Dojo.cpp, not under test
void Dojo::AddNetFrame(const char*) {}
void Dojo::FillDelay() {}

static void AppendMessage(std::ofstream& fout, Dojo::Message::Writer& w)
{
  std::vector<uint8_t> message = w.Msg();   // updates the writer's size member first
  fout.write((const char*)message.data(), (std::streamsize)(w.GetSize() + (uint32_t)HEADER_LEN));
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

static void AppendFrames(std::ofstream& fout, int firstFrame, int count, bool bothPlayers)
{
  for (int i = 0; i < count; i++) {
    Dojo::Message::Writer w;
    w.AppendHeader(0, GAME_BUFFER);
    w.AppendInt(FRAME_SIZE);
    std::string f1 = Dojo::Frame::Create(firstFrame + i, 0, 0, 0xABCD);
    w.AppendContinuousData(f1.data(), FRAME_SIZE);
    if (bothPlayers) {
      std::string f2 = Dojo::Frame::Create(firstFrame + i, 1, 0, 0x1234);
      w.AppendContinuousData(f2.data(), FRAME_SIZE);
    }
    AppendMessage(fout, w);
  }
}

int main()
{
  std::filesystem::create_directories("/tmp/dojo_replay_test");
  const std::string dir = "/tmp/dojo_replay_test/";

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

  // --- version 7: unknown future version must read as invalid, not crash
  {
    std::ofstream fout(dir + "v7.supr", std::ios::binary);
    auto s = StartMsg("vf3", "Alice", "Bob", 7);
    AppendMessage(fout, s);
  }
  auto hv7 = Dojo::Replay::ReadReplayHeader(dir + "v7.supr");
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
