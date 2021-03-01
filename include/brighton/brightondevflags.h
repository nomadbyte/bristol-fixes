
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

#ifndef BRIGHTONDEVFLAGS_H
#define BRIGHTONDEVFLAGS_H

/*
 * These need to be reviewed, I have a feeling they are not all exclusive after
 * too much hectic development. (17/06/08 0.20.6)
 *
 * For rendering
 */
#define BRIGHTON_DIRECTION_MASK	0x00000003
#define BRIGHTON_REVERSE		0x00000001
#define BRIGHTON_VERTICAL		0x00000002
#define BRIGHTON_NOSHADOW		0x00000008
#define BRIGHTON_WITHDRAWN		0x00000020
#define BRIGHTON_ANTIALIAS		0x00000040
#define BRIGHTON_HALFSHADOW		0x00000080
#define BRIGHTON_REDRAW			0x00000100
#define BRIGHTON_STRETCH		0x00010000
#define BRIGHTON_POST_WINDOW	0x00020000
#define BRIGHTON_WINDOW			0x00040000
#define BRIGHTON_KEY_PANEL		0x00080000

/* #define BRIGHTON_CHECKBUTTON 0x0010 */
#define BRIGHTON_SHIFTKEY		0x0020
/* #define BRIGHTON_TESSELATE		0x0040 */
#define BRIGHTON_THREEWAY		0x0100
#define BRIGHTON_CENTER			0x0200
#define BRIGHTON_FIVEWAY		0x0400
#define BRIGHTON_TRACKING		0x0800 /* Mouse following */
#define BRIGHTON_NO_TOGGLE		0x1000
/* #define BRIGHTON_REPAINT		0x1000 Blank out (button) image and redraw */

#define BRIGHTON_ROTARY			0
#define BRIGHTON_SCALE			1
#define BRIGHTON_BUTTON			2
#define BRIGHTON_DISPLAY		3
#define BRIGHTON_PIC			4
#define BRIGHTON_TOUCH			5
#define BRIGHTON_VUMETER		6
#define BRIGHTON_HAMMOND		7
#define BRIGHTON_LEDBLOCK		8
#define BRIGHTON_HBUTTON		9
#define BRIGHTON_LEVER			10
#define BRIGHTON_MODWHEEL		11
#define BRIGHTON_LED			12
#define BRIGHTON_RIBBONKBD		13

/*
 * rotary. Notch is also used here.
 */
#define BRIGHTON_CONTINUOUS 0x01
/* #define BRIGHTON_NOSHADOW 0x08 */
#define BRIGHTON_STEPPED		0x1000 /* Limited rotary motion */

/*
 * Buttons
 */
#define BRIGHTON_BLUE 0x01
#define BRIGHTON_HALF_REVERSE 0x04
#define BRIGHTON_CHECKBUTTON 0x010
#define BRIGHTON_RADIOBUTTON 0x080

/*
 * Scalers
 */
#define BRIGHTON_HSCALE 0x04
#define BRIGHTON_NOTCH 0x0400
#define BRIGHTON_WIDE 0x0400

#define BRIGHTON_CONTROLKEY 0x0800

#endif /* BRIGHTONDEVFLAGS_H */

