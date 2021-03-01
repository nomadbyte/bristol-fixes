
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

/*
 * For the explorer globally
 */
#define MIX_OSC1		0x001
#define MIX_OSC2		0x002
#define MIX_OSC3		0x004
#define MIX_EXT			0x008
#define MIX_NOISE		0x010
#define OSC3_TRACKING	0x020
#define OSC3_LFO		0x040
#define MULTI_LFO		0x080
#define SYNC_1_2		0x100
#define FM_3_1			0x200
#define FILTER_MODE		0x400

/*
 * For the two mod busses
 *
 * Mod Sources
 */
#define MOD_MASK		0x0ff
#define MOD_TRI			0x001
#define MOD_SQU			0x002
#define MOD_SH			0x004
#define MOD_OSC3		0x008
#define MOD_EXT			0x010
#define MOD_SHS			0x020
/*
 * Mod shaping
 */
#define SHAPE_MASK		0x0ff00
#define SHAPE_OFF		0x00100
#define SHAPE_KEY		0x00200
#define SHAPE_FILTENV	0x00400
#define SHAPE_ON		0x00800
/*
 * Mod destinations
 */
#define DEST_MASK		0x0fff0000
#define OSC_1_MOD		0x00010000
#define OSC_2_MOD		0x00020000
#define OSC_3_MOD		0x00040000
#define FILTER_MOD		0x00080000
#define FSPACE_MOD		0x00100000
#define WAVE_1_MOD		0x00200000
#define WAVE_2_MOD		0x00400000
#define WAVE_3_MOD		0x00800000

typedef struct Modbus {
	unsigned int flags;
	float gain;
} modbus;

typedef struct BExp {
	modbus bus[2];
	float fenvgain;
	float noisegain;
} bExp;

