#ifndef COLOR_H
#define COLOR_H
#ifndef _MSC_VER
#include <SDL2/SDL.h>
#else
#include <SDL.h>
#endif
SDL_bool pickColor(SDL_Renderer *rend, SDL_Texture *font, SDL_Color *ret);
SDL_bool pickColorHSV(SDL_Renderer *rend, SDL_Texture *font, SDL_Color *ret);
#endif
