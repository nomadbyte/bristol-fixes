
/*
 *  Diverse Bristol audio routines.
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

#include <stdlib.h> /* only for init library */

#include "bristol.h"

void *
bristolmalloc(size)
{
	char *mem;

	mem = malloc(size);

#ifdef DEBUG
	printf("bristolmalloc: %x, %i\n", mem, size);
#endif

	return(mem);
}

void *
bristolmalloc0(size)
{
	char *mem;

	mem = bristolmalloc(size);

#ifdef DEBUG
	printf("bristolmalloc0: %x, %i\n", mem, size);
#endif

	bzero(mem, size);

	return(mem);
}

void
bristolfree(void *mem)
{
#ifdef DEBUG
	printf("bristolfree: %x\n", mem);
#endif

	if (mem != NULL)
		free(mem);
#ifdef DEBUG
	else
		printf("attempt to free (null)\n");
#endif
}

void
bristolbzero(char *mem, int count)
{
#ifdef DEBUG
	printf("bristolzero: %x\n", mem);
#endif

	if (mem == NULL)
		return;

	bzero(mem, count);
}

