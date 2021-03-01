
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

#define BME700LOCAL ((bme700mods *) baudio->mixlocals)

#define BME700_VIBRA_ENV	0x00000001
#define BME700_VIBRA_TRI1	0x00000002
#define BME700_VIBRA_TRI2	0x00000004
#define BME700_PWM_TRI		0x00000008
#define BME700_PWM_DOUBLE	0x00000010

#define BME700_GLIDE		0x00000020

#define BME700_PWM_MAN		0x00000040
#define BME700_PWM_ENV		0x00000080

#define BME700_FILT_TRI		0x00000100
#define BME700_FILT_MOD1	0x00000200
#define BME700_FILT_KBD		0x00000400

#define BME700_TREM_MOD1	0x00000800
#define BME700_TREM_TRI		0x00001000

#define BME700_LFO1_MULTI	0x00002000
#define BME700_LFO2_MULTI	0x00004000
#define BME700_NOISE_MULTI	0x00008000

typedef struct BME700Mods {
	unsigned int flags;

	float volume;
	float oscmix;
	float resmix;
	float envmix1;
	float envmix2;
	float f_modmix;
	float a_modmix;
	float vibramix;
	float vibra;
	float pwm;
	float glide;

	float *lfo1_tri;
	float *lfo1_sqr;
	float *lfo2_tri;
	float *lfo2_sqr;
	float *freqbuf;
	float *pwmbuf;
	float *oscbuf;
	float *noisebuf;
	float *adsr1buf;
	float *adsr2buf;
	float *zerobuf;
	float *scratch;
	float *outbuf;
	float *resbuf;
	float *filtbuf;
} bme700mods;

