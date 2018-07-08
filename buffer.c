#ifndef _MSC_VER
#include <SDL2/SDL.h>
#else
#include <SDL.h>
#endif
#include <stdlib.h>
#include <string.h>
#include "platform.h"
#include "buffer.h"
#include "gui.h"
#define STBI_NO_JPEG		/* Jaypegs in a sprite editor? NOPE. */
#include "stb_image.h"

#define GETINDEX(px, py) (buf->sizex*(py)*buf->datachannels)+(buf->datachannels*(px))

int lastpx = -1;
int lastpy = -1;

palette *defaultPalette()
{
	palette *ret = malloc(sizeof(palette));
	ret->size = 10;
	ret->len = 6;
	ret->colors = malloc(sizeof(SDL_Color) * ret->size);
	ret->scroll = 0;
	ret->colors[0] = (SDL_Color) {
	0, 0, 0, 0xFF};
	ret->colors[1] = (SDL_Color) {
	0xFF, 0xFF, 0xFF, 0xFF};
	ret->colors[2] = (SDL_Color) {
	0xFF, 0, 0, 0xFF};
	ret->colors[3] = (SDL_Color) {
	0, 0xFF, 0, 0xFF};
	ret->colors[4] = (SDL_Color) {
	0, 0, 0xFF, 0xFF};
	ret->colors[5] = (SDL_Color) {
	0xFF, 0xFF, 0, 0xFF};
	return ret;
}

void addColorToPalette(palette * pal, SDL_Color color)
{
	pal->colors[pal->len] = color;
	pal->len++;
	if (pal->len > pal->size - 2) {
		pal->size *= 2;
		pal->colors = realloc(pal->colors, sizeof(SDL_Color) * pal->size);
	}
}

buffer *makeBuffer(char *name)
{
	buffer *ret = malloc(sizeof(buffer));
	ret->name = MakeSlice(name);
	ret->filename = NULL;
	ret->panx = 0;
	ret->pany = 0;
	ret->zoom = 1;
	ret->sizex = 0;
	ret->sizey = 0;
	ret->datachannels = 4;	/* SDL RGBA */
	ret->data = NULL;
	ret->changedp = 1;	/* Generate a new texture when ready */
	ret->tool = 0;
	ret->selcontext = 0;
	ret->pal = defaultPalette();
	ret->primary = (SDL_Color) {
	0, 0, 0, 0xFF};
	ret->secondary = (SDL_Color) {
	0xFF, 0xFF, 0xFF, 0xFF};
	ret->undoList = NULL;
	ret->saveUndo = NULL;
	return ret;
}

void internal_deleteRedoBranch(undo * branch)
{
	while (branch != NULL) {
		undo *obranch = branch;
		branch = branch->next;
		free(obranch);
	}
}

void internal_deleteUndoBranch(undo * branch)
{
	while (branch != NULL) {
		undo *obranch = branch;
		branch = branch->prev;
		free(obranch);
	}
}

void killBuffer(buffer * buf)
{
	DestroySlice(buf->name);
	if (buf->filename != NULL) {
		free(buf->filename);
	}
	stbi_image_free(buf->data);
	free(buf->pal->colors);
	free(buf->pal);
	if (buf->undoList != NULL) {
		internal_deleteRedoBranch(buf->undoList->next);
		internal_deleteUndoBranch(buf->undoList);
	}
	free(buf);
}

buffer *makeBufferFromFile(char *filename)
{
	char *bn = basename(filename);
	buffer *ret = makeBuffer(bn);
	setBufferFileName(filename, ret);
#ifdef _WIN32
	free(bn);
#endif
	int of;
	ret->data = stbi_load(filename, &ret->sizex, &ret->sizey, &of, ret->datachannels);
	if (ret->data == NULL) {
		fprintf(stderr, "%s\n", stbi_failure_reason());
	}
	return ret;
}

void setBufferFileName(char *filename, buffer * buf)
{
	int l = strlen(filename);
	buf->filename = malloc(l);
	strcpy(buf->filename, filename);
}

SDL_Texture *textureFromBuffer(buffer * buf, SDL_Renderer * rend)
{
	if (buf->data == NULL) {
		return NULL;
	}

	Uint32 rmask, gmask, bmask, amask;
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
	int shift = (req_format == STBI_rgb) ? 8 : 0;
	rmask = 0xff000000 >> shift;
	gmask = 0x00ff0000 >> shift;
	bmask = 0x0000ff00 >> shift;
	amask = 0x000000ff >> shift;
#else				/* little endian, like x86 */
	rmask = 0x000000ff;
	gmask = 0x0000ff00;
	bmask = 0x00ff0000;
	amask = (buf->datachannels == STBI_rgb) ? 0 : 0xff000000;
#endif

	int depth, pitch;
	if (buf->datachannels == STBI_rgb) {
		depth = 24;
		pitch = 3 * buf->sizex;	/* 3 bytes per pixel * pixels per row */
	} else {		/* STBI_rgb_alpha (RGBA) */
		depth = 32;
		pitch = 4 * buf->sizex;
	}

	SDL_Surface *surf = SDL_CreateRGBSurfaceFrom((void *)buf->data, buf->sizex,
						     buf->sizey, depth, pitch,
						     rmask, gmask, bmask, amask);

	SDL_Texture *ret = SDL_CreateTextureFromSurface(rend, surf);
	SDL_FreeSurface(surf);
	return ret;
}

void internal_bufferSetPixel(buffer * buf, int index, SDL_Color color)
{
	buf->data[index] = color.r;
	buf->data[index + 1] = color.g;
	buf->data[index + 2] = color.b;
	if (buf->datachannels == STBI_rgb_alpha)
		buf->data[index + 3] = color.a;
	buf->changedp = 1;
}

Uint32 internal_packSDLColor(SDL_Color color)
{
	return (color.r << 24) | (color.g << 16) | (color.b << 8) | (color.a);
}

Uint32 internal_packDataColor(buffer * buf, int index)
{
	Uint32 ret = buf->data[index];
	for (int i = 1; i < 4; i++) {
		ret = ret << 8;
		ret |= buf->data[index + i];
	}
	return ret;
}

SDL_Color internal_unpackColor(Uint32 color)
{
	SDL_Color ret;
	ret.r = ((color & 0xFF000000) >> 24);
	ret.g = ((color & 0xFF0000) >> 16);
	ret.b = ((color & 0xFF00) >> 8);
	ret.a = color & 0xFF;
	return ret;
}

undo *internal_appendUndo(buffer * buf)
{
	buf->undoList->next = malloc(sizeof(undo));
	buf->undoList->next->prev = buf->undoList;
	buf->undoList->next->next = NULL;
	return buf->undoList->next;
}

void bufferSetPixel(buffer * buf, int px, int py, SDL_Color color)
{
	int index = GETINDEX(px, py);
	undo *u = internal_appendUndo(buf);
	buf->undoList = u;
	u->type = PixelUndo;
	u->data.pixUndoData = (pixDiff) {
	index, internal_packDataColor(buf, index), internal_packSDLColor(color)};
	internal_bufferSetPixel(buf, index, color);
}

/* https://rosettacode.org/wiki/Bitmap/Bresenham%27s_line_algorithm#C */
void bufferDrawLine(buffer * buf, SDL_Color color, int x0, int y0, int x1, int y1)
{
	int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
	int dy = abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
	int err = (dx > dy ? dx : -dy) / 2, e2;

	for (;;) {
		bufferSetPixel(buf, x0, y0, color);
		if (x0 == x1 && y0 == y1)
			break;
		e2 = err;
		if (e2 > -dx) {
			err -= dy;
			x0 += sx;
		}
		if (e2 < dy) {
			err += dx;
			y0 += sy;
		}
	}
}

#define COLORSAME(index, col) (buf->data[index] == col.r && buf->data[index + 1] == col.g && buf->data[index + 2] == col.b && (buf->datachannels == STBI_rgb || buf->data[index + 3] == col.a))
#define SDLCOLORSAME(cola, colb) (cola.r == colb.r&&cola.g == colb.g&&cola.b == colb.b&&cola.a == colb.a)

void bufferFloodFill(buffer * buf, int px, int py, SDL_Color old, SDL_Color new)
{
	/* quit early if OOB */
	if (px >= buf->sizex || px < 0 || py >= buf->sizey || py < 0)
		return;
	int index = GETINDEX(px, py);
	/* If color same as old color */
	if (COLORSAME(index, old)) {
		bufferSetPixel(buf, px, py, new);
		bufferFloodFill(buf, px, py + 1, old, new);
		bufferFloodFill(buf, px, py - 1, old, new);
		bufferFloodFill(buf, px + 1, py, old, new);
		bufferFloodFill(buf, px - 1, py, old, new);
	}
}

void bufferDoFloodFill(buffer * buf, int px, int py, SDL_Color new)
{
	int index = GETINDEX(px, py);
	if (COLORSAME(index, new))
		return;

	bufferStartUndo(buf);
	bufferFloodFill(buf, px, py, bufferGetColorAt(buf, px, py), new);
	bufferEndUndo(buf);
}

void bufferDitherFill(buffer * buf, int px, int py, SDL_Color old, SDL_Color a, SDL_Color b)
{
	/* quit early if OOB */
	if (px >= buf->sizex || px < 0 || py >= buf->sizey || py < 0)
		return;
	int index = GETINDEX(px, py);
	/* If color same as old color */
	if (COLORSAME(index, old)) {
		bufferSetPixel(buf, px, py, a);
		bufferDitherFill(buf, px, py + 1, old, b, a);
		bufferDitherFill(buf, px, py - 1, old, b, a);
		bufferDitherFill(buf, px + 1, py, old, b, a);
		bufferDitherFill(buf, px - 1, py, old, b, a);
	}
}

void bufferDoFloodFillDither(buffer * buf, int px, int py, SDL_Color a, SDL_Color b)
{
	if (SDLCOLORSAME(a, b)) {
		bufferDoFloodFill(buf, px, py, a);
	} else {
		bufferStartUndo(buf);
		bufferDitherFill(buf, px, py, bufferGetColorAt(buf, px, py), a, b);
		bufferEndUndo(buf);
	}
}

void bufferPencil(buffer * buf, int px, int py, SDL_Color color)
{
	if (lastpx != -1 && lastpy != -1) {
		bufferDrawLine(buf, color, lastpx, lastpy, px, py);
	} else {
		bufferSetPixel(buf, px, py, color);
	}
	lastpx = px;
	lastpy = py;
}

int bufferIsDirty(buffer * buf)
{
	return buf->undoList != buf->saveUndo;
}

void bufferStartUndo(buffer * buf)
{
	if (buf->undoList == NULL) {
		buf->undoList = malloc(sizeof(undo));
		buf->undoList->next = NULL;
		buf->undoList->prev = NULL;
		buf->undoList->type = StartUndo;
	} else {
		internal_deleteRedoBranch(buf->undoList->next);
		if (buf->undoList->type != StartUndo) {
			buf->undoList = internal_appendUndo(buf);
			buf->undoList->type = StartUndo;
		}
	}
}

void bufferEndUndo(buffer * buf)
{
	buf->undoList = internal_appendUndo(buf);
	buf->undoList->type = EndUndo;
	lastpx = lastpy = -1;
}

void bufferDoUndo(buffer * buf)
{
	if (buf->undoList == NULL || buf->undoList->prev == NULL)
		return;
	do {
		if (buf->undoList->type == PixelUndo) {
			internal_bufferSetPixel(buf, buf->undoList->data.pixUndoData.index,
						internal_unpackColor(buf->undoList->data.
								     pixUndoData.oldColor));
		}
		buf->undoList = buf->undoList->prev;
	} while (buf->undoList->type != StartUndo);
	if (buf->saveUndo==NULL && buf->undoList->prev==NULL) {
		buf->saveUndo = buf->undoList;
	}
}

void bufferDoRedo(buffer * buf)
{
	if (buf->undoList == NULL || buf->undoList->next == NULL)
		return;
	do {
		if (buf->undoList->type == PixelUndo) {
			internal_bufferSetPixel(buf, buf->undoList->data.pixUndoData.index,
						internal_unpackColor(buf->undoList->
								     data.pixUndoData.newColor));
		}
		buf->undoList = buf->undoList->next;
	} while (buf->undoList->type != EndUndo);
}

void bufferDoRectOutline(buffer * buf, int x1, int y1, int x2, int y2, SDL_Color color)
{
	/* West border */
	bufferDrawLine(buf, color, x1, y1, x1, y2);
	/* North border */
	bufferDrawLine(buf, color, x1, y1, x2, y1);
	/* East border */
	bufferDrawLine(buf, color, x2, y1, x2, y2);
	/* South border */
	bufferDrawLine(buf, color, x1, y2, x2, y2);
}

void bufferDoRectFill(buffer * buf, int x1, int y1, int x2, int y2, SDL_Color color,
		      SDL_Color border)
{
	for (int x = x1; x <= x2; x++) {
		for (int y = y1; y <= y2; y++) {
			bufferSetPixel(buf, x, y, color);
		}
	}
	bufferDoRectOutline(buf, x1, y1, x2, y2, border);
}

SDL_Color bufferGetColorAt(buffer * buf, int x, int y)
{
	int index = GETINDEX(x, y);
	SDL_Color ret;
	ret.r = buf->data[index];
	ret.g = buf->data[index + 1];
	ret.b = buf->data[index + 2];
	if (buf->datachannels == STBI_rgb_alpha)
		ret.a = buf->data[index + 3];
	return ret;
}

buffer *makeNewBuffer(SDL_Renderer * rend, SDL_Texture * font)
{
	SDL_Rect sizexbox = {10+(8*LETWIDTH), 10, 8*LETWIDTH, 2+LETHEIGHT};
	SDL_Rect sizeybox = {10+(8*LETWIDTH), sizexbox.y+sizexbox.h+10, 8*LETWIDTH, 2+LETHEIGHT};
	SDL_Rect okButton = {10, sizeybox.y+sizeybox.h+10, 4*LETWIDTH, 2+LETHEIGHT};
	SDL_Rect cancelButton = {20+okButton.w, sizeybox.y+sizeybox.h+10, 8*LETWIDTH, 2+LETHEIGHT};
	char sizexstr[10];
	char sizeystr[10];
	for (int i = 0; i<10; i++) {
		sizexstr[i] = '\0';
		sizeystr[i] = '\0';
	}
	int sizexw = 0;
	int sizeyw = 0;
	int focused = 0;
	SDL_Event event;

	for (;;) {
		BGCOL;
		SDL_RenderClear(rend);
		FGCOL;
		SDL_RenderDrawRect(rend, &sizexbox);
		SDL_RenderDrawRect(rend, &sizeybox);
		SDL_RenderDrawRect(rend, &okButton);
		SDL_RenderDrawRect(rend, &cancelButton);
		drawText(rend, "Width:", font, 10, sizexbox.y+1);
		drawText(rend, sizexstr, font, sizexbox.x+1, sizexbox.y+1);
		drawText(rend, "Height:", font, 10, sizeybox.y+1);
		drawText(rend, sizeystr, font, sizeybox.x+1, sizeybox.y+1);
		drawText(rend, "OK", font, okButton.x+LETWIDTH, okButton.y+1);
		drawText(rend, "Cancel", font, cancelButton.x+LETWIDTH, cancelButton.y+1);
		if (focused == 0) {
			SDL_RenderDrawLine(rend, sizexbox.x + 1 + (sizexw*LETWIDTH), sizexbox.y,
				sizexbox.x + 1 + (sizexw*LETWIDTH), sizexbox.y+sizexbox.h-1);
		} else {
			SDL_RenderDrawLine(rend, sizeybox.x + 1 + (sizeyw*LETWIDTH), sizeybox.y,
				sizeybox.x + 1 + (sizeyw*LETWIDTH), sizeybox.y+sizeybox.h-1);
		}
		SDL_RenderPresent(rend);

		while (SDL_PollEvent(&event)) {
			switch (event.type) {
			case SDL_QUIT:
				return NULL;

			case SDL_MOUSEBUTTONUP:;
				int mx = event.button.x;
				int my = event.button.y;
				if (INSIDE_RECT(sizexbox, mx, my)) focused = 0;
				else if (INSIDE_RECT(sizeybox, mx, my)) focused = 1;
				else if (INSIDE_RECT(cancelButton, mx, my)) return NULL;
				else if (INSIDE_RECT(okButton, mx, my) && sizexw>0 && sizeyw>0) {
					buffer * retval = makeBuffer("untitled");
					retval->sizex = atoi(sizexstr);
					retval->sizey = atoi(sizeystr);
					retval->datachannels = 4;
					retval->data = malloc((retval->sizex*retval->sizey)*4);
					for (int i = 0; i < (retval->sizex*retval->sizey)*4; i++) {
						retval->data[i] = 0xFF;
					}
					return retval;
				}
				break;

			case SDL_KEYUP:;
				char add;
				if (event.key.keysym.sym == SDLK_BACKSPACE) {
					if (focused == 0) {
						if (sizexw>0) {
							sizexw--;
							sizexstr[sizexw] = '\0';
						}
					} else {
						if (sizeyw>0) {
							sizeyw--;
							sizeystr[sizeyw] = '\0';
						}
					}
					break;
				}
				else if (event.key.keysym.sym == SDLK_TAB) {
					focused ^= 1;
					break;
				} else if (event.key.keysym.sym == SDLK_0) add = '0';
				else if (event.key.keysym.sym == SDLK_1) add = '1';
				else if (event.key.keysym.sym == SDLK_2) add = '2';
				else if (event.key.keysym.sym == SDLK_3) add = '3';
				else if (event.key.keysym.sym == SDLK_4) add = '4';
				else if (event.key.keysym.sym == SDLK_5) add = '5';
				else if (event.key.keysym.sym == SDLK_6) add = '6';
				else if (event.key.keysym.sym == SDLK_7) add = '7';
				else if (event.key.keysym.sym == SDLK_8) add = '8';
				else if (event.key.keysym.sym == SDLK_9) add = '9';
				else break;
				if (focused == 0) {
					if(sizexw < 8) {
						sizexstr[sizexw] = add;
						sizexw++;
					}
				} else {
					if(sizeyw < 8) {
						sizeystr[sizeyw] = add;
						sizeyw++;
					}
				}
				break;
			}
		}
	}
}
