#include "gui.h"
#include "color.h"
#include "hsv.h"
#include <stdio.h>
#include <math.h>
#include "stb_image.h"

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

const char *fstring = "%3i (0x%02X)";

SDL_bool pickColor(SDL_Renderer * rend, SDL_Texture * font, SDL_Color * ret)
{
	int mx, my;
	SDL_Rect colorPreviewRect =
	    { 0, 0, SLIDERWIDTH + SLIDEROFFSET + SLIDERTHUMBSIZE, PREVIEWHEIGHT };
	SDL_Rect okButRect = { 0, PREVIEWHEIGHT + 2, 4 * LETWIDTH, LETHEIGHT + 2 };
	SDL_Rect cancelButRect = { okButRect.w + 1, okButRect.y, 8 * LETWIDTH, LETHEIGHT + 2 };
	SDL_Rect rThumb =
	    { SLIDERWIDTH + SLIDEROFFSET, PREVIEWHEIGHT + (2 * SLIDERTHUMBSIZE), SLIDERTHUMBSIZE,
		SLIDERTHUMBSIZE
	};
	SDL_Rect gThumb =
	    { SLIDERWIDTH + SLIDEROFFSET, PREVIEWHEIGHT + (6 * SLIDERTHUMBSIZE), SLIDERTHUMBSIZE,
		SLIDERTHUMBSIZE
	};
	SDL_Rect bThumb =
	    { SLIDERWIDTH + SLIDEROFFSET, PREVIEWHEIGHT + (10 * SLIDERTHUMBSIZE), SLIDERTHUMBSIZE,
		SLIDERTHUMBSIZE
	};
	SDL_Rect aThumb =
	    { SLIDERWIDTH + SLIDEROFFSET, PREVIEWHEIGHT + (14 * SLIDERTHUMBSIZE), SLIDERTHUMBSIZE,
		SLIDERTHUMBSIZE
	};
	SDL_bool done = SDL_FALSE;
	char rString[11] = "255 (0xFF)";
	char gString[11] = "255 (0xFF)";
	char bString[11] = "255 (0xFF)";
	char aString[11] = "255 (0xFF)";

 LOOPSTART:
	BGCOL;
	SDL_RenderClear(rend);
	FGCOL;
	XRECT(colorPreviewRect);
	SDL_RenderDrawRect(rend, &okButRect);
	SDL_RenderDrawRect(rend, &cancelButRect);
	drawText(rend, "OK", font, okButRect.x + LETWIDTH, okButRect.y + 1);
	drawText(rend, "Cancel", font, cancelButRect.x + LETWIDTH, cancelButRect.y + 1);
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
			if (event.motion.state == 0)
				break;
 HANDLEMOUSE:
			if (INSLIDERAREA(rThumb)) {
				SETTHUMBX(rThumb);
				ret->r = ((rThumb.x - SLIDEROFFSET) * COLORMAX) / SLIDERWIDTH;
				snprintf(rString, 11, fstring, ret->r, ret->r);
			} else if (INSLIDERAREA(gThumb)) {
				SETTHUMBX(gThumb);
				ret->g = ((gThumb.x - SLIDEROFFSET) * COLORMAX) / SLIDERWIDTH;
				snprintf(gString, 11, fstring, ret->g, ret->g);
			} else if (INSLIDERAREA(bThumb)) {
				SETTHUMBX(bThumb);
				ret->b = ((bThumb.x - SLIDEROFFSET) * COLORMAX) / SLIDERWIDTH;
				snprintf(bString, 11, fstring, ret->b, ret->b);
			} else if (INSLIDERAREA(aThumb)) {
				SETTHUMBX(aThumb);
				ret->a = ((aThumb.x - SLIDEROFFSET) * COLORMAX) / SLIDERWIDTH;
				snprintf(aString, 11, fstring, ret->a, ret->a);
			}
			break;
		}
	}
	goto LOOPSTART;		/* Hey Dijkstra, goto HOME */
}

SDL_Texture *LoadHSVTexture(SDL_Renderer * rend)
{
	/* based on the tutorial in the SDL documentation */
	int req_format = STBI_rgb;
	int width, height, orig_format;
	unsigned char *data =
	    stbi_load_from_memory(HSV_PNG, HSV_PNG_LEN, &width, &height, &orig_format, req_format);
	if (data == NULL) {
		SDL_Log("Loading image failed: %s", stbi_failure_reason());
		exit(1);
	}
	/*
	 * Set up the pixel format color masks for RGB(A) byte arrays.
	 * Only STBI_rgb (3) and STBI_rgb_alpha (4) are supported here!
	 */
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
	amask = (req_format == STBI_rgb) ? 0 : 0xff000000;
#endif

	int depth, pitch;
	if (req_format == STBI_rgb) {
		depth = 24;
		pitch = 3 * width;	/* 3 bytes per pixel * pixels per row */
	} else {		/* STBI_rgb_alpha (RGBA) */
		depth = 32;
		pitch = 4 * width;
	}

	SDL_Surface *surf = SDL_CreateRGBSurfaceFrom((void *)data, width, height, depth, pitch,
						     rmask, gmask, bmask, amask);

	SDL_Texture *ret = SDL_CreateTextureFromSurface(rend, surf);

	/* when you don't need the surface anymore, free it.. */
	SDL_FreeSurface(surf);
	/* .. *and* the data used by the surface! */
	stbi_image_free(data);

	return ret;
}

/* Ported from https://github.com/lucasb-eyer/go-colorful/blob/master/colors.go#L148 */
void hsv(double H, double S, double V, SDL_Color * ret)
{
	double Hp = H / 60.0;
	double C = V * S;
	double X = C * (1.0 - fabs(fmod(Hp, 2.0) - 1.0));

	double m = V - C;
	double r, g, b;
	r = g = b = 0.0;

	if (0.0 <= Hp && Hp < 1.0) {
		r = C;
		g = X;
	} else if (1.0 <= Hp && Hp < 2.0) {
		r = X;
		g = C;
	} else if (2.0 <= Hp && Hp < 3.0) {
		g = C;
		b = X;
	} else if (3.0 <= Hp && Hp < 4.0) {
		g = X;
		b = C;
	} else if (4.0 <= Hp && Hp < 5.0) {
		r = X;
		b = C;
	} else if (5.0 <= Hp && Hp < 6.0) {
		r = C;
		b = X;
	}

	ret->r = (int)((m + r) * 255.0);
	ret->g = (int)((m + g) * 255.0);
	ret->b = (int)((m + b) * 255.0);
}

#define HSVX 10
#define HSVY 10
#define XHAIRW 3
#define XHAIRH 5
#define xhairn xhair[0]
#define xhairs xhair[1]
#define xhaire xhair[2]
#define xhairw xhair[3]
#define UPDATEHSV hsv((double)hueSel, ((double)(100-vSel))/100.0, ((double)satSel)/100.0, ret);	\
				snprintf(rString, 11, fstring, ret->r, ret->r);\
				snprintf(gString, 11, fstring, ret->g, ret->g);\
				snprintf(bString, 11, fstring, ret->b, ret->b)

SDL_bool pickColorHSV(SDL_Renderer * rend, SDL_Texture * font, SDL_Color * ret)
{
	SDL_Rect xhair[4] =
	    { {0, 0, XHAIRW, XHAIRH}, {0, 0, XHAIRW, XHAIRH}, {0, 0, XHAIRH, XHAIRW}, {0, 0, XHAIRH,
										       XHAIRW}
	};
	SDL_Rect hsvRect = { HSVX, HSVY, 360, 100 };
	SDL_Rect previewRect = { HSVX, HSVY + 110, 360, 20 };
	SDL_Texture *hsvText = LoadHSVTexture(rend);
	char focusedItem = 0;
	int hueSel = 0;
	int vSel = 0;
	int satSel = 100;
	SDL_Rect okButRect = { HSVX, HSVY + 131, 4 * LETWIDTH, LETHEIGHT + 2 };
	SDL_Rect cancelButRect =
	    { HSVX + okButRect.w + 1, okButRect.y, 8 * LETWIDTH, LETHEIGHT + 2 };
	SDL_Rect satThumb =
	    { SLIDERWIDTH + SLIDEROFFSET, HSVY + 130 + (2 * SLIDERTHUMBSIZE), SLIDERTHUMBSIZE,
		SLIDERTHUMBSIZE
	};
	SDL_Rect aThumb =
	    { SLIDERWIDTH + SLIDEROFFSET, HSVY + 130 + (4 * SLIDERTHUMBSIZE), SLIDERTHUMBSIZE,
		SLIDERTHUMBSIZE
	};
	int textinfoAnchor = aThumb.y + (2 * SLIDERTHUMBSIZE);
	char rString[11] = "255 (0xFF)";
	char gString[11] = "255 (0xFF)";
	char bString[11] = "255 (0xFF)";
	char aString[11] = "255 (0xFF)";
	char satString[4] = "100";
	char hString[4] = "0";
	char vString[4] = "100";
	UPDATEHSV;
	int mx, my;

	for (;;) {
		BGCOL;
		SDL_RenderClear(rend);

		FGCOL;
		XRECT(previewRect);
		SDL_SetRenderDrawColor(rend, UNWRAP_COL_PT(ret));
		SDL_RenderFillRect(rend, &previewRect);

		BGCOL;
		SDL_RenderCopy(rend, hsvText, NULL, &hsvRect);
		xhairn.x = xhairs.x = hueSel + HSVX;
		xhairn.y = (vSel - (2 + XHAIRH)) + HSVY;
		xhairs.y = vSel + 2 + HSVY;
		xhairw.y = xhaire.y = vSel + HSVY;
		xhairw.x = (hueSel - (2 + XHAIRH)) + HSVX;
		xhaire.x = hueSel + 2 + HSVX;
		SDL_RenderFillRects(rend, xhair, 4);

		FGCOL;
		SDL_RenderDrawRect(rend, &previewRect);
		SDL_RenderDrawRects(rend, xhair, 4);
		SDL_RenderDrawRect(rend, &okButRect);
		SDL_RenderDrawRect(rend, &cancelButRect);
		drawText(rend, "OK", font, okButRect.x + LETWIDTH, okButRect.y + 1);
		drawText(rend, "Cancel", font, cancelButRect.x + LETWIDTH, cancelButRect.y + 1);
		DRAWSLIDER(satThumb, satString, "Saturation");
		DRAWSLIDER(aThumb, aString, "Alpha");
		drawText(rend, "Red:", font, 0, textinfoAnchor);
		drawText(rend, rString, font, 8 * LETWIDTH, textinfoAnchor);
		drawText(rend, "Green:", font, 0, textinfoAnchor + LETHEIGHT);
		drawText(rend, gString, font, 8 * LETWIDTH, textinfoAnchor + LETHEIGHT);
		drawText(rend, "Blue:", font, 0, textinfoAnchor + (2 * LETHEIGHT));
		drawText(rend, bString, font, 8 * LETWIDTH, textinfoAnchor + (2 * LETHEIGHT));
		drawText(rend, "Alpha:", font, 0, textinfoAnchor + (3 * LETHEIGHT));
		drawText(rend, aString, font, 8 * LETWIDTH, textinfoAnchor + (3 * LETHEIGHT));
		drawText(rend, "Hue:", font, 20 * LETWIDTH, textinfoAnchor);
		drawText(rend, hString, font, 32 * LETWIDTH, textinfoAnchor);
		drawText(rend, "Saturation:", font, 20 * LETWIDTH, textinfoAnchor + LETHEIGHT);
		drawText(rend, satString, font, 32 * LETWIDTH, textinfoAnchor + LETHEIGHT);
		drawText(rend, "Value:", font, 20 * LETWIDTH,
			 textinfoAnchor + LETHEIGHT + LETHEIGHT);
		drawText(rend, vString, font, 32 * LETWIDTH,
			 textinfoAnchor + LETHEIGHT + LETHEIGHT);
		SDL_RenderPresent(rend);

		SDL_Event event;
		while (SDL_PollEvent(&event)) {
			switch (event.type) {
			case SDL_QUIT:
				return SDL_FALSE;

			case SDL_MOUSEBUTTONDOWN:
				mx = event.button.x;
				my = event.button.y;
				if (INSIDE_RECT(hsvRect, mx, my))
					focusedItem = 1;
				else if (INSLIDERAREA(satThumb))
					focusedItem = 2;
				else if (INSLIDERAREA(aThumb))
					focusedItem = 3;
				else if (INSIDE_RECT(okButRect, mx, my))
					return SDL_TRUE;
				else if (INSIDE_RECT(cancelButRect, mx, my))
					return SDL_FALSE;
				break;

			case SDL_MOUSEBUTTONUP:
				mx = event.button.x;
				my = event.button.y;
				focusedItem = 0;
				break;

			case SDL_MOUSEMOTION:
				mx = event.motion.x;
				my = event.motion.y;
				break;
			}
		}

		if (focusedItem == 1) {
			/* Hue/Val area */
			if (mx < HSVX)
				hueSel = 0;
			else if (mx > HSVX + 360)
				hueSel = 360;
			else
				hueSel = mx - HSVX;
			if (my < HSVY)
				vSel = 0;
			else if (my > HSVY + 100)
				vSel = 100;
			else
				vSel = my - HSVY;
			snprintf(hString, 4, "%i", hueSel);
			snprintf(vString, 4, "%i", 100 - vSel);
			UPDATEHSV;
		} else if (focusedItem == 2) {
			/* Saturation slider */
			SETTHUMBX(satThumb);
			satSel = ((satThumb.x - SLIDEROFFSET) * 100) / SLIDERWIDTH;
			snprintf(satString, 4, "%i", satSel);
			UPDATEHSV;
		} else if (focusedItem == 3) {
			/* Alpha slider */
			SETTHUMBX(aThumb);
			ret->a = ((aThumb.x - SLIDEROFFSET) * COLORMAX) / SLIDERWIDTH;
			snprintf(aString, 11, fstring, ret->a, ret->a);
		}
	}
}
