#include "gui.h"
#include "color.h"
#include <stdio.h>

#define SLIDEROFFSET 20
#define SLIDERWIDTH 360
#define SLIDERTHUMBSIZE 20
#define PREVIEWHEIGHT 250
#define COLORMAX 255
#define DRAWSLIDER(rect, label, title) drawText(rend, (title), font, SLIDEROFFSET, (rect).y - (2+LETHEIGHT)); \
	SDL_RenderDrawRect(rend, &(rect)); SDL_RenderDrawLine(rend, SLIDEROFFSET, (rect).y+(SLIDERTHUMBSIZE/2), SLIDEROFFSET+SLIDERWIDTH+(rect).w, (rect).y+(SLIDERTHUMBSIZE/2)); \
	drawText(rend, (label), font, SLIDEROFFSET + SLIDERWIDTH+(rect).w+1, (rect).y)
#define INSLIDERAREA(rect) ((rect).y<my&&my<(rect).y+(rect).h)
#define SETTHUMBX(rect) if(mx<SLIDEROFFSET) (rect).x = SLIDEROFFSET; else if(SLIDEROFFSET+SLIDERWIDTH<mx) (rect.x) = SLIDEROFFSET+SLIDERWIDTH; else (rect).x = mx

SDL_bool pickColor(SDL_Renderer *rend, SDL_Texture *font, SDL_Color *ret) {
	int mx, my;
	SDL_Rect colorPreviewRect = {0, 0, SLIDERWIDTH+SLIDEROFFSET+SLIDERTHUMBSIZE, PREVIEWHEIGHT};
	SDL_Rect okButRect = {0, PREVIEWHEIGHT+2, 4*LETWIDTH, LETHEIGHT+2};
	SDL_Rect cancelButRect = {okButRect.w+1, okButRect.y, 8*LETWIDTH, LETHEIGHT+2};
	SDL_Rect rThumb = {SLIDERWIDTH+SLIDEROFFSET, PREVIEWHEIGHT+(2*SLIDERTHUMBSIZE), SLIDERTHUMBSIZE, SLIDERTHUMBSIZE};
	SDL_Rect gThumb = {SLIDERWIDTH+SLIDEROFFSET, PREVIEWHEIGHT+(6*SLIDERTHUMBSIZE), SLIDERTHUMBSIZE, SLIDERTHUMBSIZE};
	SDL_Rect bThumb = {SLIDERWIDTH+SLIDEROFFSET, PREVIEWHEIGHT+(10*SLIDERTHUMBSIZE), SLIDERTHUMBSIZE, SLIDERTHUMBSIZE};
	SDL_Rect aThumb = {SLIDERWIDTH+SLIDEROFFSET, PREVIEWHEIGHT+(14*SLIDERTHUMBSIZE), SLIDERTHUMBSIZE, SLIDERTHUMBSIZE};
	SDL_bool done = SDL_FALSE;
	char rString[11] = "255 (0xFF)";
	char gString[11] = "255 (0xFF)";
	char bString[11] = "255 (0xFF)";
	char aString[11] = "255 (0xFF)";
	char *fstring = "%3i (0x%02X)";

 LOOPSTART:
	BGCOL;
	SDL_RenderClear(rend);
	FGCOL;
	XRECT(colorPreviewRect);
	SDL_RenderDrawRect(rend, &okButRect);
	SDL_RenderDrawRect(rend, &cancelButRect);
	drawText(rend, "OK", font, okButRect.x+LETWIDTH, okButRect.y+1);
	drawText(rend, "Cancel", font, cancelButRect.x+LETWIDTH, cancelButRect.y+1);
	SDL_SetRenderDrawColor(rend, UNWRAP_COL_PT(ret));
	SDL_RenderFillRect(rend, &colorPreviewRect);
	FGCOL;
	SDL_RenderDrawRect(rend, &colorPreviewRect);
	DRAWSLIDER(rThumb, rString, "Red");
	DRAWSLIDER(gThumb, gString, "Green");
	DRAWSLIDER(bThumb, bString, "Blue");
	DRAWSLIDER(aThumb, aString, "Alpha");
	SDL_RenderPresent(rend);

	SDL_Event event;
	while (SDL_PollEvent(&event)) {
		switch (event.type) {
		case SDL_QUIT:
			return SDL_FALSE;
			break;

		case SDL_MOUSEBUTTONDOWN:
		case SDL_MOUSEBUTTONUP:
			mx = event.button.x;
			my = event.button.y;
			if (INSIDE_RECT(okButRect, mx, my))
				return SDL_TRUE;
			else if (INSIDE_RECT(cancelButRect, mx, my))
				return SDL_FALSE;
			else
				goto HANDLEMOUSE;
		case SDL_MOUSEMOTION:
			mx = event.motion.x;
			my = event.motion.y;
			if (event.motion.state == 0) break;
		HANDLEMOUSE:
			if (INSLIDERAREA(rThumb)) {
				SETTHUMBX(rThumb);
				ret->r = ((rThumb.x-SLIDEROFFSET)*COLORMAX)/SLIDERWIDTH;
				snprintf(rString, 11, fstring, ret->r, ret->r);
			} else if (INSLIDERAREA(gThumb)) {
				SETTHUMBX(gThumb);
				ret->g = ((gThumb.x-SLIDEROFFSET)*COLORMAX)/SLIDERWIDTH;
				snprintf(gString, 11, fstring, ret->g, ret->g);
			} else if (INSLIDERAREA(bThumb)) {
				SETTHUMBX(bThumb);
				ret->b = ((bThumb.x-SLIDEROFFSET)*COLORMAX)/SLIDERWIDTH;
				snprintf(bString, 11, fstring, ret->b, ret->b);
			} else if (INSLIDERAREA(aThumb)) {
				SETTHUMBX(aThumb);
				ret->a = ((aThumb.x-SLIDEROFFSET)*COLORMAX)/SLIDERWIDTH;
				snprintf(aString, 11, fstring, ret->a, ret->a);
			}
			break;
		}
	}
	goto LOOPSTART; // Hey Dijkstra, goto HOME
}
