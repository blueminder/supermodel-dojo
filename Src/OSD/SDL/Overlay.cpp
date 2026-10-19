#include "Overlay.h"

#include <chrono>
#include <imgui.h>
#include <imgui_impl_sdl2.h>
#include <imgui_impl_opengl3.h>
#include "SDLIncludes.h"
#include "roboto_medium.h"

// Transient notice, shown for 3 s or until the next one arrives.
static std::string s_notice;
static std::chrono::steady_clock::time_point s_noticeTime;
static constexpr float s_noticeSeconds = 3.0f;

static SDL_Window* s_window = nullptr;

void ShowNotice(const std::string& notice)
{
  s_notice = notice;
  s_noticeTime = std::chrono::steady_clock::now();
}

void InitOverlay(SDL_Window* window)
{
  s_window = window;

  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO& io = ImGui::GetIO();
  static constexpr ImWchar ranges[] = { 0x0020, 0xFFFF, 0 };
  io.Fonts->AddFontFromMemoryCompressedTTF(roboto_medium_compressed_data, roboto_medium_compressed_size, 20.0f, nullptr, ranges);
  ImGui_ImplSDL2_InitForOpenGL(window, SDL_GL_GetCurrentContext());
  ImGui_ImplOpenGL3_Init(nullptr);
}

void ShutdownOverlay()
{
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplSDL2_Shutdown();
  ImGui::DestroyContext();
}

void DrawOverlay()
{
  if (s_notice.empty())
    return;

  const float elapsed = std::chrono::duration<float>(std::chrono::steady_clock::now() - s_noticeTime).count();
  if (elapsed >= s_noticeSeconds)
  {
    s_notice.clear();
    return;
  }

  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplSDL2_NewFrame();
  ImGui::NewFrame();

  const ImGuiStyle& style = ImGui::GetStyle();
  const ImVec2 textSize = ImGui::CalcTextSize(s_notice.c_str());
  const ImVec2 windowSize(textSize.x + style.WindowPadding.x * 2.0f, textSize.y + style.WindowPadding.y * 2.0f);
  const ImVec2 displaySize = ImGui::GetIO().DisplaySize;
  ImGui::SetNextWindowPos(ImVec2((displaySize.x - windowSize.x) * 0.5f, displaySize.y - windowSize.y - 30.0f));
  ImGui::SetNextWindowBgAlpha(0.65f);

  constexpr ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoMove |
                                     ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_AlwaysAutoResize |
                                     ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing |
                                     ImGuiWindowFlags_NoNav;
  ImGui::Begin("##Notice", nullptr, flags);
  ImGui::TextUnformatted(s_notice.c_str());
  ImGui::End();

  ImGui::Render();
  ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}
