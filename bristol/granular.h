
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

#define GRANULAR_OSC1_SAW	0x001
#define GRANULAR_OSC1_SQR	0x002
#define GRANULAR_OSC2_SAW	0x004
#define GRANULAR_OSC2_SQR	0x008
#define GRANULAR_OSC3_SAW	0x010
#define GRANULAR_OSC3_SQR	0x020
 */

#define MULTI_LFO		0x0001
#define KEY_TRACKING	0x0002

#define GRAIN_COUNT 1024

#define MOD_ROUTE_1 0x001
#define MOD_ROUTE_2 0x002
#define MOD_ROUTE_3 0x004
#define MOD_ROUTE_4 0x008
#define MOD_ROUTE_5 0x010
#define MOD_ROUTE_6 0x020
#define MOD_ROUTE_7 0x040
#define MOD_ROUTE_8 0x080
#define MOD_ROUTE_9 0x100

typedef struct GranularMods {
	unsigned int env1mod;
	unsigned int env2mod;
	unsigned int env3mod;
	unsigned int noisemod;
	unsigned int lfomod;
} granularmods;

