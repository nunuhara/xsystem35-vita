/*
 * s39ain.c  System39.ain read
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
/* $Id: s39ain.c,v 1.9 2003/07/21 23:06:47 chikama Exp $ */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "portab.h"
#include "system.h"
#include "LittleEndian.h"
#include "nact.h"
#include "s39ain.h"
#include "xsystem35.h"

#ifdef ENABLE_MODULES
#include "modules.h"
#endif

/* short cut */
#define dll  nact->ain.dll
#define msg  nact->ain.msg
#define fnc  nact->ain.fnc
#define path_to_ain nact->files.ain
#define dllnum nact->ain.dllnum
#define fncnum nact->ain.fncnum
#define varnum nact->ain.varnum
#define msgnum nact->ain.msgnum

/*
  system39.ain の読み込み
*/
int s39ain_init(void) {
	FILE *fp;
	long len;
	char *buf;
	unsigned char *p;
	int i, errors = 0;
	
	if (path_to_ain == NULL) {
		return NG;
	}
	
	if (NULL == (fp =  fopen(PATH(path_to_ain), "rb"))) {
		WARNING("fail to open %s\n", path_to_ain);
		return NG;
	}
	
	fseek(fp, 0, SEEK_END);
	len = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	buf = malloc(len + 4); /* +4 : VARI/MSGI... 拡張のため */
	fread(buf, 1, len, fp);
	fclose(fp);
	
	p = buf;
	/* first check */
	if (0 != strncmp(p, "AINI", 4)) {
		WARNING("%s is not ain file\n", path_to_ain);
		free(buf);
		return NG;
	}
	
	/* decode .ain file (thanx to Tajiri) */
	i = len -4;
	p = buf +4;
	for (; i > 0; i--) {
		unsigned char b = (*p) >> 6;
		unsigned char c = (*p) << 2;
		*p = b | c;
		p++;
	}
	
	p = buf +8;
	if (0 != strncmp(p, "HEL0", 4)) {
		WARNING("%s is illigal ain file\n", path_to_ain);
		free(buf);
		return NG;
	}
	p += 8;
	dllnum = LittleEndian_getDW(p, 0);
	dll = malloc(sizeof(S39AIN_DLLINF) * dllnum);
	
	p += 4;
	for (i = 0; i < dllnum; i++) {
		int fn, j;
		
		dll[i].name = strdup(p); /* DLL name */
		p += strlen(p) + 1;
		
		fn = LittleEndian_getDW(p, 0); /* number of function in DLL */
		p += 4;
		
		dll[i].function_num = fn;
		dll[i].function = malloc(sizeof(S39AIN_DLLFN) * fn);
		for (j = 0; j < fn; j++) {
			int argc, k;
			
			dll[i].function[j].name = strdup(p); /* function name */
			p += strlen(p) + 1;
			
			argc = LittleEndian_getDW(p, 0); /* number of argument */
			p += 4;
			
			dll[i].function[j].argc = argc;
			dll[i].function[j].argv = malloc(sizeof(int) * argc);
			for (k = 0; k < argc; k++) {
				dll[i].function[j].argv[k] = LittleEndian_getDW(p, 0);
				p += 4;
			}
			dll[i].function[j].entrypoint = NULL;
		}
	}
	
	/* check FUNC */
	if (0 == strncmp(p, "FUNC", 4)) {
		fncnum = LittleEndian_getDW(p, 8);
		
		fnc = malloc(sizeof(S39AIN_FUNCNAME) * fncnum);
		p += 12;
		
		for (i = 0; i < fncnum; i++) {
			fnc[i].name = strdup(p);
			p += strlen(p) + 1;
			fnc[i].page  = LittleEndian_getW(p, 0);
			fnc[i].index = LittleEndian_getDW(p, 2);
			p += 6;
		}
	}
	
	/* check VARI */
	if (0 == strncmp(p, "VARI", 4)) {
		varnum = LittleEndian_getDW(p, 8);
		p += 12;
		for (i = 0; i < varnum; i++) {
			p += strlen(p) + 1;
		}
	}
	
	/* check MSGI */
	if (0 == strncmp(p, "MSGI", 4)) {
		msgnum = LittleEndian_getDW(p, 8);
		
		msg = malloc(sizeof(char *) * msgnum);
		p += 12;
		
		for (i = 0; i < msgnum; i++) {
			msg[i] = strdup(p);
			p += strlen(p) + 1;
		}
	}

	free(buf);

#ifdef ENABLE_MODULES
	for (i = 0; i < dllnum; i++)
		resolve_module(&dll[i]);
#endif
	return OK;
}
