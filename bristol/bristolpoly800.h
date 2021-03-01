
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

#define P800LOCAL ((p800mods *) baudio->mixlocals)

#define P800_PWM		0x0001
#define HAVE_STARTED	0x0002
#define HAVE_FINNISHED	0x0004

#define P800_M_VCO		0x0010
#define P800_M_VCA		0x0020
#define P800_M_VCF		0x0040

#define P800_DOUBLE		0x0080
#define P800_LFOMULTI	0x0100
#define P800_FILTMULTI	0x0200
#define P800_POLARITY	0x0400
#define P800_SYNC		0x0800

typedef struct P800Mods {
	unsigned int flags;
	int voicecount;
	int maxkey;
	bristolVoice *maxvoice;
	float volume;
	float bend;
	float envamount;
	float lfodelay;
	float lforate;
	float lfodelays[BRISTOL_MAXVOICECOUNT];
	float lfogains[BRISTOL_MAXVOICECOUNT];
	float vcomod;
	float vcfmod;
	float mod2vcf;
	float mod2vco;
	float mod2vcodepth;
	float dco1pwm;
	float dco2pwm;
	float *freqbuf;
	float *oscabuf;
	float *oscbbuf;
	float *syncbuf;
	float *pwmbuf;
	float *adsrbuf;
	float *outbuf;
	float *zerobuf;
	float *noisebuf;
	float *filtbuf;
	float *lfobuf;
	float *scratch;
} p800mods;

