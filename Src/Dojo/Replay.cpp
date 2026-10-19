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
#ifdef _WIN32
  if (!std::filesystem::exists("Replays")) {
    std::filesystem::create_directory("Replays");
  }
  std::string filename = "Replays/" + game_name + '_' + timestamp;
#else
  std::string filename = Util::Format() << FileSystemPath::GetPath(FileSystemPath::Replays) << game_name + '_' + timestamp;
#endif

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
  spectate_start.AppendInt(1);
  spectate_start.AppendString(game_name);
  spectate_start.AppendString("Player1");
  spectate_start.AppendString("Player2");

  spectate_start.AppendString("Quark");
  spectate_start.AppendString("MatchCode");

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

    if (memcmp(frame.data(), "000000000000", FRAME_SIZE) == 0)
    {
      // send remaining frames
      if (replay_frame_count % FRAME_BATCH > 0)
      {
        std::vector<uint8_t> message = replay_msg.Msg();
        fout.write((const char*)message.data(), message.size());
      }
    }
    fout.close();
  }
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
  if (version < 1 || version > 4)
    return meta;

  meta.game_name = readString();
  meta.player1 = readString();
  meta.player2 = readString();

  if (offset <= (int)body_size)
    meta.valid = true;

  return meta;
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
