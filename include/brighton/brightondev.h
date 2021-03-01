
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

#ifndef BRIGHTONDEV_H
#define BRIGHTONDEV_H

typedef struct BrightonILocations {
	int type;
	int index, panel;
	float x, y, width, height;
	int ax, ay, aw, ah;
	float from, to;
	brightonCallback callback;
	brightonBitmap *image;
	brightonBitmap *image2;
	unsigned int flags;
	struct brightonDevice *dev;
} brightonILocations;

typedef struct brightonIResource {
	brightonBitmap *image;
	brightonBitmap *surface;
	brightonBitmap *canvas;
	unsigned int flags;
	brightonRoutine init;
	brightonRoutine configure;
	brightonCallback callback;
	int x, y, width, height;
	int sx, sy, sw, sh;
	int ndevices;
	brightonILocations *devlocn;
} brightonIResource;

typedef struct BrightonIApp {
	unsigned int flags;
	brightonRoutine init;
	int width, height;
	int nresources;
	brightonIResource *resources; /* This is actually a pointer to an array */
} brightonIApp;

#endif /* BRIGHTONDEV_H */

