/* Buffer for image data */
#ifndef BUFFER_H
#define BUFFER_H
#include <SDL2/SDL.h>
#include "slice.h"

typedef struct {
	SDL_Color *colors;
	int len;
	int size;
	int scroll;
} palette;

typedef struct {
	StrSlice *name;
	char *filename;
	int zoom;
	int panx;
	int pany;
	int sizex;
	int sizey;
	int datachannels;
	unsigned char *data;
	char changedp;
	char tool;
	SDL_Color primary;
	SDL_Color secondary;
	palette *pal;
} buffer;

palette *defaultPalette();

buffer *makeBuffer(char *name);

buffer *makeBufferFromFile(char *filename);

void setBufferFileName(char *filename, buffer *buf);

void killBuffer(buffer *buf);

SDL_Texture *textureFromBuffer(buffer* buf, SDL_Renderer *rend);
#endif
