
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

#ifndef __BRISTOL_ARPEG_H
#define __BRISTOL_ARPEG_H

/*
 * The access method for arpeggiator, sequencer and chording is the same, it
 * uses operator 125.
 */
#define BRISTOL_ARPEGGIATOR 64

/*
 * Commands to the arpeggiator code. Poly-2 is a special case done for the 
 * Jupiter8. All notes always sound (done with Unison mode) and the arpeggiator
 * spreads the available voices over the notes pressed.
 */
#define BRISTOL_ARPEG_ENABLE	0
#define BRISTOL_ARPEG_DIR		1
#define BRISTOL_ARPEG_RANGE		2
#define BRISTOL_ARPEG_RATE		3
#define BRISTOL_ARPEG_CLOCK		4
#define BRISTOL_ARPEG_TRIGGER	5
#define BRISTOL_ARPEG_POLY_2	6 /* Only works with Unison/Arpeg enabled */

#define BRISTOL_CHORD_ENABLE	16
#define BRISTOL_CHORD_RESEQ		17 /* Relearn chord */
#define BRISTOL_CHORD_KEY		18 /* note sent from GUI memory */

#define BRISTOL_SEQ_ENABLE		32
#define BRISTOL_SEQ_DIR			33
#define BRISTOL_SEQ_RANGE		34
#define BRISTOL_SEQ_RATE		35
#define BRISTOL_SEQ_CLOCK		36
#define BRISTOL_SEQ_TRIGGER		37
#define BRISTOL_SEQ_RESEQ		38 /* Relearn sequence */
#define BRISTOL_SEQ_KEY			39 /* note sent from GUI memory */

/* The sequencer uses the same direction flags */
#define BRISTOL_ARPEG_DOWN	0
#define BRISTOL_ARPEG_UD	1
#define BRISTOL_ARPEG_UP	2
#define BRISTOL_ARPEG_RND	3

/*
 * This is the real number of steps we can keep.
 */
#define BRISTOL_SEQ_MIN			4
#define BRISTOL_CHORD_MAX		8
#define BRISTOL_ARPEG_MAX		128
#define BRISTOL_SEQ_MAX			256

/*
 * Some flags for runtime stuff
 */
#define BRISTOL_SEQ_LEARN	0x0001
#define BRISTOL_CHORD_LEARN	0x0002
#define BRISTOL_A_TRIGGER	0x0004 /* Currently internal only */
#define BRISTOL_A_CLOCK		0x0008
#define BRISTOL_S_TRIGGER	0x0010 /* Currently internal only */
#define BRISTOL_S_CLOCK		0x0020
#define BRISTOL_POLY_2		0x0040 /* All notes always on, split between keys */
#define BRISTOL_REQ_TRIGGER	0x0080
#define BRISTOL_DONE_FIRST	0x0100

/*
 * They key id and velocity are stored when learning from a keyboard however
 * this is not honoured as velocity is inherited from the from the voice, not
 * from the arpeggiator. Also, when programmed internally they only have a key
 * id and velocity is fixed. Changes to that are FFS.
 */
typedef struct ArpNote {
	int k, v;
} arpNote;

typedef struct ArpSeq {
	int rate; /* Always configured */
	int span; /* Configured number of octaves to cover */
	int dir; /* Configured sequence direction */
	int max;	/* Total number of notes in the sequence (config/learnt) */
	int count; /* current scan through the rate */
	int step; /* going up or down through the sequence */
	//int d_offset; /* sample accurate correction for note events */
	int dif; /* Note difference - for sequencer only */
	int vdif; /* velocity difference - for sequencer only */
	int current; /* current note index */
	int octave; /* current octave */
	arpNote notes[BRISTOL_SEQ_MAX + 1];
} arpSeq;

/*
 * This gets buried into the baudio structure for the emulation
 */
typedef struct Arpeggiator {
	unsigned int flags;
	/*
	 * These is for arpeggiating, sequencing and chording respectively. The
	 * chording uses very few parameters and also far fewer notes than in
	 * this structure however it is cleaner this way.
	 */
	arpSeq a;
	arpSeq s;
	arpSeq c;
} arpeggiator;

/*
 * This typedef is used by the GUI so that it can bury the settings into a 
 * normally formatted bristol memory in the sequencer directory and have the
 * synth reload it automatically.
 *
 * Stored notes from the GUI do not carry velocity.
 */
#define BRISTOL_AM_SMAX 0
#define BRISTOL_AM_CCOUNT 1
typedef struct ArpeggiatorMemory {
	float s_max;
	float c_count;
	float s_dif;
	float c_dif;
	float sequence[BRISTOL_SEQ_MAX + 1];
	float chord[BRISTOL_CHORD_MAX + 1];
} arpeggiatorMemory;

#endif

