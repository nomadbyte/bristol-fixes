
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
/*#define DEBUG */

#include <math.h>
#include "bristol.h"
#include "bristolmm.h"

extern void mapVelocityCurve(int, float []);

/*
 * Use of these jupiter global buffers will be an issue with use of multiple
 * audio threads, unless we ensure a single thread deals with any given algo
 * type, since then they are only used sequentially.
 */
static float *freqbuf = (float *) NULL;
static float *adsr1buf = (float *) NULL;
static float *adsr2buf = (float *) NULL;
static float *filtbuf = (float *) NULL;
static float *dco1buf = (float *) NULL;
static float *dco2buf = (float *) NULL;
static float *zerobuf = (float *) NULL;
static float *outbuf = (float *) NULL;
static float *lfo1buf = (float *) NULL;
static float *lfo2buf = (float *) NULL;
static float *noisebuf = (float *) NULL;
static float *scratchbuf = (float *) NULL;
static float *pwmbuf = (float *) NULL;
static float *vcofmbuf = (float *) NULL;
static float *modbuf = (float *) NULL;
static float *syncbuf = (float *) NULL;

/*
 * These need to go into some local structure for multiple instances
 * of the jupiter - malloc()ed into the baudio->mixlocals.
 */
#define JUPITER_LOWER_LAYER	0x00000001
#define JUPITER_NOISE_UNI	0x00000002
#define JUPITER_LFO1_UNI	0x00000004
#define JUPITER_LFO1_MASK	0x000000F0
#define JUPITER_LFO1_TRI	0x00000010
#define JUPITER_LFO1_RAMP	0x00000020
#define JUPITER_LFO1_SQR	0x00000040
#define JUPITER_LFO1_SH		0x00000080
#define JUPITER_SYNC_2_2_1	0x00000100
#define JUPITER_ENV_INV		0x00000200
#define JUPITER_FM			0x00000400
#define JUPITER_NO_DCO1		0x00000800
#define FREQ_MOD_VCO1		0x00001000
#define FREQ_MOD_VCO2		0x00002000
#define JUPITER_PWM_LFO1	0x00004000
#define JUPITER_PWM_MAN		0x00008000
#define JUPITER_PWM_ENV1	0x00010000
#define JUPITER_VCF_ENV1	0x00020000
#define JUPITER_PWM_DCO1	0x00040000
#define JUPITER_PWM_DCO2	0x00080000
#define JUPITER_SYNC_1_2_2	0x00100000
#define JUPITER_XMOD_ENV	0x00200000
#define JUPITER_MOD_ENABLE	0x00400000

typedef struct bMods {
	unsigned int flags;
	float *lout;
	float *rout;
	int voicecount;
	float lpan;
	float rpan;
	float gain;
	float lmix;
	float rmix;
	float xmod;
	float noisegain;
	float dcoLFO1FM;
	float dcoEnv1FM;
	float pw;
	float vcfEnv;
	float vcfLFO;
	float vcaEnv;
	float vcaLFO;
	float bendVco1;
	float bendVco2;
	float bendVcf;
	float lfo2vco;
	float lfo2vcf;
	float *dco2buf[BRISTOL_VOICECOUNT];
} bmods;

int
jupiterController(Baudio *baudio, u_char operator,
u_char controller, float value)
{
	int tval = value * CONTROLLER_RANGE;

#ifdef DEBUG
	printf("bristolupiterControl(%i, %i, %f)\n", operator, controller, value);
#endif

	if (operator != 126)
		return(0);

	switch (controller) {
		case 0:
			baudio->glide = value * value * baudio->glidemax;
			break;
		case 1:
			baudio->gtune = 1.0
				+ (baudio->note_diff - 1)
				* (value * 2 - 1);

			buildCurrentTable(baudio, baudio->gtune);
			alterAllNotes(baudio);
			break;
		case 2:
			/*
			 * When we go into Uni mode then configure 6 voices on this layer
			 * otherwise just three.
			 */
			if (tval == 2)
				baudio->voicecount = 1;
			else if (tval == 1)
				baudio->voicecount = ((bmods *) baudio->mixlocals)->voicecount;
			else if (((bmods *) baudio->mixlocals)->flags & JUPITER_LOWER_LAYER)
				baudio->voicecount =
					((bmods *) baudio->mixlocals)->voicecount >> 1;
			else
				baudio->voicecount = ((bmods *) baudio->mixlocals)->voicecount;
			break;
		case 3:
			((bmods *) baudio->mixlocals)->gain = value * 0.01625;
			break;
		case 4:
			((bmods *) baudio->mixlocals)->lpan = (1.0 - value);
			((bmods *) baudio->mixlocals)->rpan = value;
			/*
			if (tval == 0)
				value += 1.0f/CONTROLLER_RANGE;
			else if (tval == CONTROLLER_RANGE)
				value -= 1.0f/CONTROLLER_RANGE;
				
			((bmods *) baudio->mixlocals)->lpan =
				-log10f(sinf(M_PI/2.0f * value));
			((bmods *) baudio->mixlocals)->rpan =
				-log10f(cosf(M_PI/2.0f * value));
			 */
			break;
		case 5:
			((bmods *) baudio->mixlocals)->noisegain = value;
			break;
		case 6:
			/*
			 * This is a correct midi paninng calculation roughly as taken from
			 * MMA corrective notes for stereo panning. It does not work
			 * too well for an L/R mix though as it applies a constant power 
			 * algorithm and gets very unbalanced at full throws.
			 *
			if (tval == 0)
				value += 1.0f/CONTROLLER_RANGE;
			else if (tval == CONTROLLER_RANGE)
				value -= 1.0f/CONTROLLER_RANGE;
				
			((bmods *) baudio->mixlocals)->lmix = 20 *
				log10f(cosf(M_PI/2.0f * value));
			((bmods *) baudio->mixlocals)->rmix = 20 *
				log10f(sinf(M_PI/2.0f * value));

			printf("mix now %f/%f\n",
				((bmods *) baudio->mixlocals)->lmix,
				((bmods *) baudio->mixlocals)->rmix);
			 */

			((bmods *) baudio->mixlocals)->lmix = (1.0 - value) * 5;
			((bmods *) baudio->mixlocals)->rmix = value * 5;
			break;
		case 7:
			if (value == 0)
				baudio->mixflags &= ~BRISTOL_V_UNISON;
			else
				baudio->mixflags |= BRISTOL_V_UNISON;
			break;
		case 8:
			((bmods *) baudio->mixlocals)->xmod = value * 0.4;
			break;
		case 9:
			((bmods *) baudio->mixlocals)->dcoLFO1FM = value * 0.05;
			break;
		case 10:
			((bmods *) baudio->mixlocals)->dcoEnv1FM = value;
			break;
		case 11:
			switch (tval) {
				case 0:
					((bmods *) baudio->mixlocals)->flags &= ~FREQ_MOD_VCO1;
					((bmods *) baudio->mixlocals)->flags |= FREQ_MOD_VCO2;
					break;
				case 2:
					((bmods *) baudio->mixlocals)->flags |= FREQ_MOD_VCO1;
					((bmods *) baudio->mixlocals)->flags &= ~FREQ_MOD_VCO2;
					break;
				default:
					((bmods *) baudio->mixlocals)->flags |= FREQ_MOD_VCO1;
					((bmods *) baudio->mixlocals)->flags |= FREQ_MOD_VCO2;
			}
			break;
		case 12:
			if (value == 0)
				((bmods *) baudio->mixlocals)->flags &= ~JUPITER_LFO1_TRI;
			else {
				((bmods *) baudio->mixlocals)->flags &= ~JUPITER_LFO1_MASK;
				((bmods *) baudio->mixlocals)->flags |= JUPITER_LFO1_TRI;
			}
			break;
		case 13:
			if (value == 0)
				((bmods *) baudio->mixlocals)->flags &= ~JUPITER_LFO1_RAMP;
			else {
				((bmods *) baudio->mixlocals)->flags &= ~JUPITER_LFO1_MASK;
				((bmods *) baudio->mixlocals)->flags |= JUPITER_LFO1_RAMP;
			}
			break;
		case 14:
			if (value == 0)
				((bmods *) baudio->mixlocals)->flags &= ~JUPITER_LFO1_SQR;
			else {
				((bmods *) baudio->mixlocals)->flags &= ~JUPITER_LFO1_MASK;
				((bmods *) baudio->mixlocals)->flags |= JUPITER_LFO1_SQR;
			}
			break;
		case 15:
			if (value == 0)
				((bmods *) baudio->mixlocals)->flags &= ~JUPITER_LFO1_SH;
			else {
				((bmods *) baudio->mixlocals)->flags &= ~JUPITER_LFO1_MASK;
				((bmods *) baudio->mixlocals)->flags |= JUPITER_LFO1_SH;
			}
			break;
		case 16:
			((bmods *) baudio->mixlocals)->pw = value * 100;
			break;
		case 17:
			switch (tval) {
				case 0:
					((bmods *) baudio->mixlocals)->flags |= JUPITER_PWM_ENV1;
					((bmods *) baudio->mixlocals)->flags &= ~JUPITER_PWM_MAN;
					((bmods *) baudio->mixlocals)->flags &= ~JUPITER_PWM_LFO1;
					break;
				case 2:
					((bmods *) baudio->mixlocals)->flags &= ~JUPITER_PWM_ENV1;
					((bmods *) baudio->mixlocals)->flags |= JUPITER_PWM_MAN;
					((bmods *) baudio->mixlocals)->flags &= ~JUPITER_PWM_LFO1;
					break;
				default:
					((bmods *) baudio->mixlocals)->flags &= ~JUPITER_PWM_ENV1;
					((bmods *) baudio->mixlocals)->flags &= ~JUPITER_PWM_MAN;
					((bmods *) baudio->mixlocals)->flags |= JUPITER_PWM_LFO1;
			}
			break;
		case 18:
			((bmods *) baudio->mixlocals)->vcfEnv = value;
			break;
		case 19:
			if (tval == 0)
				((bmods *) baudio->mixlocals)->flags |= JUPITER_VCF_ENV1;
			else
				((bmods *) baudio->mixlocals)->flags &= ~JUPITER_VCF_ENV1;
			break;
		case 20:
			((bmods *) baudio->mixlocals)->vcfLFO = value * 0.7;
			break;
		case 21:
			((bmods *) baudio->mixlocals)->vcaEnv = value;
			break;
		case 22:
			((bmods *) baudio->mixlocals)->vcaLFO = value;
			break;
		case 23:
			/* MODS: Bend to VCO1 */
			((bmods *) baudio->mixlocals)->bendVco1 = value * 2;
			break;
		case 24:
			/* MODS: Bend to VCO2 */
			((bmods *) baudio->mixlocals)->bendVco2 = value * 2;
			break;
		case 25:
			/* MODS: Bend to VCF */
			((bmods *) baudio->mixlocals)->bendVcf = value * 2;
			break;
		case 26:
			/* MODS: LFO-2 to VCO */
			((bmods *) baudio->mixlocals)->lfo2vco = value * 0.1;
			break;
		case 27:
			/* MODS: LFO-2 to VCF */
			((bmods *) baudio->mixlocals)->lfo2vcf = value * 0.5;
			break;
		case 29:
			if (value == 0)
				((bmods *) baudio->mixlocals)->flags &= ~JUPITER_LFO1_UNI;
			else
				((bmods *) baudio->mixlocals)->flags |= JUPITER_LFO1_UNI;
			break;
		case 30:
			if (value == 0)
				((bmods *) baudio->mixlocals)->flags &= ~JUPITER_XMOD_ENV;
			else
				((bmods *) baudio->mixlocals)->flags |= JUPITER_XMOD_ENV;
			break;
		case 31:
			if (value == 0)
				((bmods *) baudio->mixlocals)->flags &= ~JUPITER_NOISE_UNI;
			else
				((bmods *) baudio->mixlocals)->flags |= JUPITER_NOISE_UNI;
			break;
		case 33:
			if (value == 0)
				((bmods *) baudio->mixlocals)->flags |= JUPITER_PWM_DCO1;
			else
				((bmods *) baudio->mixlocals)->flags &= ~JUPITER_PWM_DCO1;
			break;
		case 34:
			if (value == 0)
				((bmods *) baudio->mixlocals)->flags |= JUPITER_PWM_DCO2;
			else
				((bmods *) baudio->mixlocals)->flags &= ~JUPITER_PWM_DCO2;
			break;
		case 38:
			((bmods *) baudio->mixlocals)->flags &=
				 ~(JUPITER_SYNC_1_2_2|JUPITER_SYNC_2_2_1);
			if (tval == 0)
				((bmods *) baudio->mixlocals)->flags |= JUPITER_SYNC_1_2_2;
			else if (tval == 2)
				((bmods *) baudio->mixlocals)->flags |= JUPITER_SYNC_2_2_1;
			break;
		case 39:
			if (value == 0)
				((bmods *) baudio->mixlocals)->flags &= ~JUPITER_ENV_INV;
			else
				((bmods *) baudio->mixlocals)->flags |= JUPITER_ENV_INV;
			break;
		case 40:
			if (value == 0)
				((bmods *) baudio->mixlocals)->flags &= ~JUPITER_MOD_ENABLE;
			else
				((bmods *) baudio->mixlocals)->flags |= JUPITER_MOD_ENABLE;
			break;
		case 42:
		{
			/*
			 * This will only access the exponential maps, the others will not
			 * be available from this interface. See audiothread.c for code.
			 *
			 * This has moved into BRISTOL_NRP_VELOCITY.
			 */
			if (tval < 0)
				tval = 0;
			else if (tval > 25)
				tval = 25;
			mapVelocityCurve(tval + 500, &baudio->velocitymap[0]);
			break;
		}
		case 101:
		case 126:
			/* This is the dummy entry */
			break;
	}
	return(0);
}

/*
 * Bristol preops are given a sometimes arbitrary voice selection, something
 * that will affect the operation of the LFO-2 and LFO-1 when configured
 * as UNI. Also, the LFO should only run once per pair of layers so that
 * the same LFO is applied to both of them. This cannot only be run in the
 * lower layer even though we do have a layer flag available for the simple
 * reason that the lower layer may not actually undergo preops (if no keys
 * from the lower layer are being played). For that reason we use the rather
 * ugly lfo1exclusion flag. The semi arbitrary voice is actually the 1st one on
 * the playlist for this emulation, typically the most recent key pressed except
 * when seconding has been applied. This means we will mostly get the LFO to
 * track the most recent velocity at the expense of envelope retriggers. At
 * steady state the envelope should continue to open as expected.
 */
static int lfo1exclusion = 0;

int
jupiterPreops(audioMain *audiomain, Baudio *baudio,
bristolVoice *voice, register float *startbuf)
{
	if (freqbuf == NULL)
		return(0);

	bristolbzero(((bmods *) baudio->mixlocals)->lout, audiomain->segmentsize);
	bristolbzero(((bmods *) baudio->mixlocals)->rout, audiomain->segmentsize);

	/*
	 * This will eventually manage LFO_MULTI options, separately per LFO.
	 */
	if (((bmods *) baudio->mixlocals)->flags & JUPITER_NOISE_UNI)
	{
		bristolbzero(noisebuf, audiomain->segmentsize);

		audiomain->palette[4]->specs->io[0].buf = noisebuf;
		(*baudio->sound[10]).operate(
			(audiomain->palette)[4],
			voice,
			(*baudio->sound[10]).param,
			voice->locals[voice->index][10]);
	}

	/*
	 * LFO. We need to review flags to decide where to place our buffers, kick
	 * off with a sine wave and take it from there (sine should be the default
	 * if all others are deselected?).
	 */
	if (((bmods *) baudio->mixlocals)->flags & JUPITER_LFO1_UNI)
	{
		bristolbzero(scratchbuf, audiomain->segmentsize);
		bristolbzero(lfo1buf, audiomain->segmentsize);

		audiomain->palette[16]->specs->io[0].buf = noisebuf;
		audiomain->palette[16]->specs->io[1].buf = 0;
		audiomain->palette[16]->specs->io[2].buf = 0;
		audiomain->palette[16]->specs->io[3].buf = 0;
		audiomain->palette[16]->specs->io[4].buf = 0;
		audiomain->palette[16]->specs->io[5].buf = 0;

		switch (((bmods *) baudio->mixlocals)->flags & JUPITER_LFO1_MASK) {
			case JUPITER_LFO1_TRI:
				audiomain->palette[16]->specs->io[1].buf = scratchbuf;
				break;
			case JUPITER_LFO1_RAMP:
				audiomain->palette[16]->specs->io[5].buf = scratchbuf;
				break;
			case JUPITER_LFO1_SQR:
				audiomain->palette[16]->specs->io[2].buf = scratchbuf;
				break;
			case JUPITER_LFO1_SH:
				audiomain->palette[16]->specs->io[3].buf = scratchbuf;
				break;
			default:
				audiomain->palette[16]->specs->io[4].buf = scratchbuf;
				break;
		}

		(*baudio->sound[0]).operate(
			(audiomain->palette)[16],
			voice,
			(*baudio->sound[0]).param,
			baudio->locals[voice->index][0]);

		/*
		 * We should now generate the ADSR and feed the LFO through it.
		 */
		audiomain->palette[1]->specs->io[0].buf = adsr1buf;
		(*baudio->sound[2]).operate(
			(audiomain->palette)[1],
			voice,
			(*baudio->sound[2]).param,
			voice->locals[voice->index][2]);

		audiomain->palette[2]->specs->io[0].buf = scratchbuf;
		audiomain->palette[2]->specs->io[1].buf = adsr1buf;
		audiomain->palette[2]->specs->io[2].buf = lfo1buf;
		(*baudio->sound[8]).operate(
			(audiomain->palette)[2],
			voice,
			(*baudio->sound[8]).param,
			baudio->locals[voice->index][8]);
	}

	if (lfo1exclusion)
		return(0);

	/*
	 * LFO2, from the mod panel, sine only.
	 */
	bristolbzero(scratchbuf, audiomain->segmentsize);
	bristolbzero(lfo2buf, audiomain->segmentsize);

	audiomain->palette[16]->specs->io[0].buf = noisebuf;
	audiomain->palette[16]->specs->io[1].buf = 0;
	audiomain->palette[16]->specs->io[2].buf = 0;
	audiomain->palette[16]->specs->io[3].buf = 0;
	audiomain->palette[16]->specs->io[4].buf = scratchbuf; /* Sine */
	audiomain->palette[16]->specs->io[5].buf = 0;
	audiomain->palette[16]->specs->io[6].buf = 0;

	(*baudio->sound[1]).operate(
		(audiomain->palette)[16],
		voice,
		(*baudio->sound[1]).param,
		baudio->locals[voice->index][1]);

	/*
	 * We should now generate the ADSR and feed the LFO through it.
	 */
	audiomain->palette[1]->specs->io[0].buf = adsr1buf;
	(*baudio->sound[3]).operate(
		(audiomain->palette)[1],
		voice,
		(*baudio->sound[3]).param,
		voice->locals[voice->index][3]);

	audiomain->palette[2]->specs->io[0].buf = scratchbuf;
	audiomain->palette[2]->specs->io[1].buf = adsr1buf;
	audiomain->palette[2]->specs->io[2].buf = lfo2buf;
	(*baudio->sound[8]).operate(
		(audiomain->palette)[2],
		voice,
		(*baudio->sound[8]).param,
		baudio->locals[voice->index][8]);

	lfo1exclusion = 1;

	return(0);
}

int
operateOneJupiter(audioMain *audiomain, Baudio *baudio,
bristolVoice *voice, register float *startbuf)
{
	bmods *mixlocal = (bmods *) baudio->mixlocals;

	dco2buf = mixlocal->dco2buf[voice->index];

	/*
	 * Kick this off with a single oscillator into an AMP with env. When this
	 * works with memories from the GUI and layers in the engine then we can
	 * extend the features.
	 */
	if (freqbuf == NULL)
		return(0);

/* printf("operate %i, %x\n", voice->index, baudio); */
	bristolbzero(dco1buf, audiomain->segmentsize);
	bristolbzero(zerobuf, audiomain->segmentsize);
	bristolbzero(outbuf, audiomain->segmentsize);
	bristolbzero(filtbuf, audiomain->segmentsize);

	if ((mixlocal->flags & JUPITER_NOISE_UNI) == 0)
	{
		bristolbzero(noisebuf, audiomain->segmentsize);

		audiomain->palette[4]->specs->io[0].buf = noisebuf;
		(*baudio->sound[10]).operate(
			(audiomain->palette)[4],
			voice,
			(*baudio->sound[10]).param,
			voice->locals[voice->index][10]);
	}

/* One LFO MULTI */
	/*
	 * LFO. We need to review flags to decide where to place our buffers, kick
	 * off with a sine wave and take it from there (sine should be the default
	 * if all others are deselected?).
	 */
	if ((mixlocal->flags & JUPITER_LFO1_UNI) == 0)
	{
		/* bristolbzero(scratchbuf, audiomain->segmentsize); */
		bristolbzero(lfo1buf, audiomain->segmentsize);

		audiomain->palette[16]->specs->io[0].buf = noisebuf;
		audiomain->palette[16]->specs->io[1].buf = 0;
		audiomain->palette[16]->specs->io[2].buf = 0;
		audiomain->palette[16]->specs->io[3].buf = 0;
		audiomain->palette[16]->specs->io[4].buf = 0;
		audiomain->palette[16]->specs->io[5].buf = 0;

		switch (mixlocal->flags & JUPITER_LFO1_MASK) {
			case JUPITER_LFO1_TRI:
				audiomain->palette[16]->specs->io[1].buf = scratchbuf;
				break;
			case JUPITER_LFO1_RAMP:
				audiomain->palette[16]->specs->io[5].buf = scratchbuf;
				break;
			case JUPITER_LFO1_SQR:
				audiomain->palette[16]->specs->io[2].buf = scratchbuf;
				break;
			case JUPITER_LFO1_SH:
				audiomain->palette[16]->specs->io[3].buf = scratchbuf;
				break;
			default:
				audiomain->palette[16]->specs->io[4].buf = scratchbuf;
				break;
		}

		(*baudio->sound[0]).operate(
			(audiomain->palette)[16],
			voice,
			(*baudio->sound[0]).param,
			baudio->locals[voice->index][0]);

		/*
		 * We should now generate the ADSR and feed the LFO through it.
		 */
		audiomain->palette[1]->specs->io[0].buf = adsr1buf;
		(*baudio->sound[2]).operate(
			(audiomain->palette)[1],
			voice,
			(*baudio->sound[2]).param,
			voice->locals[voice->index][2]);

		audiomain->palette[2]->specs->io[0].buf = scratchbuf;
		audiomain->palette[2]->specs->io[1].buf = adsr1buf;
		audiomain->palette[2]->specs->io[2].buf = lfo1buf;
		(*baudio->sound[8]).operate(
			(audiomain->palette)[2],
			voice,
			(*baudio->sound[8]).param,
			baudio->locals[voice->index][8]);
	}

/* ADSR Processing */
	/*
	 * Run the AMP ADSR - this should actually happen after the filter ADSR
	 * to ensure the flags don't get mangled, but either way we need this 
	 * before the DCO since we may be applying PWM with them.
	 */
	audiomain->palette[1]->specs->io[0].buf = adsr1buf;
	(*baudio->sound[7]).operate(
		(audiomain->palette)[1],
		voice,
		(*baudio->sound[7]).param,
		voice->locals[voice->index][7]);
	audiomain->palette[1]->specs->io[0].buf = adsr2buf;
	(*baudio->sound[9]).operate(
		(audiomain->palette)[1],
		voice,
		(*baudio->sound[9]).param,
		voice->locals[voice->index][9]);

	if (mixlocal->flags & JUPITER_ENV_INV)
	{
		int i, s = audiomain->samplecount;

		for (i = 0; i < s; i++)
			adsr1buf[i] = -adsr1buf[i];
	}
/* End of ADSR Processing */

/* DCO MOD processing */
	fillFreqTable(baudio, voice, freqbuf, audiomain->samplecount, 1);

	/*
	 * Put in LFO1 and ENV1 into vcofmbuf at specified levels
	 */
	if (mixlocal->dcoLFO1FM != 0.0) {
		bufmerge(lfo1buf, mixlocal->dcoLFO1FM,
			vcofmbuf, 0.0, audiomain->samplecount);
		if (mixlocal->dcoEnv1FM != 0.0)
			bufmerge(adsr1buf, mixlocal->dcoEnv1FM,
				vcofmbuf, 1.0, audiomain->samplecount);
	} else {
		if (mixlocal->dcoEnv1FM != 0.0)
			bufmerge(adsr1buf, mixlocal->dcoEnv1FM,
				vcofmbuf, 0.0, audiomain->samplecount);
		else
			bristolbzero(vcofmbuf, audiomain->segmentsize);
	}

	if (mixlocal->pw != 0.0) {
		if (mixlocal->flags & JUPITER_PWM_ENV1)
			bufmerge(adsr1buf, mixlocal->pw,
				pwmbuf, 0.0, audiomain->samplecount);
		else if (mixlocal->flags & JUPITER_PWM_LFO1) {
			bufmerge(lfo1buf, mixlocal->pw,
				pwmbuf, 0.0, audiomain->samplecount);
		} else /* Fix a single value */ {
			register int i, s = audiomain->samplecount;
			register float pw = mixlocal->pw;
			register float *pwmb = pwmbuf;

			for (i = 0; i < s; i++) *pwmb++ = pw;
		}
	} else
		bristolbzero(pwmbuf, audiomain->segmentsize);

/* End DCO-1 MOD processing */

/* DCO-1 processing */
	/*
	 * Run DCO-1. 
	 */
	bufmerge(freqbuf, 1.0, scratchbuf, 0.0, audiomain->samplecount);

	/*
	 * Freq mods can be two LFO, oscB and Env1
	 */
	if (mixlocal->xmod != 0.0) {
		if (mixlocal->flags & JUPITER_XMOD_ENV) {
			bufmerge(dco2buf, 0.05, dco2buf, 0.0, audiomain->samplecount);

			audiomain->palette[2]->specs->io[0].buf = dco2buf;
			audiomain->palette[2]->specs->io[1].buf = adsr1buf;
			audiomain->palette[2]->specs->io[2].buf = scratchbuf;
			(*baudio->sound[8]).operate(
				(audiomain->palette)[2],
				voice,
				(*baudio->sound[8]).param,
				baudio->locals[voice->index][8]);
		} else
			bufmerge(dco2buf, mixlocal->xmod,
				scratchbuf, 1.0, audiomain->samplecount);
	}

	if (mixlocal->flags & FREQ_MOD_VCO1)
		bufmerge(vcofmbuf, 1.0, scratchbuf, 1.0, audiomain->samplecount);

	if ((mixlocal->lfo2vco != 0)
		&& (((bmods *) baudio->mixlocals)->flags & JUPITER_MOD_ENABLE))
		bufmerge(lfo2buf, mixlocal->lfo2vco,
			scratchbuf, 1.0, audiomain->samplecount);

	if ((mixlocal->bendVco1 != 0) &&
		(((bmods *) baudio->mixlocals)->flags & JUPITER_MOD_ENABLE))
	{
		register int i;
		register float j;

		/*
		 * We need to normalise the controller value, scale it by the depth
		 * of bend then add into the filter cutoff
		 */
		j = (baudio->contcontroller[1] - 0.5) * 2 * mixlocal->bendVco1;

		for (i = 0; i < audiomain->samplecount; i++)
			scratchbuf[i] += j;
	}

	audiomain->palette[30]->specs->io[0].buf = scratchbuf;
	audiomain->palette[30]->specs->io[1].buf = dco1buf;
	audiomain->palette[30]->specs->io[2].buf = pwmbuf; /* PWM buff */
	if (mixlocal->flags & JUPITER_SYNC_2_2_1) {
		audiomain->palette[30]->specs->io[3].buf = syncbuf; /* Sync buff */
		audiomain->palette[30]->specs->io[4].buf = NULL; /* Sync out buff */
	} else {
		audiomain->palette[30]->specs->io[3].buf = zerobuf;
		audiomain->palette[30]->specs->io[4].buf = syncbuf; /* Sync out buff */
	}

	(*baudio->sound[4]).operate(
		(audiomain->palette)[30],
		voice,
		(*baudio->sound[4]).param,
		voice->locals[voice->index][4]);
/* End of DCO-1 processing */

/* DCO-2 processing */
	bristolbzero(dco2buf, audiomain->segmentsize);

	/*
	 * Then DCO-2 and its mods.
	 */
	bufmerge(freqbuf, 1.0, scratchbuf, 0.0, audiomain->samplecount);

	/*
	 * Freq mods can be two LFO, oscB and Env1
	 */
	if (mixlocal->flags & FREQ_MOD_VCO2)
		bufmerge(vcofmbuf, 1.0, scratchbuf, 1.0, audiomain->samplecount);

	if ((mixlocal->lfo2vco != 0)
		&& (((bmods *) baudio->mixlocals)->flags & JUPITER_MOD_ENABLE))
		bufmerge(lfo2buf, mixlocal->lfo2vco,
			scratchbuf, 1.0, audiomain->samplecount);

	if ((mixlocal->bendVco2 != 0)
		&& (((bmods *) baudio->mixlocals)->flags & JUPITER_MOD_ENABLE))
	{
		register int i;
		register float j;

		/*
		 * We need to normalise the controller value, scale it by the depth
		 * of bend then add into the filter cutoff
		 */
		j = (baudio->contcontroller[1] - 0.5) * 2 * mixlocal->bendVco2;

		for (i = 0; i < audiomain->samplecount; i++)
			scratchbuf[i] += j;
	}

	audiomain->palette[30]->specs->io[0].buf = scratchbuf;
	audiomain->palette[30]->specs->io[1].buf = dco2buf;
	audiomain->palette[30]->specs->io[2].buf = pwmbuf; /* PWM buff */
	if (mixlocal->flags & JUPITER_SYNC_1_2_2) {
		audiomain->palette[30]->specs->io[3].buf = syncbuf; /* Sync buff */
		audiomain->palette[30]->specs->io[4].buf = NULL; /* Sync buff */
	} else {
		audiomain->palette[30]->specs->io[3].buf = zerobuf;
		audiomain->palette[30]->specs->io[4].buf = syncbuf;
	}

	(*baudio->sound[5]).operate(
		(audiomain->palette)[30],
		voice,
		(*baudio->sound[5]).param,
		voice->locals[voice->index][5]);
/* DCO-2 processing */

	/* Add in the noise */
	if (mixlocal->noisegain != 0.0)
		bufmerge(noisebuf, mixlocal->noisegain,
			dco2buf, 1.0, audiomain->samplecount);

	/* Add in DCO-1 */
	bufmerge(dco2buf, mixlocal->rmix * 512.0,
		dco1buf, mixlocal->lmix * 512.0,
		audiomain->samplecount);

	/*
	 * Put the HPF filter in.
	 */
	audiomain->palette[10]->specs->io[0].buf = dco1buf;
	audiomain->palette[10]->specs->io[1].buf = dco1buf;
	(*baudio->sound[11]).operate(
		(audiomain->palette)[10],
		voice,
		(*baudio->sound[11]).param,
		voice->locals[voice->index][11]);

/* Filter processing */
	/*
	 * Filter mods may be Env1 or 2, LFO-1 or LFO-2. Dump them into scratch.
	 */
	if (mixlocal->vcfEnv != 0.0) {
		if (mixlocal->flags & JUPITER_VCF_ENV1)
			bufmerge(adsr1buf, mixlocal->vcfEnv,
			scratchbuf, 0.0, audiomain->samplecount);
		else
			bufmerge(adsr2buf, mixlocal->vcfEnv,
			scratchbuf, 0.0, audiomain->samplecount);

		if (mixlocal->vcfLFO != 0.0)
			bufmerge(lfo1buf, mixlocal->vcfLFO,
			scratchbuf, 1.0, audiomain->samplecount);
	} else {
		if (mixlocal->vcfLFO != 0.0)
			bufmerge(lfo1buf, mixlocal->vcfLFO,
			scratchbuf, 0.0, audiomain->samplecount);
		else
			bristolbzero(scratchbuf, audiomain->segmentsize);
	}

	if ((mixlocal->lfo2vcf != 0)
		&& (((bmods *) baudio->mixlocals)->flags & JUPITER_MOD_ENABLE))
		bufmerge(lfo2buf, mixlocal->lfo2vcf,
			scratchbuf, 1.0, audiomain->samplecount);

	if ((mixlocal->bendVcf != 0)
		&& (((bmods *) baudio->mixlocals)->flags & JUPITER_MOD_ENABLE))
	{
		register int i;
		register float j;

		/*
		 * We need to normalise the controller value, scale it by the depth
		 * of bend then add into the filter cutoff
		 */
		j = (baudio->contcontroller[1] - 0.5) * 2 * mixlocal->bendVcf;

		for (i = 0; i < audiomain->samplecount; i++)
			scratchbuf[i] += j;
	}

	/*
	 * Run the DCO output through the filter using the first ADSR
	 */
	audiomain->palette[3]->specs->io[0].buf = dco1buf;
	audiomain->palette[3]->specs->io[1].buf = scratchbuf;
	audiomain->palette[3]->specs->io[2].buf = filtbuf;
	(*baudio->sound[6]).operate(
		(audiomain->palette)[3],
		voice,
		(*baudio->sound[6]).param,
		baudio->locals[voice->index][6]);
/* End of Filter processing */

/* VCA processing */
	/*
	 * Env mods are Env-2 and two LFO.
	 */
	bufmerge(lfo1buf, mixlocal->vcaLFO,
		adsr2buf, mixlocal->vcaEnv,
		audiomain->samplecount);

	/*
	 * Run filter output through the AMP using the second ADSR
	 */
	audiomain->palette[2]->specs->io[0].buf = filtbuf;
	audiomain->palette[2]->specs->io[1].buf = adsr2buf;
	audiomain->palette[2]->specs->io[2].buf = outbuf;
	(*baudio->sound[8]).operate(
		(audiomain->palette)[2],
		voice,
		(*baudio->sound[8]).param,
		baudio->locals[voice->index][8]);
/* VCA processing */

/* Panning */
	bufmerge(outbuf, mixlocal->lpan,
		mixlocal->lout, 1.0,
		audiomain->samplecount);
	bufmerge(outbuf, mixlocal->rpan,
		mixlocal->rout, 1.0,
		audiomain->samplecount);
/* Panning */

	return(0);
}

int
jupiterPostops(audioMain *audiomain, Baudio *baudio,
bristolVoice *voice, register float *startbuf)
{
	if (freqbuf == NULL)
		return(0);

	bufmerge(((bmods *) baudio->mixlocals)->lout,
		((bmods *) baudio->mixlocals)->gain,
		baudio->leftbuf, 1.0,
		audiomain->samplecount);
	bufmerge(((bmods *) baudio->mixlocals)->rout,
		((bmods *) baudio->mixlocals)->gain,
		baudio->rightbuf, 1.0,
		audiomain->samplecount);

	lfo1exclusion = 0;

	return(0);
}

int
static bristolJupiterDestroy(audioMain *audiomain, Baudio *baudio)
{
	int i;

	printf("removing one jupiter\n");

	for (i = 0; i < BRISTOL_VOICECOUNT; i++)
		bristolfree(((bmods *) baudio->mixlocals)->dco2buf[i]);

	bristolfree(((bmods *) baudio->mixlocals)->lout);
	bristolfree(((bmods *) baudio->mixlocals)->rout);

	/*
	 * The following can be left up to the library. If we free this here then
	 * we do need to NULL the pointer otherwise glibc may get picky.
	 */
	bristolfree(baudio->mixlocals);
	baudio->mixlocals = NULL;

	/*
	 * We need to leave these, we may have multiple invocations running
	 */
	return(0);

	if (freqbuf != NULL)
		bristolfree(freqbuf);
	if (adsr1buf != NULL)
		bristolfree(adsr1buf);
	if (adsr2buf != NULL)
		bristolfree(adsr2buf);
	if (filtbuf != NULL)
		bristolfree(filtbuf);
	if (dco1buf != NULL)
		bristolfree(dco1buf);
	if (zerobuf != NULL)
		bristolfree(zerobuf);
	if (outbuf != NULL)
		bristolfree(outbuf);
	if (lfo2buf != NULL)
		bristolfree(lfo2buf);
	if (lfo1buf != NULL)
		bristolfree(lfo1buf);
	if (noisebuf != NULL)
		bristolfree(noisebuf);
	if (scratchbuf != NULL)
		bristolfree(scratchbuf);
	if (vcofmbuf != NULL)
		bristolfree(vcofmbuf);
	if (modbuf != NULL)
		bristolfree(modbuf);
	if (syncbuf != NULL)
		bristolfree(syncbuf);

	freqbuf = NULL;
	adsr1buf = NULL;
	adsr2buf = NULL;
	filtbuf = NULL;
	dco2buf = NULL;
	dco1buf = NULL;
	zerobuf = NULL;
	outbuf = NULL;
	lfo1buf = NULL;
	lfo2buf = NULL;
	noisebuf = NULL;
	scratchbuf = NULL;
	pwmbuf = NULL;
	modbuf = NULL;
	syncbuf = NULL;
	vcofmbuf = NULL;

	return(0);
}

int
bristolJupiterInit(audioMain *audiomain, Baudio *baudio)
{
	int i;

	printf("initialising jupiter: %i voices\n", baudio->voicecount);
	/*
	 * Two LFO possibly with one envelope each.
	 * Two DCO
	 * Filter with Env
	 * Amp with Env
	 * Noise source
	 */
	baudio->soundCount = 12; /* Number of operators in this voice */
	/*
	 * Assign an array of sound pointers.
	 */
	baudio->sound = (bristolSound **)
		bristolmalloc0(sizeof(bristolOP *) * baudio->soundCount);
	baudio->effect = (bristolSound **)
		bristolmalloc0(sizeof(bristolOP *) * baudio->soundCount);

	/* Two LFO with ADSR (for delay) */
	initSoundAlgo(16,	0, baudio, audiomain, baudio->sound);
	initSoundAlgo(16,	1, baudio, audiomain, baudio->sound);
	initSoundAlgo( 1,	2, baudio, audiomain, baudio->sound);
	initSoundAlgo( 1,	3, baudio, audiomain, baudio->sound);
	/* Two oscillator - these still need harmonic mods */
	initSoundAlgo(30,	4, baudio, audiomain, baudio->sound);
	initSoundAlgo(30,	5, baudio, audiomain, baudio->sound);
	/* A filter with ADSR */
	initSoundAlgo( 3,	6, baudio, audiomain, baudio->sound);
	initSoundAlgo( 1,	7, baudio, audiomain, baudio->sound);
	/* An amplifier with ADSR */
	initSoundAlgo( 2,	8, baudio, audiomain, baudio->sound);
	initSoundAlgo( 1,	9, baudio, audiomain, baudio->sound);
	/* An noise source */
	initSoundAlgo( 4,	10, baudio, audiomain, baudio->sound);
	/* HPF */
	initSoundAlgo(10,	11, baudio, audiomain, baudio->sound);

	baudio->param = jupiterController;
	baudio->destroy = bristolJupiterDestroy;
	baudio->operate = operateOneJupiter;
	baudio->preops = jupiterPreops;
	baudio->postops = jupiterPostops;

	/*
	 * Put in integrated effects here.
	initSoundAlgo(12, 0, baudio, audiomain, baudio->effect);
	 */

	baudio->mixlocals = (float *) bristolmalloc0(sizeof(bmods));

	for (i = 0; i < BRISTOL_VOICECOUNT; i++)
		((bmods *) baudio->mixlocals)->dco2buf[i] =
			(float *) bristolmalloc0(audiomain->segmentsize);

	/*
	 * If we were the first requested emulation then this is the lower layer
	 * and needs to be flagged here to ensure correct 'stereo' panning.
	 */
	if (freqbuf == NULL)
		((bmods *) baudio->mixlocals)->flags |= JUPITER_LOWER_LAYER;

	((bmods *) baudio->mixlocals)->lout =
		(float *) bristolmalloc0(audiomain->segmentsize);
	((bmods *) baudio->mixlocals)->rout =
		(float *) bristolmalloc0(audiomain->segmentsize);

	((bmods *) baudio->mixlocals)->voicecount = baudio->voicecount;

	/*
	 * And request any buffer space we want
	 */
	if (freqbuf == NULL)
		freqbuf = (float *) bristolmalloc0(audiomain->segmentsize);
	if (adsr1buf == NULL)
		adsr1buf = (float *) bristolmalloc0(audiomain->segmentsize);
	if (adsr2buf == NULL)
		adsr2buf = (float *) bristolmalloc0(audiomain->segmentsize);
	if (filtbuf == NULL)
		filtbuf = (float *) bristolmalloc0(audiomain->segmentsize);
	if (dco1buf == NULL)
		dco1buf = (float *) bristolmalloc0(audiomain->segmentsize);
	if (zerobuf == NULL)
		zerobuf = (float *) bristolmalloc0(audiomain->segmentsize);
	if (outbuf == NULL)
		outbuf = (float *) bristolmalloc0(audiomain->segmentsize);
	if (noisebuf == NULL)
		noisebuf = (float *) bristolmalloc0(audiomain->segmentsize);
	if (lfo2buf == NULL)
		lfo2buf = (float *) bristolmalloc0(audiomain->segmentsize);
	if (lfo1buf == NULL)
		lfo1buf = (float *) bristolmalloc0(audiomain->segmentsize);
	if (scratchbuf == NULL)
		scratchbuf = (float *) bristolmalloc0(audiomain->segmentsize);
	if (pwmbuf == NULL)
		pwmbuf = (float *) bristolmalloc0(audiomain->segmentsize);
	if (vcofmbuf == NULL)
		vcofmbuf = (float *) bristolmalloc0(audiomain->segmentsize);
	if (modbuf == NULL)
		modbuf = (float *) bristolmalloc0(audiomain->segmentsize);
	if (syncbuf == NULL)
		syncbuf = (float *) bristolmalloc0(audiomain->segmentsize);

	baudio->mixflags |= BRISTOL_STEREO;

	return(0);
}

