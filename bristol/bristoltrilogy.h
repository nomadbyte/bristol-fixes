
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

#define TRILOGYLCL ((bTrilogy *) baudio->mixlocals)

/*
 * For the Trilogy
 */
#define MULTI_LFO		0x00000001
#define VCO_SYNC		0x00000002
#define VCO_TRILL		0x00000004
#define VCO_ALTERNATE	0x00000008

#define LFO_TRI			0x00000010
#define LFO_RAMP		0x00000020
#define LFO_SAW			0x00000040
#define LFO_SQUARE		0x00000080
#define LFO_WAVE_MASK	(LFO_TRI|LFO_RAMP|LFO_SQUARE|LFO_SAW) //0x000000f0

#define VCO_GLIDE_A		0x00000100
#define VCO_GLIDE_B		0x00000200
#define VCO_GLIDE_C		0x00000400
#define VCO_GLIDE_D		0x00000800
#define VCO_GLIDE_MASK	(VCO_GLIDE_A|VCO_GLIDE_B|VCO_GLIDE_C|VCO_GLIDE_D)

#define VCO_LEGATO		0x00001000

#define ALTERNATE_A		0x00002000
#define ALTERNATE_B		0x00004000

#define FILTER_PEDAL	0x00008000
#define GLIDE_LEGATO	0x00010000

#define TOUCH_SENSE		0x00020000

typedef struct BTrilogy {
	float mastervolume;

	float spaceorgan;
	float spacestring;

	float panorgan;
	float panstring;
	float pansynth;

	float mixorgan;
	float mixstring;
	float mixsynth;

	float env2filter;

	float lfoDelay;
	float lfoSlope;
	float lfoSlopeV;
	float lfoGain;

	float lfoRouting;
	float lfoRouteO;
	float lfoRouteF;
	float lfoRouteA;

	float lfoDelays[BRISTOL_MAXVOICECOUNT];
	float lfoGains[BRISTOL_MAXVOICECOUNT];

	float glideDepth;
	float glideRate;

	float glideCurrent[BRISTOL_MAXVOICECOUNT][2];
	float glideScaler;

	float controlVCO;
	float controlVCF;

	float *freqbuf;
	float *oscabuf;
	float *oscbbuf;
	float *lfobuf;
	float *adsrbuf;
	float *zerobuf;
	float *filtbuf;
	float *scratch;
	float *syncbuf;

	float *outbuf1;
	float *outbuf2;
} bTrilogy;

