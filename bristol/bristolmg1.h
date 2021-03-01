
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

#define M_TRIG 0x0001
#define M_GATE 0x0002

typedef struct mg1Mods {
	unsigned int flags;
	float master;
	float osc1gain;
	float osc2gain;
	float noisegain;
	float tonemod;
	float filtermod;
	float filterenv;
	float arAutoTransit;
	int lfoselect;
	int vprocessed;
	int gated;

	void *lfolocals;
} mg1mods;

