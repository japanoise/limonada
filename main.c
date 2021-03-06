#ifndef _MSC_VER
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#else
#include <SDL.h>
#include <SDL_image.h>
#endif
#ifndef NO_GTK
#ifndef _MSC_VER
#include <gtk/gtk.h>
#else
#include <gtk.h>
#endif
#endif
#include <stdio.h>
#include <stdlib.h>
#include "arg.h"
#include "gui.h"
#include "slice.h"
#include "state.h"
#include "cp437.xpm"
#include "icon.xpm"
#include "tools.xpm"
#include "platform.h"
#include "stb_image_write.h"
#include "color.h"

#define TITLE "Limonada"

#define GETCURBUF buffer *buf = global->buffers->data[global->curbuf];

#define TOOLSIZE 16
#define COLORSIZE 16
#define LEFTBARWIDTH ((TOOLSIZE*2)+1)
#define RIGHTBARWIDTH (COLORSIZE+SCROLLBARWIDTH)
#define TOPBARHEIGHT ((2*LETHEIGHT)+5)
#define BOTBARHEIGHT (LETHEIGHT+1)
#define DRAWAREAHEIGHT (WINHEIGHT-TOPBARHEIGHT-BOTBARHEIGHT)
#define DRAWAREAWIDTH (WINWIDTH-LEFTBARWIDTH-RIGHTBARWIDTH)

SDL_Cursor *pointer;
SDL_Cursor *panner;
SDL_Cursor *toolCursors[10];

#define TOOL_PENCIL 0
#define TOOL_PICKER 1
#define TOOL_FILL 2
#define TOOL_DITHER 3
#define TOOL_PAINT 4
#define TOOL_ERASE 5
#define TOOL_SELECT 6
#define TOOL_LINE 7
#define TOOL_RECT 8
#define TOOL_COLORTOCOLOR 9

typedef struct {
	int num;
	char **entries;
} toolcontext;

toolcontext *contexts[10];

int linepx = -1;
int linepy = -1;
SDL_Color linec;
SDL_Color rectsecc;

int WINWIDTH = 800;
int WINHEIGHT = 600;
const Uint8 *keyboardState;
#define KEYDOWN(key) keyboardState[SDL_GetScancodeFromKey(key)]
#define CTRL_DOWN (KEYDOWN(SDLK_LCTRL)||KEYDOWN(SDLK_RCTRL))
#define SHIFT_DOWN (KEYDOWN(SDLK_LSHIFT)||KEYDOWN(SDLK_RSHIFT))

typedef SDL_bool(*menuItemCallback) (SDL_Renderer * rend, SDL_Texture * font, limonada * global);

SDL_bool actionQuit(SDL_Renderer * rend, SDL_Texture * font, limonada * global)
{
	return SDL_FALSE;
}

SDL_bool actionUndo(SDL_Renderer * rend, SDL_Texture * font, limonada * global)
{
	if (global->curbuf != -1) {
		GETCURBUF;
		bufferDoUndo(buf);
	}
	return SDL_TRUE;
}

SDL_bool actionRedo(SDL_Renderer * rend, SDL_Texture * font, limonada * global)
{
	if (global->curbuf != -1) {
		GETCURBUF;
		bufferDoRedo(buf);
	}
	return SDL_TRUE;
}

SDL_bool actionNew(SDL_Renderer * rend, SDL_Texture * font, limonada * global)
{
	buffer * buf = makeNewBuffer(rend, font);
	if (buf != NULL) {
		global->curbuf = appendBuffer(global->buffers, buf);
	}
	return SDL_TRUE;
}

SDL_bool actionOpen(SDL_Renderer * rend, SDL_Texture * font, limonada * global)
{
	char *fn = fileBrowse(rend, font, getenv("HOME"), 0);
	if (fn != NULL) {
		buffer *buf = makeBufferFromFile(fn);
		global->curbuf = appendBuffer(global->buffers, buf);
#ifndef _WIN32
		/* In win32 this is a global variable that isn't dynamically allocated */
		free(fn);
#endif
	}
	return SDL_TRUE;
}

SDL_bool actionSave(SDL_Renderer * rend, SDL_Texture * font, limonada * global)
{
	if (global->curbuf == -1)
		return SDL_TRUE;
	char *fn = fileBrowse(rend, font, getenv("HOME"), fileFlag_NewFiles);
	if (fn != NULL) {
#ifndef _WIN32
		/* Overwrite confirmation done automagically on Windows */
		if (fexist(fn)) {
			const SDL_MessageBoxButtonData buttons[] = {
				{SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT, 0, "yes"},
				{SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT, 1, "no"},
			};
			const SDL_MessageBoxData messageboxdata = {
				SDL_MESSAGEBOX_WARNING,
				NULL,
				fn,
				"File already exists, overwrite?",
				SDL_arraysize(buttons),
				buttons,
				NULL
			};
			int buttonid = -1;
			if (SDL_ShowMessageBox(&messageboxdata, &buttonid) < 0 || buttonid != 0) {
				free(fn);
				return SDL_TRUE;
			}
		}
#endif
		int l = strlen(fn);
		if (l < 4)
			goto NOSAVE;

		char *ext = malloc(5);
		strncpy(ext, &fn[l - 4], 5);	/* who needs more than 8.3 */
		for (char *i = ext + 1; *i != '\0'; i++)
			*i &= 0xDF;	/* upcase the extension */

		GETCURBUF;
		if (strcmp(buf->filename, fn)) {
			/* Update buffer name */
			setBufferFileName(fn, buf);
			char *bn = basename(fn);
			if (buf->name != NULL && buf->name->String != NULL)
				DestroySlice(buf->name);
			buf->name = MakeSlice(bn);
#ifdef _WIN32
			free(bn);
#endif
		}

		if (strcmp(ext, ".PNG") == 0) {
			stbi_write_png(fn, buf->sizex, buf->sizey, buf->datachannels, buf->data, 0);
			buf->saveUndo = buf->undoList;
		} else if (strcmp(ext, ".TGA") == 0) {
			stbi_write_tga(fn, buf->sizex, buf->sizey, buf->datachannels, buf->data);
			buf->saveUndo = buf->undoList;
		} else if (strcmp(ext, ".BMP") == 0) {
			stbi_write_bmp(fn, buf->sizex, buf->sizey, buf->datachannels, buf->data);
			buf->saveUndo = buf->undoList;
		} else {
			const SDL_MessageBoxButtonData buttons[] = {
				{SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT, 0, "OK"},
			};
			const SDL_MessageBoxData messageboxdata = {
				SDL_MESSAGEBOX_ERROR,
				NULL,
				ext,
				"Filetype not supported",
				SDL_arraysize(buttons),
				buttons,
				NULL
			};
			int but = 0;
			SDL_ShowMessageBox(&messageboxdata, &but);
		}
		free(ext);

 NOSAVE:	;
#ifndef _WIN32
		/* In win32 this is a global variable that isn't dynamically allocated */
		free(fn);
#endif
	}
	return SDL_TRUE;
}

typedef struct {
	StrSlice **entries;
	menuItemCallback *callbacks;
	int nentries;
	int width;
} submenu;

typedef struct {
	StrSlice **titles;
	submenu **submenus;
	int ntitles;
	int vis;
} menubar;

menubar *makeMenuBar(char **titles, int ntitles)
{
	menubar *ret = malloc(sizeof(menubar));
	ret->submenus = malloc(sizeof(submenu **) * ntitles);
	ret->titles = malloc(sizeof(StrSlice **) * ntitles);
	ret->ntitles = ntitles;
	ret->vis = -1;
	for (int i = 0; i < ntitles; i++) {
		ret->titles[i] = MakeSlice(titles[i]);
	}
	return ret;
}

submenu *makeSubmenu(char **entries, int nentries)
{
	submenu *ret = malloc(sizeof(submenu));
	ret->entries = malloc(sizeof(StrSlice **) * nentries);
	ret->callbacks = malloc(sizeof(menuItemCallback) * nentries);
	ret->nentries = nentries;
	ret->width = 0;
	for (int i = 0; i < nentries; i++) {
		ret->entries[i] = MakeSlice(entries[i]);
		if (ret->entries[i]->len > ret->width) {
			ret->width = ret->entries[i]->len;
		}
		ret->callbacks[i] = NULL;
	}
	return ret;
}

void drawMenuBar(menubar * m, SDL_Renderer * rend, SDL_Texture * font, int mx, int my)
{
	SDL_RenderDrawLine(rend, 0, LETHEIGHT + 2, WINWIDTH, LETHEIGHT + 2);
	int ix = 0;
	int ox = 0;
	for (int i = 0; i < m->ntitles; i++) {
		drawText(rend, m->titles[i]->String, font, ix + 2, 1);
		ox = ix;
		ix += (m->titles[i]->len + 1) * LETWIDTH;
		if (my < LETHEIGHT && mx >= ox && mx < ix) {
			SDL_Rect r = { ox, 0, ix - ox, LETHEIGHT + 2 };
			SDL_RenderDrawRect(rend, &r);
			if (m->vis != -1) {
				m->vis = i;
			}
		}
		if (m->vis == i && m->submenus[i] != NULL) {
			BGCOL;
			int smw = (m->submenus[i]->width * LETWIDTH) + 4;
			SDL_Rect rect = { ox, LETHEIGHT + 2, smw,
				2 + (LETHEIGHT * m->submenus[i]->nentries)
			};
			SDL_RenderFillRect(rend, &rect);
			FGCOL;
			SDL_RenderDrawRect(rend, &rect);
			for (int j = 0; j < m->submenus[i]->nentries; j++) {
				int iy = LETHEIGHT + 3 + (j * LETHEIGHT);
				drawText(rend, m->submenus[i]->entries[j]->String,
					 font, ox + 2, iy);
				if (ox <= mx && mx <= ox + smw && iy < my && my <= iy + LETHEIGHT) {
					rect.y = iy;
					rect.h = LETHEIGHT;
					SDL_RenderDrawRect(rend, &rect);
				}
			}
		}
	}
}

void drawTabBar(SDL_Renderer * rend, SDL_Texture * font, limonada * global, menubar * m, int mx,
		int my)
{
	const int botanchor = (2 * LETHEIGHT) + 4;
	SDL_RenderDrawLine(rend, 0, botanchor, WINWIDTH, botanchor);
	int ix = 0;
	int ox = 0;
	for (int i = 0; i < global->buffers->len; i++) {
		StrSlice *bufname = global->buffers->data[i]->name;
		drawText(rend, bufname->String, font, ix + 2, botanchor - LETHEIGHT);
		ox = ix;
		ix += (bufname->len + 1) * LETWIDTH + 2;
		if(bufferIsDirty(global->buffers->data[i])) {
			drawText(rend, "*", font, ix-LETWIDTH, botanchor-LETHEIGHT);
			ix += LETWIDTH;
		}
		SDL_RenderDrawLine(rend, ix - 1, botanchor - LETHEIGHT - 1, ix - 1, botanchor);
		if (i == global->curbuf) {
			SDL_RenderDrawLine(rend, ox, botanchor - 2, ix - 1, botanchor - 2);
		}
		if (LETHEIGHT + 2 < my && my < botanchor && mx >= ox && mx < ix && m->vis == -1) {
			SDL_Rect r = { ox, botanchor - LETHEIGHT - 1, ix - ox - 1, LETHEIGHT + 1 };
			SDL_RenderDrawRect(rend, &r);
		}
	}
}

SDL_Rect toolsRect = { 0, TOPBARHEIGHT, 2 * TOOLSIZE, 5 * TOOLSIZE };
SDL_Rect selToolRect = { 0, TOPBARHEIGHT, TOOLSIZE, TOOLSIZE };
SDL_Rect primaryRect =
    { 0, TOPBARHEIGHT + (5 * TOOLSIZE) + LETHEIGHT, LEFTBARWIDTH - 1, COLORSIZE };
SDL_Rect secondaryRect =
    { 0, TOPBARHEIGHT + (5 * TOOLSIZE) + (LETHEIGHT * 2) + COLORSIZE, LEFTBARWIDTH - 1, COLORSIZE };
SDL_Rect contextRect = { 0, 0, LEFTBARWIDTH - 1, LETHEIGHT + 2 };
SDL_Rect selContextRect = { 1, 0, LEFTBARWIDTH - 3, LETHEIGHT };

void drawToolBar(SDL_Renderer * rend, SDL_Texture * font, SDL_Texture * tool, limonada * global,
		 int mx, int my)
{
	SDL_RenderDrawLine(rend, LEFTBARWIDTH - 1, TOPBARHEIGHT, LEFTBARWIDTH - 1,
			   WINHEIGHT - BOTBARHEIGHT);
	SDL_RenderCopy(rend, tool, NULL, &toolsRect);
	if (global->curbuf != -1) {
		buffer *buf = global->buffers->data[global->curbuf];
		if (1 & buf->tool) {
			selToolRect.x = TOOLSIZE;
		} else {
			selToolRect.x = 0;
		}
		selToolRect.y = ((buf->tool / 2) * TOOLSIZE) + TOPBARHEIGHT;
		SDL_RenderDrawRect(rend, &selToolRect);

		/* draw black Xs under the color area; shows transparency */
		XRECT(primaryRect);
		XRECT(secondaryRect);

		drawText(rend, "1:", font, 0, TOPBARHEIGHT + (5 * TOOLSIZE));
		SDL_SetRenderDrawColor(rend, COL_PRIM);
		SDL_RenderFillRect(rend, &primaryRect);
		FGCOL;
		SDL_RenderDrawRect(rend, &primaryRect);

		drawText(rend, "2:", font, 0,
			 TOPBARHEIGHT + (5 * TOOLSIZE) + LETHEIGHT + COLORSIZE);
		SDL_SetRenderDrawColor(rend, COL_SEC);
		SDL_RenderFillRect(rend, &secondaryRect);
		FGCOL;
		SDL_RenderDrawRect(rend, &secondaryRect);

		if (mx < LEFTBARWIDTH && TOPBARHEIGHT < my && my < TOPBARHEIGHT + (5 * TOOLSIZE)) {
			selToolRect.x = 0;
			selToolRect.y =
			    (((my - TOPBARHEIGHT) / TOOLSIZE) * TOOLSIZE) + TOPBARHEIGHT;
			if (mx > TOOLSIZE) {
				selToolRect.x = TOOLSIZE;
			}
			GREYCOL;
			SDL_RenderDrawRect(rend, &selToolRect);
			FGCOL;
		}

		if (contexts[buf->tool] != NULL) {
			toolcontext *context = contexts[buf->tool];
			int anchor = secondaryRect.y + (2 * COLORSIZE);
			for (int i = 0; i < context->num; i++) {
				contextRect.y = anchor + i + (i * contextRect.h);
				drawText(rend, context->entries[i], font, 1, contextRect.y + 1);
				SDL_RenderDrawRect(rend, &contextRect);
				if (mx < LEFTBARWIDTH && contextRect.y < my
				    && my < contextRect.y + contextRect.h) {
					selContextRect.y = contextRect.y + 1;
					GREYCOL;
					SDL_RenderDrawRect(rend, &selContextRect);
					FGCOL;
				} else if (buf->selcontext == i) {
					selContextRect.y = contextRect.y + 1;
					SDL_RenderDrawRect(rend, &selContextRect);
				}
			}
		}
	}
}

SDL_Rect scrollRect = { 0, 0, SCROLLBARWIDTH, 0 };

void drawScrollBar(SDL_Renderer * rend, SDL_Texture * font, int x, int y, int numitems, int scr,
		   int containerheight, int contentItemHeight)
{
	if (scr > 0)
		drawText(rend, "\x1e", font, x + 1, y);	/* up arrow */

	if ((scr * contentItemHeight) + containerheight < (numitems) * contentItemHeight)
		drawText(rend, "\x1f", font, x + 1, (y + containerheight) - LETHEIGHT);	/* down arrow */

	/*this commented out block is a partial implementation of a classic scroll bar
	   however, I can't figure out how to calculate the thumbscrew size and position :c
	   drawText(rend, "\x1e", font, x+1, y);
	   drawText(rend, "\x1f", font, x + 1, (y + containerheight) - LETHEIGHT);
	   int lx = x + (LETWIDTH / 2);
	   SDL_RenderDrawLine(rend, lx, y + LETHEIGHT, lx, (y + containerheight) - (LETHEIGHT + 1));
	   int scrh = containerheight - (2 * LETHEIGHT);
	   scrollRect.h = scrh;
	   scrollRect.x = x;
	   scrollRect.y = y + LETHEIGHT;
	   SDL_RenderDrawRect(rend, &scrollRect); */
}

SDL_Rect colorRect = { 0, TOPBARHEIGHT, COLORSIZE, COLORSIZE };

void drawPallete(SDL_Renderer * rend, SDL_Texture * font, limonada * global, int mx, int my)
{
	int anchor = WINWIDTH - RIGHTBARWIDTH;
	SDL_RenderDrawLine(rend, anchor, TOPBARHEIGHT, anchor, WINHEIGHT - BOTBARHEIGHT);
	if (global->curbuf != -1) {
		GETCURBUF;
		colorRect.x = anchor + 1;
		colorRect.y = TOPBARHEIGHT;
		for (int i = buf->pal->scroll; i < buf->pal->len; i++) {
			XRECT(colorRect);
			SDL_SetRenderDrawColor(rend, UNWRAP_COL(buf->pal->colors[i]));
			SDL_RenderFillRect(rend, &colorRect);
			FGCOL;
			SDL_RenderDrawRect(rend, &colorRect);
			colorRect.y += COLORSIZE;
		}
		SDL_RenderDrawLine(rend, colorRect.x + (COLORSIZE / 2) - 1, colorRect.y,
				   colorRect.x + (COLORSIZE / 2) - 1,
				   colorRect.y + colorRect.h - 1);
		SDL_RenderDrawLine(rend, colorRect.x, colorRect.y + (COLORSIZE / 2),
				   colorRect.x + colorRect.w - 1, colorRect.y + (COLORSIZE / 2));
		if (anchor < mx && mx < WINWIDTH - SCROLLBARWIDTH && TOPBARHEIGHT < my
		    && my < TOPBARHEIGHT + (COLORSIZE * (buf->pal->len - buf->pal->scroll + 1))) {
			GREYCOL;
			int selcol = ((my - TOPBARHEIGHT) / COLORSIZE);
			colorRect.y = TOPBARHEIGHT + (selcol * COLORSIZE);
			SDL_RenderDrawRect(rend, &colorRect);
			FGCOL;
		}
		drawScrollBar(rend, font, anchor + COLORSIZE + 1, TOPBARHEIGHT, buf->pal->len,
			      buf->pal->scroll, DRAWAREAHEIGHT, COLORSIZE);
	}
}

#define BUFSIZE 20
char stat_size[BUFSIZE];
int stat_size_len = 0;
char stat_xy[BUFSIZE];
int stat_xy_len = 0;
#define ZOOMSIZE 10
char stat_zoom[ZOOMSIZE];
int stat_zoom_len = 0;
int px = 0;
int py = 0;
#define UPDATEPXPY px=buf->panx+((mx-LEFTBARWIDTH)/buf->zoom); py=buf->pany+((my-TOPBARHEIGHT)/buf->zoom);stat_xy_len=snprintf(stat_xy, BUFSIZE, "%i,%i", px, py);stat_zoom_len=snprintf(stat_zoom, ZOOMSIZE, "%i%%", buf->zoom*100);

void drawStatBar(SDL_Renderer * rend, SDL_Texture * font, limonada * global)
{
	int anchor = WINHEIGHT - BOTBARHEIGHT;
	SDL_RenderDrawLine(rend, 0, anchor, WINWIDTH, anchor);
	if (global->curbuf == -1) {
		drawText(rend, "No buffers open.", font, 0, anchor + 1);
		return;
	}

	int ix = 0;
	buffer *buf = global->buffers->data[global->curbuf];
	drawText(rend, buf->name->String, font, ix, anchor + 1);
	ix += (buf->name->len + 1) * LETWIDTH;
	drawText(rend, "(", font, ix, anchor + 1);
	ix += LETWIDTH;
	drawText(rend, stat_size, font, ix, anchor + 1);
	ix += (stat_size_len) * LETWIDTH;
	drawText(rend, ")", font, ix, anchor + 1);
	ix += LETWIDTH * 2;
	drawText(rend, "Zoom: ", font, ix, anchor + 1);
	ix += 6 * LETWIDTH;
	drawText(rend, stat_zoom, font, ix, anchor + 1);
	drawText(rend, stat_xy, font, WINWIDTH - (stat_xy_len * LETWIDTH), anchor + 1);
}

SDL_Rect drawArea = { LEFTBARWIDTH, TOPBARHEIGHT, 0, 0 };
SDL_Rect sprArea = { 0, 0, 0, 0 };

SDL_Texture *curtext;

void drawBuffer(SDL_Renderer * rend, limonada * global)
{
	buffer *buf = global->buffers->data[global->curbuf];
	if (buf->data != NULL && buf->changedp) {
		buf->changedp = 0;
		curtext = textureFromBuffer(buf, rend);
		stat_size_len = snprintf(stat_size, BUFSIZE, "%i,%i", buf->sizex, buf->sizey);
	}
	int sizex = (buf->sizex - buf->panx) * buf->zoom;
	int sizey = (buf->sizey - buf->pany) * buf->zoom;
	sprArea.x = buf->panx;
	sprArea.y = buf->pany;
	int boundaryX = 0;
	int boundaryY = 0;
	if (sizey == DRAWAREAHEIGHT) {
		drawArea.h = DRAWAREAHEIGHT;
		sprArea.h = sizey;
	} else if (sizey < DRAWAREAHEIGHT) {
		sprArea.h = sizey;
		drawArea.h = sizey;
		boundaryY = sizey;
	} else if (sizey > DRAWAREAHEIGHT) {
		drawArea.h = DRAWAREAHEIGHT;
		sprArea.h = DRAWAREAHEIGHT / buf->zoom;
	}
	if (sizex == DRAWAREAWIDTH) {
		drawArea.w = DRAWAREAWIDTH;
		sprArea.w = sizex;
	} else if (sizex < DRAWAREAWIDTH) {
		sprArea.w = sizex;
		drawArea.w = sizex;
		boundaryX = sizex;
	} else if (sizex > DRAWAREAWIDTH) {
		drawArea.w = DRAWAREAWIDTH;
		sprArea.w = DRAWAREAWIDTH / buf->zoom;
	}
	SDL_RenderCopy(rend, curtext, &sprArea, &drawArea);
	if (boundaryY > 0) {
		if (boundaryX > 0) {
			SDL_RenderDrawLine(rend, drawArea.x, boundaryY + drawArea.y,
					   boundaryX + drawArea.x, boundaryY + drawArea.y);
			SDL_RenderDrawLine(rend, boundaryX + drawArea.x, drawArea.y,
					   boundaryX + drawArea.x, boundaryY + drawArea.y);
		} else {
			SDL_RenderDrawLine(rend, drawArea.x, boundaryY + drawArea.y,
					   WINWIDTH - RIGHTBARWIDTH, boundaryY + drawArea.y);
		}
	} else if (boundaryX > 0) {
		SDL_RenderDrawLine(rend, boundaryX + drawArea.x, drawArea.y, boundaryX + drawArea.x,
				   WINHEIGHT - BOTBARHEIGHT);
	}
}

SDL_Texture *loadXpm(SDL_Renderer * rend, char **data)
{
	SDL_Surface *font;
	font = IMG_ReadXPMFromArray(data);
	if (!font) {
		fprintf(stderr, "IMG_ReadXPMFromArray: %s\n", IMG_GetError());
	}
	SDL_Texture *ret = SDL_CreateTextureFromSurface(rend, font);
	if (!ret) {
		fprintf(stderr, "SDL_CreateTextureFromSurface: %s\n", SDL_GetError());
	}
	SDL_FreeSurface(font);
	return ret;
}

SDL_bool click(SDL_Renderer * rend, SDL_Texture * font, limonada * global, menubar * m, int mx,
	       int my, Uint8 button)
{
	if (my <= LETHEIGHT + 2) {
		/* Clicked on the menubar */
		int ix = 0;
		int ox = 0;
		for (int i = 0; i < m->ntitles; i++) {
			ox = ix;
			ix += (m->titles[i]->len + 1) * LETWIDTH;
			if (mx >= ox && mx < ix) {
				if (i == m->vis)
					m->vis = -1;
				else
					m->vis = i;
				return SDL_TRUE;
			}
		}
	} else if (m->vis != -1) {
		/* A submenu is visible */
		int smw = (m->submenus[m->vis]->width * LETWIDTH) + 4;
		int ox = 0;
		for (int i = 0; i < m->vis; i++)
			ox += (m->titles[i]->len + 1) * LETWIDTH;

		if (ox <= mx && mx <= ox + smw) {
			int iy = LETHEIGHT + 3;
			int oy = 0;
			for (int j = 0; j < m->submenus[m->vis]->nentries; j++) {
				oy = iy;
				iy += LETHEIGHT;
				if (oy <= my && my <= iy) {
					/* Calling will be done in release mouse */
					return SDL_TRUE;
				}
			}
		}
	} else if (my <= (2 * LETHEIGHT) + 4) {
		/* Clicked on the tab bar */
		int ix = 0;
		int ox = 0;
		for (int i = 0; i < global->buffers->len; i++) {
			ox = ix;
			ix += (global->buffers->data[i]->name->len + 1)*LETWIDTH + 2;
			if (mx >= ox && mx < ix) {
				if (button == SDL_BUTTON_MIDDLE) {
					killBufferInList(global->buffers, i);
					if (global->buffers->len == 0) {
						global->curbuf = -1;
					}
				} else if (button == SDL_BUTTON_LEFT) {
					global->curbuf = i;
				}
				if (global->curbuf >= global->buffers->len) {
					global->curbuf = global->buffers->len - 1;
				}
				if (global->curbuf != -1) {
					global->buffers->data[global->curbuf]->changedp = 1;
				}
				return SDL_TRUE;
			}
		}
	} else if (global->curbuf != -1 && mx < LEFTBARWIDTH && TOPBARHEIGHT < my) {
		if (my < TOPBARHEIGHT + (5 * TOOLSIZE)) {
			/* Clicked on the toolbar */
			int seltool = ((my - TOPBARHEIGHT) / TOOLSIZE) * 2;
			if (mx > TOOLSIZE) {
				seltool |= 1;
			}
			global->buffers->data[global->curbuf]->tool = seltool;
			global->buffers->data[global->curbuf]->selcontext = 0;
		} else if (my < secondaryRect.y + secondaryRect.h && global->curbuf != -1) {
			/* Clicked on the 1ary and 2ary colors */
			GETCURBUF;
			SDL_Color temp = buf->primary;
			buf->primary = buf->secondary;
			buf->secondary = temp;
		} else if (global->curbuf != -1) {
			/* Clicked where the context menu lives */
			GETCURBUF;
			if (contexts[buf->tool] != NULL) {
				toolcontext *context = contexts[buf->tool];
				int anchor = secondaryRect.y + (COLORSIZE * 2);
				for (int i = 0; i < context->num; i++) {
					contextRect.y = anchor + i + (i * contextRect.h);
					if (mx < LEFTBARWIDTH && contextRect.y < my
					    && my < contextRect.y + contextRect.h) {
						buf->selcontext = i;
					}
				}
			}
		}
	} else if (WINWIDTH - RIGHTBARWIDTH < mx && mx < WINWIDTH - SCROLLBARWIDTH &&
		   TOPBARHEIGHT < my) {
		/* clicked on the palette */
		if (global->curbuf != -1) {
			GETCURBUF;
			if (my < TOPBARHEIGHT + (COLORSIZE * (buf->pal->len - buf->pal->scroll))) {
				int selcol = buf->pal->scroll + ((my - TOPBARHEIGHT) / COLORSIZE);
				if (button == SDL_BUTTON_LEFT) {
					buf->primary = buf->pal->colors[selcol];
				} else if (button == SDL_BUTTON_RIGHT) {
					buf->secondary = buf->pal->colors[selcol];
				}
			} else if (my <
				   TOPBARHEIGHT +
				   ((COLORSIZE) * (buf->pal->len - buf->pal->scroll + 1))) {
				SDL_Color col = { 0xFF, 0xFF, 0xFF, 0xFF };
				if (button == SDL_BUTTON_LEFT) {
					if (pickColorHSV(rend, font, &col))
						addColorToPalette(buf->pal, col);
				} else if (button == SDL_BUTTON_RIGHT) {
					if (pickColor(rend, font, &col))
						addColorToPalette(buf->pal, col);
				}
			}
		}
	} else if (global->curbuf != -1) {
		/* clicked on the paint area */
		GETCURBUF;
		UPDATEPXPY;
		if (0 <= px && px < buf->sizex && 0 <= py && py < buf->sizey) {
			SDL_Color color;
			if (button == SDL_BUTTON_LEFT) {
				color = buf->primary;
			} else {
				color = buf->secondary;
			}
			switch (buf->tool) {
			case TOOL_PENCIL:
				if (button == SDL_BUTTON_LEFT || button == SDL_BUTTON_RIGHT) {
					bufferStartUndo(buf);
					bufferPencil(buf, px, py, color);
				}
				break;
			case TOOL_PICKER:
				if (button == SDL_BUTTON_LEFT) {
					buf->primary = bufferGetColorAt(buf, px, py);
				} else if (button == SDL_BUTTON_RIGHT) {
					buf->secondary = bufferGetColorAt(buf, px, py);
				} else {
					addColorToPalette(buf->pal, bufferGetColorAt(buf, px, py));
				}
				break;
			case TOOL_RECT:
				if (button == SDL_BUTTON_LEFT || button == SDL_BUTTON_RIGHT) {
					linepx = px;
					linepy = py;
					linec = color;
					rectsecc =
					    button ==
					    SDL_BUTTON_LEFT ? buf->secondary : buf->primary;
				}
				break;
			case TOOL_LINE:
				if (button == SDL_BUTTON_LEFT || button == SDL_BUTTON_RIGHT) {
					linepx = px;
					linepy = py;
					linec = color;
				}
				break;
			case TOOL_FILL:
				if (button == SDL_BUTTON_LEFT || button == SDL_BUTTON_RIGHT) {
					bufferDoFloodFill(buf, px, py, color);
				}
				break;
			case TOOL_DITHER:
				if (button == SDL_BUTTON_LEFT) {
					bufferDoFloodFillDither(buf, px, py, buf->primary,
								buf->secondary);
				} else if (button == SDL_BUTTON_RIGHT) {
					bufferDoFloodFillDither(buf, px, py, buf->secondary,
								buf->primary);
				}
			}
		}
	}
	m->vis = -1;
	return SDL_TRUE;
}

void scroll(buffer * buf, SDL_Event event, int mx, int my)
{
	if (event.wheel.y > 0) {
		/* scroll up */
		if (mx > (WINWIDTH - RIGHTBARWIDTH)) {
			if (buf->pal->scroll > 0)
				buf->pal->scroll--;
			UPDATEPXPY;
			return;
		}
		if (CTRL_DOWN) {
			if (buf->zoom < 512)
				buf->zoom *= 2;
		} else if (SHIFT_DOWN) {
			buf->panx--;
			if (buf->panx < 0) {
				buf->panx = 0;
			}
		} else {
			buf->pany--;
			if (buf->pany < 0) {
				buf->pany = 0;
			}
		}
	} else if (event.wheel.y < 0) {
		/* scroll down */
		if (mx > (WINWIDTH - RIGHTBARWIDTH)) {
			if (buf->pal->scroll < buf->pal->len - 2)
				buf->pal->scroll++;
			UPDATEPXPY;
			return;
		}
		if (CTRL_DOWN) {
			buf->zoom /= 2;
			if (buf->zoom <= 0) {
				buf->zoom = 1;
			}
		} else if (SHIFT_DOWN) {
			buf->panx++;
			if (buf->panx > buf->sizex) {
				buf->panx = buf->sizex;
			}
		} else {
			buf->pany++;
			if (buf->pany > buf->sizey) {
				buf->pany = buf->sizey;
			}
		}
	}
	UPDATEPXPY;
}

void setupCursors()
{
	/* Bog standard pointer cursor */
	pointer = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_ARROW);
	panner = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZEALL);

	/* Use crosshairs as default */
	toolCursors[0] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_CROSSHAIR);
	for (int i = 1; i < 10; i++)
		toolCursors[i] = toolCursors[0];

	/* Just use a pointer for pencil */
	toolCursors[TOOL_PENCIL] = pointer;

	/* Hand-hack a pipette in hex because we hate efficiency at seekrit.club */
	Uint8 pick[32] =
	    { 0x40, 0x00, 0xB0, 0x00, 0x48, 0x00, 0x44, 0x00, 0x22, 0x00, 0x11, 0x00, 0x08, 0x80,
       0x04, 0x50, 0x02, 0x38, 0x01, 0x7C, 0x00, 0xF8, 0x01, 0x1C, 0x00, 0xAE, 0x00, 0x57, 0x00,
       0x0B, 0x00, 0x06 };
	Uint8 pmask[32];
	for (int i = 0; i < 32; i++)
		pmask[i] = 0x00;
	toolCursors[TOOL_PICKER] = SDL_CreateCursor(pick, pmask, 16, 16, 0, 0);
}

#define MAKE_CONTEXT(whichtool, numitems) contexts[whichtool] = malloc(sizeof(toolcontext));\
	contexts[whichtool]->num = numitems;\
	contexts[whichtool]->entries = malloc(numitems*sizeof(char*))

void setupToolContexts()
{
	for (int i = 0; i < 10; i++) {
		contexts[i] = NULL;
	}
	MAKE_CONTEXT(TOOL_RECT, 2);
	contexts[TOOL_RECT]->entries[0] = " [ ]";
	contexts[TOOL_RECT]->entries[1] = " [\xfe]";
	/* Here's to the old-school! http://dilldoe.org/images/Brush.jpg */
	MAKE_CONTEXT(TOOL_PAINT, 6);
	contexts[TOOL_PAINT]->entries[0] = " . 1";
	contexts[TOOL_PAINT]->entries[1] = " \xfe 2";
	contexts[TOOL_PAINT]->entries[2] = " \x07 4";
	contexts[TOOL_PAINT]->entries[3] = " \xfe 5";
	contexts[TOOL_PAINT]->entries[4] = " \x07 7";
	contexts[TOOL_PAINT]->entries[5] = " \xfe 8";
	MAKE_CONTEXT(TOOL_ERASE, 4);
	contexts[TOOL_ERASE]->entries[0] = " \xfe 4";
	contexts[TOOL_ERASE]->entries[1] = " \xfe 6";
	contexts[TOOL_ERASE]->entries[2] = " \xfe 8";
	contexts[TOOL_ERASE]->entries[3] = " \xfe" "10";
	/* http://dilldoe.org/images/Lines.jpg */
	MAKE_CONTEXT(TOOL_LINE, 5);
	contexts[TOOL_LINE]->entries[0] = " \xb3 1";
	contexts[TOOL_LINE]->entries[1] = " \xb3 2";
	contexts[TOOL_LINE]->entries[2] = " \xba 3";
	contexts[TOOL_LINE]->entries[3] = " \xba 4";
	contexts[TOOL_LINE]->entries[4] = " \xdb 5";
}

void usage(char *argv0)
{
	fprintf(stderr, "usage: %s [files...]\n", argv0);
	exit(1);
}

SDL_Rect rectToolRect;

int main(int argc, char *argv[])
{
	/* parse args */
	char *argv0 = argv[0];
	ARGBEGIN {
case 'h':
default:
		usage(argv0);
	}
	ARGEND;

#ifndef NO_GTK
	gtk_init(&argc, &argv);
#endif

	/* initialise state */
	limonada *global;
	if (argc > 0) {
		buflist *buffers = makeBuflistFromArgs(argc, argv);
		global = makeState(buffers);
	} else {
		buflist *buffers = makeBuflist();
		global = makeState(buffers);
	}

	/* init sdl */
	SDL_Init(SDL_INIT_EVERYTHING);

	/* get the window */
	SDL_Window *window = SDL_CreateWindow(TITLE, SDL_WINDOWPOS_UNDEFINED,
					      SDL_WINDOWPOS_UNDEFINED, WINWIDTH, WINHEIGHT,
					      SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
	if (!window) {
		fprintf(stderr, "main: %s\n", SDL_GetError());
		return 1;
	}
	/* get the renderer */
	SDL_Renderer *rend = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
	SDL_SetRenderDrawBlendMode(rend, SDL_BLENDMODE_BLEND);

	/* load the font and icon */
	SDL_Texture *font = loadXpm(rend, cp437);
	SDL_Texture *tool = loadXpm(rend, tools);
	SDL_Surface *icons = IMG_ReadXPMFromArray(icon);
	SDL_SetWindowIcon(window, icons);

	/* Initialise the interface */
	int mx, my;
	mx = -1;
	my = -1;
	char *titles[] = { "File", "Edit", "Help" };
	menubar *m = makeMenuBar(titles, 3);
	char *fileentries[] = { "New", "Open", "Save", "Quit" };
	m->submenus[0] = makeSubmenu(fileentries, 4);
	m->submenus[0]->callbacks[0] = *actionNew;
	m->submenus[0]->callbacks[1] = *actionOpen;
	m->submenus[0]->callbacks[2] = *actionSave;
	m->submenus[0]->callbacks[3] = *actionQuit;
	char *editentries[] = { "Undo", "Redo", "Copy", "Paste" };
	m->submenus[1] = makeSubmenu(editentries, 4);
	m->submenus[1]->callbacks[0] = *actionUndo;
	m->submenus[1]->callbacks[1] = *actionRedo;
	char *helpentries[] = { "On-Line Help", "About..." };
	m->submenus[2] = makeSubmenu(helpentries, 2);
	SDL_Rect paintArea = { LEFTBARWIDTH, TOPBARHEIGHT, DRAWAREAWIDTH, DRAWAREAHEIGHT };
	keyboardState = SDL_GetKeyboardState(NULL);
	setupCursors();
	setupToolContexts();

	/* main loop */
	SDL_bool running = SDL_TRUE;
	while (running) {
		/* update variables */
		SDL_GetRendererOutputSize(rend, &WINWIDTH, &WINHEIGHT);
		SDL_GetMouseState(&mx, &my);
		/* draw the window */
		BGCOL;
		SDL_RenderClear(rend);
		GREYCOL;
		SDL_RenderFillRect(rend, &paintArea);
		FGCOL;
		if (global->curbuf != -1) {
			drawBuffer(rend, global);
			if (linepx != -1 && linepy != -1) {
				GETCURBUF;
				UPDATEPXPY;
				SDL_SetRenderDrawColor(rend, UNWRAP_COL(linec));
				if (buf->tool == TOOL_LINE) {
					SDL_RenderDrawLine(rend,
							   (linepx * buf->zoom) + LEFTBARWIDTH,
							   (linepy * buf->zoom) + TOPBARHEIGHT, mx,
							   my);
				} else if (buf->tool == TOOL_RECT) {
					int rx, ry, rw, rh;
					int mpx = (linepx * buf->zoom) + LEFTBARWIDTH;
					int mpy = (linepy * buf->zoom) + TOPBARHEIGHT;
					if (mx >= mpx) {
						rx = mpx;
						rw = mx - mpx;
					} else {
						rx = mx;
						rw = mpx - mx;
					}
					if (my >= mpy) {
						ry = mpy;
						rh = my - mpy;
					} else {
						ry = my;
						rh = mpy - my;
					}
					if (rx < LEFTBARWIDTH) {
						rx = LEFTBARWIDTH;
						rw = mpx - rx;
					}
					if (ry < TOPBARHEIGHT) {
						ry = TOPBARHEIGHT;
						rh = mpy - ry;
					}
					rectToolRect.x = rx;
					rectToolRect.y = ry;
					rectToolRect.w = rw;
					rectToolRect.h = rh;
					if (buf->selcontext == 1) {
						SDL_SetRenderDrawColor(rend, UNWRAP_COL(rectsecc));
						SDL_RenderFillRect(rend, &rectToolRect);
						SDL_SetRenderDrawColor(rend, UNWRAP_COL(linec));
					}
					SDL_RenderDrawRect(rend, &rectToolRect);
				}
				FGCOL;
			}
		}
		drawToolBar(rend, font, tool, global, mx, my);
		drawPallete(rend, font, global, mx, my);
		drawStatBar(rend, font, global);
		drawTabBar(rend, font, global, m, mx, my);
		drawMenuBar(m, rend, font, mx, my);
		SDL_RenderPresent(rend);

		/* poll for events */
		SDL_Event event;
		while (SDL_PollEvent(&event)) {
			switch (event.type) {
			case SDL_QUIT:
				running = SDL_FALSE;
				break;

			case SDL_KEYDOWN:
				switch (event.key.keysym.sym) {
				case SDLK_z:
					if (CTRL_DOWN && global->curbuf != -1) {
						GETCURBUF;
						bufferDoUndo(buf);
					}
					break;
				case SDLK_y:
					if (CTRL_DOWN && global->curbuf != -1) {
						GETCURBUF;
						bufferDoRedo(buf);
					}
					break;
				}
				break;

			case SDL_MOUSEWHEEL:
				if (global->curbuf != -1) {
					buffer *buf = global->buffers->data[global->curbuf];
					scroll(buf, event, mx, my);
				}
				break;

			case SDL_MOUSEBUTTONDOWN:
				mx = event.button.x;
				my = event.button.y;
				running = click(rend, font, global, m, mx, my, event.button.button);
				break;

			case SDL_MOUSEBUTTONUP:
				mx = event.button.x;
				my = event.button.y;
				if (m->vis != -1) {
					/* A submenu is visible */
					int smw = (m->submenus[m->vis]->width * LETWIDTH) + 4;
					int ox = 0;
					for (int i = 0; i < m->vis; i++)
						ox += (m->titles[i]->len + 1) * LETWIDTH;

					if (ox <= mx && mx <= ox + smw) {
						int iy = LETHEIGHT + 3;
						int oy = 0;
						for (int j = 0; j < m->submenus[m->vis]->nentries;
						     j++) {
							oy = iy;
							iy += LETHEIGHT;
							if (oy <= my && my <= iy) {
								if (m->submenus[m->vis]->
								    callbacks[j] != NULL) {
									int sel = m->vis;
									m->vis = -1;
									running =
									    (*m->submenus
									     [sel]->callbacks[j])
									    (rend, font, global);
									break;
								}
								m->vis = -1;
								break;
							}
						}
					}
				} else if (global->curbuf != -1) {
					GETCURBUF;
					SDL_SetCursor(toolCursors[buf->tool]);
					switch (buf->tool) {
					case TOOL_PENCIL:
						if (buf->undoList != NULL
						    && buf->undoList->type != EndUndo) {
							bufferEndUndo(buf);
						}
						break;
					case TOOL_LINE:
						if (linepx != -1 && linepy != -1) {
							UPDATEPXPY;
							bufferStartUndo(buf);
							int epx = px;
							int epy = py;
							if (epx > buf->sizex)
								epx = buf->sizex - 1;
							else if (epx < 0)
								epx = 0;
							if (epy > buf->sizey)
								epy = buf->sizey - 1;
							else if (epy < 0)
								epy = 0;
							bufferDrawLine(buf, linec, linepx, linepy,
								       epx, epy);
							bufferEndUndo(buf);
							linepx = linepy = -1;
						}
						break;
					case TOOL_RECT:
						if (linepx != -1 && linepy != -1) {
							UPDATEPXPY;
							bufferStartUndo(buf);
							int epx = px;
							int epy = py;
							if (epx > buf->sizex)
								epx = buf->sizex - 1;
							else if (epx < 0)
								epx = 0;
							if (epy > buf->sizey)
								epy = buf->sizey - 1;
							else if (epy < 0)
								epy = 0;

							int x1 = epx < linepx ? epx : linepx;
							int x2 = epx > linepx ? epx : linepx;
							int y1 = epy < linepy ? epy : linepy;
							int y2 = epy > linepy ? epy : linepy;
							if (buf->selcontext == 1) {
								bufferDoRectFill(buf, x1, y1, x2,
										 y2, rectsecc,
										 linec);
							} else {
								bufferDoRectOutline(buf, x1, y1, x2,
										    y2, linec);
							}

							bufferEndUndo(buf);
							linepx = linepy = -1;
						}
					}
				}
				break;

			case SDL_MOUSEMOTION:
				mx = event.motion.x;
				my = event.motion.y;
				if (m->vis == -1 && global->curbuf != -1) {
					GETCURBUF;
					if (event.motion.state & SDL_BUTTON_MMASK) {
						/* If middle button, pan the image */
						SDL_SetCursor(panner);
						buf->panx -= event.motion.xrel;
						buf->pany -= event.motion.yrel;
						if (buf->panx > buf->sizex) {
							buf->panx = buf->sizex;
						} else if (buf->panx < 0) {
							buf->panx = 0;
						}
						if (buf->pany > buf->sizey) {
							buf->pany = buf->sizey;
						} else if (buf->pany < 0) {
							buf->pany = 0;
						}
					} else if (event.motion.state == 0) {
						/* If nothing pressed, set the cursor */
						if (mx < LEFTBARWIDTH || my < TOPBARHEIGHT
						    || my > WINHEIGHT - BOTBARHEIGHT
						    || mx > WINWIDTH - RIGHTBARWIDTH) {
							SDL_SetCursor(pointer);
						} else {
							SDL_SetCursor(toolCursors[buf->tool]);
						}
					}
					UPDATEPXPY;
					if ((event.motion.state & SDL_BUTTON_RMASK
					     || event.motion.state & SDL_BUTTON_LMASK)
					    && LEFTBARWIDTH < mx && mx < WINWIDTH - RIGHTBARWIDTH
					    && TOPBARHEIGHT < my && my < WINHEIGHT - BOTBARHEIGHT
					    && 0 <= px && px < buf->sizex && 0 <= py
					    && py < buf->sizey) {
						/* dragging for tools */
						SDL_Color color;
						if (event.motion.state & SDL_BUTTON_LMASK) {
							color = buf->primary;
						} else {
							color = buf->secondary;
						}
						switch (buf->tool) {
						case TOOL_PENCIL:
							bufferSetPixel(buf, px, py, color);
							break;
						}
					}
				}
				break;

			case SDL_WINDOWEVENT:
				paintArea.w = DRAWAREAWIDTH;
				paintArea.h = DRAWAREAHEIGHT;
				break;
			}
		}
	}

	/* Cleanup and quit back to DOS ;) */
	SDL_DestroyWindow(window);
	SDL_Quit();
}

/* Local Variables: */
/* c-basic-offset: 8 */
/* tab-width: 8 */
/* indent-tabs-mode: t */
/* End: */
