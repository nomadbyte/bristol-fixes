
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
#include "bristolcs80.h"

extern void mapVelocityCurve(int, float []);

/*
 * Use of these cs80 global buffers will be an issue with use of multiple
 * audio threads, unless we ensure a single thread deals with any given algo
 * type, since then they are only used sequentially.
 */
static float *freqbuf = (float *) NULL;
static float *adsr1buf = (float *) NULL;
static float *adsr2buf = (float *) NULL;
static float *filtbuf = (float *) NULL;
static float *dco1buf = (float *) NULL;
static float *zerobuf = (float *) NULL;
static float *outbuf = (float *) NULL;
static float *lfo1buf = (float *) NULL;
static float *lfo2buf = (float *) NULL;
static float *noisebuf = (float *) NULL;
static float *scratchbuf = (float *) NULL;
static float *pwmbuf = (float *) NULL;
static float *sinebuf = (float *) NULL;
static float *modbuf = (float *) NULL;

/*
 * These need to go into some local structure for multiple instances
 * of the cs80 - malloc()ed into the baudio->mixlocals.
 */
#define CS80_LOWER_LAYER	0x00000001
#define CS80_NOISE_MULTI	0x00000002
#define CS80_LFO1_MULTI		0x00000004
#define CS80_LFO1_MASK		0x000001F0
#define CS80_LFO1_TRI		0x00000010
#define CS80_LFO1_RAMP		0x00000020
#define CS80_LFO1_SQR		0x00000040
#define CS80_LFO1_SH		0x00000080
#define CS80_LFO1_SAW		0x00000100

int
cs80Controller(Baudio *baudio, u_char operator,
u_char controller, float value)
{
	int tval = value * CONTROLLER_RANGE;

#ifdef DEBUG
	printf("bristolcs80Control(%i, %i, %f)\n", operator, controller, value);
#endif
printf("bristolcs80Control(%i, %i, %f)\n", operator, controller, value);

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
			((csmods *) baudio->mixlocals)->gain = value * 4;
			break;
		case 3:
			/*
			 * Since we are using here 'lpan = 1.0 - rpan' we could consider
			 * just using one value. It is maintained as two since we may still
			 * reinvoke the midi spec panning at some point.
			 */
			((csmods *) baudio->mixlocals)->lpan[CH_I] = (1.0 - value);
			((csmods *) baudio->mixlocals)->rpan[CH_I] = value;
			/*
			if (tval == 0)
				value += 1.0f/CONTROLLER_RANGE;
			else if (tval == CONTROLLER_RANGE)
				value -= 1.0f/CONTROLLER_RANGE;
				
			((csmods *) baudio->mixlocals)->lpan =
				-log10f(sinf(M_PI/2.0f * value));
			((csmods *) baudio->mixlocals)->rpan =
				-log10f(cosf(M_PI/2.0f * value));
			 */
			break;
		case 4:
			((csmods *) baudio->mixlocals)->lpan[CH_II] = (1.0 - value);
			((csmods *) baudio->mixlocals)->rpan[CH_II] = value;
			break;
		case 5:
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
				
			((csmods *) baudio->mixlocals)->channelmix = 20 *
				log10f(cosf(M_PI/2.0f * value));
			((csmods *) baudio->mixlocals)->rmix = 20 *
				log10f(sinf(M_PI/2.0f * value));

			printf("mix now %f/%f\n",
				((csmods *) baudio->mixlocals)->channelmix,
				((csmods *) baudio->mixlocals)->rmix);
			 */

			((csmods *) baudio->mixlocals)->channelmix = value;
//			((csmods *) baudio->mixlocals)->rmix = value;
			break;
		case 6:
			((csmods *) baudio->mixlocals)->brilliance = value;
			break;
		case 7:
			((csmods *) baudio->mixlocals)->brillianceL = value;
			break;
		case 8:
			((csmods *) baudio->mixlocals)->brillianceH = value;
			break;
		case 9:
			((csmods *) baudio->mixlocals)->levelL = value;
			break;
		case 10:
			((csmods *) baudio->mixlocals)->levelH = value;
			break;
		case 11:
			((csmods *) baudio->mixlocals)->chanmods[CH_I].brilliance = value;
			break;
		case 12:
			((csmods *) baudio->mixlocals)->chanmods[CH_I].level = value;
			break;
		case 13:
			((csmods *) baudio->mixlocals)->chanmods[CH_I].polybrilliance
				= value;
			break;
		case 14:
			((csmods *) baudio->mixlocals)->chanmods[CH_I].polylevel = value;
			break;
		case 15:
			((csmods *) baudio->mixlocals)->chanmods[CH_II].brilliance = value;
			break;
		case 16:
			((csmods *) baudio->mixlocals)->chanmods[CH_II].level = value;
			break;
		case 17:
			((csmods *) baudio->mixlocals)->chanmods[CH_II].polybrilliance
				= value;
			break;
		case 18:
			((csmods *) baudio->mixlocals)->chanmods[CH_II].polylevel = value;
			break;
/*
#define CS80_LFO1_MASK		0x000001F0
#define CS80_LFO1_TRI		0x00000010
#define CS80_LFO1_RAMP		0x00000020
#define CS80_LFO1_SQR		0x00000040
#define CS80_LFO1_SH		0x00000080
#define CS80_LFO1_SAW		0x00000100
*/
		case 20:
			switch (tval)
			{
				case 0:
					((csmods *) baudio->mixlocals)->flags &= ~CS80_LFO1_MASK;
					((csmods *) baudio->mixlocals)->flags |= CS80_LFO1_TRI;
					break;
				case 1:
					((csmods *) baudio->mixlocals)->flags &= ~CS80_LFO1_MASK;
					((csmods *) baudio->mixlocals)->flags |= CS80_LFO1_RAMP;
					break;
				case 2:
					((csmods *) baudio->mixlocals)->flags &= ~CS80_LFO1_MASK;
					((csmods *) baudio->mixlocals)->flags |= CS80_LFO1_SAW;
					break;
				case 3:
					((csmods *) baudio->mixlocals)->flags &= ~CS80_LFO1_MASK;
					((csmods *) baudio->mixlocals)->flags |= CS80_LFO1_SQR;
					break;
				case 4:
					((csmods *) baudio->mixlocals)->flags &= ~CS80_LFO1_MASK;
					((csmods *) baudio->mixlocals)->flags |= CS80_LFO1_SH;
					break;
				case 5:
					((csmods *) baudio->mixlocals)->flags &= ~CS80_LFO1_MASK;
					break;
				}
				break;
		case 21:
			((csmods *) baudio->mixlocals)->lfo2vco = value;
			break;
		case 22:
			((csmods *) baudio->mixlocals)->lfo2vcf = value;
			break;
		case 23:
			((csmods *) baudio->mixlocals)->lfo2vca = value;
			break;
		case 24:
			((csmods *) baudio->mixlocals)->chanmods[CH_I].pwmdepth
				= value * 1000;
			break;
		case 25:
			((csmods *) baudio->mixlocals)->chanmods[CH_II].pwmdepth
				= value * 1000;
			break;
		case 26:
			((csmods *) baudio->mixlocals)->chanmods[CH_I].noiselevel
				= value * 48;
			break;
		case 27:
			((csmods *) baudio->mixlocals)->chanmods[CH_II].noiselevel
				= value * 48;
			break;
		case 28:
			((csmods *) baudio->mixlocals)->chanmods[CH_I].filterlevel = value;
			break;
		case 29:
			((csmods *) baudio->mixlocals)->chanmods[CH_II].filterlevel = value;
			break;
		case 30:
			((csmods *) baudio->mixlocals)->chanmods[CH_I].filterbrill = value;
			break;
		case 31:
			((csmods *) baudio->mixlocals)->chanmods[CH_II].filterbrill = value;
			break;
		case 101:
			/* This is the dummy entry */
			break;
	}

/*
if (controller >= 6)
printf("Brill: %1.2f %1.2f %1.2f %1.2f %1.2f %1.2f %1.2f\n\
Level: %1.2f %1.2f %1.2f %1.2f %1.2f %1.2f\n",
((csmods *) baudio->mixlocals)->brilliance,
((csmods *) baudio->mixlocals)->brillianceL,
((csmods *) baudio->mixlocals)->brillianceH,
((csmods *) baudio->mixlocals)->chanmods[CH_I].brilliance,
((csmods *) baudio->mixlocals)->chanmods[CH_I].polybrilliance,
((csmods *) baudio->mixlocals)->chanmods[CH_II].brilliance,
((csmods *) baudio->mixlocals)->chanmods[CH_II].polybrilliance,

((csmods *) baudio->mixlocals)->levelL,
((csmods *) baudio->mixlocals)->levelH,
((csmods *) baudio->mixlocals)->chanmods[CH_I].level,
((csmods *) baudio->mixlocals)->chanmods[CH_II].polylevel,
((csmods *) baudio->mixlocals)->chanmods[CH_II].level,
((csmods *) baudio->mixlocals)->chanmods[CH_I].polylevel);
*/

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
int
cs80Preops(audioMain *audiomain, Baudio *baudio,
bristolVoice *voice, register float *startbuf)
{
	if (freqbuf == NULL)
		return(0);

	bristolbzero(((csmods *) baudio->mixlocals)->lout, audiomain->segmentsize);
	bristolbzero(((csmods *) baudio->mixlocals)->rout, audiomain->segmentsize);

	audiomain->palette[16]->specs->io[0].buf = NULL;
	audiomain->palette[16]->specs->io[1].buf = NULL;
	audiomain->palette[16]->specs->io[2].buf = NULL;
	audiomain->palette[16]->specs->io[3].buf = NULL;
	audiomain->palette[16]->specs->io[4].buf = pwmbuf;
	audiomain->palette[16]->specs->io[5].buf = NULL;
	audiomain->palette[16]->specs->io[6].buf = NULL;

	/*
	 * This will eventually manage LFO_MULTI options, separately per LFO.
	 */
	if (((csmods *) baudio->mixlocals)->flags & CS80_NOISE_MULTI)
	{
		bristolbzero(noisebuf, audiomain->segmentsize);

		audiomain->palette[4]->specs->io[0].buf = noisebuf;
		(*baudio->sound[16]).operate(
			(audiomain->palette)[4],
			voice,
			(*baudio->sound[16]).param,
			voice->locals[voice->index][16]);
	}

	/*
	 * LFO. We need to review flags to decide where to place our buffers, kick
	 * off with a sine wave and take it from there (sine should be the default
	 * if all others are deselected?).
	 * There are actually two global LFO, the second one is just for the ring
	 * modulator and that ones uses an env.
	 */
	if (((csmods *) baudio->mixlocals)->flags & CS80_LFO1_MULTI)
	{
		bristolbzero(scratchbuf, audiomain->segmentsize);

		audiomain->palette[16]->specs->io[0].buf = noisebuf;
		audiomain->palette[16]->specs->io[1].buf = 0;
		audiomain->palette[16]->specs->io[2].buf = 0;
		audiomain->palette[16]->specs->io[3].buf = 0;
		audiomain->palette[16]->specs->io[4].buf = 0;
		audiomain->palette[16]->specs->io[5].buf = 0;

		switch (((csmods *) baudio->mixlocals)->flags & CS80_LFO1_MASK) {
			case CS80_LFO1_TRI:
				audiomain->palette[16]->specs->io[1].buf = lfo1buf;
				break;
			case CS80_LFO1_SAW:
				audiomain->palette[16]->specs->io[6].buf = lfo1buf;
				break;
			case CS80_LFO1_RAMP:
				audiomain->palette[16]->specs->io[5].buf = lfo1buf;
				break;
			case CS80_LFO1_SQR:
				audiomain->palette[16]->specs->io[2].buf = lfo1buf;
				break;
			case CS80_LFO1_SH:
				audiomain->palette[16]->specs->io[3].buf = lfo1buf;
				break;
			default: /* Sine */
				audiomain->palette[16]->specs->io[4].buf = lfo1buf;
				break;
		}

		(*baudio->sound[15]).operate(
			(audiomain->palette)[16],
			voice,
			(*baudio->sound[15]).param,
			baudio->locals[voice->index][15]);
	}

	return(0);
}

void
operateOneCs80Channel(audioMain *audiomain, Baudio *baudio,
bristolVoice *voice, bristolSound **sound, int channel)
{
	csmods *csmod = ((csmods *) baudio->mixlocals);
	int layer;
	float bkey = voice->key.key - 24, lkey;

	layer = channel == CH_I? 0: 6;
	csmod->channel = channel;

	/*
	 * There are several sources of brilliance and level, all of which act
	 * together and which need to be consolidated down to a single pair of
	 * values for the channel.
	 *
	 * Brilliance Inputs are:
	 *
	 * 	Key ID, normalised to GUI range and affected by KeyRange parameters
	 * 	Velocity affected by Initial control.
	 *	PolyPressure affected by After control.
	 * 	Overall brilliance
	 *
	 * Level Inputs are:
	 *
	 * 	Key ID, normalised to GUI range and affected by KeyRange parameters
	 * 	Velocity affected by Initial control.
	 *	PolyPressure affected by After control.
	 *
	 * We need some defined method to pass these values to the filter, ADSR
	 * and oscillator. I may be lazy and used a shared structure? Better to 
	 * reference the voice->baudio->mixlocals->csMods[voice->index] though.
	 *
	 * The KeyID stuff used a unity at mid keyboard and then a gradual change 
	 * up to the outer limits using the parameters, something that means we
	 * have to split the calculation up a little bit.
	 *
	 * Need to make sure the filters work to expectation as well since the
	 * brilliance value also widens the bandpass apeture.
	 */
	if (bkey <= 30) {
		if (bkey <= 0) {
			lkey = csmod->levelL;
			bkey = csmod->brillianceL;
		} else {
			lkey = 0.5 + (csmod->levelL - 0.5) * (30 - bkey) / 30;
			bkey = 0.5 + (csmod->brillianceL - 0.5) * (30 - bkey) / 30;
		}
	} else if (bkey >= 60) {
		lkey = csmod->levelH;
		bkey = csmod->brillianceH;
	} else {
		lkey = 0.5 + (csmod->levelH - 0.5) * (bkey - 30) / 30;
		bkey = 0.5 + (csmod->brillianceH - 0.5) * (bkey - 30) / 30;
	}

	csmod->chanmods[channel].keybrill[voice->index] =
		csmod->brilliance *
		(
			/* Key brilliance equalisation */
			bkey
		+
			/* Velocity */
			voice->velocity * csmod->chanmods[channel].brilliance
		+
			/* Poly/pressure */
			(voice->chanpressure + voice->press)
				* csmod->chanmods[channel].polybrilliance
		);

	csmod->chanmods[channel].keylevel[voice->index] =
		(
			/* Key volume equalisation */
			lkey
		+
			/* Velocity */
			voice->velocity * csmod->chanmods[channel].level
		+
			/* Poly/pressure */
			(voice->chanpressure + voice->press)
				* csmod->chanmods[channel].polylevel
		);

/*
if (voice->flags & (BRISTOL_KEYON|BRISTOL_KEYREON))
printf("Voice %i %i: %1.2f/%1.2f %1.2f/%1.2f\n", voice->key.key, voice->key.key - 24,
csmod->chanmods[channel].keybrill[voice->index],
csmod->chanmods[channel].keylevel[voice->index],
bkey, lkey);
*/

/* DONE Brilliance and level evaluation */

/* DCO MOD processing */
	bufmerge(freqbuf, 1.0, scratchbuf, 0.0, audiomain->samplecount);

	audiomain->palette[16]->specs->io[0].buf = NULL;
	audiomain->palette[16]->specs->io[1].buf = NULL;
	audiomain->palette[16]->specs->io[2].buf = NULL;
	audiomain->palette[16]->specs->io[3].buf = NULL;
	audiomain->palette[16]->specs->io[4].buf = pwmbuf;
	audiomain->palette[16]->specs->io[5].buf = NULL;
	audiomain->palette[16]->specs->io[6].buf = NULL;
	/*
	 * Run PWM LFO and put out a sine wave
	 */
	(*baudio->sound[0 + layer]).operate(
		(audiomain->palette)[16],
		voice,
		(*baudio->sound[0 + layer]).param,
		baudio->locals[voice->index][0 + layer]);
	bufmerge(pwmbuf, 0.0, pwmbuf, csmod->chanmods[channel].pwmdepth,
		audiomain->samplecount);

	/* Put in an amount of LFO1 mod if configured */
	bufmerge(lfo1buf, csmod->lfo2vco, scratchbuf, 1.0, audiomain->samplecount);
/* End DCO-1 MOD processing */

/* DCO-1 processing */
	bristolbzero(sinebuf, audiomain->segmentsize);
	bristolbzero(dco1buf, audiomain->segmentsize);
	/*
	 * Run DCO-1. 
	 */
	audiomain->palette[31]->specs->io[0].buf = scratchbuf;
	audiomain->palette[31]->specs->io[1].buf = dco1buf;
	audiomain->palette[31]->specs->io[2].buf = pwmbuf; /* PWM buff */
	audiomain->palette[31]->specs->io[3].buf = sinebuf;

	(*baudio->sound[1 + layer]).operate(
		(audiomain->palette)[31],
		voice,
		(*baudio->sound[1 + layer]).param,
		voice->locals[voice->index][1 + layer]);
/* End of DCO-1 processing */

	bufmerge(noisebuf, csmod->chanmods[channel].noiselevel,
		dco1buf, 96.0, audiomain->samplecount);

/* ADSR Processing - first the filter env, then the AMP env */
	audiomain->palette[1]->specs->io[0].buf = adsr1buf;
	(*baudio->sound[4 + layer]).operate(
		(audiomain->palette)[1],
		voice,
		(*baudio->sound[4 + layer]).param,
		voice->locals[voice->index][4 + layer]);
	audiomain->palette[1]->specs->io[0].buf = adsr2buf;
	(*baudio->sound[5 + layer]).operate(
		(audiomain->palette)[1],
		voice,
		(*baudio->sound[5 + layer]).param,
		voice->locals[voice->index][5 + layer]);
/* End of ADSR Processing */

/* Filter processing */
	bufmerge(lfo1buf, csmod->lfo2vcf, adsr1buf, 1.0, audiomain->samplecount);
	bristolbzero(filtbuf, audiomain->segmentsize);
	bristolbzero(scratchbuf, audiomain->segmentsize);
	bufadd(adsr1buf,
		csmod->chanmods[channel].filterbrill
			* csmod->chanmods[channel].keybrill[voice->index],
		audiomain->samplecount);
	// dummy filter:
	audiomain->palette[3]->specs->io[0].buf = dco1buf;
	audiomain->palette[3]->specs->io[1].buf = adsr1buf;
	audiomain->palette[3]->specs->io[2].buf = scratchbuf;
	(*baudio->sound[2 + layer]).operate(
		(audiomain->palette)[3],
		voice,
		(*baudio->sound[2 + layer]).param,
		baudio->locals[voice->index][2 + layer]);
	audiomain->palette[3]->specs->io[0].buf = scratchbuf;
	audiomain->palette[3]->specs->io[1].buf = adsr1buf;
	audiomain->palette[3]->specs->io[2].buf = filtbuf;
	(*baudio->sound[3 + layer]).operate(
		(audiomain->palette)[3],
		voice,
		(*baudio->sound[3 + layer]).param,
		baudio->locals[voice->index][3 + layer]);
/* End of Filter processing */

/* VCA processing */
	bristolbzero(outbuf, audiomain->segmentsize);

	bufmerge(sinebuf, 1.0,
		filtbuf, csmod->chanmods[channel].filterlevel, audiomain->samplecount);

	/* Put in an amount of LFO1 mod if configured */
	bufmerge(lfo1buf, csmod->lfo2vca, adsr2buf, 5.0, audiomain->samplecount);

	audiomain->palette[2]->specs->io[0].buf = filtbuf;
	audiomain->palette[2]->specs->io[1].buf = adsr2buf;
	audiomain->palette[2]->specs->io[2].buf = outbuf;
	(*baudio->sound[17]).operate(
		(audiomain->palette)[2],
		voice,
		(*baudio->sound[17]).param,
		baudio->locals[voice->index][17]);
/* VCA processing */
}

int
operateOneCs80(audioMain *audiomain, Baudio *baudio,
bristolVoice *voice, float *startbuf)
{
	csmods *mixlocal = (csmods *) baudio->mixlocals;
	int koflag = 0;

	if (freqbuf == NULL)
		return(0);

	if ((mixlocal->flags & CS80_NOISE_MULTI) == 0)
	{
		bristolbzero(noisebuf, audiomain->segmentsize);

		audiomain->palette[4]->specs->io[0].buf = noisebuf;
		(*baudio->sound[16]).operate(
			(audiomain->palette)[4],
			voice,
			(*baudio->sound[16]).param,
			voice->locals[voice->index][16]);
	}

/* One LFO MULTI */
	/*
	 * LFO. We need to review flags to decide where to place our buffers, kick
	 * off with a sine wave and take it from there (sine should be the default
	 * if all others are deselected?).
	 */
	if ((mixlocal->flags & CS80_LFO1_MULTI) == 0)
	{
		/* bristolbzero(scratchbuf, audiomain->segmentsize); */
		audiomain->palette[16]->specs->io[0].buf = noisebuf;
		audiomain->palette[16]->specs->io[1].buf = 0;
		audiomain->palette[16]->specs->io[2].buf = 0;
		audiomain->palette[16]->specs->io[3].buf = 0;
		audiomain->palette[16]->specs->io[4].buf = 0;
		audiomain->palette[16]->specs->io[5].buf = 0;

		switch (mixlocal->flags & CS80_LFO1_MASK) {
			case CS80_LFO1_TRI:
				audiomain->palette[16]->specs->io[1].buf = lfo1buf;
				break;
			case CS80_LFO1_RAMP:
				audiomain->palette[16]->specs->io[5].buf = lfo1buf;
				break;
			case CS80_LFO1_SAW:
				audiomain->palette[16]->specs->io[6].buf = lfo1buf;
				break;
			case CS80_LFO1_SQR:
				audiomain->palette[16]->specs->io[2].buf = lfo1buf;
				break;
			case CS80_LFO1_SH:
				audiomain->palette[16]->specs->io[3].buf = lfo1buf;
				break;
			default:
				audiomain->palette[16]->specs->io[4].buf = lfo1buf;
				break;
		}

		(*baudio->sound[15]).operate(
			(audiomain->palette)[16],
			voice,
			(*baudio->sound[15]).param,
			baudio->locals[voice->index][15]);
	}

	fillFreqTable(baudio, voice, freqbuf, audiomain->samplecount, 1);

	/*
	 * I need logic here to know when both layers have finished before setting
	 * the layer flags to DONE. The layers do not need their own logic for the
	 * two envelope they manage since the filter envelope only uses KEYON/REON
	 * events, it never closes the voice.
	 */
	operateOneCs80Channel(audiomain, baudio, voice, &(baudio->sound[0]), CH_I);
	bufmerge(outbuf, mixlocal->channelmix * mixlocal->lpan[CH_I],
		mixlocal->lout, 1.0,
		audiomain->samplecount);
	bufmerge(outbuf, mixlocal->channelmix * mixlocal->rpan[CH_I],
		mixlocal->rout, 1.0,
		audiomain->samplecount);
	if (voice->flags & BRISTOL_KEYDONE)
		koflag++;

	operateOneCs80Channel(audiomain, baudio, voice, &(baudio->sound[6]), CH_II);
	bufmerge(outbuf, (1.0 - mixlocal->channelmix) * mixlocal->lpan[CH_II],
		mixlocal->lout, 1.0,
		audiomain->samplecount);
	bufmerge(outbuf, (1.0 - mixlocal->channelmix) * mixlocal->rpan[CH_II],
		mixlocal->rout, 1.0,
		audiomain->samplecount);
	if (voice->flags & BRISTOL_KEYDONE)
		koflag++;

	if (koflag != 2)
		voice->flags &= ~BRISTOL_KEYDONE;

	return(0);
}

int
cs80Postops(audioMain *audiomain, Baudio *baudio,
bristolVoice *voice, register float *startbuf)
{
	if (freqbuf == NULL)
		return(0);

	/*
	 * Panning and global effects. Note that the layers also have pan so as the
	 * FX kick in the stereo positioning will get damaged due to the vibraChorus
	 * being mono.
	 */
	bufmerge(((csmods *) baudio->mixlocals)->lout,
		((csmods *) baudio->mixlocals)->gain,
		baudio->leftbuf, 1.0,
		audiomain->samplecount);
	bufmerge(((csmods *) baudio->mixlocals)->rout,
		((csmods *) baudio->mixlocals)->gain,
		baudio->rightbuf, 1.0,
		audiomain->samplecount);

	return(0);
}

int
static bristolCs80Destroy(audioMain *audiomain, Baudio *baudio)
{
	/*
	 * The following can be left up to the library. If we free this here then
	 * we do need to NULL the pointer otherwise glibc may get picky.
	 *
	 * We do need to clean this up, though.
	 */
	bristolfree(((csmods *) baudio->mixlocals)->chanmods[CH_I].keybrill);
	bristolfree(((csmods *) baudio->mixlocals)->chanmods[CH_II].keybrill);
	bristolfree(((csmods *) baudio->mixlocals)->chanmods[CH_I].keylevel);
	bristolfree(((csmods *) baudio->mixlocals)->chanmods[CH_II].keylevel);

	bristolfree(((csmods *) baudio->mixlocals)->lout);
	bristolfree(((csmods *) baudio->mixlocals)->rout);

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
	if (sinebuf != NULL)
		bristolfree(sinebuf);
	if (modbuf != NULL)
		bristolfree(modbuf);

	freqbuf = NULL;
	adsr1buf = NULL;
	adsr2buf = NULL;
	filtbuf = NULL;
	dco1buf = NULL;
	zerobuf = NULL;
	outbuf = NULL;
	lfo1buf = NULL;
	lfo2buf = NULL;
	noisebuf = NULL;
	scratchbuf = NULL;
	pwmbuf = NULL;
	modbuf = NULL;
	sinebuf = NULL;
}

int
bristolCs80Init(audioMain *audiomain, Baudio *baudio)
{
	printf("initialising cs80: %i voices\n", baudio->voicecount);

	/*
	 * This is going to be quite a big list since the synth effectively has
	 * two layers however since it is not split or double then we are going
	 * to process it as a single entity.
	 *
	 * Channel operators:
	 *
	 *	LFO - PWM
	 *	DCO - modified bitone oscillator.
	 *	VCF - two, one HP onoe LP
	 *	ENV - two, filter and amp. One needs some mods for IL
	 *
	 * Global operators:
	 *
	 *	Noise source - just insert a single source.
	 *	RingMod
	 *	LFO - subosc modifier.
	 *	VibraChorus
	 *	Tremelo
	 *
	 * We need to work on ensuring that brilliance is implemented and that
	 * polyphonic aftertouch is emulated. Brilliance should affect serveral 
	 * components:
	 *
	 *	Filter cutoff should be adjusted
	 *	VCO harmoonics should be layred in
	 *
	 * The brilliance also has several options that probably need to be totalled
	 * up to a maximum value then applies:
	 *
	 * 	Global setting
	 * 	Key Velocity setting
	 * 	Polyphonic pressure setting
	 * 	Keyboard span setting
	 *
	 * Keyboard response also has its own modifiers from low to high. Touch also
	 * drives into the global LFO speed and moification depths but need to find
	 * out if this was velocity or polypressure.
	 */
	baudio->soundCount = 19; /* Number of operators in this voice */

	/*
	 * Assign an array of sound pointers.
	 */
	baudio->sound = (bristolSound **)
		bristolmalloc0(sizeof(bristolOP *) * baudio->soundCount);
	baudio->effect = (bristolSound **)
		bristolmalloc0(sizeof(bristolOP *) * baudio->soundCount);

	/*
	 * Two layer parameters each in turn to allow us to put is some routines
	 * Will use a single noise source.
	 */
	/* LFO for PWM */
	initSoundAlgo(16,	0, baudio, audiomain, baudio->sound);
	/* One oscillator - modified bitone osc for brilliance */
	initSoundAlgo(31,	1, baudio, audiomain, baudio->sound);
	/* two filter with ADSR */
	initSoundAlgo( 3,	2, baudio, audiomain, baudio->sound);
	initSoundAlgo( 3,	3, baudio, audiomain, baudio->sound);
	initSoundAlgo(32,	4, baudio, audiomain, baudio->sound);
	/* Amp ADSR */
	initSoundAlgo( 1,	5, baudio, audiomain, baudio->sound);

	/* LFO for PWM */
	initSoundAlgo(16,	6, baudio, audiomain, baudio->sound);
	/* One oscillator - these still need harmonic mods */
	initSoundAlgo(31,	7, baudio, audiomain, baudio->sound);
	/* Two filter with ADSR */
	initSoundAlgo( 3,	8, baudio, audiomain, baudio->sound);
	initSoundAlgo( 3,	9, baudio, audiomain, baudio->sound);
	initSoundAlgo(32,	10, baudio, audiomain, baudio->sound);
	/* Amp ADSR */
	initSoundAlgo( 1,	11, baudio, audiomain, baudio->sound);

	/* LFO/Env and RingMod Effects section */
	initSoundAlgo(16,	12, baudio, audiomain, baudio->sound);
	initSoundAlgo( 1,	13, baudio, audiomain, baudio->sound);
	initSoundAlgo(20,	14, baudio, audiomain, baudio->sound);

	/* GP LFO */
	initSoundAlgo(16,	15, baudio, audiomain, baudio->sound);

	/* Noise */
	initSoundAlgo(4,	16, baudio, audiomain, baudio->sound);

	/* AMP */
	initSoundAlgo(2,	17, baudio, audiomain, baudio->sound);

	/* VibraChorus - Tremelo is an osc that gets mixed in with the AMP ADSR */
	initSoundAlgo(16,	18, baudio, audiomain, baudio->sound);

	baudio->param = cs80Controller;
	baudio->destroy = bristolCs80Destroy;
	baudio->operate = operateOneCs80;
	baudio->preops = cs80Preops;
	baudio->postops = cs80Postops;

	/*
	 * Put in integrated effects here.
	initSoundAlgo(12, 0, baudio, audiomain, baudio->effect);
	 */

	baudio->mixlocals = (float *) bristolmalloc0(sizeof(csmods));

	/*
	 * If we were the first requested emulation then this is the lower layer
	 * and needs to be flagged here to ensure correct 'stereo' panning.
	 */
	((csmods *) baudio->mixlocals)->chanmods[CH_I].keybrill = (float *)
		bristolmalloc0(sizeof(float) * BRISTOL_MAXVOICECOUNT);
	((csmods *) baudio->mixlocals)->chanmods[CH_II].keybrill = (float *)
		bristolmalloc0(sizeof(float) * BRISTOL_MAXVOICECOUNT);
	((csmods *) baudio->mixlocals)->chanmods[CH_I].keylevel = (float *)
		bristolmalloc0(sizeof(float) * BRISTOL_MAXVOICECOUNT);
	((csmods *) baudio->mixlocals)->chanmods[CH_II].keylevel = (float *)
		bristolmalloc0(sizeof(float) * BRISTOL_MAXVOICECOUNT);

	((csmods *) baudio->mixlocals)->lout =
		(float *) bristolmalloc0(audiomain->segmentsize);
	((csmods *) baudio->mixlocals)->rout =
		(float *) bristolmalloc0(audiomain->segmentsize);

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
	if (sinebuf == NULL)
		sinebuf = (float *) bristolmalloc0(audiomain->segmentsize);
	if (modbuf == NULL)
		modbuf = (float *) bristolmalloc0(audiomain->segmentsize);

	baudio->mixflags |= BRISTOL_STEREO;

	return(0);
}

