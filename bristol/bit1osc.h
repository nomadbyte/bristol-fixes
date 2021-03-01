
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

#ifndef BITONE_H
#define BITONE_H

#define BITONE_WAVE_SZE 1024
#define BITONE_WAVE_SZE_M 1023

#define BITONE_WAVE_PWMIN 50
#define BITONE_WAVE_PWMAX 975

#define BITONE_WAVE_GAIN 12

#define BITONE_H_SZE	512.0f

typedef struct BristolBITONE {
	bristolOPSpec spec;
	float *wave[8];
	float *sbuf;
	float *null;
	int volumes[16];
	int tsin, ttri;
} bristolBITONE;

typedef struct BristolBITONElocal {
	unsigned int flags;
	float wtp;
	float lsv;
	float ssg;
	float wtppw1;
	float wtppw2;
	float wtppw3;
	float wtppw4;
	float wtppw5;
} bristolBITONElocal;

#endif /* BITONE_H */

