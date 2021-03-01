
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

#ifndef SDCO_H
#define SDCO_H

#define SDCO_WAVE_SZE 1024
#define SDCO_WAVE_SZE_M 1023

#define LAYER_COUNT 2

typedef struct BristolSDCO {
	bristolOPSpec spec;
/*	float *wave[8]; */
} bristolSDCO;

typedef struct BristolSDCOlocal {
	unsigned int flags;
	float wtp;
	float wtptr[LAYER_COUNT];
	float note_diff;
	int tune_diff;
} bristolSDCOlocal;

#define LOOP_NONE -1
#define LOOP_JUMP 0

#define FILL_NEAREST 0
#define FILL_MIXED 1

#define RANGE_PIANO 0
#define RANGE_FORTE 1

#define RANGE_UP 0
#define RANGE_DOWN 1

#define HINT_UNKNOWN -1
#define HINT_RAW 0

#define HINT_RHODES 256

#define GAIN_ZERO 0
#define GAIN_NEG 1
#define GAIN_POS 2
#define GAIN_CONSTANT 3

/*
 * We are going to have multiple layers, 8 in total ala the Ensoniq ASR-10.
 * The Sample layers are grouped in the sampleData structure, and each midi
 * note has one entry.
 */
typedef struct Sample {
	float *wave;
	int nsr; /* Native sample rate, normalised to 44100 sps */
	int count;
	int lnr; /* Loop nearpoint reference */
	int lfr; /* Loop farpoint reference - loop back to nearpoint */
	int lt; /* loop type */
	int refwave; /* Refer sample and loop information to this sample entry */
	float sr;
	int fade; /* GAIN_CONSTANT, GAIN_POS or GAIN_NEG. */
} sample;

typedef struct SampleData {
	sample layer[LAYER_COUNT]; /* layer 0 will be a piano layer, 1 a forte layer */
} sampleData;

#endif /* SDCO_H */

