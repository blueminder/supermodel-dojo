#include "Dojo.h"

std::string Dojo::Replay::currentISO8601TimeUTC()
{
  auto now = std::chrono::system_clock::now();
  auto itt = std::chrono::system_clock::to_time_t(now);
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

std::string Dojo::Replay::CreateReplayFile(std::string game_name)
{
  // create timestamp string, iso8601 format
  std::string timestamp = currentISO8601TimeUTC();
  std::replace(timestamp.begin(), timestamp.end(), ':', '_');
  std::string filename = "Replays/" + game_name + "_" + timestamp;

  filename.append(".supr");

  // create replay file itself
  file_path = filename;
  std::ofstream file;
  file.open(file_path);

  AppendHeaderToFile(game_name);

  return filename;
}

void Dojo::Replay::AppendHeaderToFile(std::string game_name)
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

void Dojo::Replay::AppendFrameToFile(std::string frame)
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
      fout.write((const char*)&message[0], message.size());

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
        fout.write((const char*)&message[0], message.size());
      }
    }
    fout.close();
  }
}

void Dojo::Replay::LoadFile(std::string path)
{
  if (path == "")
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
  std::string state_path = file_path.substr(0, file_path.find_last_of("."));
  state_path.append(".st0");
  if (std::filesystem::exists(state_path))
    return state_path;
  return "";
}
