#ifndef _MSC_VER
#include <SDL2/SDL.h>
#else
#include <SDL.h>
#endif

#ifndef _WIN32
#include <dirent.h>
#else
// abandon hope all ye that enter here
#include <windows.h>
#endif

#include <stdlib.h>
#include <string.h>
#include "gui.h"
#include "platform.h"

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

#ifndef _WIN32
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

char *fileBrowse(SDL_Renderer *rend, SDL_Texture *font, char* dir, enum fileFlags flags) {
	char* curDir = malloc(strlen(dir));
	strcpy(curDir, dir);
	DIR *D = opendir(curDir);
	char *ret = NULL;
	if (D==NULL) {
		goto FAIL;
	}
	char *newfile;
	char *composition = NULL;
	int newfilelen;
	int newfilebuflen;
	int newfilecurs;
	int newfilesel;
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
	SDL_Rect texIn = {0, 0, 0, LETHEIGHT};
	SDL_Rect okRec = {0, 0, LETWIDTH*5, LETHEIGHT};
	SDL_Rect okSRec = {1, 0, (LETWIDTH*5)-2, LETHEIGHT-2};
	if(flags&fileFlag_NewFiles) {
		newfilebuflen = 30;
		newfile = malloc(newfilebuflen);
		newfile[0]='\0';
		newfilecurs = 0;
		newfilelen = 0;
		newfilesel = 0;
		SDL_StartTextInput();
	}
	curent = readdir(D);
	while (curent!=NULL) {
		files[nfiles] = curent;
		nfiles++;
		if (nfiles > bufsize-2) {
			bufsize *= 2;
			files = realloc(files, sizeof(struct dirent)*bufsize);
		}
		curent = readdir(D);
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
		texIn.y=botanchor+1;
		texIn.w=wwidth;
		texIn.h=LETHEIGHT;
		okRec.y=texIn.y+LETHEIGHT;
		okSRec.y=okRec.y+1;
		if(flags&fileFlag_NewFiles) {
			drawText(rend, newfile, font, texIn.x, texIn.y);
			drawText(rend, "OK.", font, okRec.x+LETWIDTH, okRec.y);
			SDL_RenderDrawRect(rend, &okRec);
			if (okRec.x<mousex&&mousex<okRec.x+okRec.w && okRec.y<mousey&&mousey<okRec.y+okRec.h)
				SDL_RenderDrawRect(rend, &okSRec);
		}
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

			case SDL_KEYDOWN:
				if (event.key.keysym.sym == SDLK_BACKSPACE && newfilelen>0) {
					newfilelen--;
					newfile[newfilelen]='\0';
				}
				break;

			case SDL_TEXTINPUT:
				if(newfilelen+1>newfilebuflen) {
					newfilebuflen *= 2;
					newfile = realloc(newfile, newfilebuflen);
				}
				strcat(newfile, event.text.text);
				newfilelen++;
				break;

			case SDL_TEXTEDITING:
				composition = event.edit.text;
				newfilecurs = event.edit.start;
				newfilesel = event.edit.length;

			case SDL_MOUSEBUTTONUP:
				mousex = event.button.x;
				mousey = event.button.y;
				if (event.button.button == SDL_BUTTON_LEFT) {
					if (mousex<LETWIDTH*4&&mousey<botanchor) {
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
					} else if (flags&fileFlag_NewFiles&&(okRec.x<mousex&&mousex<okRec.x+okRec.w && okRec.y<mousey&&mousey<okRec.y+okRec.h)){
						// return contents of text box
						ret = malloc(strlen(curDir)+newfilelen+3);
						strcpy(ret, curDir);
						strcat(ret, PATHSEP);
						strcat(ret, newfile);
						done = SDL_TRUE;
					}
				}
				break;
			}
		}
	}
	free(files);
	if(flags&fileFlag_NewFiles) {
		SDL_StopTextInput();
		free(newfile);
	}
 FAIL:
	free(curDir);
	return ret;
}
#else
char lpstrfilebuffer[MAX_PATH];

char *fileBrowse(SDL_Renderer *rend, SDL_Texture *font, char* dir, enum fileFlags flags) {
	// After today, I rather hope I'll never have to touch this again :)
	// Score was right, win32 is hell
	lpstrfilebuffer[0]='\0';
	OPENFILENAME  ofn;
	memset(&ofn,0,sizeof(ofn));
	ofn.lStructSize     = sizeof(ofn);
	ofn.hwndOwner       = NULL;
	ofn.lpstrFilter     = NULL;
	ofn.lpstrFile       = lpstrfilebuffer;
	ofn.nMaxFile        = MAX_PATH;
	ofn.lpstrTitle      = "Please Select A File To Open";
	ofn.Flags           = OFN_NONETWORKBUTTON |
				OFN_FILEMUSTEXIST |
				OFN_HIDEREADONLY;
	if(flags&fileFlag_NewFiles) {
		ofn.Flags |= OFN_OVERWRITEPROMPT;
		if (GetSaveFileNameA(&ofn))
			return ofn.lpstrFile;
		return NULL;
	} else {
		if (GetOpenFileNameA(&ofn))
			return ofn.lpstrFile;
		return NULL;
	}
}
#endif
