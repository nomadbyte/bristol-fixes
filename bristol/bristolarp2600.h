
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

#define A_AR_MULTI			0x0001

#define ARP_OSC1_TRACKING	0x0002
#define ARP_OSC2_TRACKING	0x0004
#define ARP_OSC3_TRACKING	0x0008

#define A_AR_ON				0x0010

#define ARP_OSC1_LFO		0x0020
#define ARP_OSC2_LFO		0x0040
#define ARP_OSC3_LFO		0x0080

/*
 * These are the output indeces.
 */
#define ARP_O_PREAMP	0
#define ARP_O_RINGMOD	1
#define ARP_O_ENVFOLLOW	2
#define ARP_O_VCO1_SAW	3
#define ARP_O_VCO1_SQR	4
#define ARP_O_VCO2_TRI	5
#define ARP_O_VCO2_SIN	6
#define ARP_O_VCO2_SAW	7
#define ARP_O_VCO2_SQR	8
#define ARP_O_VCO3_TRI	9
#define ARP_O_VCO3_SIN	10
#define ARP_O_VCO3_SAW	11
#define ARP_O_VCO3_SQR	12
#define ARP_O_VCF		13
#define ARP_O_ADSR		14
#define ARP_O_AR		15
#define ARP_O_VCA		16
#define ARP_O_MIXER		17
#define ARP_O_NOISE		18
#define ARP_O_CLOCK		19
#define ARP_O_SH		20
#define ARP_O_SWITCH	21
#define ARP_O_MIX_OUT1	22
#define ARP_O_MIX_OUT2	23
#define ARP_O_MIX_1		24
#define ARP_O_MIX_2		25
#define ARP_O_MIX_2_2	26
#define ARP_O_LAG		27
#define ARP_O_PAN		28
#define ARP_O_IN_1		29 /* These are input into the emulator, ie, outputs */
#define ARP_O_IN_2		30 /* These are input into the emulator, ie, outputs */
#define ARP_O_IN_3		31 /* These are input into the emulator, ie, outputs */
#define ARP_O_IN_4		32 /* These are input into the emulator, ie, outputs */
#define ARP_O_KBD_CV	33 /* Other outputs PRIOR to this one! */
#define ARP_ZERO		34 /* Other outputs PRIOR to this one! */
#define ARP_OUTCNT		35 /* END OF OUTPUTS */
#define BRISTOL_ARP_BUFCNT 36

#define ARP_I_ENVFOLLOW 0
#define ARP_I_RM1		1
#define ARP_I_RM2		2
#define ARP_I_VCO1_KBD	3
#define ARP_I_VCO1_SH	4
#define ARP_I_VCO1_ADSR	5
#define ARP_I_VCO1_VCO2	6
#define ARP_I_VCO2_KBD	7
#define ARP_I_VCO2_SH	8
#define ARP_I_VCO2_ADSR	9
#define ARP_I_VCO2_VCO1	10
#define ARP_I_VCO2_NSE	11
#define ARP_I_VCO3_KBD	12
#define ARP_I_VCO3_NSE	13
#define ARP_I_VCO3_ADSR	14
#define ARP_I_VCO3_SIN	15
#define ARP_I_VCO3_TRI	16
#define ARP_I_FILT_RM	17
#define ARP_I_FILT_1SQR	18
#define ARP_I_FILT_2SQR	19
#define ARP_I_FILT_3TRI	20
#define ARP_I_FILT_NSE	21
#define ARP_I_FILT_KBD	22
#define ARP_I_FILT_ADSR	23
#define ARP_I_FILT_2SIN	24
#define ARP_I_ADSR_GATE	25
#define ARP_I_AR_GATE	26
#define ARP_I_VCA_GATE	27
#define ARP_I_VCA_TRIG	28
#define ARP_I_VCA_VCF	29
#define ARP_I_VCA_RM	30
#define ARP_I_VCA_AR	31
#define ARP_I_VCA_ADSR	32
#define ARP_I_MIX_VCF	33
#define ARP_I_MIX_VCA	34
#define ARP_I_MIX_FX	35
#define ARP_I_LEFT_IN	36
#define ARP_I_RIGHT_IN	37
#define ARP_I_SWITCH_1	38
#define ARP_I_SWITCH_2	39
#define ARP_I_SH_IN		40
#define ARP_I_CLOCK_IN	41
#define ARP_I_SWITCHCLK	42
#define ARP_I_PAN		43
#define ARP_I_MIX_1_1	44
#define ARP_I_MIX_1_2	45
#define ARP_I_MIX_2_1	46
#define ARP_I_LAG		47
#define ARP_I_MIX_1_3	48
#define ARP_I_MIX_1_4	49
#define ARP_I_MIX_2_2	50
#define ARP_I_OUT_1		51
#define ARP_I_OUT_2		52
#define ARP_I_OUT_3		53
#define ARP_I_OUT_4		54
#define ARP_INCOUNT		55 /* END OF INPUTS */

typedef struct arp2600Mod {
	float *buf;   /* stuff with some buffer pointer by GUI requests */
	float gain;   /* active gain */
	float cgain;   /* configured gain from GUI */
	float o1gain; /* these are scalers to adjust the level depending on param */
	float o2gain; /* these are scalers to adjust the level depending on param */
	int oactive;
} arp2600mod;

typedef struct arp2600Mods {
	unsigned int flags;

	int adsrAuto;
	int arAuto;
	int arAutoCount;
	float arAutoTransit;

	/*
	 * The above is for the axxe and most will disappear for the Odyssey.
	 *
	 * Most of this is mod routing, this will be a table of entries for each
	 * mod. These will need to be initialised such that the respective gains
	 * accord the levels of the given mod source when selected.
	 */
	float preamp;
	float pan;
	float volume;
	float initialvolume;
	float fxLpan;
	float fxRpan;
	float lfogain;
	arp2600mod mod[ARP_INCOUNT];
	float shkbd;
} arp2600mods;

