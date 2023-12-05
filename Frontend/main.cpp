#include "imgui.h"
#include "imgui_impl_opengl2.h"
#include "imgui_impl_sdl2.h"
#include <SDL2/SDL.h>
#include <SDL_opengl.h>
#include <stdio.h>

#include <filesystem>
#include <iostream>

#include <fstream>
#include <vector>

#include "pugixml/pugixml.hpp"
#include "roboto_medium.h"
#include "tortellini.hh"

#include <thread>

#include "ImGuiFileDialog.h"

static std::map<std::string, std::string> mod_settings;
static std::map<std::string, bool> mod_settings_bool;
static std::map<std::string, int> mod_settings_int;
static std::map<std::string, int> mod_settings_double;

static bool settings_init = false;

static int current_delay = 1;
static int target_port = 6000;

static std::map<std::string, std::string> settings_desc = {
    {"MultiThreaded",
     "If set to 1, enables multi-threading; if set to 0, runs all emulation in "
     "a single thread.  Read the description of the '-no-threads' command line "
     "option for more information."},
    {"PowerPCFrequency",
     "The PowerPC frequency in MHz.  The default is 50. Equivalent to the "
     "'-ppc-frequency' command line option."},
    {"FullScreen", "If set to 1, runs in full screen mode; if set to 0, runs "
                   "in a window.  Disabled by default.  Equivalent to the "
                   "'-fullscreen' command line option."},
    {"ShowFrameRate",
     "Shows the frame rate in the window title bar when set to 1. If set to 0, "
     "the frame rate is not computed.  Disabled by default.  Equivalent to the "
     "'-show-fps' command line option."},
    {"Throttle", "Controls 60 FPS throttling.  It is enabled by setting to 1 "
                 "and disabled by setting to 0.  For more information, read "
                 "the description of the '-no-throttle' command line option."},
    {"XResolution",
     "Resolution of the display in pixels.  The default is 496x384, the Model "
     "3's native resolution.  Equivalent to the '-res' command line option."},
    {"YResolution",
     "Resolution of the display in pixels.  The default is 496x384, the Model "
     "3's native resolution.  Equivalent to the '-res' command line option."},
    {"FragmentShader",
     "Path to the external fragment or vertex shader file to use for 3D  "
     "rendering.  By default, these are not set and the internal default "
     "shaders are used.  These are equivalent to the '-frag-shader' and "
     "'-vert-shader' command line options."},
    {"EmulateDSB",
     "Emulates the Digital Sound Board if set to 1, disables it if set to 0.  "
     "See the section on audio settings for more information.  A setting of 0 "
     "is equivalent to the '-no-dsb' command line option."},
    {"EmulateSound",
     "Emulates the sound board and its two Sega Custom Sound Processors if set "
     "to 1, disables it if set to 0.  See the section on audio settings for "
     "more information.  A setting of 0 is equivalent to the '-no-sound' "
     "command line option."},
    {"FlipStereo",
     "Swaps the left and right stereo channels if set to 1.  If set to 0, "
     "outputs the channels normally.  Disabled by default.  A setting of 1 is "
     "equivalent to using the '-flip-stereo' command line option."},
    {"MusicVolume",
     "Specifies the volume of MPEG music produced by the Digital Sound Board "
     "and the audio produced by the sound board in percent.  The default is "
     "100, which is full volume, and the valid range is 0 (muted) to 200%.  "
     "See the section on audio settings for more information.  The equivalent "
     "command line options are '-music-volume' and '-sound-volume'."},
    {"SoundVolume",
     "Specifies the volume of MPEG music produced by the Digital Sound Board "
     "and the audio produced by the sound board in percent.  The default is "
     "100, which is full volume, and the valid range is 0 (muted) to 200%.  "
     "See the section on audio settings for more information.  The equivalent "
     "command line options are '-music-volume' and '-sound-volume'."},
    {"ForceFeedback",
     "If set to 1, enables force feedback emulation; if set to 0, disables it "
     "(the default behavior).  Equivalent to the '-force-feedback' command "
     "line option.  Available only on Windows."},
    {"DirectInputConstForceMax",
     "Sets strength of the four DirectInput force feedback effects in percent. "
     " Default is 100, indicating full strength.  Values exceeding 100% will "
     "distort the effects. Available only on Windows."},
    {"DirectInputFrictionMax",
     "Sets strength of the four DirectInput force feedback effects in percent. "
     " Default is 100, indicating full strength.  Values exceeding 100% will "
     "distort the effects. Available only on Windows."},
    {"DirectInputSelfCenterMax",
     "Sets strength of the four DirectInput force feedback effects in percent. "
     " Default is 100, indicating full strength.  Values exceeding 100% will "
     "distort the effects. Available only on Windows."},
    {"DirectInputVibrateMax",
     "Sets strength of the four DirectInput force feedback effects in percent. "
     " Default is 100, indicating full strength.  Values exceeding 100% will "
     "distort the effects. Available only on Windows."},
    {"XInputConstForceMax",
     "Sets strength of XInput force feedback effects in percent. Default is "
     "100, indicating full strength.  Values exceeding 100% will distort the "
     "effects.  The constant force effect is simulated using vibration.  "
     "Available only on Windows."},
    {"XInputVibrateMax",
     "Sets strength of XInput force feedback effects in percent. Default is "
     "100, indicating full strength.  Values exceeding 100% will distort the "
     "effects.  The constant force effect is simulated using vibration.  "
     "Available only on Windows."},
    {"XInputConstForceThreshold",
     "Minimum strength above which a Model 3 constant force command will be "
     "simulated on an XInput device. XInputConstForceMax determines the "
     "vibration strength for this effect.  The default value is 30.  Available "
     "only on Windows."},
    {"Network", "Enable net board"},
    {"SimulateNet", "Simulate the net board [Default]"},
    {"EmulateNet", "Emulate the net board (requires -no-threads)"},
    {"RecordSession",
     "Record all sessions as replays. Found in Replays folder."},
    {"NativeRefresh",
     "Sets refresh rate to Model 3 native 57.524 Hz. Requires "
     "variable refresh display or frame limiter. (Default: Disabled, 60 Hz)"}};

// Helper to display a little (?) mark which shows a tooltip when hovered.
void ShowHelpMarker(const char *desc) {
  ImGui::TextDisabled("(?)");
  if (ImGui::IsItemHovered()) {
    ImGui::BeginTooltip();
    ImGui::PushTextWrapPos(ImGui::GetFontSize() * 25.0f);
    ImGui::TextUnformatted(desc);
    ImGui::PopTextWrapPos();
    ImGui::EndTooltip();
  }
}

void IniScalar(const char *field) {
  if (mod_settings_int.count(field) == 0)
    return;

  int s32_one = 1;
  bool inputs_step = true;
  ImGui::PushItemWidth(150);
  ImGui::InputScalar(field, ImGuiDataType_S32, &mod_settings_int[field],
                     inputs_step ? &s32_one : NULL, NULL, "%d");
  ImGui::PopItemWidth();
  if (settings_desc.count(field)) {
    ImGui::SameLine();
    ShowHelpMarker(settings_desc[field].data());
  }
}

void IniSlider(const char *field) {
  if (mod_settings_int.count(field) == 0)
    return;

  ImGui::PushItemWidth(150);
  ImGui::SliderInt(field, (int *)&mod_settings_int[field], 0, 100);
  ImGui::PopItemWidth();
  if (settings_desc.count(field)) {
    ImGui::SameLine();
    ShowHelpMarker(settings_desc[field].data());
  }
}

bool IniCheckBox(const char *field) {
  if (mod_settings_bool.count(field) == 0)
    return false;

  ImGui::Checkbox(field, &mod_settings_bool[field]);
  if (settings_desc.count(field)) {
    ImGui::SameLine();
    ShowHelpMarker(settings_desc[field].data());
  }

  return mod_settings_bool[field];
}

static std::string bool_fields[] = {
    "Network",        "SimulateNet",   "FullScreen",   "New3DEngine",
    "QuadRendering",  "WideScreen",    "Stretch",      "WideBackground",
    "ShowFrameRate",  "Throttle",      "VSync",        "GPUMultiThreaded",
    "Crosshairs",     "MultiThreaded", "MultiTexture", "ForceFeedback",
    "LegacySoundDSP", "EmulateDSB",    "EmulateSound", "FlipStereo",
    "RecordSession",  "NativeRefresh"};

static std::string int_fields[] = {"PowerPCFrequency",
                                   "XResolution",
                                   "YResolution",
                                   "PortIn",
                                   "PortOut",
                                   "MusicVolume",
                                   "SoundVolume",
                                   "DirectInputConstForceMax",
                                   "DirectInputFrictionMax",
                                   "DirectInputSelfCenterMax",
                                   "DirectInputVibrateMax",
                                   "XInputConstForceMax",
                                   "XInputVibrateMax",
                                   "XInputConstForceThreshold"};

static std::string double_fields[] = {"RefreshRate"};

static std::string str_fields[] = {"AddressOut"};

void load_settings(std::filesystem::path ini_path) {
  tortellini::ini ini;
  std::fstream in(ini_path);
  in >> ini;

  for (std::string field : str_fields) {
    mod_settings[field] = ini["Global"][field] | "";
  }

  for (std::string field : bool_fields) {
    mod_settings_bool[field] = ini["Global"][field] | false;
  }

  for (std::string field : int_fields) {
    mod_settings_int[field] = ini["Global"][field] | 0;
  }

  for (std::string field : double_fields) {
    mod_settings_double[field] = ini["Global"][field] | 0.00;
  }

  // custom options
  if (mod_settings_bool["NativeRefresh"]) {
    mod_settings_double["RefreshRate"] = 57.524;
  } else {
    mod_settings_double["RefreshRate"] = 60.000;
  }
}

void start_netplay_popup(std::string game_name, std::string cmd, bool hosting) {
  std::string netplay_popup_name;
  if (hosting)
    netplay_popup_name = "Host##" + game_name;
  else
    netplay_popup_name = "Join##" + game_name;
  std::string out = "";
  if (ImGui::BeginPopupModal(netplay_popup_name.c_str(), NULL,
                             ImGuiWindowFlags_AlwaysAutoResize |
                                 ImGuiWindowFlags_NoMove)) {
    ImGui::Text("Enter Server Details");

    static char si[128] = "127.0.0.1";
    static char sp[128] = "6000";
    if (!hosting) {
      ImGui::InputTextWithHint("IP", "", si, IM_ARRAYSIZE(si));
      ImGui::SameLine();
      if (ImGui::Button("Paste")) {
        for (int i = 0; i < 128; i++) {
          si[i] = 0;
        }
        char *pasted_txt = SDL_GetClipboardText();
        memcpy(si, pasted_txt, strlen(pasted_txt));
      }
    }
    int s32_one = 1;
    bool inputs_step = true;
    ImGui::InputScalar("Port", ImGuiDataType_S32, &target_port,
                       inputs_step ? &s32_one : NULL, NULL, "%d");

    ImGui::SliderInt("Delay", (int *)&current_delay, 0, 20);

    if (ImGui::Button("Start")) {
      ImGui::CloseCurrentPopup();
      if (hosting) {
        cmd += " -netplay -host";
        cmd += " -delay=" + std::to_string(current_delay);
        cmd += " -target-port=" + std::to_string(target_port);
        cmd += " & pause";
        std::cout << cmd << std::endl;

        std::thread t_run([cmd]() { std::system(cmd.c_str()); });
        t_run.detach();
      } else {
        cmd += " -netplay";
        cmd += " -delay=" + std::to_string(current_delay);
        cmd += " -target-ip=" + std::string(si);
        cmd += " -target-port=" + std::to_string(target_port);
        cmd += " & pause";
        std::cout << cmd << std::endl;

        std::thread t_run([cmd]() { std::system(cmd.c_str()); });
        t_run.detach();
      }
    }

    ImGui::SameLine();

    if (ImGui::Button("Cancel")) {
      ImGui::CloseCurrentPopup();
    }
    ImGui::EndPopup();
  }
}

void StyleColorsAM3Wave() {
  ImVec4 *colors = ImGui::GetStyle().Colors;
  colors[ImGuiCol_Text] = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
  colors[ImGuiCol_TextDisabled] = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);
  colors[ImGuiCol_WindowBg] = ImVec4(0.94f, 0.94f, 0.94f, 1.00f);
  colors[ImGuiCol_ChildBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
  colors[ImGuiCol_PopupBg] = ImVec4(1.00f, 1.00f, 1.00f, 0.98f);
  colors[ImGuiCol_Border] = ImVec4(0.00f, 0.00f, 0.00f, 0.30f);
  colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
  colors[ImGuiCol_FrameBg] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
  colors[ImGuiCol_FrameBgHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.40f);
  colors[ImGuiCol_FrameBgActive] = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
  colors[ImGuiCol_TitleBg] = ImVec4(0.96f, 0.96f, 0.96f, 1.00f);
  colors[ImGuiCol_TitleBgActive] = ImVec4(0.83f, 0.92f, 0.86f, 1.00f);
  colors[ImGuiCol_TitleBgCollapsed] = ImVec4(1.00f, 1.00f, 1.00f, 0.51f);
  colors[ImGuiCol_MenuBarBg] = ImVec4(0.86f, 0.86f, 0.86f, 1.00f);
  colors[ImGuiCol_ScrollbarBg] = ImVec4(0.98f, 0.98f, 0.98f, 0.53f);
  colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.69f, 0.69f, 0.69f, 0.80f);
  colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.49f, 0.49f, 0.49f, 0.80f);
  colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.49f, 0.49f, 0.49f, 1.00f);
  colors[ImGuiCol_CheckMark] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
  colors[ImGuiCol_SliderGrab] = ImVec4(0.26f, 0.59f, 0.98f, 0.78f);
  colors[ImGuiCol_SliderGrabActive] = ImVec4(0.46f, 0.54f, 0.80f, 0.60f);
  colors[ImGuiCol_Button] = ImVec4(0.96f, 0.74f, 0.15f, 1.00f);
  colors[ImGuiCol_ButtonHovered] = ImVec4(0.95f, 0.20f, 0.20f, 1.00f);
  colors[ImGuiCol_ButtonActive] = ImVec4(0.10f, 0.49f, 0.22f, 1.00f);
  colors[ImGuiCol_Header] = ImVec4(0.26f, 0.59f, 0.98f, 0.31f);
  colors[ImGuiCol_HeaderHovered] = ImVec4(1.00f, 0.76f, 0.76f, 1.00f);
  colors[ImGuiCol_HeaderActive] = ImVec4(0.95f, 0.20f, 0.20f, 1.00f);
  colors[ImGuiCol_Separator] = ImVec4(0.39f, 0.39f, 0.39f, 0.62f);
  colors[ImGuiCol_SeparatorHovered] = ImVec4(0.14f, 0.44f, 0.80f, 0.78f);
  colors[ImGuiCol_SeparatorActive] = ImVec4(0.14f, 0.44f, 0.80f, 1.00f);
  colors[ImGuiCol_ResizeGrip] = ImVec4(0.35f, 0.35f, 0.35f, 0.17f);
  colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
  colors[ImGuiCol_ResizeGripActive] = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
  colors[ImGuiCol_Tab] = ImVec4(0.84f, 0.87f, 0.92f, 1.00f);
  colors[ImGuiCol_TabHovered] = ImVec4(0.44f, 0.69f, 0.98f, 0.80f);
  colors[ImGuiCol_TabActive] = ImVec4(0.74f, 0.85f, 0.98f, 1.00f);
  colors[ImGuiCol_TabUnfocused] = ImVec4(0.92f, 0.93f, 0.94f, 0.99f);
  colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.74f, 0.82f, 0.91f, 1.00f);
  colors[ImGuiCol_DockingPreview] = ImVec4(0.26f, 0.59f, 0.98f, 0.22f);
  colors[ImGuiCol_DockingEmptyBg] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
  colors[ImGuiCol_PlotLines] = ImVec4(0.39f, 0.39f, 0.39f, 1.00f);
  colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
  colors[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
  colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.45f, 0.00f, 1.00f);
  colors[ImGuiCol_TableHeaderBg] = ImVec4(0.78f, 0.87f, 0.98f, 1.00f);
  colors[ImGuiCol_TableBorderStrong] = ImVec4(0.57f, 0.57f, 0.64f, 1.00f);
  colors[ImGuiCol_TableBorderLight] = ImVec4(0.68f, 0.68f, 0.74f, 1.00f);
  colors[ImGuiCol_TableRowBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
  colors[ImGuiCol_TableRowBgAlt] = ImVec4(0.30f, 0.30f, 0.30f, 0.09f);
  colors[ImGuiCol_TextSelectedBg] = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);
  colors[ImGuiCol_DragDropTarget] = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
  colors[ImGuiCol_NavHighlight] = ImVec4(0.26f, 0.59f, 0.98f, 0.80f);
  colors[ImGuiCol_NavWindowingHighlight] = ImVec4(0.70f, 0.70f, 0.70f, 0.70f);
  colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.20f, 0.20f, 0.20f, 0.20f);
  colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.20f, 0.20f, 0.20f, 0.35f);
}

void SetSMWindowPos(SDL_Window *win, std::string ini_path) {
  tortellini::ini ini;
  std::fstream in(ini_path);
  in >> ini;

  int x;
  int y;
  SDL_GetWindowPosition(win, &x, &y);

  ini["Global"]["WindowXPosition"] = x + 10;
  if (y - 45 > 0)
    ini["Global"]["WindowYPosition"] = y - 45;
  else
    ini["Global"]["WindowYPosition"] = 0;

  std::ofstream out(ini_path);
  std::cout << ini;
  out << ini;
}

ImVec2 GetCmdWindowPos(SDL_Window *win, std::string ini_path) {
  tortellini::ini ini;
  std::fstream in(ini_path);
  in >> ini;

  int x;
  int y;
  SDL_GetWindowPosition(win, &x, &y);

  int win_w = ini["Global"]["XResolution"] | 800;
  int win_h = ini["Global"]["YResolution"] | 600;

  int cmd_x = x + win_w + 20;
  int cmd_y = y + 10;

  ImVec2 pos = ImVec2(cmd_x, cmd_y);
  return pos;
}

int main(int, char **) {
  // Setup SDL
  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER |
               SDL_INIT_JOYSTICK) != 0) {
    printf("Error: %s\n", SDL_GetError());
    return -1;
  }

  std::filesystem::path cwd = std::filesystem::current_path();
  auto ini_path = cwd / "Config/Supermodel.ini";
  std::string ini_path_s = ini_path.string();

  int current_position = 0;

  bool log_autoscroll = true;

  pugi::xml_document doc;

  pugi::xml_parse_result result = doc.load_file("Config/Games.xml");
  pugi::xml_node games = doc.child("games");

  // From 2.0.18: Enable native IME.
#ifdef SDL_HINT_IME_SHOW_UI
  SDL_SetHint(SDL_HINT_IME_SHOW_UI, "1");
#endif

  // Setup window
  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
  SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
  SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
  SDL_WindowFlags window_flags = (SDL_WindowFlags)(
      SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
  SDL_Window *window =
      SDL_CreateWindow("Supermodel Dojo", SDL_WINDOWPOS_CENTERED,
                       SDL_WINDOWPOS_CENTERED, 1100, 540, window_flags);
  SDL_GLContext gl_context = SDL_GL_CreateContext(window);
  SDL_GL_MakeCurrent(window, gl_context);
  SDL_GL_SetSwapInterval(1); // Enable vsync

  // Setup Dear ImGui context
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO();
  (void)io;
  io.BackendFlags |= ImGuiBackendFlags_HasGamepad;
  io.ConfigFlags |=
      ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
  io.ConfigFlags |=
      ImGuiConfigFlags_NavEnableGamepad;              // Enable Gamepad Controls
  io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;   // Enable Docking
  io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable; // Enable Multi-Viewport /
                                                      // Platform Windows
  // io.ConfigViewportsNoAutoMerge = true;
  // io.ConfigViewportsNoTaskBarIcon = true;

  // ImGui::StyleColorsDark();
  // ImGui::StyleColorsLight();
  StyleColorsAM3Wave();

  // When viewports are enabled we tweak WindowRounding/WindowBg so platform
  // windows can look identical to regular ones.
  ImGuiStyle &style = ImGui::GetStyle();

  style.GrabRounding = 4.0f;
  style.FrameBorderSize = 1.0f;

  if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
    style.WindowRounding = 0.0f;
    style.Colors[ImGuiCol_WindowBg].w = 1.0f;
  }

  ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
  ImGui_ImplOpenGL2_Init();

  static const ImWchar ranges[] = {
      0x0020,
      0xFFFF, // All chars
      0,
  };
  io.Fonts->Clear();
  const float fontSize = 17.f * 1;
  io.Fonts->AddFontFromMemoryCompressedTTF(roboto_medium_compressed_data,
                                           roboto_medium_compressed_size,
                                           fontSize, nullptr, ranges);

  ImVec4 clear_color = ImVec4(0.32f, 0.47f, 0.67f, 1.00f);

  bool show_settings = false;
  bool show_cmds = false;

  // Main loop
  bool done = false;

  std::string last_pressed_key;
  std::string selected_rom_path = "";

  while (!done) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      ImGui_ImplSDL2_ProcessEvent(&event);

      if (event.type == SDL_QUIT)
        done = true;

      if (event.type == SDL_WINDOWEVENT &&
          event.window.event == SDL_WINDOWEVENT_CLOSE &&
          event.window.windowID == SDL_GetWindowID(window))
        done = true;
    }

    ImGui_ImplOpenGL2_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();

    // if (show_demo_window)
    // ImGui::ShowDemoWindow(&show_demo_window);

    int win_x;
    int win_y;
    SDL_GetWindowPosition(window, &win_x, &win_y);

    {
      ImGui::SetNextWindowPos(ImVec2(win_x + 10, win_y + 10),
                              ImGuiCond_FirstUseEver);
      ImGui::Begin("Model 3 Game Library");

      static ImGuiTextFilter filter;
      filter.Draw("   ");
      ImGui::SameLine();

      if (ImGui::Button("Settings")) {
        settings_init = false;
        show_settings = !show_settings;
      }
      ImGui::SetItemTooltip("Adjust emulator settings");

      ImGui::SameLine();

      if (ImGui::Button("Command")) {
        show_cmds = !show_cmds;
      }
      ImGui::SetItemTooltip("Displays quick reference of emulator commands");

      ImGui::BeginChild("C", ImVec2(520, 455), true);
      namespace fs = std::filesystem;
      std::string rom_dir = "ROMs\\";
      for (pugi::xml_node game = games.child("game"); game;
           game = game.next_sibling("game")) {
        std::string title = game.child("identity").child("title").child_value();
        std::string rom_name = game.attribute("name").value();
        std::string version =
            game.child("identity").child("version").child_value();

        std::string rom_path = rom_dir + rom_name + ".zip";
        std::string cmd = "cmd /C supermodel.exe " + rom_path; // + " > output";

        // fvipers2 default settings
        // 100MHz to prevent slowdown and desyncs
        // legacy sound to prevent ear damage
        if (rom_name == "fvipers2" || rom_name == "fvipers2o") {
          cmd += " -ppc-frequency=100";
          cmd += " -legacy-scsp";
        }

        if (fs::exists(rom_path)) {
          if (filter.PassFilter(title.c_str())) {
            std::string button_txt = title + " (" + version + ")";

            if (ImGui::Selectable((const char *)button_txt.data())) {
              ImGui::OpenPopup(button_txt.data());
            }
            ImGui::SetItemTooltip(rom_name.data());

            bool open_host = false;
            bool open_guest = false;
            std::string popup_name = "Options " + title;
            if (ImGui::BeginPopupContextItem(button_txt.c_str())) {
              if (ImGui::MenuItem("Start Game")) {
                SetSMWindowPos(window, ini_path.string());
                show_cmds = true;
                ImGui::CloseCurrentPopup();

                cmd += " & pause";
                std::cout << cmd << std::endl;

                std::thread t_run([cmd]() { std::system(cmd.c_str()); });
                t_run.detach();
              }
              if (ImGui::MenuItem("Open Replay File")) {
                ImGui::SetNextWindowSize(ImVec2(650, 400));
                ImGuiFileDialog::Instance()->OpenDialog(
                    "ChooseFileDlgKey", "Choose File", ".supr", "Replays", 1,
                    nullptr, ImGuiFileDialogFlags_Modal);
                selected_rom_path = rom_path;
              }
              if (ImGui::MenuItem("Start Training Mode")) {
                SetSMWindowPos(window, ini_path.string());
                show_cmds = true;
                ImGui::CloseCurrentPopup();
                cmd += " -train";
                cmd += " & pause";
                std::cout << cmd << std::endl;

                std::thread t_run([cmd]() { std::system(cmd.c_str()); });
                t_run.detach();
              }
              if (ImGui::MenuItem("Host Netplay Session")) {
                ImGui::CloseCurrentPopup();
                open_host = true;
              }
              if (ImGui::MenuItem("Join Netplay Session")) {
                ImGui::CloseCurrentPopup();
                open_guest = true;
              }

              ImGui::EndPopup();
            }

            if (open_host) {
              std::string popup_name = "Host##" + rom_name;
              SetSMWindowPos(window, ini_path.string());
              ImGui::OpenPopup(popup_name.data());
            }

            if (open_guest) {
              std::string popup_name = "Join##" + rom_name;
              SetSMWindowPos(window, ini_path.string());
              ImGui::OpenPopup(popup_name.data());
            }

            // If Replay Selected
            if (ImGuiFileDialog::Instance()->Display("ChooseFileDlgKey")) {
              if (ImGuiFileDialog::Instance()->IsOk()) {
                std::string filePathName =
                    ImGuiFileDialog::Instance()->GetFilePathName();
                std::string filePath =
                    ImGuiFileDialog::Instance()->GetCurrentPath();

                std::cout << filePathName << std::endl;

                SetSMWindowPos(window, ini_path.string());
                show_cmds = true;
                std::string replay_cmd = "supermodel.exe " + selected_rom_path;
                replay_cmd += " -replay-file=" + filePathName;
                replay_cmd += " & pause";
                // replay_cmd += " > output";
                std::cout << replay_cmd << std::endl;

                std::thread t_run(
                    [replay_cmd]() { std::system(replay_cmd.c_str()); });
                t_run.detach();

                selected_rom_path = "";
              }

              ImGuiFileDialog::Instance()->Close();
            }

            start_netplay_popup(rom_name, cmd, true);
            start_netplay_popup(rom_name, cmd, false);
          }
        }
      }

      ImGui::EndChild();
      ImGui::End();
    }

    if (show_cmds) {
      // unordered_map to preserve iteration order
      std::unordered_map<std::string, std::string> emu_cmds;
      emu_cmds.insert({"Record Replay Clip", "Shift + 3"});
      emu_cmds.insert({"Screenshot", "Alt + S"});
      emu_cmds.insert({"Load State", "F7"});
      emu_cmds.insert({"Change Save State", "F6"});
      emu_cmds.insert({"Save State", "F5"});

      std::unordered_map<std::string, std::string> replay_cmds;
      replay_cmds.insert({"Pause", "Alt + P"});
      replay_cmds.insert({"Takeover P2", "Shift + 2"});
      replay_cmds.insert({"Takeover P1", "Shift + 1"});

      std::unordered_map<std::string, std::string> training_cmds;
      training_cmds.insert({"Toggle Play Loop", "Shift + F9"});
      training_cmds.insert({"Play Slot (1-3)", "Shift + (F4-F6)"});
      training_cmds.insert({"Record Slot (1-3)", "Shift + (F1-F3)"});
      training_cmds.insert({"Switch Player", "Shift + F7"});

      ImGui::SetNextWindowPos(GetCmdWindowPos(window, ini_path.string()),
                              ImGuiCond_FirstUseEver);
      ImGui::Begin("Command Reference");
      ImGui::SeparatorText("Emulator");
      if (ImGui::BeginTable("EmuCommands", 2)) {
        for (auto it = emu_cmds.begin(); it != emu_cmds.end(); ++it) {
          ImGui::TableNextRow();
          ImGui::TableSetColumnIndex(0);
          ImGui::Text((it->first).data());
          ImGui::TableSetColumnIndex(1);
          ImGui::Text((it->second).data());
        }
        ImGui::EndTable();
      }
      ImGui::SeparatorText("Replays");
      if (ImGui::BeginTable("ReplayCommands", 2)) {
        for (auto it = replay_cmds.begin(); it != replay_cmds.end(); ++it) {
          ImGui::TableNextRow();
          ImGui::TableSetColumnIndex(0);
          ImGui::Text((it->first).data());
          ImGui::TableSetColumnIndex(1);
          ImGui::Text((it->second).data());
        }
        ImGui::EndTable();
      }
      ImGui::SeparatorText("Training Mode");
      if (ImGui::BeginTable("TrainingCommands", 2)) {
        for (auto it = training_cmds.begin(); it != training_cmds.end(); ++it) {
          ImGui::TableNextRow();
          ImGui::TableSetColumnIndex(0);
          ImGui::Text((it->first).data());
          ImGui::TableSetColumnIndex(1);
          ImGui::Text((it->second).data());
        }
        ImGui::EndTable();
      }
      ImGui::End();
    }

    if (show_settings) {
      static char ns_buffer[3][128] = {{0}, {0}, {0}};

      // only load settings and populate fields on launch
      if (!settings_init) {
        load_settings(ini_path);

        int buffer_idx = 0;
        for (std::string field : str_fields) {
          strncpy(ns_buffer[buffer_idx], mod_settings[field].data(),
                  mod_settings[field].size());
          buffer_idx++;
        }

        settings_init = true;
      }
      ImGui::SetNextWindowPos(ImVec2(win_x + 555, win_y + 10), ImGuiCond_Once);
      ImGui::Begin("Settings");

      if (ImGui::Button("Save")) {
        tortellini::ini ini;
        std::fstream in(ini_path);
        in >> ini;

        int buffer_idx = 0;
        for (std::string field : str_fields) {
          ini["Global"][field] = (const char *)ns_buffer[buffer_idx];
          buffer_idx++;
        }

        for (std::string field : bool_fields) {
          ini["Global"][field] = (int)mod_settings_bool[field];
        }

        for (std::string field : int_fields) {
          ini["Global"][field] = mod_settings_int[field] | 0;
        }

        for (std::string field : double_fields) {
          ini["Global"][field] = mod_settings_double[field] | 0;
        }

        // custom options
        if (mod_settings_bool["NativeRefresh"]) {
          ini["Global"]["RefreshRate"] = 57.524;
        } else {
          ini["Global"]["RefreshRate"] = 60.000;
        }

        std::ofstream out(ini_path);
        std::cout << ini;
        out << ini;
      }

      ImGui::SameLine();

      if (ImGui::Button("Close")) {
        show_settings = !show_settings;
      }

      ImGui::BeginChild("S", ImVec2(450, 400), true);

      if (ImGui::BeginTabBar("SettingsTabBar")) {
        if (ImGui::BeginTabItem("Graphics")) {
          IniCheckBox("New3DEngine");
          IniCheckBox("QuadRendering");
          IniCheckBox("MultiTexture");
          IniCheckBox("ShowFrameRate");

          IniCheckBox("VSync");
          IniCheckBox("Throttle");
          IniCheckBox("NativeRefresh");

          IniScalar("XResolution");
          IniScalar("YResolution");
          IniCheckBox("FullScreen");

          if (IniCheckBox("WideScreen")) {
            IniCheckBox("Stretch");
            IniCheckBox("WideBackground");
          }

          IniCheckBox("Crosshairs");
          ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("System")) {
          IniScalar("PowerPCFrequency");
          IniCheckBox("GPUMultiThreaded");
          IniCheckBox("MultiThreaded");
          ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Audio")) {
          IniCheckBox("EmulateSound");
          IniCheckBox("FlipStereo");
          IniCheckBox("EmulateDSB");
          IniCheckBox("LegacySoundDSP");
          IniSlider("MusicVolume");
          IniSlider("SoundVolume");
          ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Controls")) {
          if (ImGui::Button("Set Controls")) {
            std::string cmd = "cmd /C supermodel.exe -config-inputs & pause";
            system(cmd.c_str());
          }
          ImGui::SameLine();
          ShowHelpMarker("Sets controls via interactive command line prompt.");

          if (ImGui::Button("View Controls")) {
            std::string cmd = "cmd /C supermodel.exe -print-inputs & pause";

            std::thread t_run([cmd]() { std::system(cmd.c_str()); });
            t_run.detach();
          }
          ImGui::SameLine();
          ShowHelpMarker("Shows current controls in Log window.");

          if (IniCheckBox("ForceFeedback")) {
            IniScalar("DirectInputConstForceMax");
            IniScalar("DirectInputFrictionMax");
            IniScalar("DirectInputSelfCenterMax");
            IniScalar("DirectInputVibrateMax");
            IniScalar("XInputConstForceMax");
            IniScalar("XInputVibrateMax");
            IniScalar("XInputConstForceThreshold");
          }
          ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Net Board")) {
          if (IniCheckBox("Network")) {
            IniCheckBox("SimulateNet");

            ImGui::PushItemWidth(250);
            ImGui::InputTextWithHint("AddressOut", "AddressOut", ns_buffer[0],
                                     IM_ARRAYSIZE(ns_buffer[0]));
            ImGui::PopItemWidth();

            IniScalar("PortIn");
            IniScalar("PortOut");
          }
          ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Dojo")) {
          IniCheckBox("RecordSession");

          ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
      }
      ImGui::EndChild();

      ImGui::End();
    }

    // Rendering
    ImGui::Render();
    glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
    glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w,
                 clear_color.z * clear_color.w, clear_color.w);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());

    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
      SDL_Window *backup_current_window = SDL_GL_GetCurrentWindow();
      SDL_GLContext backup_current_context = SDL_GL_GetCurrentContext();
      ImGui::UpdatePlatformWindows();
      ImGui::RenderPlatformWindowsDefault();
      SDL_GL_MakeCurrent(backup_current_window, backup_current_context);
    }

    SDL_GL_SwapWindow(window);
  }

  // Cleanup
  ImGui_ImplOpenGL2_Shutdown();
  ImGui_ImplSDL2_Shutdown();
  ImGui::DestroyContext();

  SDL_GL_DeleteContext(gl_context);
  SDL_DestroyWindow(window);
  SDL_Quit();

  return 0;
}
