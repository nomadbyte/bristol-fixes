
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
#include <math.h>

#include "brightoninternals.h"
#include "brightonX11.h"

#define COLOR_START 8
#define SHIFT_BITS 12
/* This is silly, should be cleared out */
#define SCAN_R r
#define SCAN_G g
#define SCAN_B b

typedef struct CCEntry {
	short last, next;
	short p_index;
	unsigned short g, b;
} cc_entry;

typedef struct CCRow {
	unsigned short count;
	unsigned short start;
	cc_entry *entry;
} c_row;

struct {
	int redshift; /* defines cache tablesize */
	unsigned short mask; /* defines color matching mask, f(redshift) */
	struct {
		int hits;
		int miss_no_row;
		int miss_no_color;
		int miss_no_entry;
		int miss_no_green;
		int miss_no_blue;
		int miss_eol;
		int inserts;
		int missedinserts;
		int deletes;
		int deleted;
		int errors;
		int rowinserts;
		int rerows;
		float depth;
		float lsd;
	} stats;
	c_row *row;
} c_table;

static void
initColorCache(brightonWindow *bwin, int shift)
{
	int rcount = pow(2, shift), i;

	c_table.redshift = 16 - shift;
	c_table.mask = 0xffff << c_table.redshift;

	c_table.row = brightonmalloc(sizeof(c_row) * rcount);

	/*
	 * Initialise each row with COLOR_START colors. These may have to be
	 * rehashed later. There are arguments to make this 'none', in fact they
	 * are good enough to implement it now, ie, later.
	 */
	for (i = 0; i < rcount; i++)
		c_table.row[i].count = 0;
}

extern int xcolorcount;
#define ASD c_table.stats.depth = c_table.stats.depth * 0.99999f + d * 0.00001f; c_table.stats.lsd = d

void
printColorCacheStats(brightonWindow *bwin)
{
	int rcount = pow(2, 16 - c_table.redshift), i, j, occ, cum = 0;

	printf("\nBrighton Color Cache Stats:\n---------------------------\n\n");
	printf("quality:    %4i\n", 16 - c_table.redshift);
	printf("redshift:   %4i\n", c_table.redshift);
	printf("colormask:  %4x\n", c_table.mask);
	printf("bucketsize: %4i\n", COLOR_START);
	printf("redbuckets: %4i\n", rcount);
	printf("\n");
	printf("    hits:        %8i\n", c_table.stats.hits);
	printf("\n");

	printf("    miss row:    %8i    ", c_table.stats.miss_no_row);
	printf("    missed:      %8i\n", c_table.stats.missedinserts);
	printf("    miss line:   %8i    ", c_table.stats.miss_no_entry);
	printf("    deletes:     %8i\n", c_table.stats.deletes);
	printf("    miss EOL:    %8i    ", c_table.stats.miss_eol);
	printf("    deleted:     %8i\n", c_table.stats.deleted);
	printf("    miss green:  %8i    ", c_table.stats.miss_no_green);
	printf("    errors:      %8i\n", c_table.stats.errors);
	printf("    miss blue:   %8i    ", c_table.stats.miss_no_blue);
	printf("    new rows:    %8i\n", c_table.stats.rowinserts);
	printf("    miss color:  %8i    ", c_table.stats.miss_no_color);
	printf("    new buckets: %8i\n", c_table.stats.rerows);
	printf("    misses total:%8i    ",
		c_table.stats.miss_eol +
		c_table.stats.miss_no_row +
		c_table.stats.miss_no_color +
		c_table.stats.miss_no_entry +
		c_table.stats.miss_no_green +
		c_table.stats.miss_no_blue);
	printf("    inserts:     %8i\n", c_table.stats.inserts);
	printf("\n");
	printf("    ASD:         %8.1f    ", c_table.stats.depth);
	printf("    LSD:         %8.1f\n", c_table.stats.lsd);
	printf("\n");
	printf("Red bucket stats:\n");
	printf("----------------------------------");
	printf("----------------------------------\n");
	for (i = 0; i < (rcount>>1); i++)
	{
		occ = 0;
		if (c_table.row[i].count > 0)
		{
			for (j = c_table.row[i].start;
					j >= 0;
					j = c_table.row[i].entry[j].next)
				occ++;
		}
		printf("%3i: sz %5i, st %3i, occ %5i | ", i,
			c_table.row[i].count,
			c_table.row[i].start,
			occ);
		cum += occ;

		occ = 0;
		if (c_table.row[i+(rcount>>1)].count > 0)
		{
			for (j = c_table.row[i+(rcount>>1)].start;
					j >= 0;
					j = c_table.row[i+(rcount>>1)].entry[j].next)
				occ++;
		}
		printf("%3i: sz %5i, st %3i, occ %5i\n", i+(rcount>>1),
			c_table.row[i+(rcount>>1)].count,
			c_table.row[i+(rcount>>1)].start,
			occ);
		cum += occ;
	}
	printf("----------------------------------");
	printf("----------------------------------\n");

	/*
	 * Scan the table of colors to see how much of the palette has not been
	 * allocated by the library.
	 */
	occ = 0;
	for (i = 0; i < bwin->cmap_size; i++)
		if ((bwin->display->palette[i].uses > 0) &&
			((bwin->display->palette[i].gc == 0) &&
			 (bwin->display->palette[i].pixel < 0)))
			occ++;

	/*
	 * Color stats: the numbers typically appear to not add up, the cache has
	 * most, more than in the dumped XPM files and in the middle sits the 
	 * number allocated by the window system interface library (B11). The 
	 * reason is that the cache contains all the colors requested from all the
	 * bitmap files - not all colors may be rendered since the bitmaps are
	 * scaled/rotated, etc, and some pixels are missed out of the drawn image
	 * however are still in the cache if they are needed.
	 *
	 * The interface library has more than in the dumped image since the image
	 * changes due to shading and transparencies, so colors that may have been
	 * rendered in one image may not be in the next depending on location of
	 * the controllers and panels once drawn then withdrawn often have a
	 * lot of unseen controls. It will probably never have the same value as
	 * the cache since GC are scarce and are only requested in the interface
	 * library when they are actually required to be painted.
	 *
	 * So, the following figures should more or less add up. There is always a
	 * difference of '1' due to the use of 'Blue' as transparency, hence never
	 * rendered.
	 */
	printf("Total cache entries: %i, Window System %i, no GC (unused): %i\n",
		cum, xcolorcount, occ);

	printf("\n");
}

/*
 * When a row fills up then we have to rebuild it in a new longer cache line.
 */
static int
cacheReRow(unsigned short row)
{
	cc_entry *n_entry;
	int i, new = 0;

	n_entry = brightonmalloc(sizeof(cc_entry)
		* (c_table.row[row].count += COLOR_START));

	/*
	 * Initialise the new table
	 */
	for (i = 0; i < c_table.row[row].count; i++)
		n_entry[i].last = n_entry[i].next = n_entry[i].p_index = -1;

	/*
	 * Insert the first entry
	 */
	i = c_table.row[row].start;
	n_entry[new].g = c_table.row[row].entry[i].g;
	n_entry[new].b = c_table.row[row].entry[i].b;
	n_entry[new].p_index = c_table.row[row].entry[i].p_index;

	/*
	 * Move to the next entry and start the convergence.
	 */
	i = c_table.row[row].entry[i].next;

	for (; i >= 0; i = c_table.row[row].entry[i].next)
	{
		new++;
		/*
		 * Take this entry and converge it to the new list
		 */
		n_entry[new].g = c_table.row[row].entry[i].g;
		n_entry[new].b = c_table.row[row].entry[i].b;
		n_entry[new].p_index = c_table.row[row].entry[i].p_index;
		n_entry[new].last = new - 1;
		n_entry[new].next = -1;
		n_entry[new - 1].next = new;
	}

	brightonfree(c_table.row[row].entry);

	c_table.row[row].entry = n_entry;
	c_table.row[row].start = 0;

	c_table.stats.rerows++;

	return(0);
}

/*
 * There are a couple of things we have to watch out for. Firstly we have to
 * correctly reorder each red row when a color is inserted. Then if we have
 * reached the end of the table we need to insert a larger size for this red
 * hue and convert the existing one over. Hm.
 */
int
cacheInsertColor(unsigned short r, unsigned short g, unsigned short b,
unsigned short p_index)
{
	c_row *row;
	cc_entry *entry;
	int i, j, free;

	/*
	 * Get our red row. If it is new stuff in the values.
	 */
	row = &c_table.row[SCAN_R >> c_table.redshift];

	/*
	 * So we have a free entry. Now we need to scan to find the target point
	 * to insert the new color
	 *
	 * Make these values 'searchable':
	 */
	SCAN_G &= c_table.mask;
	SCAN_B &= c_table.mask;

	if ((entry = row->entry) <= 0)
	{
//printf("insert NEW: 0 (%i)\n", r>>c_table.redshift);
		/*
		 * Create the row if it has not been done, by implication this already
		 * means we should generate a new color, done in the calling party.
		 */
		row->count = COLOR_START;
		row->start = 0;

		c_table.row[SCAN_R >> c_table.redshift].entry =
			brightonmalloc(sizeof(cc_entry) * COLOR_START);

		for (i = 0; i < COLOR_START; i++)
			row->entry[i].last = row->entry[i].next = row->entry[i].p_index
				= -1;

		row = &c_table.row[SCAN_R >> c_table.redshift];

		row->entry[0].last = row->entry[0].next = -1;
		row->entry[0].p_index = p_index;
		row->entry[0].g = SCAN_G;
		row->entry[0].b = SCAN_B;

		c_table.stats.rowinserts++;
		c_table.stats.inserts++;

		return(0);
	}

	/*
	 * We need a free entry in this row to proceed
	 */
	for (free = 0; free < row->count; free++)
		if (row->entry[free].p_index < 0)
			break;

	/*
	 * If we passed the end of row then it is full. We need to rebuild the row
	 * but will work that later as it comes up. For now, return blue.
	 */
	if (free == row->count)
	{
		c_table.stats.missedinserts++;
	
		cacheReRow(r>>c_table.redshift);

		entry = row->entry;
	}

	entry[free].last = -1;
	entry[free].next = -1;
	entry[free].g = SCAN_G;
	entry[free].b = SCAN_B;
	entry[free].p_index = p_index;

	/*
	 * We first want to scan for green, that will be more efficient than 
	 * scanning that both green and blue match as this can terminate earlier.
	 *
	 * We have to start with the first entry since it may need extra processing.
	 */
	if ((entry[row->start].g > g) ||
		((entry[row->start].g == g) && (entry[row->start].b > b)))
	{
		/*
		 * If start is already greater then insert our new entry.
		 */
		entry[free].next = row->start;
		entry[free].last = -1;
		entry[row->start].last = free;
		row->start = free;

//printf("insert SOL: %i (%i)\n", free, r>>c_table.redshift);
		c_table.stats.inserts++;
		return(free);
	}

	j = row->start;
	while (entry[j].SCAN_G < SCAN_G)
	{
		/*
		 * If we have hit the end of the line and our new green is still
		 * bigger append the new entry and return.
		 */
		if (entry[j].next < 0)
		{
			entry[j].next = free;
			entry[free].last = j;
//printf("insert EOS: %i (%i)\n", free, r>>c_table.redshift);

			c_table.stats.inserts++;
			return(free);
		}

		j = entry[j].next;
	}

	/*
	 * At this point we have scanned to where the cached green is equal
	 * to the targetted value. Now scan for a matching blue until end of
	 * list or green no longer matches.
	 */
	for (; j >= 0; j = entry[j].next)
	{
		/*
		 * Check green still matches. If it doesn't then insert the new entry
		 * behind this one?
		 */
		if ((entry[j].SCAN_G != SCAN_G) ||
			(entry[j].SCAN_B > SCAN_B))
		{
			entry[free].next = j;
			entry[free].last = entry[j].last;
			entry[entry[j].last].next = free;
			entry[j].last = free;

//printf("insert INL: %i (%i) %i: %x %x/%x %x\n", free, r>>c_table.redshift,
//SCAN_R>>c_table.redshift, SCAN_G, entry[j].SCAN_G, SCAN_B, entry[j].SCAN_B);

			c_table.stats.inserts++;
			return(free);
		}

		/*
		 * If the next entry is the end of the list then append this new one
		 */
		if (entry[j].next < 0)
		{
			entry[free].last = j;
			entry[j].next = free;
//printf("insert EOL: %i (%i)\n", free, r>>c_table.redshift);

			c_table.stats.inserts++;
			return(free);
		}
	}

	/*
	 * So we are at the end of the list? Hm. Return "blue".
	printf("colour cache insert: we should not have got here\n");
	 */
	c_table.stats.errors++;
	return(0);
}

static void
cacheFreeColor(unsigned short r, unsigned short g, unsigned short b, int pindex)
{
	c_table.stats.deletes++;

	/*
	 * Search through the row depicted by R for the selected pindex.
	 *
	 * Hm, leave this for now - we are not actually big on deleting GC due to
	 * the way the cache works. If deletes gets out of sync with deleted then
	 * we can implement this.
	 *
	for (i = c_table.row[r >> c_table.redshift]; i >= 0; j = entry[i].next)
	{
		if (
	}
	 */
}

static int
cacheFindColor(unsigned short r, unsigned short g, unsigned short b, int cm)
{
	c_row *row;
	cc_entry *entry;
	int j = 0;
	int d = 0;
	unsigned short mask = c_table.mask; // 0xffff << (16 - cm);

	if ((r == 0) && (g == 0) && (b == 0xff00))
		return(0);

	/*
	 * First select the row using a hash of the red value
	 */
	if ((row = &c_table.row[SCAN_R >> c_table.redshift]) <= 0)
	{
		c_table.stats.miss_no_row++;
		return(-1);
	}

	/*
	 * Make these values 'searchable'
	 */
	SCAN_G &= mask;
	SCAN_B &= mask;

	/*
	 * We have the red matched, scan through the current line for a green
	 * match.
	 */
	if ((entry = row->entry) <= 0)
	{
		c_table.stats.miss_no_entry++;
		return(-1);
	}

	/*
	 * We first want to scan for green, that will be more efficient than 
	 * scanning that both green and blue match as this can terminate earlier.
	 */
	for (j = row->start; j >= 0; j = entry[j].next)
	{
		if ((d++) > 10000)
			return(-1);

		if (entry[j].SCAN_G == SCAN_G)
			break;

		if (entry[j].SCAN_G > SCAN_G)
		{
			c_table.stats.miss_no_green++;
			ASD;
			return(-1);
		}

		/*
		 * If we hit the end of the scan and still have no match on green
		 * then return.
		 */
		if (entry[j].next < 0)
		{
			c_table.stats.miss_eol++;
			ASD;
			return(-1);
		}
	}

	/*
	 * At this point we have scanned to where the cached green is greater/equal
	 * to the targetted value. Now scan for a matching blue until end of
	 * list or green no longer matches.
	 */
	for (; j >= 0; j = entry[j].next)
	{
		if ((d++) > 10000)
			return(-1);

		/* Check green still matches */
		if (entry[j].SCAN_G != SCAN_G)
		{
			c_table.stats.miss_no_color++;
			ASD;
			return(-1);
		}
		if (entry[j].SCAN_B > SCAN_B)
		{
			c_table.stats.miss_no_blue++;
			ASD;
			return(-1);
		}

		/*
		 * If this is a blue match, return the palette
		 */
		if (entry[j].SCAN_B == SCAN_B)
		{
			c_table.stats.hits++;

			ASD;
//printf("found %i\n", entry[j].p_index);
			return(entry[j].p_index);
		}
	}

	c_table.stats.miss_no_color++;
	ASD;

	/*
	 * If we get here then we have scanned until end of list. We could insert
	 * the new color however we will leave that to the calling party as we
	 * don't have a palette index.
	 */
	return(-1);
}

/*
 * See if we already have this color somewhere. This can be a slow operation
 * so we will make a couple of changes. Firstly, we will use a color cache to
 * accelerate the searches, and in the event of not finding the color then it
 * should be built in here.
 *
 * Cache will consist of a table hashed by red, with each table entry being
 * sorted then by green. As such we can jump very fast to the red matches and
 * search the greens for a blue match.
 *
 * We should add another call to insert a hashed entry.
 */
int
brightonFindColor(brightonPalette *palette, int ncolors,
unsigned short r, unsigned short g, unsigned short b, int match)
{
	register int i;
	register unsigned short rmin, rmax, gmin, gmax, bmin, bmax;
	float lesser = match , greater = 1 / match;

	return(cacheFindColor(r, g, b, match));

	/* printf("find %i, %i, %i %f %i\n", r, g, b, match, ncolors); */

	rmin = (lesser * ((float) r));
	if ((greater * ((float) r)) > 65535)
		rmax = 65535;
	else
		rmax = (greater * ((float) r));

	gmin = (lesser * ((float) g));
	if ((greater * ((float) g)) > 65535)
		gmax = 65535;
	else
		gmax = (greater * ((float) g));

	bmin = (lesser * ((float) b));
	if ((greater * ((float) b)) > 65535)
		bmax = 65535;
	else
		bmax = (greater * ((float) b));

	if (lesser > greater)
		lesser = greater = 1.0;

	for (i = 0; i < ncolors; i++)
	{
		if (palette[i].flags & BRIGHTON_INACTIVE_COLOR)
			continue;

		if ((palette[i].red >= rmin) &&
			(palette[i].red <= rmax) &&
			(palette[i].green >= gmin) &&
			(palette[i].green <= gmax) &&
			(palette[i].blue >= bmin) &&
			(palette[i].blue <= bmax))
			return(i);
	}
	return(-1);
}

int
brightonFindFreeColor(brightonPalette *palette, int ncolors)
{
	int i;

	for (i = 0; i < ncolors; i++)
		if (palette[i].flags & BRIGHTON_INACTIVE_COLOR)
			return(i);

	return(-1);
}

int
brightonFreeGC(brightonWindow *bwin, int index)
{
	if (index < 0)
		return(0);
	if (index >= bwin->cmap_size)
		return(0);
	if (--bwin->display->palette[index].uses == 0)
	{
		BFreeColor(bwin->display, &bwin->display->palette[index]);

		cacheFreeColor(
			bwin->display->palette[index].red,
			bwin->display->palette[index].green,
			bwin->display->palette[index].blue,
			index);
	}
	return(0);
}

/*
 * The primary use of GCs is for color selection, and then also primarily in the
 * forgreound. We are going to request colors as R/G/B tuples, as yet not with
 * any structure management although this may happen. We will consider the use
 * of hash table lookup with best fit, limiting the total number of colours
 * that brighton can reserve, and keep stats on lookup performance for different
 * hashing functions.
 *
 * To minimise color requirements it is preferable for multiple synths to be
 * children, ie, share the same contexts.
 */
int
brightonGetGC(brightonWindow *bwin,
unsigned short r, unsigned short g, unsigned short b)
{
	register int pindex;

	/*printf("brightonGetGC(%x, %x, %x)\n", r, g, b); */

	/*
	 * See if we can find this color
	 */
	if ((pindex = cacheFindColor(r, g, b, bwin->quality)) >= 0)
	{
		bwin->display->palette[pindex].uses++;
		return(pindex);
	}
	/*
	if ((pindex = brightonFindColor(bwin->display->palette, bwin->cmap_size,
		r, g, b, bwin->quality)) >= 0)
	{
		bwin->display->palette[pindex].uses++;
		return(pindex);
	}
	*/

	/*
	 * If we have no free colors, then palette[0] is the default
	 */
	if ((pindex =
		brightonFindFreeColor(bwin->display->palette, bwin->cmap_size)) < 0)
		return(0);

	bwin->display->palette[pindex].red =  r;
	bwin->display->palette[pindex].green = g;
	bwin->display->palette[pindex].blue = b;
	bwin->display->palette[pindex].uses++;

	bwin->display->palette[pindex].flags &= ~BRIGHTON_INACTIVE_COLOR;
	bwin->display->palette[pindex].uses++;

	cacheInsertColor(r, g, b, pindex);

	return(pindex);
}

/*
 * With named colors we need to allocate them in advance, we cannot leave this
 * up to the BRendering process. We should only really be using this for one
 * color as the match is exact, however the XPM bitmap format accepts names
 * as well as RGB even though pretty much all of the shipped bitmaps avoid
 * using names ("Blue" is just re-interpreted from 'none' in the bitmap) and
 * has to be exact to ensure transparency.
 */
int haveblue = -1;

int
brightonGetGCByName(brightonWindow *bwin, char *name)
{
	int pindex;

	if ((strcmp(name, "Blue") == 0) && (haveblue >= 0))
	{
		/*
		 * We only want to map blue once even though it may well be requested
		 * from multiple bitmaps.
		 */
		bwin->display->palette[haveblue].uses++;
		return(haveblue);
	}

	/*
	 * If we have no free colors, then palette[0] is the default. This is
	 * typically "Blue" which may have been a bad choice.....
	 */
	if ((pindex =
		brightonFindFreeColor(bwin->display->palette, bwin->cmap_size)) < 0)
		return(0);

	bwin->display->palette[pindex].uses++;

	BAllocColorByName(bwin->display, &bwin->display->palette[pindex], name);

	bwin->display->palette[pindex].flags &= ~BRIGHTON_INACTIVE_COLOR;
	bwin->display->palette[pindex].uses++;

	if (strcmp(name, "Blue") == 0)
		haveblue = pindex;

	return(pindex);
}

/*
 * Make sure we have a suitable visual, then try and get a shitload of colors.
 * We should consider looking for any existing visuals of an acceptable type?
 */
brightonPalette *
brightonInitColormap(brightonWindow *bwin, int ncolors)
{
	/*printf("brightonInitColormap(%i)\n", ncolors); */

	initColorCache(bwin, bwin->quality); /* Init cache with digit shift */

	if (bwin->display->palette == NULL)
	{
		int i;

		bwin->display->palette = (brightonPalette *)
			brightonmalloc(ncolors * sizeof(brightonPalette));

		for (i = 0; i < ncolors; i++)
		{
			bwin->display->palette[i].flags |= BRIGHTON_INACTIVE_COLOR;
			bwin->display->palette[i].pixel = -1;
		}
	}

	return(BInitColorMap(bwin->display));
}

void
brightonSprintColor(brightonWindow *bwin, char *cstring, int pixel)
{
	sprintf(cstring, "#%02x%02x%02x",
		bwin->display->palette[pixel].red >> 8,
		bwin->display->palette[pixel].green >> 8,
		bwin->display->palette[pixel].blue >> 8);
}

