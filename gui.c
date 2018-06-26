#ifndef _MSC_VER
#include <SDL2/SDL.h>
#else
#include <SDL.h>
#endif

#ifndef _WIN32
#include <dirent.h>
#define PATHSEP "/"
#else
#include "dirent.h"
#define PATHSEP "\\"
#endif

#include <stdlib.h>
#include <string.h>
#include "gui.h"

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

#define LEFT_FIRST -1
#define EQUAL 0
#define RIGHT_FIRST 1

int compareDirent(const void* l, const void* r) {
	struct dirent **left = (struct dirent**) l;
	struct dirent **right = (struct dirent**) r;
	if ((*left)->d_type==DT_DIR && (*right)->d_type!=DT_DIR) return LEFT_FIRST;
	else if ((*left)->d_type!=DT_DIR && (*right)->d_type==DT_DIR) return RIGHT_FIRST;
	else return strcmp((*left)->d_name, (*right)->d_name);
}

char *fileBrowse(SDL_Renderer *rend, SDL_Texture *font, char* dir) {
	char* curDir = malloc(strlen(dir));
	strcpy(curDir, dir);
	DIR *D = opendir(curDir);
	char *ret = NULL;
	if (D==NULL) {
		goto FAIL;
	}
	SDL_bool done = SDL_FALSE;
	int bufsize = 20;
	int nfiles = 0;
	struct dirent **files = malloc(sizeof(struct dirent*)*bufsize);
	struct dirent *curent;
	int scroll = 0;
	int wheight = 0;
	int wwidth = 0;
	int mousex = -1;
	int mousey = -1;
	SDL_Rect selrec = {LETWIDTH*4,0,0,LETHEIGHT};
	while ((curent = readdir(D))!=NULL) {
		files[nfiles] = curent;
		nfiles++;
		if (nfiles > bufsize-2) {
			bufsize *= 2;
			files = realloc(files, sizeof(struct dirent)*bufsize);
		}
	}
	qsort(files, nfiles, sizeof(struct dirent*), compareDirent);
	while (!done) {
		SDL_GetRendererOutputSize(rend, &wwidth, &wheight);
		SDL_GetMouseState(&mousex, &mousey);
		BGCOL;
		SDL_RenderClear(rend);
		FGCOL;
		drawText(rend, curDir, font, 0, 0);
		if (scroll>0) {
			drawText(rend, "\x18", font, 0, 2*LETHEIGHT);
		}
		int y = 2*LETHEIGHT;
		int i = scroll;
		int sel = -1;
		int botanchor = wheight-(2*LETHEIGHT);
		while (y<botanchor-LETHEIGHT&&i<nfiles) {
			if (files[i]->d_type == DT_DIR) {
				drawText(rend, "\x1c", font, LETWIDTH*2, y);
			} else {
				drawText(rend, "f", font, LETWIDTH*2, y);
			}
			drawText(rend, files[i]->d_name, font, LETWIDTH*4, y);
			if (mousex>LETWIDTH*4 && y< mousey&&mousey <y+LETHEIGHT) {
				selrec.y=y;
				selrec.w=wwidth-(LETWIDTH*4);
				SDL_RenderDrawRect(rend, &selrec);
				sel = i;
			}
			i++;
			y+=LETHEIGHT;
		}
		if (i<nfiles) {
			drawText(rend, "\x19", font, 0, y-LETHEIGHT);
		}
		SDL_RenderDrawLine(rend, 0, botanchor, wwidth, botanchor);
		SDL_RenderPresent(rend);

		SDL_Event event;
		while (SDL_PollEvent(&event)) {
			switch (event.type) {
			case SDL_QUIT:
				done = SDL_TRUE;
				break;

			case SDL_MOUSEWHEEL:
				if (event.wheel.y > 0) {
					// scroll up
					if (scroll>0)
						scroll--;
				} else {
					// scroll down
					if (i<nfiles&&(scroll<nfiles-2))
						scroll++;
				}
				break;

			case SDL_MOUSEBUTTONUP:
				mousex = event.button.x;
				mousey = event.button.y;
				if (event.button.button == SDL_BUTTON_LEFT) {
					if (mousex<LETWIDTH*4) {
						if (mousey<wheight/2) {
							// scroll up
							if (scroll>0)
								scroll--;
						} else {
							// scroll down
							if (i<nfiles&&(scroll<nfiles-2))
								scroll++;
						}
					} else if (sel!=-1) {
						if (files[sel]->d_type == DT_DIR) {
							// change directory
							curDir = realloc(curDir, strlen(curDir)+strlen(files[sel]->d_name)+3);
							strcat(curDir, PATHSEP);
							strcat(curDir, files[sel]->d_name);
							nfiles = 0;
							scroll = 0;
							char *tmp = malloc(strlen(curDir));
							realpath(curDir, tmp);
							strcpy(curDir, tmp);
							free(tmp);
							D = opendir(curDir);
							while ((curent = readdir(D))!=NULL) {
								files[nfiles] = curent;
								nfiles++;
								if (nfiles > bufsize-2) {
									bufsize *= 2;
									files = realloc(files, sizeof(struct dirent)*bufsize);
								}
							}
							qsort(files, nfiles, sizeof(struct dirent*), compareDirent);
						} else {
							// return filename
							ret = malloc(strlen(curDir)+strlen(files[sel]->d_name)+3);
							strcpy(ret, curDir);
							strcat(ret, PATHSEP);
							strcat(ret, files[sel]->d_name);
							done = SDL_TRUE;
						}
					}
				}
				break;
			}
		}
	}
	free(files);
 FAIL:
	free(curDir);
	return ret;
}
