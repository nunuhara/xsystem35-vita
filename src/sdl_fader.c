/*
 * Copyright (C) 2021 <KichikuouChrome@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
*/

#include "config.h"

#include <assert.h>
#include <math.h>
#include <SDL.h>

#include "system.h"
#include "sdl_core.h"
#include "sdl_private.h"

#define HAS_SDL_RenderGeometry SDL_VERSION_ATLEAST(2, 0, 18)

enum sdl_effect from_nact_effect(enum nact_effect effect) {
	switch (effect) {
	case NACT_EFFECT_CROSSFADE:        return EFFECT_CROSSFADE;
	case NACT_EFFECT_PENTAGRAM_IN_OUT: return EFFECT_PENTAGRAM_IN_OUT;
	case NACT_EFFECT_PENTAGRAM_OUT_IN: return EFFECT_PENTAGRAM_OUT_IN;
	case NACT_EFFECT_HEXAGRAM_IN_OUT:  return EFFECT_HEXAGRAM_IN_OUT;
	case NACT_EFFECT_HEXAGRAM_OUT_IN:  return EFFECT_HEXAGRAM_OUT_IN;
	case NACT_EFFECT_WINDMILL:         return EFFECT_WINDMILL;
	case NACT_EFFECT_WINDMILL_180:     return EFFECT_WINDMILL_180;
	case NACT_EFFECT_WINDMILL_360:     return EFFECT_WINDMILL_360;
	default:                           return EFFECT_INVALID;
	}
}

enum sdl_effect from_sact_effect(enum sact_effect effect) {
	switch (effect) {
	case SACT_EFFECT_PENTAGRAM_IN_OUT: return EFFECT_PENTAGRAM_IN_OUT;
	case SACT_EFFECT_PENTAGRAM_OUT_IN: return EFFECT_PENTAGRAM_OUT_IN;
	case SACT_EFFECT_HEXAGRAM_IN_OUT:  return EFFECT_HEXAGRAM_IN_OUT;
	case SACT_EFFECT_HEXAGRAM_OUT_IN:  return EFFECT_HEXAGRAM_OUT_IN;
	case SACT_EFFECT_ROTATE_OUT:       return EFFECT_ROTATE_OUT;
	case SACT_EFFECT_ROTATE_IN:        return EFFECT_ROTATE_IN;
	case SACT_EFFECT_ROTATE_OUT_CW:    return EFFECT_ROTATE_OUT_CW;
	case SACT_EFFECT_ROTATE_IN_CW:     return EFFECT_ROTATE_IN_CW;
	default:                           return EFFECT_INVALID;
	}
}

struct sdl_fader {
	void (*step)(struct sdl_fader *this, int step);
	void (*finish)(struct sdl_fader *this);
	enum sdl_effect effect;
	SDL_Rect dst_rect;
	SDL_Texture *tx_old, *tx_new;
};

static void fader_init(struct sdl_fader *fader, int sx, int sy, int w, int h, int dx, int dy, enum sdl_effect effect) {
	fader->effect = effect;
	fader->dst_rect.x = dx;
	fader->dst_rect.y = dy;
	fader->dst_rect.w = w;
	fader->dst_rect.h = h;

	SDL_Surface *sf_old = SDL_CreateRGBSurfaceWithFormat(0, w, h, 32, SDL_PIXELFORMAT_RGB888);
	SDL_Surface *sf_new = SDL_CreateRGBSurfaceWithFormat(0, w, h, 32, SDL_PIXELFORMAT_RGB888);

	SDL_Rect src_rect = { sx, sy, w, h };
	SDL_BlitSurface(sdl_display, &fader->dst_rect, sf_old, NULL);
	SDL_BlitSurface(sdl_dib, &src_rect, sf_new, NULL);

	fader->tx_old = SDL_CreateTextureFromSurface(sdl_renderer, sf_old);
	fader->tx_new = SDL_CreateTextureFromSurface(sdl_renderer, sf_new);

	SDL_FreeSurface(sf_old);
	SDL_FreeSurface(sf_new);

	sdl_copyArea(sx, sy, w, h, dx, dy);
}

static void fader_finish(struct sdl_fader *fader) {
	SDL_RenderCopy(sdl_renderer, fader->tx_new, NULL, &fader->dst_rect);
	SDL_RenderPresent(sdl_renderer);

	SDL_DestroyTexture(fader->tx_old);
	SDL_DestroyTexture(fader->tx_new);
}

// EFFECT_CROSSFADE

static void crossfade_step(struct sdl_fader *fader, int step);
static void crossfade_free(struct sdl_fader *fader);

static struct sdl_fader *crossfade_new(int sx, int sy, int w, int h, int dx, int dy) {
	struct sdl_fader *fader = calloc(1, sizeof(struct sdl_fader));
	if (!fader)
		NOMEMERR();
	fader_init(fader, sx, sy, w, h, dx, dy, EFFECT_CROSSFADE);
	fader->step = crossfade_step;
	fader->finish = crossfade_free;
	return fader;
}

static void crossfade_step(struct sdl_fader *fader, int step) {
	SDL_RenderCopy(sdl_renderer, fader->tx_old, NULL, &fader->dst_rect);
	SDL_SetTextureBlendMode(fader->tx_new, SDL_BLENDMODE_BLEND);
	SDL_SetTextureAlphaMod(fader->tx_new, step * 255 / SDL_FADER_MAXSTEP);
	SDL_RenderCopy(sdl_renderer, fader->tx_new, NULL, &fader->dst_rect);
	SDL_SetTextureBlendMode(fader->tx_new, SDL_BLENDMODE_NONE);
	SDL_RenderPresent(sdl_renderer);
}

static void crossfade_free(struct sdl_fader *fader) {
	fader_finish(fader);
	free(fader);
}

// EFFECT_PENTAGRAM_*, EFFECT_HEXAGRAM_*, EFFECT_WINDMILL*

struct polygon_mask_fader {
	struct sdl_fader f;
	SDL_Texture *tx_tmp;
};

static void polygon_mask_step(struct sdl_fader *fader, int step);
static void polygon_mask_free(struct sdl_fader *fader);

static struct sdl_fader *polygon_mask_new(int sx, int sy, int w, int h, int dx, int dy, enum sdl_effect effect) {
#if HAS_SDL_RenderGeometry
	if (SDL_RenderTargetSupported(sdl_renderer)) {
		struct polygon_mask_fader *pmf = calloc(1, sizeof(struct polygon_mask_fader));
		if (!pmf)
			NOMEMERR();
		fader_init(&pmf->f, sx, sy, w, h, dx, dy, effect);
		pmf->tx_tmp = SDL_CreateTexture(sdl_renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_TARGET, w, h);
		pmf->f.step = polygon_mask_step;
		pmf->f.finish = polygon_mask_free;
		return &pmf->f;
	}
#endif // HAS_SDL_RenderGeometry

	WARNING("Effect %d is not supported in this system. Falling back to crossfade.\n", effect);
	return crossfade_new(sx, sy, w, h, dx, dy);
}

#if HAS_SDL_RenderGeometry

static void draw_pentagram(int center_x, int center_y, double radius, double rotate) {
	const SDL_FPoint p[10] = {
		{ 1.5,     0     },
		{ 0.4635,  0.3368},
		{ 0.4635,  1.4266},
		{-0.1771,  0.5449},
		{-1.2135,  0.8817},
		{-0.5729,  0     },
		{-1.2135, -0.8817},
		{-0.1771, -0.5449},
		{ 0.4635, -1.4266},
		{ 0.4635, -0.3368},
	};
	SDL_Vertex v[10];
	double sin_r = sin(rotate);
	double cos_r = cos(rotate);
	for (int i = 0; i < 10; i++) {
		v[i].position.x = radius * (p[i].x * cos_r - p[i].y * sin_r) + center_x;
		v[i].position.y = radius * (p[i].x * sin_r + p[i].y * cos_r) + center_y;
		v[i].color = (SDL_Color){0, 0, 0, 0};
	}
	const int indices[15] = {
		0, 3, 7,
		2, 5, 9,
		4, 7, 1,
		6, 9, 3,
		8, 1, 5
	};
	SDL_RenderGeometry(sdl_renderer, NULL, v, 10, indices, 15);
}

static void draw_hexagram(int center_x, int center_y, double radius, double rotate) {
	const SDL_FPoint p[6] = {
		{ 0,     -1.0},
		{-0.866,  0.5},
		{ 0.866,  0.5},

		{-0.866, -0.5},
		{ 0,      1.0},
		{ 0.866, -0.5}
	};
	SDL_Vertex v[6];
	double sin_r = sin(rotate);
	double cos_r = cos(rotate);
	for (int i = 0; i < 6; i++) {
		v[i].position.x = radius * (p[i].x * cos_r - p[i].y * sin_r) + center_x;
		v[i].position.y = radius * (p[i].x * sin_r + p[i].y * cos_r) + center_y;
		v[i].color = (SDL_Color){0, 0, 0, 0};
	}
	SDL_RenderGeometry(sdl_renderer, NULL, v, 6, NULL, 0);
}

static void draw_windmill_90(int w, int h, double theta) {
	double cx = w / 2.0;
	double cy = h / 2.0;
	double r = max(cx, cy) * sqrt(2);
	double r_sin = r * sin(theta);
	double r_cos = r * cos(theta);

	SDL_Vertex v[13] = {
		// center
		{.position = {cx,         cy        }},
		// top-right
		{.position = {cx,         cy - r    }},
		{.position = {cx + r,     cy - r    }},
		{.position = {cx + r_sin, cy - r_cos}},
		// bottom-right
		{.position = {cx + r,     cy        }},
		{.position = {cx + r,     cy + r    }},
		{.position = {cx + r_cos, cy + r_sin}},
		// bottlm-left
		{.position = {cx,         cy + r    }},
		{.position = {cx - r,     cy + r    }},
		{.position = {cx - r_sin, cy + r_cos}},
		// top-left
		{.position = {cx - r,     cy        }},
		{.position = {cx - r,     cy - r    }},
		{.position = {cx - r_cos, cy - r_sin}},
	};

	if (theta <= M_PI / 4) {
		const int indices[12] = {
			0, 1, 3,
			0, 4, 6,
			0, 7, 9,
			0, 10, 12
		};
		SDL_RenderGeometry(sdl_renderer, NULL, v, 13, indices, 12);
	} else {
		const int indices[24] = {
			0, 1, 2,
			0, 2, 3,
			0, 4, 5,
			0, 5, 6,
			0, 7, 8,
			0, 8, 9,
			0, 10, 11,
			0, 11, 12
		};
		SDL_RenderGeometry(sdl_renderer, NULL, v, 13, indices, 24);
	}
}

static void draw_windmill_180(int w, int h, double theta) {
	double cx = w / 2.0;
	double cy = h / 2.0;
	double r = max(cx, cy) * sqrt(2);
	double r_sin = r * sin(theta);
	double r_cos = r * cos(theta);

	SDL_Vertex v[11] = {
		{.position = {cx,         cy        }},  // center

		{.position = {cx - r,     cy - r    }},  // left-top
		{.position = {cx,         cy - r    }},  // top
		{.position = {cx + r,     cy - r    }},  // top-right
		{.position = {cx + r,     cy        }},  // right
		{.position = {cx - r_cos, cy - r_sin}},  // theta

		{.position = {cx + r,     cy + r    }},  // right-bottom
		{.position = {cx,         cy + r    }},  // bottom
		{.position = {cx - r,     cy + r    }},  // left-bottom
		{.position = {cx - r,     cy        }},  // left
		{.position = {cx + r_cos, cy + r_sin}},  // theta+M_PI
	};

	int indices[24];
	int i = 0;

	indices[i++] = 0;  // center
	indices[i++] = 9;  // left
	for (int j = 1; theta > M_PI / 4 * j; j++) {
		indices[i++] = j;
		indices[i++] = 0;  // center
		indices[i++] = j;
	}
	indices[i++] = 5;  // theta

	indices[i++] = 0;  // center
	indices[i++] = 4;  // right
	for (int j = 1; theta > M_PI / 4 * j; j++) {
		indices[i++] = j + 5;
		indices[i++] = 0;  // center
		indices[i++] = j + 5;
	}
	indices[i++] = 10;  // theta+M_PI

	SDL_RenderGeometry(sdl_renderer, NULL, v, 11, indices, i);
}

static void draw_windmill_360(int w, int h, double theta) {
	double cx = w / 2.0;
	double cy = h / 2.0;
	double r = max(cx, cy) * sqrt(2);
	double r_sin = r * sin(theta);
	double r_cos = r * cos(theta);

	SDL_Vertex v[10] = {
		{.position = {cx,         cy        }},  // center
		{.position = {cx - r,     cy - r    }},  // left-top
		{.position = {cx,         cy - r    }},  // top
		{.position = {cx + r,     cy - r    }},  // top-right
		{.position = {cx + r,     cy        }},  // right
		{.position = {cx + r,     cy + r    }},  // right-bottom
		{.position = {cx,         cy + r    }},  // bottom
		{.position = {cx - r,     cy + r    }},  // left-bottom
		{.position = {cx - r,     cy        }},  // left
		{.position = {cx - r_cos, cy - r_sin}},  // theta
	};

	int indices[24];
	int i = 0;

	indices[i++] = 0;  // center
	indices[i++] = 8;  // left
	for (int j = 1; theta > M_PI / 4 * j; j++) {
		indices[i++] = j;
		indices[i++] = 0;  // center
		indices[i++] = j;
	}
	indices[i++] = 9;  // theta
	SDL_RenderGeometry(sdl_renderer, NULL, v, 10, indices, i);
}

static void polygon_mask_step(struct sdl_fader *fader, int step) {
	struct polygon_mask_fader *this = (struct polygon_mask_fader *)fader;
	int w = fader->dst_rect.w;
	int h = fader->dst_rect.h;
	double t = (double)step / SDL_FADER_MAXSTEP;
	SDL_SetRenderTarget(sdl_renderer, this->tx_tmp);
	SDL_RenderCopy(sdl_renderer, fader->tx_old, NULL, NULL);
	switch (fader->effect) {
	case EFFECT_PENTAGRAM_IN_OUT:
		draw_pentagram(w / 2, h / 2, max(w, h) * t, M_PI * t);
		break;
	case EFFECT_PENTAGRAM_OUT_IN:
		draw_pentagram(w / 2, h / 2, max(w, h) * (1 - t), M_PI * t);
		break;
	case EFFECT_HEXAGRAM_IN_OUT:
		draw_hexagram(w / 2, h / 2, max(w, h) * t, M_PI * t);
		break;
	case EFFECT_HEXAGRAM_OUT_IN:
		draw_hexagram(w / 2, h / 2, max(w, h) * (1 - t), M_PI * t);
		break;
	case EFFECT_WINDMILL:
		draw_windmill_90(w, h, M_PI / 2 * t);
		break;
	case EFFECT_WINDMILL_180:
		draw_windmill_180(w, h, M_PI * t);
		break;
	case EFFECT_WINDMILL_360:
		draw_windmill_360(w, h, M_PI * 2 * t);
		break;
	default:
		assert(!"Cannot happen");
	}
	SDL_SetRenderTarget(sdl_renderer, NULL);
	SDL_RenderCopy(sdl_renderer, fader->tx_new, NULL, &fader->dst_rect);
	SDL_SetTextureBlendMode(this->tx_tmp, SDL_BLENDMODE_BLEND);
	SDL_RenderCopy(sdl_renderer, this->tx_tmp, NULL, &fader->dst_rect);
	SDL_RenderPresent(sdl_renderer);
}

static void polygon_mask_free(struct sdl_fader *fader) {
	struct polygon_mask_fader *this = (struct polygon_mask_fader *)fader;
	SDL_DestroyTexture(this->tx_tmp);
	fader_finish(&this->f);
	free(this);
}

#endif // HAS_SDL_RenderGeometry

// EFFECT_ROTATE_*

static void rotate_step(struct sdl_fader *fader, int step);
static void rotate_free(struct sdl_fader *fader);

static struct sdl_fader *rotate_new(int sx, int sy, int w, int h, int dx, int dy, enum sdl_effect effect) {
	struct sdl_fader *fader = calloc(1, sizeof(struct sdl_fader));
	if (!fader)
		NOMEMERR();
	fader_init(fader, sx, sy, w, h, dx, dy, effect);
	fader->step = rotate_step;
	fader->finish = rotate_free;
	return fader;
}

static void rotate_step(struct sdl_fader *fader, int step) {
	SDL_Texture *bg_texture, *fg_texture;
	double angle = step * 360.0 / SDL_FADER_MAXSTEP;
	double scale = step / (double)SDL_FADER_MAXSTEP;
	switch (fader->effect) {
	case EFFECT_ROTATE_OUT:
		scale = 1.0 - scale;
		bg_texture = fader->tx_new;
		fg_texture = fader->tx_old;
		angle *= -1.0;
		break;
	case EFFECT_ROTATE_IN:
		bg_texture = fader->tx_old;
		fg_texture = fader->tx_new;
		angle *= -1.0;
		break;
	case EFFECT_ROTATE_OUT_CW:
		scale = 1.0 - scale;
		bg_texture = fader->tx_new;
		fg_texture = fader->tx_old;
		break;
	case EFFECT_ROTATE_IN_CW:
		bg_texture = fader->tx_old;
		fg_texture = fader->tx_new;
		break;
	default:
		assert(!"Cannot happen");
	}
	SDL_Rect r = fader->dst_rect;
	r.x += (1.0 - scale) * r.w / 2;
	r.y += (1.0 - scale) * r.h / 2;
	r.w *= scale;
	r.h *= scale;
	SDL_RenderCopy(sdl_renderer, bg_texture, NULL, &fader->dst_rect);
	SDL_RenderCopyEx(sdl_renderer, fg_texture, NULL, &r, angle, NULL, SDL_FLIP_NONE);
	SDL_RenderPresent(sdl_renderer);
}

static void rotate_free(struct sdl_fader *fader) {
	fader_finish(fader);
	free(fader);
}


struct sdl_fader *sdl_fader_init(int sx, int sy, int w, int h, int dx, int dy, enum sdl_effect effect) {
	switch (effect) {
	case EFFECT_CROSSFADE:
		return crossfade_new(sx, sy, w, h, dx, dy);
	case EFFECT_PENTAGRAM_IN_OUT:
	case EFFECT_PENTAGRAM_OUT_IN:
	case EFFECT_HEXAGRAM_IN_OUT:
	case EFFECT_HEXAGRAM_OUT_IN:
	case EFFECT_WINDMILL:
	case EFFECT_WINDMILL_180:
	case EFFECT_WINDMILL_360:
		return polygon_mask_new(sx, sy, w, h, dx, dy, effect);
	case EFFECT_ROTATE_OUT:
	case EFFECT_ROTATE_IN:
	case EFFECT_ROTATE_OUT_CW:
	case EFFECT_ROTATE_IN_CW:
		return rotate_new(sx, sy, w, h, dx, dy, effect);
	default:
		WARNING("Unknown effect %d\n", effect);
		return crossfade_new(sx, sy, w, h, dx, dy);
	}
}

void sdl_fader_step(struct sdl_fader *fader, int step) {
	fader->step(fader, step);
}

void sdl_fader_finish(struct sdl_fader *fader) {
	fader->finish(fader);
}
