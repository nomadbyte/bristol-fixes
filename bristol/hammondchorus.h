
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

#ifndef HCHORUS_H
#define HCHORUS_H

#define TAPS 9
#define HISTSIZE 32
#define MEMSIZE (HISTSIZE * TAPS)

typedef struct BristolHCHORUS {
	bristolOPSpec spec;
} bristolHCHORUS;

typedef struct BristolHCTap {
	float last;
	float out;
} bristolHCTap;

typedef struct BristolHCHORUSlocal {
	unsigned int flags;
	bristolHCTap phase[TAPS+1];
	float tap[TAPS+1];
	int tcount;
	int dir;
	int ctap;
	int clab;
	float ph;
	float g1;
	float g2;
	float g3;
	float g4;
	float g5;
	float g6;
	int mstart[TAPS+1];
	int mend[TAPS+1];
	int mcur[TAPS+1];
} bristolHCHORUSlocal;

#endif /* HCHORUS_H */

