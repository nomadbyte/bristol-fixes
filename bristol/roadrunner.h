
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

#define RR_OSC1_SAW	0x001
#define RR_OSC1_SQR	0x002
#define RR_OSC1_TRI	0x004

#define RR_OSC2_SAW	0x010
#define RR_OSC2_SQR	0x020
#define RR_OSC2_TRI	0x040

#define RR_OSC3_SAW	0x100
#define RR_OSC3_SQR	0x200
#define RR_OSC3_TRI	0x400

#define MULTI_LFO	0x1000

typedef struct roadrunnerMods {
	float pwm;
	float vibrato;
	float tremelo;
	int lowsplit;
	int highsplit;
} roadrunnermods;

