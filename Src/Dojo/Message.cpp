#include "Dojo.h"

uint32_t Dojo::Message::GetSize(const uint8_t* buffer)
{
  return *((uint32_t*)(&buffer[0]));
}

uint32_t Dojo::Message::GetSeq(const uint8_t* buffer) { return *((uint32_t*)(&buffer[4])); }
uint32_t Dojo::Message::GetCmd(const uint8_t* buffer) { return *((uint32_t*)(&buffer[8])); }

uint32_t Dojo::Message::ReadInt(const char* buffer, int* offset)
{
  uint32_t value;
  uint32_t len = sizeof(uint32_t);
  memcpy(&value, buffer + *offset, len);
  offset[0] += len;
  return value;
}

std::string Dojo::Message::ReadString(const char* buffer, int* offset)
{
  uint32_t len = ReadInt(buffer, offset);
  std::string out = std::string(buffer + *offset, len);
  offset[0] += len;
  return out;
}

std::string Dojo::Message::ReadContinuousData(const char* buffer, int* offset, uint32_t len)
{
  std::string out = std::string(buffer + *offset, len);
  offset[0] += len;
  return out;
}

std::vector<std::string> Dojo::Message::SplitString(const std::string input, const char& delimiter)
{
  std::vector<std::string> sections;
  std::stringstream sstr(input);
  std::string section;

  while (getline(sstr, section, delimiter))
  {
      sections.push_back(section);
  }

  return sections;
}

std::vector<std::string> Dojo::Message::ReadPlayerInfo(const char* buffer, int* offset)
{
  std::string player_str = ReadString(buffer, offset);
  std::size_t sep = player_str.find_last_of("#");

  std::string player_name = player_str.substr(0, sep);
  std::string player_details = player_str.substr(sep + 1);

  auto player_info = SplitString(player_details, ',');
  player_info.push_back(player_name);
  std::rotate(player_info.rbegin(), player_info.rbegin() + 1, player_info.rend());

  return player_info;
}

Dojo::Message::Writer::Writer()
{
  size = 0;
};

void Dojo::Message::Writer::AppendHeader(uint32_t _sequence, uint32_t _command)
  {
    AppendInt(0);
    AppendInt(_sequence);
    AppendInt(_command);
  }

uint32_t Dojo::Message::Writer::UpdateSize()
{
  size = message.size() - HEADER_LEN;
  message[0] = (uint8_t)(size & 0xFF);
  message[1] = (uint8_t)((size >> 8) & 0xFF);
  message[2] = (uint8_t)((size >> 16) & 0xFF);
  message[3] = (uint8_t)((size >> 24) & 0xFF);
  return size;
}

uint32_t Dojo::Message::Writer::GetSize()
{
  return size;
}

void Dojo::Message::Writer::AppendInt(uint32_t value)
{
  message.push_back((uint8_t)(value & 0xFF));
  message.push_back((uint8_t)((value >> 8) & 0xFF));
  message.push_back((uint8_t)((value >> 16) & 0xFF));
  message.push_back((uint8_t)((value >> 24) & 0xFF));
}

void Dojo::Message::Writer::AppendString(std::string value)
{
  AppendInt(value.size() + 1);
  for (int i = 0; i < value.size() + 1; i++)
  {
    message.push_back((uint8_t)value.data()[i]);
  }
}

void Dojo::Message::Writer::AppendData(const char* value, uint32_t size)
{
  AppendInt(size);
  for (int i = 0; i < size; i++)
  {
    message.push_back((uint8_t)value[i]);
  }
}

// append int by divisible data size after header before calling
void Dojo::Message::Writer::AppendContinuousData(const char* value, uint32_t size)
{
  for (int i = 0; i < size; i++)
  {
    message.push_back((uint8_t)value[i]);
  }
}

std::vector<uint8_t> Dojo::Message::Writer::Msg()
{
  UpdateSize();
  return message;
}

void Dojo::Message::ProcessBody(uint32_t cmd, uint32_t body_size, const char* buffer, int* offset)
{
  if (cmd == SPECTATE_START)
  {
    uint32_t v = ReadInt((const char*)buffer, offset);
    std::string GameName = Message::ReadString((const char*)buffer, offset);
    std::string PlayerName = Message::ReadString((const char*)buffer, offset);
    std::string OpponentName = Message::ReadString((const char*)buffer, offset);
    std::string Quark = Message::ReadString((const char*)buffer, offset);
    std::string MatchCode = Message::ReadString((const char*)buffer, offset);

    std::cout << "Game: " << GameName << std::endl;

    //if (!received_player_info)
    //{
      std::cout << "Player: " << PlayerName << std::endl;
      std::cout << "Opponent: " << OpponentName << std::endl;
    //}

    std::cout << "Quark: " << Quark << std::endl;
    std::cout << "Match Code: " << MatchCode << std::endl;

    //Net::Receiver::receiver_header_read = true;
  }
  else if (cmd == PLAYER_INFO)
  {
    auto p1_info = Message::ReadPlayerInfo(buffer, offset);
    auto p2_info = Message::ReadPlayerInfo(buffer, offset);

    auto player_name = p1_info[0];
    auto opponent_name = p2_info[0];

    //received_player_info = true;

    std::cout << "P1: " << player_name << std::endl;
    std::cout << "P2: " << opponent_name << std::endl;
  }
  else if (cmd == GAME_BUFFER)
  {
    uint32_t frame_size = Message::ReadInt((const char*)buffer, offset);

    // read frames
    while (*offset < body_size)
    {
      std::string frame = Message::ReadContinuousData((const char*)buffer, offset, frame_size);
      Dojo::AddNetFrame(frame.data());
    }
  }
  else if (cmd == GAME_END)
  {
    //Dojo::Session::disconnect_toggle = true;
  }
  else if (cmd == GAME_START)
  {
    uint32_t session_delay = Message::ReadInt((const char*)buffer, offset);
    Dojo::delay = session_delay;

    Dojo::FillDelay();

    std::cout << "Starting Netplay Session, P" << Dojo::player + 1 << " D" << Dojo::delay << std::endl;
  }
}
