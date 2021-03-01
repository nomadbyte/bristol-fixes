
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

#ifndef DXOP_H
#define DXOP_H

#define DXOP_WAVE_SZE 1024
#define DXOP_WAVE_SZE_M 1023

#define STATE_DONE BRISTOL_KEYDONE /* 4 */
#define STATE_RELEASE BRISTOL_KEYOFF /* 2 */
#define STATE_START 10
#define STATE_ATTACK 11
#define STATE_DECAY 12
#define STATE_SUSTAIN 13
#define STATE_ATTACK2 14

#define BRISTOL_DXOFF 0x100

typedef struct BristolDXOP {
	bristolOPSpec spec;
	float *wave[8];
} bristolDXOP;

typedef struct BristolDXOPlocal {
	unsigned int flags;
	int cstate;
	float wtp;
	float note_diff;
	int tune_diff;
	float cgain;
	float egain;
} bristolDXOPlocal;

typedef struct Dxmix {
	float igain, pan, vol;
	unsigned int flags;
} dxmix;

#define DX_ALGO_M	0x1f
#define DX_ALGO_1	0
#define DX_ALGO_2	1
#define DX_ALGO_3	2
#define DX_ALGO_4	3
#define DX_ALGO_5	4
#define DX_ALGO_6	5
#define DX_ALGO_7	6
#define DX_ALGO_8	7

#define DX_LFO 1
#define DX_KEY 2
#define DX_IGC 4

#endif /* DXOP_H */

