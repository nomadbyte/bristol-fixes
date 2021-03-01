
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

#ifndef LFO_H
#define LFO_H

#define LFO_WAVE_SZE 16384

typedef struct BristolLFO {
	bristolOPSpec spec;
	float *wave[8];
} bristolLFO;

typedef struct BristolLFOlocal {
	unsigned int flags;
	float wtp;
	float sandh;
	float note_diff;
	int tune_diff;
} bristolLFOlocal;

#endif /* LFO_H */

