
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

#ifndef TRILOGY_H
#define TRILOGY_H

#define TRILOGY_WAVE_SZE 1024
#define TRILOGY_WAVE_SZE_M 1023
#define TRILOGY_WAVE_GAIN 12
#define TRILOGY_H_SZE	512.0f

typedef struct BristolTRILOGY {
	bristolOPSpec spec;
	float *wave[8];
	int volumes[16];
} bristolTRILOGY;

typedef struct BristolTRILOGYlocal {
	float wtp;
	float lsv;

	float wtppw1;
	float wtppw2;
	float wtppw3;
	float wtppw4;
	float wtppw5;

	float wtppw11;
	float wtppw12;
	float wtppw13;
	float wtppw14;
	float wtppw15;
} bristolTRILOGYlocal;

#endif /* TRILOGY_H */

