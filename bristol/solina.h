
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

#define SOLINA_OSC1_SAW	0x001
#define SOLINA_OSC1_SQR	0x002
#define SOLINA_OSC2_SAW	0x004
#define SOLINA_OSC2_SQR	0x008
#define SOLINA_OSC3_SAW	0x010
#define SOLINA_OSC3_SQR	0x020
#define SOLINA_OSCGLIDE	0x040

#define MULTI_LFO		0x0100

typedef struct solinaMods {
	float pwm;
	float vibrato;
	float tremelo;
	float glide;
	int lowsplit;
	int highsplit;
} solinamods;

