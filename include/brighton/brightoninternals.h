
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

#ifndef BRIGHTONINTERNALS_H
#define BRIGHTONINTERNALS_H

#include <stdio.h>

#include "brightondevflags.h"
#include "brightonevents.h"
#include "brightonvectorgraphics.h"

#define BRIGHTON_ST_CLOCK	0
#define BRIGHTON_ST_REQ		1
#define BRIGHTON_ST_CANCEL	2 /* Cancel single callback */
#define BRIGHTON_ST_FIRST	BRIGHTON_ST_CANCEL

#define BRIGHTON_FT_CLOCK		0
#define BRIGHTON_FT_REQ			1
#define BRIGHTON_FT_STOP		2
#define BRIGHTON_FT_START		3
#define BRIGHTON_FT_STEP		4
#define BRIGHTON_FT_RESET		5
#define BRIGHTON_FT_CANCEL		6 /* Cancel all */
#define BRIGHTON_FT_TICKTIME	7
#define BRIGHTON_FT_DUTYCYCLE	8

#define isblue(x,y,z) \
			((x >= 0) && \
			(y != NULL) && \
			(z != NULL) && \
			((z[x] < 0) || \
			((y[z[x]].red == 0) && \
			(y[z[x]].green == 0) && \
			(y[z[x]].blue == 0xffff))))

/*
 * Some return codes.
 */
#define BRIGHTON_OK 0
#define BRIGHTON_XISSUE 1

#include "brighton.h"

/*
 * Shadow rendering
 */
#define BRIGHTON_STATIC 1
#define BRIGHTON_DYNAMIC 2
typedef struct BrightonShadow {
	unsigned int flags;
	brightonCoord *coords;
	int *mask;
	int minx, maxx, miny, maxy;
	int ccount;
} brightonShadow;

/*
 * palette flags
 */
#define BRIGHTON_CMAP_SIZE 16384
#define BRIGHTON_INACTIVE_COLOR 0x01
#define BRIGHTON_QR_QSIZE 128
#define BRIGHTON_QR_COLORS 24
typedef struct BrightonPalette {
	unsigned int flags;
	int uses;
	unsigned short red, green, blue;
	long pixel;
	void *color;
	void *gc;
} brightonPalette;

/*
 * Flags are for preferred rendering.
#define BRIGHTON_ANTIALIAS 0x01
 */
typedef struct BrightonBitmap {
	unsigned int flags;
	struct BrightonBitmap *next, *last;
	int uses;
	char *name;
	int width, height, ncolors, ctabsize, istatic, ostatic;
	int *pixels;
	int *colormap;
} brightonBitmap;

#include "brightondev.h"

/*
 * Flags for brightonPanel.flags - LSW.
 */
#define BRIGHTON_ACTIVE 0x80000000
#define TOO_SMALL 0x02
#define BRIGHTON_GREYSCALE 0x04
#define BRIGHTON_DEV_ACTIVE 0x08
#define BRIGHTON_SET_SIZE 0x010
typedef struct BrightonPanel {
	unsigned int flags;
	struct BrightonPanel *next, *last;
	int x, y;
	unsigned int width, height, depth, border;
	brightonBitmap *image;
	brightonBitmap *surface;
	brightonBitmap *canvas; /* surface overlaid with image */
	brightonResource *resource;
	struct BrightonDevice **devices;
	brightonSize *locations;
	int activedev;
} brightonPanel;

#define BRIGHTON_DEFAULT_DISPLAY ":0.0"
#define BRIGHTON_NAMESIZE 64
#define BRIGHTON_LIB_DEBUG		0x00010000
#define BRIGHTON_BIMAGE			0x00020000
#define BRIGHTON_ANTIALIAS_1	0x00040000
#define BRIGHTON_ANTIALIAS_2	0x00080000
#define BRIGHTON_ANTIALIAS_3	0x00100000
#define BRIGHTON_ANTIALIAS_4	0x00200000
#define BRIGHTON_ANTIALIAS_5	0x00400000
#define _BRIGHTON_WINDOW		0x00800000
#define BRIGHTON_BVIMAGE		0x01000000
typedef struct BrightonDisplay {
	unsigned int flags;
	struct BrightonDisplay *next, *last;
	brightonPalette *palette;
	void *display; /* this type depends on underlying window system */
	void *image; /* this type depends on underlying window system */
	struct BrightonWindow *bwin;
	char name[BRIGHTON_NAMESIZE];
	int width, height, depth;
} brightonDisplay;

/*
 * Flags for brightonWindow.flags
 */
#define BRIGHTON_NO_DRAW		0x00000001
/* SET_SIZE, DEV_ACTIVE are in here too */
#define BRIGHTON_BUSY			0x00000020
#define BRIGHTON_NO_LOGO		0x00000040
#define BRIGHTON_NO_ASPECT		0x00000080
#define _BRIGHTON_POST			0x00000100
#define BRIGHTON_DEBUG			0x00000200
#define BRIGHTON_EXITING		0x00000400
#define BRIGHTON_AUTOZOOM		0x00000800
#define BRIGHTON_SET_RAISE		0x00001000
#define BRIGHTON_SET_LOWER		0x00002000
#define _BRIGHTON_SET_HEIGHT	0x00004000
#define BRIGHTON_ROTARY_UD		0x00008000

#define BRIGHTON_ITEM_COUNT		512 /* max devices per panel */
/*
 * These are used to hold items that have been placed on the top layer. We
 * need this structure since these items may be inserted and removed.
 */
#define BRIGHTON_LAYER_PUT		0x01
#define BRIGHTON_LAYER_PLACE	0x02
#define BRIGHTON_LAYER_ALL		0x04
typedef struct BrightonLayerItem {
	int flags;
	int id;
	brightonBitmap *image;
	int x, y;
	int w, h;
	int d; /* Rotational point? */
	int scale;
} brightonLayerItem;

typedef struct BrightonNRPcontrol {
	int nrp;
	struct brightonDevice *device;
} brightonNRPcontrol;

#define BRIGHTON_NRP_COUNT 128
#define BRIGHTON_GANG_COUNT 8

typedef struct BrightonWindow {
	unsigned int flags;
	struct BrightonWindow *next, *last;
	brightonDisplay *display;
	brightonBitmap *image;
	brightonBitmap *surface;
	brightonPanel *panels;
	brightonBitmap *bitmaps;
	brightonBitmap *canvas; /* surface overlaid with image, scaled. */
	brightonBitmap *dlayer; /* device rendering layer. */
	brightonBitmap *slayer; /* shadow rendering layer. */
	brightonBitmap *tlayer; /* patch rendering layer (should be a stack?). */
	brightonBitmap *mlayer; /* menu rendering layer (should be a stack?). */
	brightonBitmap *render; /* output bitmap to screen */
	brightonBitmap *renderalias; /* output alias bitmap to screen */
	brightonLayerItem items[BRIGHTON_ITEM_COUNT];
	float opacity;
	int quality;
	float antialias;
	int grayscale;
	int win;
	void *gc;
	int x, y;
	unsigned int width, height, depth, border;
	unsigned int minw, minh;
	unsigned int maxw, maxh;
	float aspect;
	int cmap_size;
	int id;
	char *window_name;
	int lightX, lightY, lightH;
	float lightI;
	brightonRoutine callbacks[BLASTEvent];
	brightonIApp *app;
	brightonApp *template;
	brightonIResource *activepanel;
	brightonILocations *activedev;
	int parentwin;
	/* These should not really be in this structure, but in the synth. */
	int midimap[128]; /* This could be u_char */
	/*
	 * Only build support for 128 controls. Supporting 16384 of them would be
	 * rather large on memory however a bigger value could be useful - FFS.
	 */
	int GM2values[128];
	/* These are for CC tracking */
	struct brightonDevice *midicontrol[128][BRIGHTON_GANG_COUNT];
	float midicontrolval[128][BRIGHTON_GANG_COUNT];
	float midicontrolscaler[128];
	/* These are for NRP tracking */
	int nrpcount;
	brightonNRPcontrol *nrpcontrol; //[BRIGHTON_NRP_COUNT];
	/* CC value mapping tables */
	u_char valuemap[128][128];
	int kbdmap[256][2];
	int dcTimeout;
} brightonWindow;

typedef int (*brightonDeviceAlgo)();

/*
 * Global structure for any given renderable object.
 */
#define BRIGHTON_DEV_INITED 0x01
typedef struct BrightonDevice {
	unsigned int flags;
	struct BrightonDevice *next, *last;
	int device; /* device type. 0=rotary, etc. */
	int index; /* index of this instance */
	int panel; /* parent panel of this instance */
	brightonWindow *bwin;
	brightonPanel *parent;
	int originx, originy;
	int x, y; /* Location onto parent window. */
	int width, height; /* Target width and height */
	brightonBitmap *imagebg; /* background image, not rotated */
	brightonBitmap *image; /* Generic rotatatable */
	brightonBitmap *image0; /* Generic rotatatable */
	brightonBitmap *image1; /* Not rotated. If null, render circle of fg */
	brightonBitmap *image2; /* Not rotated. If null, render circle of fg */
	brightonBitmap *image3; /* Not rotated. If null, render circle of fg */
	brightonBitmap *image4; /* Not rotated. If null, render circle of fg */
	brightonBitmap *image5; /* Not rotated. If null, render circle of fg */
	brightonBitmap *image6; /* Not rotated. If null, render circle of fg */
	brightonBitmap *image7; /* Not rotated. If null, render circle of fg */
	brightonBitmap *image8; /* Not rotated. If null, render circle of fg */
	brightonBitmap *image9; /* Not rotated. If null, render circle of fg */
	brightonBitmap *imagec; /* Not rotated. If null, render circle of fg */
	brightonBitmap *imagee; /* Not rotated. If null, render circle of fg */
	int bg, fg; /* background palette - usual not used. FG for top of pot. */
	float value;
	float lastvalue;
	float position;
	float lastposition;
	float value2;
	float lastvalue2;
	float position2;
	float lastposition2;
	char text[64]; /* displayable name for device. */
	int varindex; /* index into global variable table - device's tracking var. */
	brightonDeviceAlgo destroy;
	brightonDeviceAlgo configure;
	brightonShadow shadow;
} brightonDevice;

#define BRIGHTON_DEFAULT_ICON 1

extern void brightonStretchAlias(brightonWindow *, brightonBitmap *, brightonBitmap *, int, int, int, int, float);
extern void brightonStretch(brightonWindow *, brightonBitmap *, brightonBitmap *, int, int, int, int, int);
extern brightonDisplay *brightonOpenDisplay(char *);
extern brightonDisplay *brightonFindDisplay(brightonDisplay *, brightonDisplay *);
extern brightonWindow *brightonCreateWindow(brightonDisplay *, brightonApp *, int, int, int, int, int, int);
extern void brightonDestroyWindow(brightonWindow *);
extern void brightonDestroyPanel(brightonPanel *);
extern int brightonEventLoop(brightonDisplay **);
extern void brightonInitDefHandlers(brightonWindow *);
extern int brightonFillBase(brightonWindow *, char *, char *);
extern void *brightonmalloc(size_t);
extern FILE *brightonfopen(char *, char *);
extern char *brightonfgets(char *, int, FILE *);
extern int brightonfclose(FILE *);
extern brightonPalette *brightonInitColormap(brightonWindow *, int);
extern int brightonFindColor(brightonPalette *, int, unsigned short, unsigned short, unsigned short, int);
extern int brightonFindFreeColor(brightonPalette *, int);
extern int brightonGetGC(brightonWindow *, unsigned short, unsigned short, unsigned short);
extern int brightonGetGCByName(brightonWindow *, char *);
extern int brightonFreeGC(brightonWindow *, int);
extern brightonBitmap *brightonFreeBitmap(brightonWindow *, brightonBitmap *);
extern brightonBitmap *brightonCreateNamedBitmap(brightonWindow *, int, int, char *);
extern brightonBitmap *brightonCreateBitmap(brightonWindow *, int, int);
extern brightonBitmap *brightonReadImage(brightonWindow *, char *);
extern int brightonDestroyImage(brightonWindow *, brightonBitmap *);
extern int brightonRender(brightonWindow *, brightonBitmap *, brightonBitmap *, int, int, int, int, int);
extern int brightonTesselate(brightonWindow *, brightonBitmap *, brightonBitmap *, int, int, int, int, int);
extern int brightonRotate(brightonWindow *, brightonBitmap *, brightonBitmap *, int, int, int, int, double);
extern int brightonWorldChanged(brightonWindow *, int, int);
extern brightonDevice *brightonCreateDevice(brightonWindow *, int, int, int, char *);
extern void brightonDestroyDevice(brightonDevice *);
extern void brightonConfigureDevice(brightonDevice *, brightonEvent *);
extern void brightonDeviceEvent(brightonDevice *, brightonEvent *);
extern int brightonForwardEvent(brightonWindow *);
extern int brightonLocation(brightonWindow *, int, int, int, int, int);
extern int brightonXpmWrite(brightonWindow *, char *);

extern int brightonRequestResize(brightonWindow *, int, int);

extern brightonILocations *brightonLocator(brightonWindow *, int, int);
extern brightonIResource *brightonPanelLocator(brightonWindow *, int, int);
extern brightonILocations *brightonDeviceLocator(brightonIResource *, int, int);

extern int brightonCreateInterface(brightonWindow *, brightonApp *);
extern int brightonDestroyInterface(brightonWindow *);

extern int brightonRenderShadow(brightonDevice *, int);
extern int brightonFinalRender(brightonWindow *, int, int, int, int);
extern int brightonDevUndraw(brightonWindow *, brightonBitmap *, int, int, int, int);
extern int brightonRePlace(brightonWindow *);
extern int brightonPlace(brightonWindow *, char *, int, int, int, int);
extern int brightonPut(brightonWindow *, char *, int, int, int, int);
extern int brightonRemove(brightonWindow *, int);

extern int brightonSlowTimer(brightonWindow *, brightonDevice *, int);
extern int brightonFastTimer(brightonWindow *, int, int, int, int);

extern void brightonAlphaLayer(brightonWindow *, brightonBitmap *, brightonBitmap *, int, int, int, int, double);

extern int brightonInitBitmap(brightonBitmap *, int);
extern int brightonDoFinalRender(brightonWindow *, int, int, int, int);

extern int BAutoRepeat(brightonDisplay *, int);

extern int brightonGetDCTimer();
extern int brightonDoubleClick();
extern void brightonfree(void *);

extern void brightonRegisterController(brightonDevice *);

#endif /* BRIGHTONINTERNALS_H */

