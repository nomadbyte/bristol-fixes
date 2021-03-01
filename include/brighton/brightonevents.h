
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

#ifndef BRIGHTONEVENTS_H
#define BRIGHTONEVENTS_H

#define BLASTEvent 35

typedef struct BrightonKey {
	short code, mod;
} brightonKey;

typedef struct BrightonCoord {
	short x, y;
} brightonCoord;

typedef struct BrightonSize {
	int x, y, width, height;
} brightonSize;

#define BRIGHTON_FLOAT 0
#define BRIGHTON_MEM 1
#define BRIGHTON_INT 2

#define BRIGHTON_NONE -1
#define BRIGHTON_RESIZE 0
#define BRIGHTON_MOTION 1
#define BRIGHTON_BUTTONPRESS 2
#define BRIGHTON_BUTTONRELEASE 3
#define BRIGHTON_PARAMCHANGE 4
#define BRIGHTON_KEYPRESS 5
#define BRIGHTON_KEYRELEASE 6
#define BRIGHTON_ENTER 7
#define BRIGHTON_LEAVE 8
#define BRIGHTON_FOCUS_IN 9
#define BRIGHTON_FOCUS_OUT 10
#define BRIGHTON_CONFIGURE 22 // 9
#define BRIGHTON_CONFIGURE_REQ 23 // 10
#define BRIGHTON_EXPOSE 11
#define BRIGHTON_GEXPOSE 12
#define BRIGHTON_CLIENT 15
#define BRIGHTON_DESTROY 17
#define BRIGHTON_LINK 18 /* Patch cable control */
#define BRIGHTON_UNLINK 19 /* Patch cable control */
#define BRIGHTON_SLOW_TIMER 20 /* 500ms timer sequence with ID - flashing LED */
#define BRIGHTON_FAST_TIMER 21 /* ms timer sequence with ID - sequencer*/

#define BRIGHTON_BUTTON1 1
#define BRIGHTON_BUTTON2 2
#define BRIGHTON_BUTTON3 3

#define BRIGHTON_MOD_SHIFT 1
#define BRIGHTON_MOD_CONTROL 4

/*
 * This is an oversimplified conversion of the XEvent strcture for brighton use
 */
typedef struct BrightonEvent {
	unsigned int flags;
	struct BrightonEvent *next;
	int wid;
	int type;
	int command;
	int x, y, w, h;
	int key;
	int mod;
	float value;
	int intvalue;
	void *m;
	//int ox, oy;
} brightonEvent;


#endif /* BRIGHTONEVENTS_H */

