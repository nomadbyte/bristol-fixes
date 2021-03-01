
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

/*
 * Global structures for the bandwidth limited waveform support.
 */

#ifndef _BRISTOL_BLO_H
#define _BRISTOL_BLO_H

#define BRISTOL_BLO_SIZE 1024

#define BRISTOL_BLO 0x01
#define BRISTOL_LWF 0x02

#define BLO_RAMP	1
#define BLO_SAW		2
#define BLO_TRI		3
#define BLO_SQUARE	4
#define BLO_SINE	5
#define BLO_COSINE	6
#define BLO_PULSE	7

struct {
	int flags;
	int harmonics;
	int cutin;
	float cutoff;
	int min;
	float fraction;
	float samplerate;
} blo;

extern float blosine[BRISTOL_BLO_SIZE];
extern float blosquare[BRISTOL_BLO_SIZE];
extern float bloramp[BRISTOL_BLO_SIZE];
extern float blosaw[BRISTOL_BLO_SIZE];
extern float blotriangle[BRISTOL_BLO_SIZE];
extern float blopulse[BRISTOL_BLO_SIZE];

extern void generateBLOwaveforms(int, float, int, float, int, float, int);
extern void generateBLOwaveform(int, float *, float, int);
extern void generateBLOwaveformF(float, float *, int);
extern int bristolBLOcheck(float);

#endif /* _BRISTOL_BLO_H */

