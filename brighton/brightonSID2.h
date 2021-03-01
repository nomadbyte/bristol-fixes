
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

typedef struct Sid2mod {
	float param[6];
	float touch;
} sid2mod;

typedef struct Sid2dest {
	float osc1PWM;
	float osc1Freq;
	float osc2PWM;
	float osc2Freq;
	float osc3PWM;
	float osc3Freq;
	float filter;
} sid2dest;

typedef struct Sid2Osc {
	float param[16];
} sid2Osc;

typedef struct Sid2Pmod {
	float opt1;
	float opt2;
	float opt3;
	float opt4;
	float opt5;
	float opt6;
	float opt7;
	sid2mod osc;
	sid2mod env;
	sid2mod prod;
	float ppad[7];
	sid2dest destbus[6];
} sid2Pmod;

typedef struct Sid2Voice {
	sid2Osc osc[3];
	float filter[8];
	float pan;
	float volume;
	sid2Pmod mod;
} sid2Voice;

/*
 * This will mimic the locations structure which holds GUI settings, then a
 * separate set of structures for the poly voice programming.
58 + 9 + 27 + 29 + 7 + 28 + 42 + 24 + 5 + 8 + 16 + 1
 */
typedef struct Sid2Mem {
	sid2Voice guivoice;
	float mode[9]; // Break these out
	float mmods1[27];
	float mmods2[29];
	float pad2[23];
	float version;
	float modShadow[5];
	float globals[7];
	float masterVolume;
	float memMidi[16];
	float display;
	/*
	 * These are the actual voice memories
	 */
	sid2Voice vmem[5];
} sid2Mem;

