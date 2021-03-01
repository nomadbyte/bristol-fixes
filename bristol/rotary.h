
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

#ifndef ROTARY_H
#define ROTARY_H

#define LESLIE_CLICK 0x0001
#define LESLIE_TBL_SZE 4096

/*
#define TAP1 7145
#define TAP2 7953
#define TAP3 8031
#define TAP4 8169
*/
#define TAP1 3573
#define TAP2 3467
#define TAP3 4015
#define TAP4 4089

#define TABSIZE 360
#define HISTSIZE 8192

typedef struct BristolLESLIE {
	bristolOPSpec spec;
} bristolLESLIE;

typedef struct BristolLESLIElocal {
	unsigned int flags;
	float Vol1;
	float Vol2;
	float Freq1;
	float Histin;
	float Histout;
	float Revout;
	float *tbuf;
	float scanp;
	float scanr;
	float a[8];
	float crossover;
	int nSpeed;
	int inertia;
	float tinc;
	int toff;
} bristolLESLIElocal;

#endif /* ROTARY_H */

