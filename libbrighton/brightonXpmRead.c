
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

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "brightoninternals.h"

static int hex2num(char);
static int xpmchar2num(brightonBitmap *,char);
static int convertindex(brightonBitmap *, char *, int);
static int convertcolor(char *);

extern void brightonSprintColor(brightonWindow *, char *, int);

#define BUFSIZE 8192

#define CTAB_SIZE 2048

/*
 * In the next peice of code we are calling brightonfopen(), brightonfgets()
 * and brightonfclose(), these are empty stubs that would get removed at
 * -O3 or so however I have used them at times for debugging heap corruption
 * and the overhead is minimal and only affects startup, not runtime.
 */
brightonBitmap *
xpmread(brightonWindow *bwin, char *filename)
{
	int color, width = 0, height = 0, colors = 0, bpcolor = 0, i = 1, j;
	int innerstatic = -1, outerstatic = -1;
	FILE *fd;
	char line[BUFSIZE];
	int *colormap;
	brightonBitmap *bitmap;

	/*
	 * This code was added to allow using compressed images reducing the size
	 * of the installation to about 1/5th. It spools the gz to /tmp if the xpm
	 * is not found and gunzips.
	 */
	if ((fd = brightonfopen(filename, "r")) == (FILE *) NULL)
	{
		int status;
		char tfn[256];;
		pid_t child;
		char *params[10];

		sprintf(line, "%s.gz", filename);
		sprintf(tfn, "/tmp/bbm_%i.xpm.gz", getpid());

		if ((fd = brightonfopen(line, "r")) == (FILE *) NULL)
			return(NULL);

		brightonfclose(fd);

		if ((child = fork()) == 0)
		{
			params[0] = "cp";
			params[1] = "-f";
			params[2] = line;
			params[3] = tfn;
			params[4] = NULL;
			if ((child = execvp("cp", params)) < 0) {
				printf("\nCannot spool the compressed source bitmap\n\n");
			}
			exit(1);
		} else
			waitpid(child, &status, 0);

		if (status) {
			printf("Error copying %s to %s: %i\n", line, tfn, status);
			exit(1);
		}

		if ((child = fork()) == 0)
		{
			params[0] = "gunzip";
			params[1] = "-f";
			params[2] = tfn;
			params[3] = NULL;
			if ((child = execvp("gunzip", params)) < 0) {
				printf("\nCannot finding the gunzip binary, install it or ");
				printf("email author for an\nalternative resolution\n\n");
			}
			exit(1);
		} else
			waitpid(child, &status, 0);

		if (status)
			exit(1);

		unlink(tfn); /* unlink the zipped file - on error may exist */
		sprintf(tfn, "/tmp/bbm_%i.xpm", getpid());

		if ((fd = brightonfopen(tfn, "r")) == (FILE *) NULL)
		{
			unlink(tfn);
			return(NULL);
		}

		unlink(tfn);
	}

	/* printf("xpmread(\"%s\")\n", filename); */

	while ((brightonfgets(line, BUFSIZE, fd)) != 0)
	{
		/*
		 * We are looking for numbers:
		 */
		if (!isdigit(line[1]))
			continue;
		else {
			/*
			 * Should be able to get width, height, colors and planes from this
			 * line of text.
			 */
			while (isdigit(line[i]))
				width = width * 10 + line[i++] - '0';
			if (line[i++] != ' ')
			{
				brightonfclose(fd);
				return(0);
			}

			while (isdigit(line[i]))
				height = height * 10 + line[i++] - '0';
			if (line[i++] != ' ')
			{
				brightonfclose(fd);
				return(0);
			}

			while (isdigit(line[i]))
				colors = colors * 10 + line[i++] - '0';
			if (line[i++] != ' ')
			{
				brightonfclose(fd);
				return(0);
			}

			while (isdigit(line[i]))
				bpcolor = bpcolor * 10 + line[i++] - '0';

			if (line[i] != '"')
			{
				while (line[i] == ' ')
					i++;
				innerstatic = 0;

				while (isdigit(line[i]))
					innerstatic = innerstatic * 10 + line[i++] - '0';

				if (line[i] != '"')
				{
					while (line[i] == ' ')
						i++;
					outerstatic = 0;

					while (isdigit(line[i]))
						outerstatic = outerstatic * 10 + line[i++] - '0';
				}

				if (line[i] != '"')
				{
					brightonfclose(fd);
					return(0);
				}
			}

			break;
		}
	}

	bitmap = brightonCreateBitmap(bwin, width, height);

	if (bitmap->pixels == NULL)
		bitmap->pixels = (int *) brightonmalloc((2 + width) * (2 + height)
			* sizeof(int));
	bitmap->width = width;
	bitmap->height = height;
	bitmap->ncolors = colors;
	bitmap->ctabsize = colors;
	bitmap->uses = 0;

	bitmap->istatic = innerstatic;
	bitmap->ostatic = width > height? height / 2: width /2;
	if (outerstatic != -1)
	{
		if (width > height) {
			if (outerstatic < (height / 2))
				bitmap->ostatic = outerstatic;
		} else {
			if (outerstatic < (width / 2))
				bitmap->ostatic = outerstatic;
		}
	}

	colormap = (int *) brightonmalloc(colors * sizeof(int));
	if (bitmap->colormap)
		brightonfree(bitmap->colormap);
	bitmap->colormap = colormap;

	/*
	 * We now have some reasonable w, h, c and p. Need to parse c color lines.
	 * We have to build a table of the character indeces from the XPM file. To
	 * do this we need a character table and index. Rather than define them in
	 * this file I am going to re-use some parts of the bitmap structure.
	 */
	bitmap->name = brightonmalloc(CTAB_SIZE * sizeof(char));

	for (i = 0; i < colors;i++)
	{
		short r, g, b;

		if (brightonfgets(line, BUFSIZE, fd) == 0)
		{
			brightonfclose(fd);
			printf("e1: %s\n", filename);
			return(bitmap);
		}

		if (((line[bpcolor + 1] != '\t') && (line[bpcolor + 1] != ' ')) ||
			((line[bpcolor + 2] != 'c') && (line[bpcolor + 2] != 'g')))
		{
			brightonfclose(fd);
			printf("e2: %s: %i %i, %s\n", filename, bpcolor, colors, line);
			return(bitmap);
		}

		if (strncmp("None", &line[4 + bpcolor], 4) == 0)
		{
			color = convertindex(bitmap, &line[1], bpcolor);
			colormap[i] = brightonGetGCByName(bwin, "Blue");
			continue;
		}

		/*
		 * We need to make a new convertindex that builds its own index table
		 * based on the characters in the xpm file.
		 */
		color = convertindex(bitmap, &line[1], bpcolor);

		if ((color = convertcolor(&line[bpcolor + 4])) < 0)
		{
			line[strlen(line) - 3] = '\0';
			colormap[i] = brightonGetGCByName(bwin, &line[bpcolor + 4]);
		} else {

			/*
			 * Call the color manager to get a GC with this color for eventual
			 * rendering.
			 *
			 * XPM has color from 0 to 255. getGC wants 16 bit values.
			 */
			r = (color >> 16) * 256;
			g = ((color >> 8) & 0xff) * 256;
			b = (color & 0x00ff) * 256;

			/*
			 * Brighton color manager is going to find this color, and return
			 * it if found, return a new GC if there is space, or a best match
			 * if not.
			 */
			colormap[i] = brightonGetGC(bwin, r, g, b);
		}
	}

	for (i = 0; i < height;i++)
	{
		if (brightonfgets(line, BUFSIZE, fd) == 0)
		{
			brightonfclose(fd);
			printf("e3: %s\n", filename);
			return(bitmap);
		}
		if (line[0] != '\"')
			continue;
		for (j = 0; j < width * bpcolor; j+=bpcolor)
		{
			color = convertindex(bitmap, &line[j + 1], bpcolor);
			if (color < 0)
			{
				brightonfclose(fd);
				printf("e4: %s, %i/%i\n", filename, color, colors);
				return(bitmap);
			} else if (color >= colors) {
//				printf("e5: %s, %i/%i\n", filename, color, colors);
				bitmap->pixels[i * width + j / bpcolor] = colormap[colors - 1];
			} else {
				bitmap->pixels[i * width + j / bpcolor] = colormap[color];
			}
		}
	}

	/* Free this temporary holder */
	brightonfree(bitmap->name);

	bitmap->name = (char *) brightonmalloc(strlen(filename) + 1);
	sprintf(bitmap->name, "%s", filename);

/*	bitmap->colormap = colormap; */

	bitmap->uses = 1;

	brightonfclose(fd);

	return(bitmap);
}

static int
hex2num(char c)
{
	if (isxdigit(c))
	{
		switch (c) {
			case '0':
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
			case '8':
			case '9':
				return(c - '0');
			case 'a':
			case 'b':
			case 'c':
			case 'd':
			case 'e':
			case 'f':
				return(c - 'a' + 10);
			default:
				return(c - 'A' + 10);
		}
	}
	return(-1);
}

static int
convertcolor(char *line)
{
	int i = 0, color = 0, digit;

	if (strcmp(line, "none") == 0)
		return(0);
	else if (strcmp(line, "None") == 0)
		return(0);

	if (line[0] != '#')
		return(-1);

	line++;

	while (line[i] != '"')
	{
		if ((digit = hex2num(line[i])) < 0)
			return(0);

		color = color * 16 + digit;
		i++;
	}

	return(color);
}

static int
convertindex(brightonBitmap *bitmap, char *line, int bpc)
{
	int cindex = 0, i = 0, j = 1;

	while (i < bpc)
	{
		cindex = cindex + xpmchar2num(bitmap, line[i]) * j;
		j += 91;
		i++;
	}

	return(cindex);
}

int
xpmchar2num(brightonBitmap *bitmap, char c)
{
	int i;

	for (i = 0; i < bitmap->uses; i++)
		if (c == bitmap->name[i])
			return(i);

/*printf("", bitmap->uses, i); */
	/*
	 * If we got here then the character has not been assigned yet.
	 */
	bitmap->name[bitmap->uses] = (int) c;
	bitmap->uses++;

	return(i);
}

int
writeLine(int fd, char *line)
{
	return(write(fd, line, strlen(line)));
}

#define SAVE_IMAGE
#ifdef SAVE_IMAGE

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

int
brightonXpmWrite(brightonWindow *bwin, char *file)
{
	int fd, x, y, color, lindex;
	int colors[4096], cindex = 0, ccnt = 0, coff = 0, cagg = 0;
	long *points;
	char *line;
	char cstring[16], filename[64];

	sprintf(filename, "/tmp/%s.xpm", bwin->template->name);

	if ((fd = open(filename, O_WRONLY|O_CREAT|O_TRUNC, 0770)) < 0)
		return(0);

	points = brightonmalloc(sizeof(long) * bwin->width * bwin->height);
	line = brightonmalloc(sizeof(char) * (bwin->width + 10) * 2);

	writeLine(fd, "/* XPM */\n");
	writeLine(fd, "static char * brighton_xpm[] = {\n");

	/*
	 * We need to scan the image to see how many colors it implements. From this
	 * we should build a character table, and then we should scan through the
	 * image printing these characters.
	 */
	for (y = 0; y < bwin->render->height; y++)
	{
		for (x = 0; x < bwin->render->width; x++)
		{
			color = bwin->render->pixels[x + y * bwin->render->width];

			for (cindex = 0; cindex < ccnt; cindex++)
			{
				if (color == colors[cindex])
				{
					/*
					 * This can't work: points[x][y] = cindex; hence
					 */
					points[x + y * bwin->render->width] = cindex;
					break;
				}
			}
			if (cindex == ccnt)
			{
				colors[cindex] = color;
				ccnt++;
			}
			/*
			 * This can't work: points[x][y] = cindex; hence
			 */
			points[x + y * bwin->render->width] = cindex;
		}
	}

	sprintf(line, "\"%i %i %i %i\",\n", bwin->width, bwin->height, ccnt, 2);
	writeLine(fd, line);

	for (cindex = 0; cindex < ccnt; cindex++)
	{
		/*
		 * We have to be reasonably intelligent with the color indeces. The
		 * first attempt failed when we went over about 80 colors. The index
		 * needs to become a string. We could go for two digits immediately?
		 */
		brightonSprintColor(bwin, cstring, colors[cindex]);
		sprintf(line, "\"%c%c	c %s\",\n", coff + 35, cagg + 35, cstring);

		if (++cagg >= 90)
		{
			cagg = 0;
			coff++;
		}

		writeLine(fd, line);
	}

	for (y = 0; y < bwin->height; y++)
	{
		lindex = 1;
		sprintf(line, "\"");

		for (x = 0; x < bwin->width; x++)
		{
			cagg = points[x + y * bwin->render->width] % 90;
			coff = points[x + y * bwin->render->width] / 90;
			sprintf(&line[lindex], "%c%c", coff + 35, cagg + 35);
			lindex+=2;
		}

		sprintf(&line[lindex], "\"\n");
		writeLine(fd, line);
	}

	writeLine(fd, "};\n\n");

	brightonfree(points);
	brightonfree(line);

	close(fd);

	printf("Image written to %s, %i colors\n", filename, ccnt);
	printf("Width %i, Height %i\n", bwin->width, bwin->height);

	return(0);
}
#endif /* SAVE_IMAGE */

