
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

#ifndef BRIGHTON_H
#define BRIGHTON_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>

#include <config.h>

#include "brightondevflags.h"
#include "brightonevents.h"

struct BrightonWindow;
typedef int (*brightonRoutine)();
typedef int (*brightonCallback)(struct BrightonWindow *, int, int, float);

typedef struct BrightonLocations {
	char *name;
	int device;
	float x, y;
	float width, height;
	float from, to;
	brightonCallback callback;
	char *image;
	char *image2;
	unsigned int flags;
	int var;
	int val;
} brightonLocations;

typedef struct brightonResource {
	char *name;
	char *image;
	char *surface;
	unsigned int flags;
	brightonRoutine init;
	brightonRoutine configure;
	brightonCallback callback;
	int x, y, width, height;
	int ndevices;
	brightonLocations *devlocn;
} brightonResource;

#define RESOURCE_COUNT 64 /* Max # of panels */

/* Emulation defaults - can be overridden */
typedef struct BrightonEmulation {
	int voices;
	int detune;
	int gain;
	int pwd;
	int glide;
	int velocity;
	int opacity;
	int panel;
} brightonEmulation;

typedef struct BrightonApp {
	char *name;
	char *image;
	char *surface;
	unsigned int flags;
	brightonRoutine init;
	brightonRoutine configure;
	brightonCallback callback;
	brightonRoutine destroy;
	brightonEmulation emulate;
	int width, height, x, y;
	/*
	 * It would be nice to integrate the following as defaults that would be
	 * set when synth is found and overridden by subsequent options:
	int voices, detune, gain, glide; This is now brightonEmulation.
	 */
	int nresources;
	brightonResource resources[RESOURCE_COUNT];
} brightonApp;

extern int brightonRemoveInterface(struct BrightonWindow *);
extern int brightonParamChange(struct BrightonWindow *, int, int, brightonEvent *);
extern int brightonOpacity(struct BrightonWindow *, float);
extern int brightonColorQuality(struct BrightonWindow *, int);
extern struct BrightonWindow *brightonInterface(brightonApp *, int, int, int, float, int, int, int);
extern void brightonLogo(struct BrightonWindow *);
extern int brightonEventMgr();

#endif /* BRIGHTON_H */

