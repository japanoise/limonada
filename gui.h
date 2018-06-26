#ifndef GUI_H
#define GUI_H 1
#ifndef _MSC_VER
#include <SDL2/SDL.h>
#else
#include <SDL.h>
#endif
#define LETHEIGHT 12
#define LETWIDTH 6
#define COL_BG 0xFF, 0xFF, 0xFF, SDL_ALPHA_OPAQUE
#define GREY 0x88
#define GREY_BG GREY, GREY, GREY, SDL_ALPHA_OPAQUE
#define COL_FG 0, 0, 0, SDL_ALPHA_OPAQUE
#define UNWRAP_COL(col) (col).r, (col).g, (col).b, (col).a
#define COL_PRIM UNWRAP_COL(buf->primary)
#define COL_SEC UNWRAP_COL(buf->secondary)
#define BGCOL SDL_SetRenderDrawColor(rend, COL_BG)
#define FGCOL SDL_SetRenderDrawColor(rend, COL_FG)
#define GREYCOL SDL_SetRenderDrawColor(rend, GREY_BG)
#define SCROLLBARWIDTH LETWIDTH+2

void drawText(SDL_Renderer *rend, char *text, SDL_Texture *font, int x, int y);
char *fileBrowse(SDL_Renderer *rend, SDL_Texture *font, char* dir);
#endif
