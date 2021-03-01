
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

#ifndef HAMMOND_H
#define HAMMOND_H

#define HAMMOND_WAVE_SZE 1024
#define HAMMOND_WAVE_GAIN 8

#define CLICK_STAGE2 0x01
#define CLICK_STAGE3 0x02
#define CLICK_STAGE4 0x04

typedef struct BristolHAMMOND {
	bristolOPSpec spec;
	float *wave[8];
	int volumes[16];
	int clickoffset;
} bristolHAMMOND;

typedef struct BristolHAMMONDlocal {
	unsigned int flags;
	float wtp;
} bristolHAMMONDlocal;

#endif /* HAMMOND_H */

