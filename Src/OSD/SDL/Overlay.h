#ifndef INCLUDED_OVERLAY_H
#define INCLUDED_OVERLAY_H

#include <string>

struct SDL_Window;

void InitOverlay(SDL_Window* window);
void ShutdownOverlay();
void ShowNotice(const std::string& notice);
void DrawOverlay();

#endif // !INCLUDED_OVERLAY_H
