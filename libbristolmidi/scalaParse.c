
/*
 *  Diverse Bristol midi routines.
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
 * This will take a character filename and a float mapping, attempt to find the
 * specified file with a few different names/locations and then parse the
 * contents into the float array. The end result is a set of frequency tables
 * that can be converted separately into the midi frequency map.
 *
 * Bounds checking is done to limit the float map to 128 entries.
 *
 * We need to look for the full path name, then look for the filename in
 * our profiles directory, then look for the <filename>.scl in the same
 * profiles directory.
 *
 * The midihandlers.c code will parse emulation mappings and uses midiFileMgt.c
 * code here in the library.

! meanquar.scl
!
1/4-comma meantone scale. Pietro Aaron's temperament (1523)
 12
!
 76.04900
 193.15686
 310.26471
 5/4
 503.42157
 579.47057
 696.57843
 25/16
 889.73529
 1006.84314
 1082.89214
 2/1

 */

#include <string.h>
#include <stdio.h>
#include <ctype.h>

#include "bristolmidi.h"

#define BUFSIZE 256

static float
getScalaFreq(char *line)
{
	float freq;
	char *denominator;

	/* See if we have a fraction */
	if ((denominator = index(line, '/')) != NULL)
	{
		freq = (atoi(line));
		return(freq / atoi(++denominator));
	}

	/* See if we have a decimal and treat it then as cents */
	if (index(line, '.') != NULL)
		return(1.0 + atof(line) / 1200);

	/* Assume we have a raw number */
	return(atoi(line));
}

static char *
spanWhiteSpace(char *line)
{
	int i = 0;

	while (isspace(line[i]))
		i++;

	return(&line[i]);
}

static int
scalaParse(FILE *fd, float *freq)
{
	char line[BUFSIZE];
	char *scan;
	int lineid = 0; /* To decide what to parse */
	int freqindex = 0, count = 0;

    while ((fgets(line, BUFSIZE, fd)) != 0)
	{
		if (line[0] == '!')
			continue;

		switch (lineid++)
		{
			case 0:
				printf("Scale info: %s", line);
				lineid = 1;
				break;
			case 1:
				scan = spanWhiteSpace(line);
				if ((count = atoi(scan)) < 0)
					return(-1);
				if (count > 128)
				{
					printf("Scala: cannot converge %i notes\n", count);
					return(-2);
				}
				lineid = 2;
				break;
			default:
				/* The rest should be frequencies */
				scan = spanWhiteSpace(line);
				if ((freq[freqindex] = getScalaFreq(scan)) <= 0)
					continue;
				freqindex++;
				break;
		}
	}

	return(freqindex);
}

int
bristolParseScala(char *file, float *freq)
{
	FILE *fd;
	char *cache;
	int ncount;

	if ((cache = getBristolCache(file)) == NULL)
	{
		printf("Could not resolve cache\n");
		return(-10);
	}

	if (file[0] == '/')
	{
		if ((fd = fopen(file, "r")) == (FILE *) NULL)
		{
			printf("Could not find scala file\n");
			return(-1);
		}
	} else {
		char filename[1024];

		if (strlen(file) > 200)
		{
			printf("Will not open stupidly named file: %s\n", file);
			return(-2);
		}

		/*
		 * First look for the complete filename:
		 */
		sprintf(filename, "%s/memory/profiles/%s", cache, file);

		if ((fd = fopen(filename, "r")) == (FILE *) NULL)
		{
			sprintf(filename, "%s/memory/profiles/%s.scl", cache, file);

			if ((fd = fopen(filename, "r")) == (FILE *) NULL)
			{
				printf("Could not open named scala file %s\n", filename);
				return(-3);
			}
		}
	}

	if ((ncount = scalaParse(fd, freq)) < 0)
	{
		printf("Could not parse named scala file %s\n", file);
		fclose(fd);
		return(-4);
	}

	fclose(fd);

	printf("Converged %i notes from scala file %s\n", ncount, file);

	return(ncount);
}

