
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


/*
 * This is the sementic representation of the bussed algorithms, including
 * algorithm names, parameters and value.
 *
 * I would prefer for this to be parsed into the frontend, simply for ease of
 * development. It is compiled in for now.
 */

#define FB_CONTROLS 8
#define FB_ALGO_COUNT 32

#define POT		1 /* Pot is a rotary continuous controller */
#define ENTRY	2 /* Entry is an integer, typically for bus or algo id */
#define SLIDER	3 /* Slider is a linear continuous controller */
#define LPOT	4 /* LargePot is a rotary continuous controller */
#define MON		5 /* For per track VU meter display */

#define UNK			0
#define FB_DELAY	1
#define FB_GAIN		2
#define FB_FREQ		3
#define FB_INV		4 /* display by subtracting value from its max */
#define FB_FREQ2	5 /* For bass frequencies */
#define FB_RATE		6
#define FB_CYCLE	7

/*
 * Used as conversion flags.
 */
#define SAMPLE_COUNT 0
#define SAMPLE_OFFSET 1

#ifdef FLOAT_PROC
#define FLOAT_LWFILTER	0
#define FLOAT_FILTER	1	/* filt 0 - 4 */
#define FLOAT_RESFILTER	2	/* filt 0 - 4 */
#define FLOAT_LWRESFILT	3	/* filt 0 - 4 */

#define FLOAT_SEND		5	/* ext 5 - 8 */
#define FLOAT_POSTSEND	6
#define FLOAT_INSERT	7

#define FLOAT_PAN		9	/* mix 9 - 12 */
#define FLOAT_FINDMAX	10
#define FLOAT_GAIN		11

#define FLOAT_GATE		14	/* dyn 14 - 19 */
#define FLOAT_COMPRESS	15
#define FLOAT_LIMIT		16

#define FLOAT_REVERB	20	/* fx 20 - 30 */
#define FLOAT_ECHO		21
#define FLOAT_CHORUS	22
#define FLOAT_DELAY		23
#define FLOAT_TREM		24
#define FLOAT_VIB		25
#define FLOAT_UDELAY	26
#endif

/*
 * If you need memory, this is pretty much the define limit.
 */
#define HISTORY_SIZE 65536 /* Each operator may take 65K samples */

typedef struct FbParam {
	char name[32];
	char longName[32];
	int type;
	int min;
	int max;
	int factor;
	int value;
	char fg[32];
	char bg[32];
	int semantics; /* Used to interpret display units */
} fbparam;

typedef struct fbOperator {
	char name[32];
	char longname[32];
	fbparam fbControls[FB_CONTROLS];
	int flags;
} fbop;

typedef struct fbControls {
	int val;
	int type;
} fbcontrols;

typedef struct fbAlgo {
	int algo;
	char *memory;
	float fhold[FB_CONTROLS]; /* This is primarily float ops */
	int flags;
} fbalgo;

