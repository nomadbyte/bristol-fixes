
/*
 *  Diverse SLab audio routines.
 *  Copyright (c) by Nick Copeland <nickycopeland@hotmail.com> 1996,2012
 *
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, see <http://www.gnu.org/licenses/>.
 *
 */


/*
 * These are just used to separate out the C code from the GUI (and potentially
 * other) libraries.
 */

#include <stdio.h>
#include <stdlib.h>

#define SLAB_STAT_TIME 0
#define SLAB_STAT_SIZE 1
#define SLAB_STAT_NAME 2

extern char *slabctime();
extern char *slabstrchr();
extern char *slabgetenv();

extern int slabsc();
extern int slabsnc();
extern char *slabscopy();
extern char *slabsncopy();
extern int slabslen();

extern int *slabprintf();
extern int *slabsprintf();

extern void *slabfdopen();
extern void *slabfopen();
extern int slabfclose();
extern char *slabfgets();

extern int *slabopendir();
extern char *slabreaddir();

extern char *slabuts();
extern char *slabstu();

extern int slabvfork();

extern int slabsqrt();

extern int errno;
#define EAGAIN 11

char *slabindex();
char *slabrindex();

#define SLAB_SIGHUP		1
#define SLAB_SIGINT		2
#define SLAB_SIGQUIT	3
#define SLAB_SIGFPE		8
#define SLAB_SIGKILL	9
#define SLAB_SIGSEGV	11
#define SLAB_SIGTERM	15
#define SLAB_SIGSTOP	17
#define SLAB_SIGCONT	18
#define SLAB_SIGCHLD	20
#define SLAB_SIGUSR1	30
#define SLAB_SIGUSR2	31

#define SLAB_SIG_IGN	-1

#define SLAB_WNOHANG	1

#ifndef _G_BUFSIZ
#define _G_BUFSIZ 8192
#endif
#ifndef _IO_BUFSIZ
#define _IO_BUFSIZ _G_BUFSIZ
#endif
#ifndef BUFSIZ
#define BUFSIZ _IO_BUFSIZ
#endif

#define FILE void

#define SLAB_OCREAT		0x100
#define SLAB_OTRUNC		0x200

#define SLAB_ORDWR		0x400
#define SLAB_ORDONLY	0x800
#define SLAB_OWRONLY	0x1000

#ifndef SLAB_SEEK_SET
#define SLAB_SEEK_SET	0
#define SLAB_SEEK_GET	1
#define SLAB_SEEK_CUR	2
#endif

#define SLAB_ENOENT	1
#define SLAB_EACCES	2
#define SLAB_ECHILD	3

