#include "config.h"

#include <limits.h>
#include <math.h>
#include <SDL.h>
#include <SDL_ttf.h>

#include "portab.h"
#include "system.h"
#include "font.h"
#include "sdl_private.h"
#include "utfsjis.h"

// defined in sdl_draw.c
int sdl_nearest_color(int r, int g, int b);

typedef struct {
	int      size;
	int      type;
	TTF_Font *id;
} FontTable;

#define FONTTABLEMAX 256
static FontTable fonttbl[FONTTABLEMAX];
static int       fontcnt = 0;

static TTF_Font *fontset;

static FONT *this;

static void font_insert(int size, int type, TTF_Font *fontset) {
	fonttbl[fontcnt].size = size;
	fonttbl[fontcnt].type = type;
	fonttbl[fontcnt].id   = fontset;
	
	if (fontcnt >= (FONTTABLEMAX -1)) {
		WARNING("Font table is full.\n");
	} else {
		fontcnt++;
	}
}

static FontTable *font_lookup(int size, int type) {
	int i;
	
	for (i = 0; i < fontcnt; i++) {
		if (fonttbl[i].size == size && fonttbl[i].type == type) { 
			return &fonttbl[i];
		}
	}
	return NULL;
}

static void font_sdlttf_sel_font(int type, int size) {
	FontTable *tbl;


	if (NULL == (tbl = font_lookup(size, type))) {
		TTF_Font *fs;
		
		fs = TTF_OpenFontIndex(PATH(this->name[type]), size, this->face[type]);
		
		if (fs == NULL) {
			WARNING("%s is not found:\n", PATH(this->name[type]));
			return;
		}
		
		font_insert(size, type, fs);
		fontset = fs;
	} else {
		fontset = tbl->id;
	}
}

static void *font_sdlttf_get_glyph(unsigned char *msg) {
	return NULL;
}

// SDL can't blit ARGB to an indexed bitmap properly, so we do it ourselves.
static void sdl_drawAntiAlias_8bpp(int dstx, int dsty, SDL_Surface *src, unsigned long col)
{
	SDL_LockSurface(sdl_dib);

	Uint8 cache[256*7];
	memset(cache, 0, 256);

	for (int y = 0; y < src->h; y++) {
		BYTE *sp = (BYTE*)src->pixels + y * src->pitch;
		BYTE *dp = (BYTE*)sdl_dib->pixels + (dsty + y) * sdl_dib->pitch + dstx;
		for (int x = 0; x < src->w; x++) {
			Uint8 r, g, b, a, alpha, max_alpha;
			SDL_GetRGBA(*((Uint32*)sp), src->format, &r, &g, &b, &a);
			// reduce depth of alpha to 3 bits
			max_alpha = src->format->Amask >> src->format->Ashift;
			alpha = round(7.0 * ((float)a / (float)max_alpha));
			if (!alpha) {
				// Transparent, do nothing
			} else if (alpha == 7) {
				*dp = col; // Fully opaque
			} else if (cache[*dp] & 1 << alpha) {
				*dp = cache[alpha << 8 | *dp]; // use cached value
			} else {
				// find nearest color in palette
				cache[*dp] |= 1 << alpha;
				int c = sdl_nearest_color(
					(sdl_col[col].r * alpha + sdl_col[*dp].r * (7 - alpha)) / 7,
					(sdl_col[col].g * alpha + sdl_col[*dp].g * (7 - alpha)) / 7,
					(sdl_col[col].b * alpha + sdl_col[*dp].b * (7 - alpha)) / 7);
				cache[alpha << 8 | *dp] = c;
				*dp = c;
			}
			sp += src->format->BitsPerPixel / 8;
			dp++;
		}
	}

	SDL_UnlockSurface(sdl_dib);
}

static int font_sdlttf_draw_glyph(int x, int y, unsigned char *str, int cl) {
	SDL_Surface *fs;
	SDL_Rect r_src, r_dst;
	int w, h;
	BYTE *conv;
	
	if (!*str)
		return 0;
	
	conv = sjis2lang(str);
	if (this->antialiase_on) {
		fs = TTF_RenderUTF8_Blended(fontset, conv, sdl_col[cl]);
	} else {
		fs = TTF_RenderText_Solid(fontset, conv, sdl_col[cl]);
	}
	if (!fs) {
		WARNING("Text rendering failed: %s\n", TTF_GetError());
		return 0;
	}
	
	TTF_SizeText(fontset, str, &w, &h);
	
	setRect(r_src, 0, 0, w, h);
	setRect(r_dst, x, y, w, h);
	
	if (sdl_dib->format->BitsPerPixel == 8 && this->antialiase_on) {
		sdl_drawAntiAlias_8bpp(x, y, fs, cl);
	} else {
		SDL_BlitSurface(fs, &r_src, sdl_dib, &r_dst);
	}

	SDL_FreeSurface(fs);
	
	return w;
}

static boolean drawable() {
	return TRUE;
}

FONT *font_sdlttf_new() {
	FONT *f = malloc(sizeof(FONT));
	
	f->sel_font   = font_sdlttf_sel_font;
	f->get_glyph  = font_sdlttf_get_glyph;
	f->draw_glyph = font_sdlttf_draw_glyph;
	f->self_drawable = drawable;
	f->antialiase_on = FALSE;
	
	if (TTF_Init() == -1)
		SYSERROR("Failed to intialize SDL_ttf: %s\n", TTF_GetError());
	
	this = f;
	
	NOTICE("FontDevice SDL_ttf\n");
	
	return f;
}
