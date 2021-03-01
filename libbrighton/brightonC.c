
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

#include <stdlib.h>
#include <stdio.h>
#include <strings.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

void *
brightonmalloc(size_t size)
{
	void *mem;

	/*
	 * Man, this is an ugly workaround. Somewhere in the app I am writing
	 * past the end of a memory block, leading to heap corruption and it is
	 * naturally causing segfaults in malloc(). This works around the issue
	 * until I can track it down.
	 *
	 * We could use calloc here and let -O3 remove the whole shim.
	 */
	if ((mem = malloc(size)) == NULL)
	{
		printf("NULL malloc, exiting\n");
		exit(-10);
	}
	bzero(mem, size);

	return(mem);
}

void
brightonfree(void *mem)
{
	if (mem)
		free(mem);
}

FILE *
brightonfopen(char *filename, char *permissions)
{
	return(fopen(filename, permissions));
}

char *
brightonfgets(char *line, int size, FILE *fd)
{
	return(fgets(line, size, fd));
}

int
brightonfclose(FILE *fd)
{
	/*
	 * This may or may not cause a fault at some point.
	 */
	return(fclose(fd));
}

#ifdef nothing
/*
 * This was really just to debug the heap corruption.
 */
FILE *
brightonfopen(char *filename, char *permissions)
{
	/*
	 * We only use fopen for reading work, it was easy but gave problems
	 */
	return((FILE *) open(filename, O_RDONLY));
}

char *
brightonfgets(char *line, int size, FILE *FD)
{
	int fd = (int) FD, count = 0;

	/*
	 * This is not going to be the fastest code, we can work on buffer
	 * optimisation later.
	 */
	while (count < size) {
		if (read(fd, &line[count], 1) < 0)
		{
			if (count == 0)
				return(NULL);
			line[count] = '\0';
			return(line);
		}

		if ((line[count] == '\n')
			|| (line[count] == '\r'))
		{
			line[count] = '\0';
			return(line);
		}

		count++;
	}

	return(line);
}

int
brightonfclose(FILE *fd)
{
	/*
	 * This may or may not cause a fault at some point.
	 */
	return(close((int) fd));
}
#endif
