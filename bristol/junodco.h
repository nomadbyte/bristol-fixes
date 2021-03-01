
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

#ifndef JUNODCO_H
#define JUNODCO_H

#define JUNODCO_WAVE_SZE 1024

typedef struct BristolJUNODCO {
	bristolOPSpec spec;
	float *wave[8];
} bristolJUNODCO;

typedef struct BristolJUNODCOlocal {
	unsigned int flags;
	float wtp;
	float subw;
	float cpwm;
	float note_diff;
	int tune_diff;
} bristolJUNODCOlocal;

#endif /* JUNODCO_H */

