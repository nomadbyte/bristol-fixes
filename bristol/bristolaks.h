
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

#define AKS_OSC1_TRACKING	0x0002
#define AKS_OSC2_TRACKING	0x0004
#define AKS_OSC3_TRACKING	0x0008

#define A_AR_ON				0x0010

/*
 * These are the output indeces.
 */
#define AKS_O_CH1		0
#define AKS_O_CH2		1
#define AKS_O_OSC1		2
#define AKS_O_OSC2		3
#define AKS_O_OSC3_SQR	4
#define AKS_O_OSC3_TRI	5
#define AKS_O_NOISE		6
#define AKS_O_INPUT_1	7
#define AKS_O_INPUT_2	8
#define AKS_O_VCF		9
#define AKS_O_ADSR		10
#define AKS_O_VCA		11
#define AKS_O_RM		12
#define AKS_O_REVERB	13
#define AKS_O_H			14
#define AKS_O_V			15
#define AKS_O_METER		16
#define AKS_O_LEFT		17
#define AKS_O_RIGHT		18
#define AKS_O_TRIGGER	19
#define AKS_OUTCOUNT	20 /* END OF OUTPUTS */
#define BRISTOL_AKS_BUFCNT 20

#define AKS_I_CH1		0
#define AKS_I_METER		1
#define AKS_I_CH2		2
#define AKS_I_ENVELOPE	3
#define AKS_I_RM1		4
#define AKS_I_RM2		5
#define AKS_I_REVERB	6
#define AKS_I_FILTER	7
#define AKS_I_OSC1		8
#define AKS_I_OSC2		9
#define AKS_I_OSC3		10
#define AKS_I_DECAY		11
#define AKS_I_REVERBMIX	12
#define AKS_I_VCF		13
#define AKS_I_CH1_LVL	14
#define AKS_I_CH2_LVL	15
#define AKS_INCOUNT		16 /* END OF INPUTS */

typedef struct aksMod {
	float *buf;   /* stuff with some buffer pointer by GUI requests */
	float gain;   /* active gain */
	float cgain;   /* configured gain from GUI */
} aksmod;

typedef struct aksMods {
	unsigned int flags;

	/*
	 * The above is for the axxe and most will disappear for the Odyssey.
	 *
	 * Most of this is mod routing, this will be a table of entries for each
	 * mod. These will need to be initialised such that the respective gains
	 * accord the levels of the given mod source when selected.
	 */
	float hrange, vrange;
	float lingain, ringain;
	float routfilt, loutfilt;
	/* Channel stuff */
	float chan1gain, chan1pan;
	float chan2gain, chan2pan;
	int chan1route, chan2route;
	float fs1, fs2;
	float signal, trapezoid;
} aksmods;

