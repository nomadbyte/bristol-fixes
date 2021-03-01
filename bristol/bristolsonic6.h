
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

#define SONICLOCAL ((bSonic6 *) baudio->mixlocals)

/*
 * For the Sonic-6 globally
 */
#define MULTI_LFO		0x00000001
#define MULTI_NOISE		0x00000002

#define X_TRI			0x00000004
#define X_RAMP			0x00000008
#define X_SAW			0x00000010
#define X_SQUARE		0x00000020
#define X_WAVE_MASK		(X_TRI|X_RAMP|X_SQUARE|X_SAW) //0x0000003c
#define X_MOD_ADSR		0x00000040
#define X_MOD_WHEEL		0x00000080
#define X_MOD_MASTER	0x00000100
#define X_MOD_MASK		(X_MOD_ADSR|X_MOD_WHEEL|X_MOD_MASTER) //0x000001c0

#define Y_TRI			0x00000200
#define Y_RAMP			0x00000400
#define Y_SAW			0x00000800
#define Y_RAND			0x00001000
#define Y_WAVE_MASK		(Y_TRI|Y_RAMP|Y_RAND|Y_SAW) //0x00001e00
#define Y_MOD_ADSR		0x00002000
#define Y_MOD_WHEEL		0x00004000
#define Y_MOD_MASTER	0x00008000
#define Y_MOD_MASK		(Y_MOD_ADSR|Y_MOD_WHEEL|Y_MOD_MASTER) //0x0000e000

#define X_MULTI			0x00010000
#define Y_MULTI			0x00020000

#define RM_A			0x00040000
#define RM_XY			0x00080000

#define A_LFO			0x00100000
#define A_KBD			0x00200000

#define BYPASS			0x00400000

#define X_TRIGGER		0x00800000
#define Y_TRIGGER		0x01000000

typedef struct BSonic6 {
	float mastervolume;

	float genAB;
	float genXY;

	float directA;
	float directB;
	float directRM;

	float moda_xy;
	float moda_adsr;
	float modb_a;
	float modb_xy;
	float modb_pwm;

	float mix_ab;
	float mix_rm;
	float mix_ext;
	float mix_noise;

	float fmodadsr;
	float fmodxy;

	float xtrigstate;
	float ytrigstate;

	float *freqbuf;
	float *oscabuf;
	float *oscbbuf;
	float *lfoxbuf;
	float *lfoybuf;
	float *lfoxybuf;
	float *noisebuf;
	float *adsrbuf;
	float *zerobuf;
	float *filtbuf;
	float *rmbuf;
	float *outbuf;
	float *scratch;
} bSonic6;

