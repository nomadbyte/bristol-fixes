
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

#ifndef ARPDCO_H
#define ARPDCO_H

#define ARPDCO_WAVE_SZE 1024
#define ARPDCO_WAVE_SZE_M 1023
#define ARPDCO_WAVE_PWMIN 50
#define ARPDCO_WAVE_PWMAX 975
#define ARPDCO_SYNC 0x01

typedef struct BristolARPDCO {
	bristolOPSpec spec;
	float *wave[8];
} bristolARPDCO;

typedef struct BristolARPDCOlocal {
	unsigned int flags;
	float wtp;
	float cpwm;
	float note_diff;
	float lsv;
	float ssg;
	int tune_diff;
} bristolARPDCOlocal;

#endif /* ARPDCO_H */

