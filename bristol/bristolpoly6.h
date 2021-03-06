
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

#define P6_PWM			0x0001
#define HAVE_STARTED	0x0002
#define HAVE_FINISHED	0x0004
#define P6_M_VCO		0x0008
#define P6_M_VCA		0x0010
#define P6_M_VCF		0x0020

typedef struct P6Mods {
	float lfo_fgain;
	unsigned int flags;
	void *lfolocals;
	void *lfo2locals;
	void *adsrlocals;
	float pwmlevel;
} p6mods;

