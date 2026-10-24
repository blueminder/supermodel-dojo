#include "Dojo.h"
#include "OSD/FileSystemPath.h"

// replay recording state, internal to this file
static uint32_t replay_frame_count;
static Dojo::Message::Writer replay_msg;

std::string Dojo::Replay::currentISO8601TimeUTC()
{
  time_t itt;
  time ( &itt );
#ifndef _MSC_VER
  char buf[128] = { 0 };
  strftime(buf, sizeof buf, "%Y-%m-%dT%H:%M:%SZ", gmtime(&itt));
  return buf;
#else
  std::ostringstream ss;
  ss << std::put_time(gmtime(&itt), "%FT%TZ");
  return ss.str();
#endif
}

std::string Dojo::Replay::CreateReplayFile(const std::string& game_name, const std::string& state_path)
{
  // create timestamp string, iso8601 format
  std::string timestamp = currentISO8601TimeUTC();
  std::replace(timestamp.begin(), timestamp.end(), ':', '_');
  std::string replays_dir = FileSystemPath::GetPath(FileSystemPath::Replays);
  std::error_code ec;
  std::filesystem::create_directories(replays_dir, ec);
  std::string filename = replays_dir + game_name + '_' + timestamp;

  if (!state_path.empty())
  {
    if (std::filesystem::exists(state_path))
    {
      std::string state_filename = filename + ".st0";
      std::filesystem::copy_file(state_path, state_filename);
    }
  }

  filename.append(".supr");

  // create replay file itself
  file_path = filename;
  std::ofstream file;
  file.open(file_path);

  AppendHeaderToFile(game_name);

  return filename;
}

void Dojo::Replay::AppendHeaderToFile(const std::string& game_name)
{
  std::ofstream fout(Dojo::Replay::file_path,
    std::ios::out | std::ios::binary | std::ios_base::app);

  Message::Writer spectate_start;

  spectate_start.AppendHeader(1, SPECTATE_START);

  // version
  spectate_start.AppendInt(5);
  spectate_start.AppendString(game_name);
  spectate_start.AppendString("Player1");
  spectate_start.AppendString("Player2");

  spectate_start.AppendString("Quark");
  spectate_start.AppendString("MatchCode");

  // settings used by this session; playback applies them to emulation
  spectate_start.AppendInt(Dojo::ppc_frequency);
  spectate_start.AppendInt(Dojo::refresh_rate_millihz);

  std::vector<uint8_t> message = spectate_start.Msg();

  fout.write((const char*)spectate_start.Msg().data(), (std::streamsize)(spectate_start.GetSize() + (uint32_t)HEADER_LEN));
  fout.close();
}

void Dojo::Replay::AppendPlayerInfoToFile()
{
  std::ofstream fout(Dojo::Replay::file_path,
    std::ios::out | std::ios::binary | std::ios_base::app);

  Message::Writer player_info;
  player_info.AppendHeader(1, PLAYER_INFO);
  player_info.AppendString(std::string("Player1") + std::string("#undefined,0,XX"));
  player_info.AppendString(std::string("Player2") + std::string("#undefined,0,XX"));

  std::vector<uint8_t> message = player_info.Msg();

  fout.write((const char*)player_info.Msg().data(), (std::streamsize)(player_info.GetSize() + (uint32_t)HEADER_LEN));
  fout.close();
}

void Dojo::Replay::AppendFrameToFile(const std::string& frame)
{
  if (frame.size() == FRAME_SIZE)
  {
    // append frame data to replay file
    std::ofstream fout(Dojo::Replay::file_path,
      std::ios::out | std::ios::binary | std::ios_base::app);

    if (replay_frame_count == 0)
    {
      replay_msg = Message::Writer();
      replay_msg.AppendHeader(0, GAME_BUFFER);
      replay_msg.AppendInt(FRAME_SIZE);
    }

    replay_msg.AppendContinuousData(frame.data(), FRAME_SIZE);
    replay_frame_count++;

    if (replay_frame_count % FRAME_BATCH == 0)
    {
      std::vector<uint8_t> message = replay_msg.Msg();
      fout.write((const char*)message.data(), message.size());

      replay_msg = Message::Writer();
      replay_msg.AppendHeader(0, GAME_BUFFER);
      replay_msg.AppendInt(FRAME_SIZE);
    }

    fout.close();
  }
}

void Dojo::Replay::FinalizeRecording()
{
  if (replay_frame_count % FRAME_BATCH != 0)
  {
    std::ofstream fout(Dojo::Replay::file_path,
      std::ios::out | std::ios::binary | std::ios_base::app);

    std::vector<uint8_t> message = replay_msg.Msg();
    fout.write((const char*)message.data(), (std::streamsize)message.size());
  }

  if (replay_frame_count > 0)
    std::cout << "Recording Finalized: " << replay_frame_count << " Input Records" << std::endl;

  replay_frame_count = 0;
  replay_msg = Message::Writer();
}

void Dojo::Replay::ValidateFinalFrame()
{
  final_frame = 0;

  // must start at frame 0: min key 0 and max key size-1 means no gaps
  if (net_inputs[0].empty() || net_inputs[0].begin()->first != 0)
  {
    Dojo::replay_rejected = true;
    return;
  }

  const uint32_t last = net_inputs[0].rbegin()->first;
  if (net_inputs[0].size() != last + 1)
  {
    Dojo::replay_rejected = true;
    return;
  }

  // netplay replays: both players must cover the same complete sequence
  if (net_replay &&
      (net_inputs[1].size() != last + 1 || net_inputs[1].begin()->first != 0))
  {
    Dojo::replay_rejected = true;
    return;
  }

  final_frame = last;
}

void Dojo::Replay::LoadFile(const std::string& path)
{
  if (path.empty())
    return;

  // add string in increments of FRAME_SIZE to net_inputs
  std::ifstream fin(path,
    std::ios::in | std::ios::binary);

  char header_buf[HEADER_LEN] = { 0 };
  std::vector<uint8_t> body_buf;
  int offset = 0;

  // read messages until file ends
  while (fin)
  {
    // read header
    memset((void*)header_buf, 0, HEADER_LEN);
    fin.read(header_buf, HEADER_LEN);

    uint32_t body_size = Message::GetSize((uint8_t*)header_buf);
    uint32_t seq = Message::GetSeq((uint8_t*)header_buf);
    uint32_t cmd = Message::GetCmd((uint8_t*)header_buf);

    // read body
    body_buf.resize(body_size);
    fin.read((char*)body_buf.data(), body_size);

    offset = 0;

    Message::ProcessBody(cmd, body_size, (const char*)body_buf.data(), &offset);
  }

  std::cout << "Replay File Loaded: " << path << std::endl;
}

// read game and player names from the first SPECTATE_START message, without LoadFile side effects
Dojo::Replay::ReplayHeader Dojo::Replay::ReadReplayHeader(const std::string& path)
{
  ReplayHeader meta;
  std::ifstream fin(path,
    std::ios::in | std::ios::binary);
  if (!fin.is_open())
    return meta;

  char header_buf[HEADER_LEN] = { 0 };
  fin.read(header_buf, HEADER_LEN);
  if (fin.gcount() < (std::streamsize)HEADER_LEN)
    return meta;

  const uint32_t body_size = Message::GetSize((uint8_t*)header_buf);
  const uint32_t cmd = Message::GetCmd((uint8_t*)header_buf);
  if (cmd != SPECTATE_START || body_size < 4)
    return meta;

  std::vector<char> body(body_size);
  fin.read(body.data(), body_size);
  if (fin.gcount() < (std::streamsize)body_size)
    return meta;

  const char* buffer = body.data();
  int offset = 0;
  auto readInt = [&]() -> uint32_t {
    if (offset + 4 > (int)body_size)
      return 0;
    uint32_t v;
    memcpy(&v, buffer + offset, 4);
    offset += 4;
    return v;
  };
  auto readString = [&]() -> std::string {
    const uint32_t len = readInt();
    if (len > 0 && (int64_t)offset + (int64_t)len <= (int64_t)body_size)
    {
      std::string s(buffer + offset, len);
      offset += len;
      // stored strings carry trailing null byte
      s.erase(std::find(s.begin(), s.end(), '\0'), s.end());
      return s;
    }
    offset = (int)body_size + 1;
    return "";
  };

  const uint32_t version = readInt();
  if (version < 1 || version > 5)
    return meta;

  meta.game_name = readString();
  meta.player1 = readString();
  meta.player2 = readString();

  if (version >= 5)
  {
    readString(); // quark
    readString(); // matchcode
    meta.ppc_frequency_mhz = readInt();
    meta.refresh_rate_millihz = readInt();
  }

  if (offset <= (int)body_size)
    meta.valid = true;

  return meta;
}

// count game frames by scanning message headers, skipping message bodies;
// frame byte 0 is the player index, so max frames per player gives game frames
uint32_t Dojo::Replay::GetReplayFrameCount(const std::string& path)
{
  std::ifstream fin(path,
    std::ios::in | std::ios::binary);
  if (!fin.is_open())
    return 0;

  fin.seekg(0, std::ios_base::end);
  const std::streamoff file_size = fin.tellg();
  fin.seekg(0, std::ios_base::beg);
  if (!fin.good() || file_size < HEADER_LEN)
    return 0;

  uint32_t player_frames[2] = { 0, 0 };
  char header_buf[HEADER_LEN] = { 0 };
  std::vector<char> body;
  std::streamoff pos = 0;

  while (pos + (std::streamoff)HEADER_LEN <= file_size)
  {
    fin.read(header_buf, HEADER_LEN);
    if (fin.gcount() != (std::streamsize)HEADER_LEN)
      break;
    pos += HEADER_LEN;

    const uint32_t body_size = Message::GetSize((uint8_t*)header_buf);
    const uint32_t cmd = Message::GetCmd((uint8_t*)header_buf);

    // guard against truncated or corrupt files
    if (body_size > file_size - pos)
      break;

    if (cmd == GAME_BUFFER && body_size > 4)
    {
      body.resize(body_size);
      fin.read(body.data(), body_size);
      if (fin.gcount() != (std::streamsize)body_size)
        break;

      const uint32_t num_frames = (body_size - 4) / FRAME_SIZE;
      for (uint32_t i = 0; i < num_frames; i++)
      {
        const uint32_t player = (uint32_t)(uint8_t)body[4 + i * FRAME_SIZE];
        if (player < 2)
          player_frames[player]++;
      }
    }
    else
    {
      // skip the message body
      fin.seekg(pos + body_size);
      if (!fin.good())
        break;
    }
    pos += body_size;
  }

  return std::max(player_frames[0], player_frames[1]);
}

std::string Dojo::Replay::Takeover(int player)
{
  if (Dojo::Replay::p1_override || Dojo::Replay::p2_override || player > 1)
    return "Replay Already Taken Over";

  if (player < 0 || player > 1)
    return "Player Index Out of Bounds";

  std::ostringstream NoticeStream;

  if (player == 0)
    p1_override = true;
  else if (player == 1)
    p2_override = true;

  Dojo::training = true;

  NoticeStream << "Replay Takeover, Player " << player + 1 << " Override";
  return NoticeStream.str();
}

std::string Dojo::Replay::GetStatePath()
{
  std::string state_path = file_path.substr(0, file_path.find_last_of('.'));
  state_path.append(".st0");
  if (std::filesystem::exists(state_path))
    return state_path;
  return "";
}
