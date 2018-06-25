#ifndef GUI_H
#define GUI_H 1
#ifndef _MSC_VER
#include <SDL2/SDL.h>
#else
#include <SDL.h>
#endif
#define LETHEIGHT 12
#define LETWIDTH 6

void drawText(SDL_Renderer *rend, char *text, SDL_Texture *font, int x, int y);
#endif
