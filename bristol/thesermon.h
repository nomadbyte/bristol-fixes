
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

#define OSC_COUNT 109
#define OSC_LIMIT 91
#define WAVE_SIZE 1024
#define WAVE_COUNT 8
#define BUS_COUNT 9

#define NEW_CLICK

/*
 * These are the midi note offsets for any given key harmonics.
 */
static int offsets[BUS_COUNT] = {-12, 7, 0, 12, 19, 24, 28, 31, 36};

/*
 * Get a humungous chunk of memory, SAMPLE_COUNT should come from the audiomain
 * structure. This is the pointer to that chunk of 'current buffer working mem'.
 */
static float *oscillators = NULL;

static int wi[256] = {
	0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, /* Lower foldbacks not used */
	0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, /* Lower foldbacks not used */
	0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, /* Foldback */
	0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, /* First octave */
	12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23,
	24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35,
	36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47,
	48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59,
	60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71,
	72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83,
	84, 85, 86, 87, 88, 89, 90, 79, 80, 81, 82, 83, /* Foldback starts again */
	84, 85, 86, 87, 88, 89, 90, 79, 80, 81, 82, 83, /* And repeats from here */
	84, 85, 86, 87, 88, 89, 90, 79, 80, 81, 82, 83,
	84, 85, 86, 87, 88, 89, 90, 79, 80, 81, 82, 83,
	84, 85, 86, 87, 88, 89, 90, 79, 80, 81, 82, 83,
	84, 85, 86, 87, 88, 89, 90, 79, 80, 81, 82, 83,
	84, 85, 86, 87, 88, 89, 90, 79, 80, 81, 82, 83,
	84, 85, 86, 87, 88, 89, 90, 79, 80, 81, 82, 83,
	84, 85, 86, 87, 88, 89, 90, 79, 80, 81, 82, 83,
	84, 85, 86, 87, 88, 89, 90, 79, 80, 81, 82, 83,
};

/*
 * This is the compartments configuration, alterable in the tonewheel config
 * file.
 */
static int compartments[24][4] = {
{0,	48,	12,	60},
{24,	72,	37,	-1},
{7,	55,	19,	67},
{31,	79,	43,	86},
{2,	50,	14,	62},
{26,	74,	38,	-1},
{9,	57,	21,	69},
{33,	81,	45,	88},
{4,	52,	16,	64},
{28,	76,	40,	-1},
{11,	59,	23,	71},
{35,	83,	47,	90},
{6,	54,	18,	66},
{30,	78,	42,	85},
{1,	49,	13,	61},
{25,	73,	37,	-1},
{8,	56,	20,	68},
{32,	80,	44,	54},
{3,	51,	15,	63},
{27,	75,	13,	-1},
{10,	58,	22,	70},
{34,	82,	46,	89},
{5,	53,	17,	65},
{29,	77,	41,	84}
};

/*
 * This will emulate the tapering resistors of the drawbar box. Each line from
 * the tonewheel engine was drawn through a cable tree to each note and would
 * traverse different distances and signal loss for which tapering resistors
 * were added to even out the gains. There is an aspect of crosstalk that has
 * not been taken care of here. These are hidden in the tonewheel structure.
 */
#define R100 0.4
#define R50 0.6
#define R34 0.8
#define R24 1.0
#define R15 1.2
#define R10 1.4
#define R0 0.4
#define R1 0.6
#define R2 0.8
#define R3 1.0
#define R4 1.2
#define R5 1.2

static float tresistors[6] = {R100, R50, R34, R24, R15, R10};
static float deftp[BUS_COUNT] = {R100, R50, R34, R34, R24, R24, R24, R15, R10};

/*
 * We then need to add a tapering configuration, this should also be in the
 * tonewheel config.
 */
static float tapers[OSC_LIMIT][9] = {
{R0, R2, R1, R2, R5, R5, R4, R3, R3},
{R0, R2, R1, R2, R5, R5, R4, R3, R3},
{R0, R2, R1, R2, R5, R5, R4, R3, R3},
{R0, R2, R1, R2, R5, R5, R4, R3, R3},
{R0, R2, R1, R2, R5, R5, R4, R3, R3},
{R0, R2, R1, R2, R5, R5, R4, R3, R3},
{R0, R2, R1, R2, R5, R5, R4, R3, R3},
{R0, R2, R1, R2, R5, R5, R4, R3, R3},
{R0, R2, R1, R2, R5, R5, R4, R3, R3},
{R0, R2, R1, R2, R5, R5, R4, R3, R3},
{R1, R2, R1, R2, R5, R5, R4, R3, R3},
{R1, R2, R1, R2, R5, R4, R4, R3, R3},
{R1, R2, R1, R2, R4, R4, R4, R3, R3},
{R1, R2, R1, R3, R4, R4, R4, R3, R3},
{R1, R3, R1, R3, R4, R4, R4, R3, R3},
{R1, R3, R2, R3, R4, R4, R4, R3, R3},
{R2, R3, R2, R3, R4, R4, R4, R3, R3},
{R2, R3, R2, R3, R4, R4, R4, R3, R3},
{R2, R3, R2, R3, R4, R4, R3, R3, R3},
{R2, R3, R2, R3, R4, R4, R3, R3, R3},
{R2, R3, R2, R3, R3, R3, R3, R3, R3},
{R2, R3, R2, R3, R3, R3, R3, R3, R3},
{R2, R3, R2, R3, R3, R3, R3, R3, R3},
{R2, R3, R3, R3, R3, R3, R3, R3, R3},
{R3, R3, R3, R3, R3, R3, R3, R3, R3},
{R3, R3, R3, R3, R3, R3, R3, R3, R3},
{R3, R3, R3, R3, R3, R3, R3, R3, R3},
{R3, R3, R3, R3, R3, R3, R3, R3, R3},
{R3, R3, R3, R3, R3, R3, R3, R3, R3},
{R3, R3, R3, R3, R3, R3, R3, R3, R3},
{R3, R3, R3, R3, R3, R3, R3, R3, R3},
{R3, R3, R3, R3, R3, R3, R3, R3, R3},
{R3, R3, R3, R3, R3, R3, R3, R3, R3},
{R3, R3, R3, R3, R3, R3, R3, R3, R3},
{R3, R3, R3, R3, R3, R3, R3, R3, R3},
{R3, R3, R3, R3, R3, R3, R3, R3, R3},
{R4, R3, R3, R3, R3, R3, R3, R3, R3},
{R4, R3, R4, R3, R3, R3, R3, R3, R3},
{R4, R4, R4, R3, R3, R3, R3, R3, R3},
{R4, R4, R4, R2, R3, R3, R3, R3, R3},
{R4, R4, R4, R2, R2, R3, R3, R3, R3},
{R4, R4, R4, R2, R2, R2, R3, R3, R3},
{R4, R4, R4, R2, R2, R2, R2, R3, R3},
{R4, R4, R4, R2, R2, R2, R2, R2, R1},
{R4, R4, R4, R2, R2, R2, R2, R2, R1},
{R4, R4, R4, R2, R2, R2, R2, R2, R1},
{R4, R4, R4, R2, R2, R2, R2, R2, R1},
{R4, R4, R4, R2, R2, R2, R2, R2, R1},
{R5, R4, R4, R2, R2, R2, R2, R1, R1},
{R5, R4, R5, R2, R2, R2, R2, R1, R1},
{R5, R5, R5, R2, R2, R2, R2, R1, R1},
{R5, R5, R5, R2, R2, R2, R1, R1, R1},
{R5, R5, R5, R2, R1, R2, R1, R1, R1},
{R5, R5, R5, R2, R1, R2, R1, R1, R1},
{R5, R5, R5, R2, R1, R2, R1, R1, R1},
{R5, R5, R5, R2, R1, R1, R1, R1, R1},
{R5, R5, R5, R2, R1, R1, R1, R1, R1},
{R5, R5, R5, R2, R1, R1, R1, R1, R1},
{R5, R5, R5, R2, R1, R1, R1, R1, R1},
{R5, R5, R5, R2, R1, R1, R1, R1, R1},
{R5, R5, R5, R2, R1, R1, R1, R1, R1}
};

#ifdef TONEMATRIX
static float rtapers[OSC_LIMIT][9] = {
{100.0, 34.0, 50.0, 34.0, 10.0, 10.0, 15.0, 24.0, 24.0},
{100.0, 34.0, 50.0, 34.0, 10.0, 10.0, 15.0, 24.0, 24.0},
{100.0, 34.0, 50.0, 34.0, 10.0, 10.0, 15.0, 24.0, 24.0},
{100.0, 34.0, 50.0, 34.0, 10.0, 10.0, 15.0, 24.0, 24.0},
{100.0, 34.0, 50.0, 34.0, 10.0, 10.0, 15.0, 24.0, 24.0},
{100.0, 34.0, 50.0, 34.0, 10.0, 10.0, 15.0, 24.0, 24.0},
{100.0, 34.0, 50.0, 34.0, 10.0, 10.0, 15.0, 24.0, 24.0},
{100.0, 34.0, 50.0, 34.0, 10.0, 10.0, 15.0, 24.0, 24.0},
{100.0, 34.0, 50.0, 34.0, 10.0, 10.0, 15.0, 24.0, 24.0},
{100.0, 34.0, 50.0, 34.0, 10.0, 10.0, 15.0, 24.0, 24.0},
{50.0, 34.0, 50.0, 34.0, 10.0, 10.0, 15.0, 24.0, 24.0},
{50.0, 34.0, 50.0, 34.0, 10.0, 15.0, 15.0, 24.0, 24.0},
{50.0, 34.0, 50.0, 34.0, 15.0, 15.0, 15.0, 24.0, 24.0},
{50.0, 34.0, 50.0, 24.0, 15.0, 15.0, 15.0, 24.0, 24.0},
{50.0, 24.0, 50.0, 24.0, 15.0, 15.0, 15.0, 24.0, 24.0},
{50.0, 24.0, 34.0, 24.0, 15.0, 15.0, 15.0, 24.0, 24.0},
{34.0, 24.0, 34.0, 24.0, 15.0, 15.0, 15.0, 24.0, 24.0},
{34.0, 24.0, 34.0, 24.0, 15.0, 15.0, 15.0, 24.0, 24.0},
{34.0, 24.0, 34.0, 24.0, 15.0, 15.0, 24.0, 24.0, 24.0},
{34.0, 24.0, 34.0, 24.0, 15.0, 15.0, 24.0, 24.0, 24.0},
{34.0, 24.0, 34.0, 24.0, 24.0, 24.0, 24.0, 24.0, 24.0},
{34.0, 24.0, 34.0, 24.0, 24.0, 24.0, 24.0, 24.0, 24.0},
{34.0, 24.0, 34.0, 24.0, 24.0, 24.0, 24.0, 24.0, 24.0},
{34.0, 24.0, 24.0, 24.0, 24.0, 24.0, 24.0, 24.0, 24.0},
{24.0, 24.0, 24.0, 24.0, 24.0, 24.0, 24.0, 24.0, 24.0},
{24.0, 24.0, 24.0, 24.0, 24.0, 24.0, 24.0, 24.0, 24.0},
{24.0, 24.0, 24.0, 24.0, 24.0, 24.0, 24.0, 24.0, 24.0},
{24.0, 24.0, 24.0, 24.0, 24.0, 24.0, 24.0, 24.0, 24.0},
{24.0, 24.0, 24.0, 24.0, 24.0, 24.0, 24.0, 24.0, 24.0},
{24.0, 24.0, 24.0, 24.0, 24.0, 24.0, 24.0, 24.0, 24.0},
{24.0, 24.0, 24.0, 24.0, 24.0, 24.0, 24.0, 24.0, 24.0},
{24.0, 24.0, 24.0, 24.0, 24.0, 24.0, 24.0, 24.0, 24.0},
{24.0, 24.0, 24.0, 24.0, 24.0, 24.0, 24.0, 24.0, 24.0},
{24.0, 24.0, 24.0, 24.0, 24.0, 24.0, 24.0, 24.0, 24.0},
{24.0, 24.0, 24.0, 24.0, 24.0, 24.0, 24.0, 24.0, 24.0},
{24.0, 24.0, 24.0, 24.0, 24.0, 24.0, 24.0, 24.0, 24.0},
{15.0, 24.0, 24.0, 24.0, 24.0, 24.0, 24.0, 24.0, 24.0},
{15.0, 24.0, 15.0, 24.0, 24.0, 24.0, 24.0, 24.0, 24.0},
{15.0, 15.0, 15.0, 24.0, 24.0, 24.0, 24.0, 24.0, 24.0},
{15.0, 15.0, 15.0, 34.0, 24.0, 24.0, 24.0, 24.0, 24.0},
{15.0, 15.0, 15.0, 34.0, 34.0, 24.0, 24.0, 24.0, 24.0},
{15.0, 15.0, 15.0, 34.0, 34.0, 34.0, 24.0, 24.0, 24.0},
{15.0, 15.0, 15.0, 34.0, 34.0, 34.0, 34.0, 24.0, 24.0},
{15.0, 15.0, 15.0, 34.0, 34.0, 34.0, 34.0, 34.0, 50.0},
{15.0, 15.0, 15.0, 34.0, 34.0, 34.0, 34.0, 34.0, 50.0},
{15.0, 15.0, 15.0, 34.0, 34.0, 34.0, 34.0, 34.0, 50.0},
{15.0, 15.0, 15.0, 34.0, 34.0, 34.0, 34.0, 34.0, 50.0},
{15.0, 15.0, 15.0, 34.0, 34.0, 34.0, 34.0, 34.0, 50.0},
{10.0, 15.0, 15.0, 34.0, 34.0, 34.0, 34.0, 50.0, 50.0},
{10.0, 15.0, 10.0, 34.0, 34.0, 34.0, 34.0, 50.0, 50.0},
{10.0, 10.0, 10.0, 34.0, 34.0, 34.0, 34.0, 50.0, 50.0},
{10.0, 10.0, 10.0, 34.0, 34.0, 34.0, 50.0, 50.0, 50.0},
{10.0, 10.0, 10.0, 34.0, 50.0, 34.0, 50.0, 50.0, 50.0},
{10.0, 10.0, 10.0, 34.0, 50.0, 34.0, 50.0, 50.0, 50.0},
{10.0, 10.0, 10.0, 34.0, 50.0, 34.0, 50.0, 50.0, 50.0},
{10.0, 10.0, 10.0, 34.0, 50.0, 50.0, 50.0, 50.0, 50.0},
{10.0, 10.0, 10.0, 34.0, 50.0, 50.0, 50.0, 50.0, 50.0},
{10.0, 10.0, 10.0, 34.0, 50.0, 50.0, 50.0, 50.0, 50.0},
{10.0, 10.0, 10.0, 34.0, 50.0, 50.0, 50.0, 50.0, 50.0},
{10.0, 10.0, 10.0, 34.0, 50.0, 50.0, 50.0, 50.0, 50.0},
{10.0, 10.0, 10.0, 34.0, 50.0, 50.0, 50.0, 50.0, 50.0}
};
#endif

static float wheeltemplates[2][OSC_LIMIT][WAVE_SIZE];
static float *tonewheel[OSC_LIMIT];
static int *wheelnumbers = &wi[36];
static float toneindeces[OSC_LIMIT];
#define NORMAL 0
#define BRIGHT 1
static float toneEQ[2][OSC_LIMIT];
static float busEQ[2][BUS_COUNT];
static float damping[2][OSC_LIMIT];
static float tonegains[OSC_LIMIT];

/*
 * Crosstalk defaults.
 */
#define XT_COUNT 9

/* Compartment members: */
#define XT_NEAR_1	0
#define XT_NEAR_2	1
#define XT_FAR_1	2
#define XT_FAR_2	3
/* Drawbar busses */
#define XT_DRAWBAR	4
/* Filter adjacencies */
#define XT_FILT_1	5
#define XT_FILT_2	6
#define XT_FILT_3	7
#define XT_FILT_4	8

/*
 * The following are for the different drawbar parameters. I would like to 
 * bring them into a single structure and will start with the structures for
 * the redefined keyclick code - nine busses, independent click and harmonic
 * startpoints.
 */
static float defct[2][XT_COUNT]
    = {{0.03, 0.05, 0.04, 0.2, 0.02, 0.07, 0.06, 0.03, 0.02},
       {0.05, 0.06, 0.02, 0.5, 0.02, 0.09, 0.07, 0.08, 0.6}};

static float defbg[9] =
	{0.0, 0.125, 0.25, 0.375, 0.5, 0.625, 0.750, 0.875, 1.0};

/*
 * These define the sample delays before the given bus makes contact, gives
 * a key click and starts mixing its harmonic.
 */
static struct DrawBar {
	float delay[2][BUS_COUNT];
	int delays[2][BUS_COUNT];
	float gain[2][BUS_COUNT];
	float invert[2][BUS_COUNT];
	float wave[2][BUS_COUNT];
	int pulse[2][BUS_COUNT];
	int tdelay[BUS_COUNT][OSC_LIMIT]; /* Target delay depending on velocity */
	int current[BUS_COUNT][OSC_LIMIT]; /* index into click buffer */
	int offset[OSC_LIMIT]; /* Current offset of note */
} drawbars;

