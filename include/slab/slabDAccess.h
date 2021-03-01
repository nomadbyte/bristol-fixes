
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


#define SLAB_D_RDONLY	1
#define SLAB_D_WRONLY	2
#define SLAB_D_RDWR		4
#define SLAB_D_NBE		8 /* For backout editing - will save a file */
#define SLAB_D_FLOAT	16 /* Denotes a floating point mixing algorithm. Pass */
                           /* this flag if you have a float algo, and the API */
                           /* will ensure you are passed correct data format */

#ifndef SLAB_SEEK_SET
#define SLAB_SEEK_SET	0
#define SLAB_SEEK_GET	1
#define SLAB_SEEK_CUR	2
#endif

#define NBE_REPLACE		1

int slabOpen();
int slabLseek();
int slabRead();
int slabWrite();
int slabClose();

/*
 * When new versions are introduced this will be made into a union
 * Note that when the header is written to the file, the 'fn' and 'desc'
 * contents are only written as last as the strings themselves, not the full
 * number of bytes.
 */
typedef struct nbe_header {
	int version;	/* for changes to data format */
	int track;		/* Where data came from */
	int samples;	/* Number of samples */
	int fragments;	/* Number of fragments */
	int offset;		/* from offset */
	int fnl;		/* Length of original filename string */
	char fn[128];	/* Filename string itself */
	int descl;		/* Length of description */
	char desc[128];	/* description string itself */
	int seekOffset;	/* point for starting read/write operations */
} nbeHeader;

extern nbeHeader *slabNBEGetHeader();

/*
 * This is going to be opaque, it is actually an internal structure, compiled
 * into the library routines. The whole point of the library is to hide the
 * complexities of this structure.
 */
