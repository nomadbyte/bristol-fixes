
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

#define MIX_OSC1		1
#define MIX_OSC2		2
#define MIX_OSC3		4
#define OSC3_TRACKING	8
#define OSC_1_MOD		0x10
#define OSC_2_MOD		0x20
#define FILTER_MOD		0x40
#define FILTER_TRACKING	0x80
#define MIX_EXT			0x100
#define MIX_NOISE		0x200
/*
 * This is a kind of global mixflag
#define MULTI_TRIG		0x10000000
 */

extern int buildCurrentTable(Baudio *, float);

typedef struct MMods {
	float mixnoise;
	float mixosc3;
	float mixmod;
	float lsample;
	float csample;
} mMods;

