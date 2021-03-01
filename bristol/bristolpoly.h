
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

#define PWM_MASK	0x000001C
#define PWM_ENV		0x0000004
#define PWM_MG1		0x0000008
#define PWM_MG2		0x0000010
#define P_SHARE		0x0000020
#define P_STARTED	0x0000040
#define P_MULTI		0x0000080
#define P_DAMP		0x0000100
#define P_CHORD		0x0000200
#define P_UNISON	0x0000400
#define P_CHORD_OFF	0x0000800
#define P_REON		0x0001000

#define P_PWM_1		0x0001000
#define P_PWM_2		0x0002000
#define P_PWM_3		0x0004000
#define P_PWM_4		0x0008000

#define P_MOD_ENV	0x0010000
#define P_MOD_SING	0x0020000
#define P_MOD_XMOD	0x0040000
#define P_MOD_SYNC	0x0080000
#define P_EFFECTS	0x0100000

#define P_ARPEG_DOWN	0x0
#define P_ARPEG_UPDOWN	0x1
#define P_ARPEG_UP		0x2
#define P_ARPEG_DOWNUP	0x3

/*
 * Have to watch out here, mixflags are also used globally in the range
 * 0xffff0000.00000000 for midi ops, we can use the rest in the algo though.
 *
 * These need to go into some local structure for multiple instances
 * of the poly - malloc()ed into the baudio->mixlocals.
 */
typedef struct pMods {
	float pwmdepth;
	int nextosc;
	float mg1intensity;
	float bendintensity;
	int mg1dst;
	int benddst;
	void *mg1locals;
	void *mg2locals;
	int firstchord;
	int lastchord;
	int kcount;
	int arpegcount;
	int arpegtotal;
	int arpegstep;
	int arpegdir;
	float detune;
	float xmod;
	float mfreq;

	float osc0gain;
	float osc1gain;
	float osc2gain;
	float osc3gain;

	struct {
		float cFreq, dFreq, cFreqstep, oFreq, cFreqmult;
		int key, lastkey, keyhold;
	} keydata[4];
} pmods;

