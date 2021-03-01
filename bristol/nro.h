
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

#define BRISTOL_TEND_SQUARE 0
#define BRISTOL_TEND_SAW 	1
#define BRISTOL_TEND_TRI	2
#define BRISTOL_TEND_SINE 	3
#define BRISTOL_TEND_RAMP 	4
#define BRISTOL_TEND_SQR2 	5

#define TEND_STATE_RAMP_1	0x01
#define TEND_STATE_RAMP_2	0x02
#define TEND_STATE_RAMP_3	0x04
#define TEND_STATE_RAMP_4	0x08

#define TENDENCY_CONV 6
#define TENDENCY_COUNT 21 /* 20 audible and one sync */
#define TENDENCY_MAX (TENDENCY_COUNT - 1)

#define TEND_FORMAT_P8	0
#define TEND_FORMAT_BME	1

#define TEND_INITED		0x01

/*
 * We will need a lot of tendencies, one per waveform per oscillator per voice,
 * ie, a unique tendency for each waveform being generated at any given point.
 * We need to separate out the specific requirements from the generic ones to
 * minimise memory requirements. conv() and gen() are generic, damp, ramp and
 * drain are generic, sr and sc are generic.
 * The limits and form are specific.
 */
typedef struct BristolNROlocal {
	unsigned int seq;
	unsigned short flags;
	unsigned short form;
	float climit; /* Current target */
	float target;
	float ls; /* Last sample generated */
	float cfc; /* Current frequency counter */
	float pw; /* Pulse width */
	float lsv; /* Last sample value for sync */
	float gn;
	float tr; /* transpose; */
} bristolNROlocal;

typedef struct BristolNRO {
	bristolOPSpec spec;
	unsigned int seq;
	float ulimit; /* upper target */
	float llimit; /* lower target */
	float vpo;	/* 'volts' per octave */
	float damp;
	float ramp; /* Square/Saw/Ramp leading edge */
	float drain; /* factor back to zero */
	float sr; /* sample rate */
	float sc; /* sample count */
	void (*conv[TENDENCY_CONV])(float *, float *, float, float, int);
	void (*gen[TENDENCY_CONV])(struct BristolNRO *, bristolNROlocal *, float *, float *, float *, float *, int);
	float *fbm;
	float *pwmb;
	float *fb;
	float *sb;
	float *zb;
} bristolNRO;

