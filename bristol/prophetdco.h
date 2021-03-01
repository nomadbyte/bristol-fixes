
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

#ifndef PROPHETDCO_H
#define PROPHETDCO_H

#define PROPHETDCO_WAVE_SZE		1024
#define PROPHETDCO_WAVE_SZE_M	1023
/* These are for PWM extremes */
#define PROPHETDCO_PW_MIN		50
#define PROPHETDCO_PW_MAX		975

#define PROPHETDCO_SYNC		0x01
#define PROPHETDCO_REWAVE	0x02

#define S_DEC 0.999f

typedef struct BristolPROPHETDCO {
	bristolOPSpec spec;
	float *wave[8];
	unsigned int flags;
} bristolPROPHETDCO;

typedef struct BristolPROPHETDCOlocal {
	unsigned int flags;
	float wtp;
	float cpwm;
	float note_diff;
	float lsv;
	float ssg;
	int tune_diff;
	float *wave0, *wave1, *wave2;
} bristolPROPHETDCOlocal;

#endif /* PROPHETDCO_H */

