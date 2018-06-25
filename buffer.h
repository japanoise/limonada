/* Buffer for image data */
#ifndef BUFFER_H
#define BUFFER_H
#ifndef _MSC_VER
#include <SDL2/SDL.h>
#else
#include <SDL.h>
#endif
#include "slice.h"

typedef struct {
	SDL_Color *colors;
	int len;
	int size;
	int scroll;
} palette;

enum undoType {
	StartUndo,
	EndUndo,
	PixelUndo,
};

typedef enum undoType undoType;

typedef struct {
	int index;
	Uint32 oldColor;
	Uint32 newColor;
} pixDiff;

struct undo {
	struct undo *prev;
	struct undo *next;
	undoType type;
	union {
		char batchUndoData;
		pixDiff pixUndoData;
	} data;
};

typedef struct undo undo;

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
	undo *undoList;
	undo *saveUndo;
} buffer;

palette *defaultPalette();

buffer *makeBuffer(char *name);

buffer *makeBufferFromFile(char *filename);

void setBufferFileName(char *filename, buffer *buf);

void killBuffer(buffer *buf);

SDL_Texture *textureFromBuffer(buffer* buf, SDL_Renderer *rend);

void bufferSetPixel(buffer *buf, int px, int py, SDL_Color color);

int bufferIsDirty(buffer *buf);

void bufferStartUndo(buffer *buf);

void bufferEndUndo(buffer *buf);

void bufferDoUndo(buffer *buf);

void bufferDoRedo(buffer *buf);

SDL_Color bufferGetColorAt(buffer *buf, int x, int y);
#endif
