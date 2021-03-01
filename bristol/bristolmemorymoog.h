
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

/* LFO Flags */
#define MMOOG_LFO_FM_OSC1	0x000001
#define MMOOG_LFO_FM_OSC2	0x000002
#define MMOOG_LFO_FM_OSC3	0x000004
#define MMOOG_LFO_PWM_OSC1	0x000008
#define MMOOG_LFO_PWM_OSC2	0x000010
#define MMOOG_LFO_PWM_OSC3	0x000020
#define MMOOG_LFO_FILT		0x000040

/* Poly Mod flags */
#define MMOOG_PM_FM_OSC1	0x000080
#define MMOOG_PM_FM_OSC2	0x000100
#define MMOOG_PM_PWM_OSC1	0x000200
#define MMOOG_PM_PWM_OSC2	0x000400
#define MMOOG_PM_FILT		0x000800
#define MMOOG_PM_CONTOUR	0x001000
#define MMOOG_PM_INVERT		0x002000

#define MMOOG_OSC_SYNC		0x004000

#define MMOOG_LFO_MULTI		0x008000
#define MMOOG_OSC3_KBD		0x010000

#define MMOOG_PEDAL_PITCH	0x020000
#define MMOOG_PEDAL_FILT	0x040000
#define MMOOG_PEDAL_VOL		0x080000
#define MMOOG_PEDAL_MOD		0x100000
#define MMOOG_PEDAL_OSC2	0x200000

typedef struct MmMods {
	int flags;
	int lfowaveform;
	float lfogain;
	float pmOsc3;
	float pmEnv;
	void *lfolocals;
	float osc1mix;
	float osc2mix;
	float osc3mix;
	float noisemix;
	float pedalamt1;
	float pedalamt2;
	float pan;
	float gain;
	float fenvgain;
} mmMods;

