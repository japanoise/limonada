#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <stdlib.h>
#include "slice.h"
#include "cp437.xpm"

#define TITLE "Sprite"
#define COL_BG 0xFF, 0xFF, 0xFF, SDL_ALPHA_OPAQUE
#define COL_FG 0, 0, 0, SDL_ALPHA_OPAQUE
#define WINWIDTH 800
#define WINHEIGHT 600
#define LETHEIGHT 12
#define LETWIDTH 6

void drawText(SDL_Renderer *rend, char *text, SDL_Texture *font, int x, int y) {
	SDL_Rect srcrect = {0,0,LETWIDTH,LETHEIGHT};
	SDL_Rect destrect = {x,y,LETWIDTH,LETHEIGHT};
	for (;*text!='\0';text++) {
		char c = *text;
		srcrect.x=(c&0x0F)*LETWIDTH;
		srcrect.y=(c>>4)*LETHEIGHT;
		SDL_RenderCopy(rend, font, &srcrect, &destrect);
		destrect.x+=LETWIDTH;
	}
}

typedef struct {
	StrSlice **titles;
	int ntitles;
} menubar;

menubar *makeMenuBar(char **titles, int ntitles) {
	menubar *ret = malloc(sizeof(menubar));
	ret->titles = malloc(sizeof(StrSlice**)*ntitles);
	ret->ntitles = ntitles;
	for(int i = 0; i<ntitles; i++) {
		ret->titles[i]=MakeSlice(titles[i]);
	}
	return ret;
}

void drawMenuBar(menubar *m, SDL_Renderer *rend, SDL_Texture *font, int mx, int my) {
	SDL_RenderDrawLine(rend, 0, LETHEIGHT+2, WINWIDTH, LETHEIGHT+2);
	int ix = 0;
	int ox = 0;
	for(int i=0; i<m->ntitles; i++) {
		drawText(rend, m->titles[i]->String, font, ix+2, 1);
		ox = ix;
		ix += (m->titles[i]->len+1)*LETWIDTH;
		if (my < LETHEIGHT && mx >=ox && mx < ix) {
			SDL_Rect r = {ox, 0, ix-ox, LETHEIGHT+2};
			SDL_RenderDrawRect(rend, &r);
		}
	}
}

SDL_Texture* loadFont(SDL_Renderer *rend) {
	SDL_Surface *font;
	font = IMG_ReadXPMFromArray(cp437);
	SDL_Texture *ret = SDL_CreateTextureFromSurface(rend, font);
	SDL_FreeSurface(font);
	return ret;
}

int main(int argc, char **argv) {
	SDL_Init(SDL_INIT_EVERYTHING);

	// get the window
	SDL_Window *window = SDL_CreateWindow(TITLE, SDL_WINDOWPOS_UNDEFINED,
		SDL_WINDOWPOS_UNDEFINED, WINWIDTH, WINHEIGHT, SDL_WINDOW_SHOWN);
	if (!window) {
		fprintf(stderr, "main: %s\n", SDL_GetError());
		return 1;
	}

	// get the renderer
	SDL_Renderer *rend = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

	// load the font
	SDL_Texture *font = loadFont(rend);

	// Initialise the interface
	int mx, my;
	mx = -1;
	my = -1;
	char *titles[] = {"File","Edit","Help"};
	menubar *m = makeMenuBar(titles, 3);

	// main loop
	SDL_bool running = SDL_TRUE;
	while (running) {
		// draw the window
		SDL_SetRenderDrawColor(rend, COL_BG);
		SDL_RenderClear(rend);
		SDL_SetRenderDrawColor(rend, COL_FG);
		drawMenuBar(m, rend, font, mx, my);
		SDL_RenderPresent(rend);

		// poll for events
		SDL_Event event;
		while (SDL_PollEvent(&event)) {
			switch (event.type) {
			case SDL_QUIT:
				running = SDL_FALSE;
				break;

			case SDL_KEYDOWN:
				fprintf(stdout, "main: %s\n", "got keydown event");
				break;

			case SDL_MOUSEMOTION:
				mx = event.motion.x;
				my = event.motion.y;
				break;
			}
		}
		SDL_Delay(16);
	}

	// Cleanup and quit back to DOS ;)
	SDL_DestroyWindow(window);
	SDL_Quit();
}
