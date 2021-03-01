
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

#define SIDLOCAL ((sidmods *) baudio->mixlocals)

#define AUD_SID			0
#define MOD_SID			1

#define _B_ATTACK		0
#define _B_DECAY		1
#define _B_SUSTAIN		2
#define _B_RELEASE		3

#define _V1_OFFSET		0
#define _V2_OFFSET		7
#define _V3_OFFSET		14

#define _SID_VOICES		3

#define B_SID_VOICE_ALL	-1

#define _SID_COUNT		2

#define B_S_KEYMODE_MONO	0
#define B_S_KEYMODE_POLY1	1 /* Poly-1 and Poly-2 use the same voice */
#define B_S_KEYMODE_POLY2	2 /* allocation algorithm */
#define B_S_KEYMODE_POLY3	3
#define B_S_KEYMODE_POLY4	4
#define B_S_KEYMODE_POLY5	5

#define S_MOD_LFO_V1_PW		0x00000001
#define S_MOD_LFO_V2_PW		0x00000002
#define S_MOD_LFO_V3_PW		0x00000004
#define S_MOD_LFO_V1_FREQ	0x00000008
#define S_MOD_LFO_V2_FREQ	0x00000010
#define S_MOD_LFO_V3_FREQ	0x00000020
#define S_MOD_LFO_FILTER	0x00000040

#define S_MOD_ENV_V1_PW		0x00010000
#define S_MOD_ENV_V2_PW		0x00020000
#define S_MOD_ENV_V3_PW		0x00040000
#define S_MOD_ENV_V1_FREQ	0x00080000
#define S_MOD_ENV_V2_FREQ	0x00100000
#define S_MOD_ENV_V3_FREQ	0x00200000
#define S_MOD_ENV_FILTER	0x00400000

#define S_MOD_T_LFO_RATE	0x02000000
#define S_MOD_T_LFO_DEPTH	0x04000000
#define S_MOD_T_ENV_DEPTH	0x08000000

#define S_MOD_NOISE			0x10000000
#define S_MOD_PITCH			0x20000000
#define S_MOD_LFO_DEPTH		0x40000000
#define S_MOD_ENV_DEPTH		0x80000000

/*
 * mods flags
 */
#define B_SID_E_GATE_V1		0x00000001
#define B_SID_E_GATE_V2		0x00000002
#define B_SID_E_GATE_V3		0x00000004
/* Arpeg flags */
#define B_SID_ARPEG_TRIG	0x01000000
#define B_SID_ARPEG_WAVE	0x02000000
#define B_SID_ARPEG_DOWN	0x04000000

/*
 * Arpeggiation tables
 */
#define B_SID_ARPEG_MAX 16

/*
 * Note logic
 */
#define B_SID_NEW	0x01
#define B_SID_TRIG	0x02
#define B_SID_REL	0x04
#define B_SID_RENEW	0x08

typedef struct SIDMods {
	unsigned int flags;
	int sidid[_SID_COUNT];
	unsigned char sidreg[_SID_COUNT][B_SID_REGISTERS];
	int pw[_SID_VOICES];
	float tune[_SID_VOICES];
	float transpose[_SID_VOICES];
	int glide[_SID_VOICES];
	int keymode;
	/* Arpeggiation */
	int arpegspeed;
	int arpegcount;
	int arpegcurrent;
	int arpegwave;
	unsigned short arpegtable[B_SID_ARPEG_MAX];
	/* Mods */
	float lfogain;
	float envgain;
	unsigned int modrouting;
	/*
	 * Note logic variables
	 */
	int ccount;
	int ckey[_SID_VOICES];
	float velocity[_SID_VOICES];
	float lvel;
	float cfreq[_SID_VOICES]; /* Current frequency */
	float dfreq[_SID_VOICES]; /* Destination frequency */
	float gliderate[_SID_VOICES]; /* From current to destination */
	unsigned char localmap[_SID_COUNT][128];
	/*
	int polykey[_SID_VOICES];
	unsigned char polykeymap[_SID_COUNT][128];
	 * Mod variables
	 */
	float noisegate;
	float noisesample;
	float modout;
	float detune;
} sidmods;

