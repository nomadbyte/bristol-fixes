
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

#define A_LFO_MULTI	0x0001
#define A_LFO_TRILL	0x0002
#define A_VCO_DUAL	0x0004

#define O_OSC1_M1 0

typedef struct odysseyMod {
	float *buf;   /* stuff with some buffer pointer by GUI requests */
	float gain;   /* active gain */
	float cgain;   /* configured gain from GUI */
	float o1gain; /* these are scalers to adjust the level depending on param */
	float o2gain; /* these are scalers to adjust the level depending on param */
	int oactive;
} odysseymod;

typedef struct odysseyMods {
	unsigned int flags;

	void *lfolocals;

	float a_adsr;

	int adsrAuto;
	int arAuto;
	int arAutoCount;
	float arAutoTransit;

	/*
	 * The above is for the axxe and most will disappear for the Odyssey.
	 *
	 * Most of this is mod routing, this will be a table of entries for each
	 * mod. These will need to be initialised such that the respective gains
	 * accord the levels of the given mod source when selected.
	 */
	float a_gain;
	odysseymod mod[20];
	float shkbd;
	int low;
	int high;
	float dfreq;
	float dFreq;
	int last;
} odysseymods;

