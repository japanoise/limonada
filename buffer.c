#include <SDL2/SDL.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
// Untested
char *basename(char *path) {
	int l = strlen(path);
	char *fn = malloc(l);
	char *ext = malloc(l);
	_splitpath(path, NULL, NULL, fn, ext);
	strcat(fn, ext);
	free(ext);
	return fn;
}
#else
#include <libgen.h>
#endif
#include "buffer.h"
#define STBI_NO_JPEG // Jaypegs in a sprite editor? NOPE.
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

buffer *makeBuffer(char *name) {
	buffer *ret = malloc(sizeof(buffer));
	ret->name = MakeSlice(name);
	ret->filename = NULL;
	ret->panx = 0;
	ret->pany = 0;
	ret->zoom = 1;
	ret->sizex = 0;
	ret->sizey = 0;
	ret->datachannels = 4; // SDL RGBA
	ret->data = NULL;
	ret->changedp = 1; // Generate a new texture when ready
	ret->tool = 0;
	return ret;
}

void killBuffer(buffer *buf) {
	DestroySlice(buf->name);
	if (buf->filename != NULL) {
		free(buf->filename);
	}
	stbi_image_free(buf->data);
	free(buf);
}

buffer *makeBufferFromFile(char *filename) {
	char* bn = basename(filename);
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

void setBufferFileName(char *filename, buffer* buf) {
	int l = strlen(filename);
	buf->filename = malloc(l);
	strcpy(buf->filename, filename);
}

SDL_Texture *textureFromBuffer(buffer* buf, SDL_Renderer *rend) {
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
#else // little endian, like x86
	rmask = 0x000000ff;
	gmask = 0x0000ff00;
	bmask = 0x00ff0000;
	amask = (buf->datachannels == STBI_rgb) ? 0 : 0xff000000;
#endif

	int depth, pitch;
	if (buf->datachannels == STBI_rgb) {
		depth = 24;
		pitch = 3*buf->sizex; // 3 bytes per pixel * pixels per row
	} else { // STBI_rgb_alpha (RGBA)
		depth = 32;
		pitch = 4*buf->sizex;
	}

	SDL_Surface* surf = SDL_CreateRGBSurfaceFrom((void*)buf->data, buf->sizex,
						     buf->sizey, depth, pitch,
						     rmask, gmask, bmask, amask);

	SDL_Texture *ret = SDL_CreateTextureFromSurface(rend, surf);
	SDL_FreeSurface(surf);
	return ret;
}
