
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

#include "bristol.h"
#include "bristolmm.h"

extern void mapVelocityCurve(int, float []);

/*
 * Use of these bitone global buffers will be an issue with use of multiple
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
static float *syncbuf = (float *) NULL;

/*
 * These need to go into some local structure for multiple instances
 * of the bitone - malloc()ed into the baudio->mixlocals.
 */
#define BITONE_LOWER_LAYER	0x00000001
#define BITONE_LFO1_UNI		0x00000002
#define BITONE_LFO2_UNI		0x00000004
#define BITONE_NOISE_UNI	0x00000008
#define BITONE_LFO1_MASK	0x000000f0
#define BITONE_LFO1_TRI		0x00000010
#define BITONE_LFO1_RAMP	0x00000020
#define BITONE_LFO1_SQR		0x00000040
#define BITONE_LFO1_SH		0x00000080
#define BITONE_LFO2_MASK	0x00000f00
#define BITONE_LFO2_TRI		0x00000100
#define BITONE_LFO2_RAMP	0x00000200
#define BITONE_LFO2_SQR		0x00000400
#define BITONE_LFO2_SH		0x00000800
#define BITONE_SYNC			0x00001000
#define BITONE_ENV_INV		0x00002000
#define BITONE_FM			0x00004000
#define BITONE_NO_DCO1		0x00008000

typedef struct bMods {
	unsigned int flags;
	float *lout;
	float *rout;
	int voicecount;
	float lpan;
	float rpan;
	float lgain;
	float rgain;
	float noisegain;
	float wheelgain;
	float lfo1DCO1gain;
	float lfo1DCO2gain;
	float lfo1VCFgain;
	float lfo1VCAgain;
	float lfo2DCO1gain;
	float lfo2DCO2gain;
	float lfo2VCFgain;
	float lfo2VCAgain;
	float dco1mix;
	float dco1fm;
	float dco1EnvPWM;
	float dco2EnvPWM;
	float lfo1dco1PWMgain;
	float lfo1dco2PWMgain;
	float lfo2dco1PWMgain;
	float lfo2dco2PWMgain;
	float mwd;
} bmods;

int
bitoneController(Baudio *baudio, u_char operator,
u_char controller, float value)
{
	int tval = value * CONTROLLER_RANGE;

#ifdef DEBUG
	printf("bristolBitoneControl(%i, %i, %f)\n", operator, controller, value);
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
			if (value > 0.0)
				baudio->voicecount = ((bmods *) baudio->mixlocals)->voicecount;
			else {
				if (((bmods *) baudio->mixlocals)->flags & BITONE_LOWER_LAYER)
					baudio->voicecount =
						((bmods *) baudio->mixlocals)->voicecount >> 1;
				else
					baudio->voicecount =
						((bmods *) baudio->mixlocals)->voicecount;
			}
			break;
		case 3:
			((bmods *) baudio->mixlocals)->lgain = value;
			break;
		case 4:
			((bmods *) baudio->mixlocals)->rgain = value;
/* printf("gain: %f %f\n",
((bmods *) baudio->mixlocals)->lgain, ((bmods *) baudio->mixlocals)->rgain); */
			break;
		case 5:
			((bmods *) baudio->mixlocals)->lpan = value;
			break;
		case 6:
			((bmods *) baudio->mixlocals)->rpan = value;
/* printf("pan: %f %f\n",
((bmods *) baudio->mixlocals)->lpan, ((bmods *) baudio->mixlocals)->rpan); */
			break;
		case 7:
			if (value == 0)
				baudio->mixflags &= ~BRISTOL_V_UNISON;
			else
				baudio->mixflags |= BRISTOL_V_UNISON;
			break;
		case 8:
			((bmods *) baudio->mixlocals)->noisegain = value * 48.0;
			break;
		case 12:
			((bmods *) baudio->mixlocals)->wheelgain = value;
			break;
		case 13:
			if (value == 0)
				((bmods *) baudio->mixlocals)->flags &= ~BITONE_LFO1_TRI;
			else {
				((bmods *) baudio->mixlocals)->flags &= ~BITONE_LFO1_MASK;
				((bmods *) baudio->mixlocals)->flags |= BITONE_LFO1_TRI;
			}
			break;
		case 14:
			if (value == 0)
				((bmods *) baudio->mixlocals)->flags &= ~BITONE_LFO1_RAMP;
			else {
				((bmods *) baudio->mixlocals)->flags &= ~BITONE_LFO1_MASK;
				((bmods *) baudio->mixlocals)->flags |= BITONE_LFO1_RAMP;
			}
			break;
		case 15:
			if (value == 0)
				((bmods *) baudio->mixlocals)->flags &= ~BITONE_LFO1_SQR;
			else {
				((bmods *) baudio->mixlocals)->flags &= ~BITONE_LFO1_MASK;
				((bmods *) baudio->mixlocals)->flags |= BITONE_LFO1_SQR;
			}
			break;
		case 16:
			if (value == 0)
				((bmods *) baudio->mixlocals)->flags &= ~BITONE_LFO1_SH;
			else {
				((bmods *) baudio->mixlocals)->flags &= ~BITONE_LFO1_MASK;
				((bmods *) baudio->mixlocals)->flags |= BITONE_LFO1_SH;
			}
			break;
		case 17:
			((bmods *) baudio->mixlocals)->lfo1DCO1gain = value;
			break;
		case 18:
			((bmods *) baudio->mixlocals)->lfo1DCO2gain = value;
			break;
		case 19:
			((bmods *) baudio->mixlocals)->lfo1VCFgain = value * 0.2;
			break;
		case 20:
			((bmods *) baudio->mixlocals)->lfo1VCAgain = value * 0.5;
			break;
		case 21:
			if (value == 0)
				((bmods *) baudio->mixlocals)->flags &= ~BITONE_LFO2_TRI;
			else {
				((bmods *) baudio->mixlocals)->flags &= ~BITONE_LFO2_MASK;
				((bmods *) baudio->mixlocals)->flags |= BITONE_LFO2_TRI;
			}
			break;
		case 22:
			if (value == 0)
				((bmods *) baudio->mixlocals)->flags &= ~BITONE_LFO2_RAMP;
			else {
				((bmods *) baudio->mixlocals)->flags &= ~BITONE_LFO2_MASK;
				((bmods *) baudio->mixlocals)->flags |= BITONE_LFO2_RAMP;
			}
			break;
		case 23:
			if (value == 0)
				((bmods *) baudio->mixlocals)->flags &= ~BITONE_LFO2_SQR;
			else {
				((bmods *) baudio->mixlocals)->flags &= ~BITONE_LFO2_MASK;
				((bmods *) baudio->mixlocals)->flags |= BITONE_LFO2_SQR;
			}
			break;
		case 24:
			if (value == 0)
				((bmods *) baudio->mixlocals)->flags &= ~BITONE_LFO2_SH;
			else {
				((bmods *) baudio->mixlocals)->flags &= ~BITONE_LFO2_MASK;
				((bmods *) baudio->mixlocals)->flags |= BITONE_LFO2_SH;
			}
			break;
		case 25:
			((bmods *) baudio->mixlocals)->lfo2DCO1gain = value;
			break;
		case 26:
			((bmods *) baudio->mixlocals)->lfo2DCO2gain = value;
			break;
		case 27:
			((bmods *) baudio->mixlocals)->lfo2VCFgain = value * 0.2;
			break;
		case 28:
			((bmods *) baudio->mixlocals)->lfo2VCAgain = value * 0.5;
			break;
		case 29:
			if (value == 0)
				((bmods *) baudio->mixlocals)->flags &= ~BITONE_LFO1_UNI;
			else
				((bmods *) baudio->mixlocals)->flags |= BITONE_LFO1_UNI;
			break;
		case 30:
			if (value == 0)
				((bmods *) baudio->mixlocals)->flags &= ~BITONE_LFO2_UNI;
			else
				((bmods *) baudio->mixlocals)->flags |= BITONE_LFO2_UNI;
			break;
		case 31:
			if (value == 0)
				((bmods *) baudio->mixlocals)->flags &= ~BITONE_NOISE_UNI;
			else
				((bmods *) baudio->mixlocals)->flags |= BITONE_NOISE_UNI;
			break;
		case 32:
			((bmods *) baudio->mixlocals)->dco1EnvPWM = value * 64;
			break;
		case 33:
			((bmods *) baudio->mixlocals)->dco2EnvPWM = value * 64;
			break;
		case 34:
			((bmods *) baudio->mixlocals)->lfo1dco1PWMgain = value * 64;
			break;
		case 35:
			((bmods *) baudio->mixlocals)->lfo1dco2PWMgain = value * 64;
			break;
		case 36:
			((bmods *) baudio->mixlocals)->lfo2dco1PWMgain = value * 64;
			break;
		case 37:
			((bmods *) baudio->mixlocals)->lfo2dco2PWMgain = value * 64;
			break;
		case 38:
			if (value == 0)
				((bmods *) baudio->mixlocals)->flags &= ~BITONE_SYNC;
			else
				((bmods *) baudio->mixlocals)->flags |= BITONE_SYNC;
			break;
		case 39:
			if (value == 0)
				((bmods *) baudio->mixlocals)->flags &= ~BITONE_ENV_INV;
			else
				((bmods *) baudio->mixlocals)->flags |= BITONE_ENV_INV;
			break;
		case 40:
			((bmods *) baudio->mixlocals)->dco1fm = value * 0.01;
			break;
		case 41:
			((bmods *) baudio->mixlocals)->dco1mix = value * 2048.0;
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
		case 43:
			((bmods *) baudio->mixlocals)->mwd = value;
			break;
		case 101:
		case 126:
			/* This is the dummy entry */
			break;
	}
	return(0);
}

int
bitonePreops(audioMain *audiomain, Baudio *baudio,
bristolVoice *voice, register float *startbuf)
{
	if (freqbuf == NULL)
		return(0);

	bristolbzero(((bmods *) baudio->mixlocals)->lout, audiomain->segmentsize);
	bristolbzero(((bmods *) baudio->mixlocals)->rout, audiomain->segmentsize);

	baudio->contcontroller[1] *= ((bmods *) baudio->mixlocals)->mwd;

	/*
	 * This will eventually manage LFO_MULTI options, separately per LFO.
	 */
	if (((bmods *) baudio->mixlocals)->flags & BITONE_NOISE_UNI)
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
	if (((bmods *) baudio->mixlocals)->flags & BITONE_LFO1_UNI)
	{
		bristolbzero(scratchbuf, audiomain->segmentsize);
		bristolbzero(lfo1buf, audiomain->segmentsize);

		audiomain->palette[16]->specs->io[0].buf = noisebuf;
		audiomain->palette[16]->specs->io[1].buf = 0;
		audiomain->palette[16]->specs->io[2].buf = 0;
		audiomain->palette[16]->specs->io[3].buf = 0;
		audiomain->palette[16]->specs->io[4].buf = 0;
		audiomain->palette[16]->specs->io[5].buf = 0;

		switch (((bmods *) baudio->mixlocals)->flags & BITONE_LFO1_MASK) {
			case BITONE_LFO1_TRI:
				audiomain->palette[16]->specs->io[1].buf = scratchbuf;
				break;
			case BITONE_LFO1_RAMP:
				audiomain->palette[16]->specs->io[5].buf = scratchbuf;
				break;
			case BITONE_LFO1_SQR:
				audiomain->palette[16]->specs->io[2].buf = scratchbuf;
				break;
			case BITONE_LFO1_SH:
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

	/*
	 * LFO. We need to review flags to decide where to place our buffers, kick
	 * off with a sine wave and take it from there (sine should be the default
	 * if all others are deselected?).
	 */
	if (((bmods *) baudio->mixlocals)->flags & BITONE_LFO2_UNI)
	{
		bristolbzero(scratchbuf, audiomain->segmentsize);
		bristolbzero(lfo2buf, audiomain->segmentsize);

		audiomain->palette[16]->specs->io[0].buf = noisebuf;
		audiomain->palette[16]->specs->io[1].buf = 0;
		audiomain->palette[16]->specs->io[2].buf = 0;
		audiomain->palette[16]->specs->io[3].buf = 0;
		audiomain->palette[16]->specs->io[4].buf = 0;
		audiomain->palette[16]->specs->io[5].buf = 0;
		audiomain->palette[16]->specs->io[6].buf = 0;

		switch (((bmods *) baudio->mixlocals)->flags & BITONE_LFO2_MASK) {
			case BITONE_LFO2_TRI:
				audiomain->palette[16]->specs->io[1].buf = scratchbuf;
				break;
			case BITONE_LFO2_RAMP:
				audiomain->palette[16]->specs->io[6].buf = scratchbuf;
				break;
			case BITONE_LFO2_SQR:
				audiomain->palette[16]->specs->io[2].buf = scratchbuf;
				break;
			case BITONE_LFO2_SH:
				audiomain->palette[16]->specs->io[3].buf = scratchbuf;
				break;
			default:
				audiomain->palette[16]->specs->io[4].buf = scratchbuf;
				break;
		}

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
	}

	return(0);
}

int
operateOneBitone(audioMain *audiomain, Baudio *baudio,
bristolVoice *voice, register float *startbuf)
{
	float lg, rg;

	/*
	 * Kick this off with a single oscillator into an AMP with env. When this
	 * works with memories from the GUI and layers in the engine then we can
	 * extend the features.
	 */
	if (freqbuf == NULL)
		return(0);

/* printf("operate %i, %x\n", voice->index, baudio); */
	bristolbzero(dco1buf, audiomain->segmentsize);
	bristolbzero(dco2buf, audiomain->segmentsize);
	bristolbzero(zerobuf, audiomain->segmentsize);
	bristolbzero(outbuf, audiomain->segmentsize);
	bristolbzero(filtbuf, audiomain->segmentsize);

/* Two LFO MULTI */
	if ((((bmods *) baudio->mixlocals)->flags & BITONE_NOISE_UNI) == 0)
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
	if ((((bmods *) baudio->mixlocals)->flags & BITONE_LFO1_UNI) == 0)
	{
		bristolbzero(scratchbuf, audiomain->segmentsize);
		bristolbzero(lfo1buf, audiomain->segmentsize);

		audiomain->palette[16]->specs->io[0].buf = noisebuf;
		audiomain->palette[16]->specs->io[1].buf = 0;
		audiomain->palette[16]->specs->io[2].buf = 0;
		audiomain->palette[16]->specs->io[3].buf = 0;
		audiomain->palette[16]->specs->io[4].buf = 0;
		audiomain->palette[16]->specs->io[5].buf = 0;

		switch (((bmods *) baudio->mixlocals)->flags & BITONE_LFO1_MASK) {
			case BITONE_LFO1_TRI:
				audiomain->palette[16]->specs->io[1].buf = scratchbuf;
				break;
			case BITONE_LFO1_RAMP:
				audiomain->palette[16]->specs->io[5].buf = scratchbuf;
				break;
			case BITONE_LFO1_SQR:
				audiomain->palette[16]->specs->io[2].buf = scratchbuf;
				break;
			case BITONE_LFO1_SH:
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

	/*
	 * LFO. We need to review flags to decide where to place our buffers, kick
	 * off with a sine wave and take it from there (sine should be the default
	 * if all others are deselected?).
	 */
	if ((((bmods *) baudio->mixlocals)->flags & BITONE_LFO2_UNI) == 0)
	{
		bristolbzero(scratchbuf, audiomain->segmentsize);
		bristolbzero(lfo2buf, audiomain->segmentsize);

		audiomain->palette[16]->specs->io[0].buf = noisebuf;
		audiomain->palette[16]->specs->io[1].buf = 0;
		audiomain->palette[16]->specs->io[2].buf = 0;
		audiomain->palette[16]->specs->io[3].buf = 0;
		audiomain->palette[16]->specs->io[4].buf = 0;
		audiomain->palette[16]->specs->io[5].buf = 0;
		audiomain->palette[16]->specs->io[6].buf = 0;

		switch (((bmods *) baudio->mixlocals)->flags & BITONE_LFO2_MASK) {
			case BITONE_LFO2_TRI:
				audiomain->palette[16]->specs->io[1].buf = scratchbuf;
				break;
			case BITONE_LFO2_RAMP:
				audiomain->palette[16]->specs->io[6].buf = scratchbuf;
				break;
			case BITONE_LFO2_SQR:
				audiomain->palette[16]->specs->io[2].buf = scratchbuf;
				break;
			case BITONE_LFO2_SH:
				audiomain->palette[16]->specs->io[3].buf = scratchbuf;
				break;
			default:
				audiomain->palette[16]->specs->io[4].buf = scratchbuf;
				break;
		}

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
	}
/* End of LFO processing */

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
/* End of ADSR Processing */

	if (((bmods *) baudio->mixlocals)->flags & BITONE_ENV_INV)
	{
		int i, s = audiomain->samplecount;

		for (i = 0; i < s; i++)
			adsr1buf[i] = -adsr1buf[i];
	}

/* DCO-1 processing */
	/*
	 * Run DCO-1. the first implementation used the ARP oscillator however it
	 * will eventually be Bit specific and optimised for this emulator.
	 */
	fillFreqTable(baudio, voice, freqbuf, audiomain->samplecount, 1);

	bufmerge(freqbuf, 1.0, scratchbuf, 0.0, audiomain->samplecount);
	if (((bmods *) baudio->mixlocals)->lfo1DCO1gain != 0.0)
		bufmerge(lfo1buf,
			(((bmods *) baudio->mixlocals)->lfo1DCO1gain
					* ((1.0 - ((bmods *) baudio->mixlocals)->wheelgain)
				+ baudio->contcontroller[1]
					* ((bmods *) baudio->mixlocals)->wheelgain)) * 0.05,
			scratchbuf, 1.0, audiomain->samplecount);
	if (((bmods *) baudio->mixlocals)->lfo2DCO1gain != 0.0)
		bufmerge(lfo2buf,
			(((bmods *) baudio->mixlocals)->lfo2DCO1gain
					* ((1.0 - ((bmods *) baudio->mixlocals)->wheelgain)
				+ baudio->contcontroller[1]
					* ((bmods *) baudio->mixlocals)->wheelgain)) * 0.05,
			scratchbuf, 1.0, audiomain->samplecount);

	if (((bmods *) baudio->mixlocals)->lfo1dco1PWMgain != 0.0)
		bufmerge(lfo1buf, ((bmods *) baudio->mixlocals)->lfo1dco1PWMgain,
			pwmbuf, 0.0, audiomain->samplecount);
	else
		bristolbzero(pwmbuf, audiomain->segmentsize);
	if (((bmods *) baudio->mixlocals)->dco1EnvPWM != 0.0)
		bufmerge(adsr1buf, ((bmods *) baudio->mixlocals)->dco1EnvPWM,
			pwmbuf, 1.0, audiomain->samplecount);
	if (((bmods *) baudio->mixlocals)->lfo2dco1PWMgain != 0.0)
		bufmerge(lfo2buf, ((bmods *) baudio->mixlocals)->lfo2dco1PWMgain,
			pwmbuf, 1.0, audiomain->samplecount);

	audiomain->palette[30]->specs->io[0].buf = scratchbuf;
	audiomain->palette[30]->specs->io[1].buf = dco1buf;
	audiomain->palette[30]->specs->io[2].buf = pwmbuf; /* PWM buff */
	audiomain->palette[30]->specs->io[3].buf = zerobuf; /* Sync buff */
	audiomain->palette[30]->specs->io[4].buf = syncbuf; /* Sync out buff */

	(*baudio->sound[4]).operate(
		(audiomain->palette)[30],
		voice,
		(*baudio->sound[4]).param,
		voice->locals[voice->index][4]);
/* End of DCO-1 processing */

/* DCO-2 processing */
	/*
	 * Then DCO-2 and its mods.
	 */
	bufmerge(freqbuf, 1.0, scratchbuf, 0.0, audiomain->samplecount);
	if (((bmods *) baudio->mixlocals)->lfo1DCO2gain != 0.0)
		bufmerge(lfo1buf,
			(((bmods *) baudio->mixlocals)->lfo1DCO2gain
					* ((1.0 - ((bmods *) baudio->mixlocals)->wheelgain)
				+ baudio->contcontroller[1]
					* ((bmods *) baudio->mixlocals)->wheelgain)) * 0.05,
			scratchbuf, 1.0, audiomain->samplecount);
	if (((bmods *) baudio->mixlocals)->lfo2DCO2gain != 0.0)
		bufmerge(lfo2buf,
			(((bmods *) baudio->mixlocals)->lfo2DCO2gain
					* ((1.0 - ((bmods *) baudio->mixlocals)->wheelgain)
				+ baudio->contcontroller[1]
					* ((bmods *) baudio->mixlocals)->wheelgain)) * 0.05,
			scratchbuf, 1.0, audiomain->samplecount);
	if (((bmods *) baudio->mixlocals)->dco1fm != 0.0)
		bufmerge(dco1buf, ((bmods *) baudio->mixlocals)->dco1fm,
			scratchbuf, 1.0, audiomain->samplecount);

	if (((bmods *) baudio->mixlocals)->dco2EnvPWM != 0.0)
		bufmerge(adsr1buf, ((bmods *) baudio->mixlocals)->dco2EnvPWM,
			pwmbuf, 0.0, audiomain->samplecount);
	else
		bristolbzero(pwmbuf, audiomain->segmentsize);
	if (((bmods *) baudio->mixlocals)->lfo1dco2PWMgain != 0.0)
		bufmerge(lfo1buf, ((bmods *) baudio->mixlocals)->lfo1dco2PWMgain,
			pwmbuf, 1.0, audiomain->samplecount);
	if (((bmods *) baudio->mixlocals)->lfo2dco2PWMgain != 0.0)
		bufmerge(lfo2buf, ((bmods *) baudio->mixlocals)->lfo2dco2PWMgain,
			pwmbuf, 1.0, audiomain->samplecount);

	audiomain->palette[30]->specs->io[0].buf = scratchbuf;
	audiomain->palette[30]->specs->io[1].buf = dco2buf;
	audiomain->palette[30]->specs->io[2].buf = pwmbuf; /* PWM buff */
	if (((bmods *) baudio->mixlocals)->flags & BITONE_SYNC)
		audiomain->palette[30]->specs->io[3].buf = syncbuf; /* Sync buff */
	else
		audiomain->palette[30]->specs->io[3].buf = zerobuf;
	audiomain->palette[30]->specs->io[4].buf = NULL;

	(*baudio->sound[5]).operate(
		(audiomain->palette)[30],
		voice,
		(*baudio->sound[5]).param,
		voice->locals[voice->index][5]);
/* DCO-2 processing */

	/* Add in DCO-1 */
	bufmerge(dco1buf, ((bmods *) baudio->mixlocals)->dco1mix,
		dco2buf, 2048.0, audiomain->samplecount);

	/* Add in the noise */
	if (((bmods *) baudio->mixlocals)->noisegain != 0.0)
		bufmerge(noisebuf, ((bmods *) baudio->mixlocals)->noisegain,
			dco2buf, 1.0, audiomain->samplecount);

/* Filter processing */
	if (((bmods *) baudio->mixlocals)->lfo1VCFgain != 0.0)
		bufmerge(lfo1buf,
			((bmods *) baudio->mixlocals)->lfo1VCFgain,
			adsr1buf, 1.0, audiomain->samplecount);
	if (((bmods *) baudio->mixlocals)->lfo2VCFgain != 0.0)
		bufmerge(lfo2buf,
			((bmods *) baudio->mixlocals)->lfo2VCFgain,
			adsr1buf, 1.0, audiomain->samplecount);

	/*
	 * Run the DCO output through the filter using the first ADSR
	 */
	audiomain->palette[3]->specs->io[0].buf = dco2buf;
	audiomain->palette[3]->specs->io[1].buf = adsr1buf;
	audiomain->palette[3]->specs->io[2].buf = filtbuf;
	(*baudio->sound[6]).operate(
		(audiomain->palette)[3],
		voice,
		(*baudio->sound[6]).param,
		baudio->locals[voice->index][6]);
/* End of Filter processing */

/* VCA processing */
	if (((bmods *) baudio->mixlocals)->lfo1VCAgain != 0.0)
		bufmerge(lfo1buf,
			((bmods *) baudio->mixlocals)->lfo1VCAgain,
			adsr2buf, 1.0, audiomain->samplecount);
	if (((bmods *) baudio->mixlocals)->lfo2VCAgain != 0.0)
		bufmerge(lfo2buf,
			((bmods *) baudio->mixlocals)->lfo2VCAgain,
			adsr2buf, 1.0, audiomain->samplecount);

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
	/*
	 * This will eventually manage stereo panning of this layer. We need to
	 * do some silly stuff here because I want to. The original could be in
	 * mono or stereo, single/split/double. If it was split/double+stereo then
	 * each synth had its own L/R output, and if split/double+mono then it was
	 * all panned dead centre. That is emulated with gain levels and has no
	 * requirements here. If we are single/mono then we are dead centre pan.
	 *
	 * All well and good.
	 *
	 * If we are single/stereo then output is arbitrarily left or right and I
	 * want that to work. We need to see how many voices we have, and if it is
	 * equal to the configured max we are in single mode and we should inter-
	 * change L/R channel pan based on voice index. If pan is dead centre this
	 * has not effect however if the pan is off centre then the voices will be
	 * sent to an arbitrary side.
	 */
	if ((((bmods *) baudio->mixlocals)->voicecount == baudio->voicecount)
		&& (((bmods *) baudio->mixlocals)->flags & BITONE_LOWER_LAYER))
	{
		if (voice->index & 0x01)
		{
			lg = ((bmods *) baudio->mixlocals)->lpan;
			rg = ((bmods *) baudio->mixlocals)->rpan;
		} else {
			rg = ((bmods *) baudio->mixlocals)->lpan;
			lg = ((bmods *) baudio->mixlocals)->rpan;
		}
	} else {
		lg = ((bmods *) baudio->mixlocals)->lpan;
		rg = ((bmods *) baudio->mixlocals)->rpan;
	}

	if (lg != 0.0)
		bufmerge(outbuf, lg,
			((bmods *) baudio->mixlocals)->lout, 1.0,
			audiomain->samplecount);
	if (rg != 0.0)
		bufmerge(outbuf, rg,
			((bmods *) baudio->mixlocals)->rout, 1.0,
			audiomain->samplecount);
/* Panning */

	return(0);
}

int
bitonePostops(audioMain *audiomain, Baudio *baudio,
bristolVoice *voice, register float *startbuf)
{
	if (freqbuf == NULL)
		return(0);

	bufmerge(((bmods *) baudio->mixlocals)->lout,
		((bmods *) baudio->mixlocals)->lgain * 0.003125,
		baudio->leftbuf, 1.0,
		audiomain->samplecount);
	bufmerge(((bmods *) baudio->mixlocals)->rout,
		((bmods *) baudio->mixlocals)->rgain * 0.003125,
		baudio->rightbuf, 1.0,
		audiomain->samplecount);

	return(0);
}

int
static bristolBitoneDestroy(audioMain *audiomain, Baudio *baudio)
{

	/*
	 * The following can be left up to the library. If we free this here then
	 * we do need to NULL the pointer otherwise glibc may get picky.
	 */
	bristolfree(((bmods *) baudio->mixlocals)->lout);
	bristolfree(((bmods *) baudio->mixlocals)->rout);

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
	if (dco2buf != NULL)
		bristolfree(dco2buf);
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
	if (pwmbuf != NULL)
		bristolfree(pwmbuf);
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
	syncbuf = NULL;

	/*
	 * The following can be left up to the library. If we free this here then
	 * we do need to NULL the pointer otherwise glibc may get picky.
	 */
	bristolfree(baudio->mixlocals);
	baudio->mixlocals = NULL;

	return(0);
}

int
bristolBitoneInit(audioMain *audiomain, Baudio *baudio)
{
	printf("initialising bitone: %i voices\n", baudio->voicecount);
	/*
	 * Two LFO possibly with one envelope each.
	 * Two DCO
	 * Filter with Env
	 * Amp with Env
	 * Noise source
	 */
	baudio->soundCount = 11; /* Number of operators in this voice */
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
	/* Two oscillator */
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

	baudio->param = bitoneController;
	baudio->destroy = bristolBitoneDestroy;
	baudio->operate = operateOneBitone;
	baudio->preops = bitonePreops;
	baudio->postops = bitonePostops;

	/*
	 * Put in integrated effects here.
	initSoundAlgo(12, 0, baudio, audiomain, baudio->effect);
	 */

	baudio->mixlocals = (float *) bristolmalloc0(sizeof(bmods));

	/*
	 * If we were the first requested emulation then this is the lower layer
	 * and needs to be flagged here to ensure correct 'stereo' panning.
	 */
	if (freqbuf == NULL)
		((bmods *) baudio->mixlocals)->flags |= BITONE_LOWER_LAYER;

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
	if (dco2buf == NULL)
		dco2buf = (float *) bristolmalloc0(audiomain->segmentsize);
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
	if (syncbuf == NULL)
		syncbuf = (float *) bristolmalloc0(audiomain->segmentsize);

	baudio->mixflags |= BRISTOL_STEREO;

	return(0);
}

