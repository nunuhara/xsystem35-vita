/*
 * gameresource.h  Game Resource (*.gr) file
 *
 * Copyright (C) 1997-1998 Masaki Chikama (Wren) <chikama@kasumi.ipl.mech.nagoya-u.ac.jp>
 *               1998-                           <masaki-c@is.aist-nara.ac.jp>
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

#ifndef __GAMERESOURCE_H__
#define __GAMERESOURCE_H__

#include "portab.h"
#include "ald_manager.h"
#include "savedata.h"

typedef struct {
	char *game_fname[DRIFILETYPEMAX][DRIFILEMAX];
	int cnt[DRIFILETYPEMAX];
	char *save_path;
	char *save_fname[SAVE_MAXNUMBER];
	char *ain;
	char *wai;
	char *bgi;
	char *sact01;
	char *init;
	char *alk[10];
} GameResource;

boolean initGameResource(GameResource *gr, const char *gr_fname);

#endif /* !__GAMERESOURCE_H__ */
