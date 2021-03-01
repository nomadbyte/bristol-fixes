
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

#define BRIGHTON_DEVTABLE_SIZE 32

#define BRIGHTON_ROTARY		0
#define BRIGHTON_SCALE		1
#define BRIGHTON_BUTTON		2
#define BRIGHTON_DISPLAY	3
#define BRIGHTON_PIC		4
/* #define BRIGHTON_PANEL 6 */
#define BRIGHTON_VUMETER	6
#define BRIGHTON_LEDBLOCK	8
#define BRIGHTON_HL_BUTTON	9
#define BRIGHTON_LEVER		10
#define BRIGHTON_MODWHEEL	11
#define BRIGHTON_LED		12
#define BRIGHTON_RIBBONKBD	13

typedef int (*brightonCreate)(brightonWindow *, brightonDevice *, int, char *);
typedef int (*brightonDestroy)(brightonDevice *);

extern int createRotary(brightonWindow *, brightonDevice *, int, char *);
extern int destroyRotary(brightonDevice *);
extern int createButton(brightonWindow *, brightonDevice *, int, char *);
extern int destroyButton(brightonDevice *);
extern int createScale(brightonWindow *, brightonDevice *, int, char *);
extern int destroyScale(brightonDevice *);
extern int createDisplay(brightonWindow *, brightonDevice *, int, char *);
extern int destroyDisplay(brightonDevice *);
extern int createPic(brightonWindow *, brightonDevice *, int, char *);
extern int destroyPic(brightonDevice *);
extern int createTouch(brightonWindow *, brightonDevice *, int, char *);
extern int destroyTouch(brightonDevice *);
extern int createVu(brightonWindow *, brightonDevice *, int, char *);
extern int destroyVu(brightonDevice *);
extern int createHammond(brightonWindow *, brightonDevice *, int, char *);
extern int destroyHammond(brightonDevice *);
/*
extern int createKey(brightonWindow *, brightonDevice *, int, char *);
extern int destroyKey(brightonDevice *);
*/
extern int createLedblock(brightonWindow *, brightonDevice *, int, char *);
extern int destroyLedblock(brightonDevice *);

extern int createHButton(brightonWindow *, brightonDevice *, int, char *);
extern int destroyHButton(brightonDevice *);

extern int createLever(brightonWindow *, brightonDevice *, int, char *);
extern int destroyLever(brightonDevice *);

extern int createModWheel(brightonWindow *, brightonDevice *, int, char *);
extern int destroyModWheel(brightonDevice *);

extern int createLed(brightonWindow *, brightonDevice *, int, char *);
extern int destroyLed(brightonDevice *);

extern int createRibbon(brightonWindow *, brightonDevice *, int, char *);
extern int destroyRibbon(brightonDevice *);

typedef struct brightonDevices {
	brightonCreate create;
	brightonDestroy destroy;
	int from; /* Table index for inheritance. */
} brightonDevices;

brightonDevices brightonDevTable[BRIGHTON_DEVTABLE_SIZE] = {
	{createRotary, destroyRotary, -1},
	{createScale, destroyScale, -1},
	{createButton, destroyButton, 0},
	{createDisplay, destroyDisplay, -1},
	{createPic, destroyPic, -1},
	{createTouch, destroyTouch, -1},
	{createVu, destroyVu, -1},
	{createHammond, destroyHammond, -1},
	{createLedblock, destroyLedblock, 0},
	{createHButton, destroyHButton, 0},
	{createLever, destroyLever, 0},
	{createModWheel, destroyModWheel, 0},
	{createLed, destroyLed, 0},
	{createRibbon, destroyRibbon, 0},
	{0, 0, -1}
};

