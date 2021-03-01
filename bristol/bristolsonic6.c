
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

/*
 * We are going to use this file, sonic-6, as the first review of have SMP 
 * support. Prior to this there was a lot of local storage that would prevent
 * having multiple threads running this code and it needs to be made multi-
 * entrant. This means amongst other things burying parameters in the baudio
 * structure passed on the stack however the different operators also need to
 * have this before we can have it generally deployable. There are issues here
 * though. We only have a single set of buffers for the whole baudio, not per
 * voice and if multiple cores start using them the result is non-determinate.
 *
 * This probably means the best we can target is that each baudio can be a 
 * separate thread, but not voices within one baudio.
 *
 * This should only affect the audio engine code, the init operation will only
 * have a single thread.
 */
/*#define DEBUG */

#include "bristol.h"
#include "bristolsonic6.h"

extern int buildCurrentTable(Baudio *, float);

static int
sonic6GlobalController(Baudio *baudio, u_char controller,
u_char operator, float value)
{
	int ivalue = value * CONTROLLER_RANGE;

#ifdef DEBUG
	printf("sonic6GlobalControl(%i, %i, %f)\n", controller, operator, value);
#endif

	/*
	 * Reverb
	 */
	if (controller == 101)
	{
		if (baudio->effect[0] == NULL)
			return(0);

		if (operator == 0)
		{
			baudio->effect[0]->param->param[operator].float_val = value;
			baudio->effect[0]->param->param[operator].int_val = 1;
		} else
			baudio->effect[0]->param->param[operator].float_val = value;
/*
printf("%f F %f, C %f, W %f, G %f ? %f\n",
baudio->effect[0]->param->param[0].float_val,
baudio->effect[0]->param->param[1].float_val,
baudio->effect[0]->param->param[2].float_val,
baudio->effect[0]->param->param[3].float_val,
baudio->effect[0]->param->param[4].float_val,
baudio->effect[0]->param->param[5].float_val);
*/

		return(0);
	}

	if (controller != 126)
		return(0);

	switch (operator) {
		case 0:
			baudio->glide = value * value * baudio->glidemax;
			break;
		case 1:
			SONICLOCAL->mastervolume = value;
			break;
		case 2:
			baudio->gtune = 1.0
				+ (baudio->note_diff - 1)
				* (value * 2 - 1);

			buildCurrentTable(baudio, baudio->gtune);
			alterAllNotes(baudio);
			break;
		case 10:
			SONICLOCAL->directA = value * 128;
			break;
		case 11:
			SONICLOCAL->directB = value * 128;
			break;
		case 12:
			SONICLOCAL->directRM = value * 64;
			break;
		case 13:
			SONICLOCAL->genAB = value;
			break;
		case 14:
			SONICLOCAL->genXY = value;
			break;
		case 15:
			/* Waveform */
			baudio->mixflags &= ~X_WAVE_MASK;
			baudio->mixflags |= (X_TRI << ivalue);
			break;
		case 16:
			/* Mod */
			baudio->mixflags &= ~X_MOD_MASK;
			baudio->mixflags |= (X_MOD_ADSR << ivalue);
			break;
		case 17:
			/* Waveform */
			baudio->mixflags &= ~Y_WAVE_MASK;
			baudio->mixflags |= (Y_TRI << ivalue);
			break;
		case 18:
			/* Mod */
			baudio->mixflags &= ~Y_MOD_MASK;
			baudio->mixflags |= (Y_MOD_ADSR << ivalue);
			break;
		case 19:
			/* Mod */
			if (value == 0)
				baudio->mixflags &= ~X_MULTI;
			else
				baudio->mixflags |= X_MULTI;
			break;
		case 20:
			/* Mod */
			if (value == 0)
				baudio->mixflags &= ~Y_MULTI;
			else
				baudio->mixflags |= Y_MULTI;
			break;
		case 21:
			SONICLOCAL->moda_xy = value * value * 50;
			break;
		case 22:
			SONICLOCAL->moda_adsr = value * 4;
			break;
		case 23:
			SONICLOCAL->modb_a = value * value * value;
			break;
		case 24:
			SONICLOCAL->modb_xy = value * value * 50;
			break;
		case 25:
			SONICLOCAL->modb_pwm = value * 512;
			break;
		case 26:
			/* Mod */
			if (value == 0)
				baudio->mixflags |= RM_A;
			else
				baudio->mixflags &= ~RM_A;
			break;
		case 27:
			/* Mod */
			if (value == 0)
				baudio->mixflags |= RM_XY;
			else
				baudio->mixflags &= ~RM_XY;
			break;
		case 28:
			SONICLOCAL->mix_ab = value * 4;
			break;
		case 29:
			SONICLOCAL->mix_rm = value * 0.5;
			break;
		case 30:
			SONICLOCAL->mix_ext = value;
			break;
		case 31:
			SONICLOCAL->mix_noise = value;
			break;
		case 32:
			baudio->mixflags &= ~(A_LFO|A_KBD);
			if (ivalue == 2)
				baudio->mixflags |= A_LFO;
			else if (ivalue == 1)
				baudio->mixflags |= A_KBD;
			break;
		case 33:
			SONICLOCAL->fmodadsr = value * 2;
			break;
		case 34:
			SONICLOCAL->fmodxy = value * value * 24;
			break;
		case 35:
			/* Mod */
			if (value == 0)
				baudio->mixflags &= ~BYPASS;
			else
				baudio->mixflags |= BYPASS;
			break;
		case 36:
			/* Mod */
			if (value == 0)
				baudio->mixflags &= ~X_TRIGGER;
			else
				baudio->mixflags |= X_TRIGGER;
			break;
		case 37:
			/* Mod */
			if (value == 0)
				baudio->mixflags &= ~Y_TRIGGER;
			else
				baudio->mixflags |= Y_TRIGGER;
			break;
	}

	return(0);
}

int
operateSonic6Preops(audioMain *audiomain, Baudio *baudio,
bristolVoice *voice, register float *startbuf)
{
	bristolbzero(SONICLOCAL->outbuf, audiomain->segmentsize);

	/*
	 * Noise - we do this early for the same reason as oscillator 3
	 */
	if ((baudio->mixflags & MULTI_NOISE) == 0)
	{
		bristolbzero(SONICLOCAL->noisebuf, audiomain->segmentsize);

		audiomain->palette[(*baudio->sound[6]).index]->specs->io[0].buf
			= SONICLOCAL->noisebuf;
		(*baudio->sound[6]).operate(
			(audiomain->palette)[4],
			voice,
			(*baudio->sound[6]).param,
			voice->locals[voice->index][6]);
	}

	if ((baudio->mixflags & X_MULTI) == 0)
	{
		/*
		 * Operate LFO if we have a single LFO per synth.
		 */
		audiomain->palette[16]->specs->io[1].buf
			= audiomain->palette[16]->specs->io[2].buf
			= audiomain->palette[16]->specs->io[3].buf
			= audiomain->palette[16]->specs->io[4].buf
			= audiomain->palette[16]->specs->io[5].buf
			= audiomain->palette[16]->specs->io[6].buf
				= NULL;

		audiomain->palette[16]->specs->io[0].buf = SONICLOCAL->noisebuf;

		switch (baudio->mixflags & X_WAVE_MASK) {
			default:
			case X_TRI:
				audiomain->palette[16]->specs->io[1].buf = SONICLOCAL->lfoxbuf;
				break;
			case X_SAW:
				audiomain->palette[16]->specs->io[6].buf = SONICLOCAL->lfoxbuf;
				break;
			case X_RAMP:
				audiomain->palette[16]->specs->io[5].buf = SONICLOCAL->lfoxbuf;
				break;
			case X_SQUARE:
				audiomain->palette[16]->specs->io[2].buf = SONICLOCAL->lfoxbuf;
				break;
		}

		(*baudio->sound[7]).operate(
			(audiomain->palette)[16],
			voice,
			(*baudio->sound[7]).param,
			baudio->locals[voice->index][7]);

		bufadd(SONICLOCAL->lfoxbuf, 1.0, audiomain->samplecount);
	}

	if ((baudio->mixflags & Y_MULTI) == 0)
	{
		audiomain->palette[16]->specs->io[1].buf
			= audiomain->palette[16]->specs->io[2].buf
			= audiomain->palette[16]->specs->io[3].buf
			= audiomain->palette[16]->specs->io[4].buf
			= audiomain->palette[16]->specs->io[5].buf
			= audiomain->palette[16]->specs->io[6].buf
				= NULL;

		audiomain->palette[16]->specs->io[0].buf = SONICLOCAL->noisebuf;

		switch (baudio->mixflags & Y_WAVE_MASK) {
			default:
			case Y_TRI:
				audiomain->palette[16]->specs->io[1].buf = SONICLOCAL->lfoybuf;
				break;
			case Y_SAW:
				audiomain->palette[16]->specs->io[6].buf = SONICLOCAL->lfoybuf;
				break;
			case Y_RAMP:
				audiomain->palette[16]->specs->io[5].buf = SONICLOCAL->lfoybuf;
				break;
			case Y_RAND:
				audiomain->palette[16]->specs->io[3].buf = SONICLOCAL->lfoybuf;
				break;
		}

		(*baudio->sound[8]).operate(
			(audiomain->palette)[16],
			voice,
			(*baudio->sound[8]).param,
			baudio->locals[voice->index][8]);

		bufadd(SONICLOCAL->lfoybuf, 1.0, audiomain->samplecount);
	}

	return(0);
}

int
operateOneSonic6Voice(audioMain *audiomain, Baudio *baudio,
bristolVoice *voice, register float *startbuf)
{
#ifdef DEBUG
	audiomain->debuglevel = -1;
#endif

	bristolbzero(SONICLOCAL->freqbuf, audiomain->segmentsize);
	bristolbzero(SONICLOCAL->oscabuf, audiomain->segmentsize);
	bristolbzero(SONICLOCAL->oscbbuf, audiomain->segmentsize);
	bristolbzero(SONICLOCAL->filtbuf, audiomain->segmentsize);

/* Multi Mods */
	if (baudio->mixflags & MULTI_NOISE)
	{
		bristolbzero(SONICLOCAL->noisebuf, audiomain->segmentsize);

		audiomain->palette[(*baudio->sound[6]).index]->specs->io[0].buf
			= SONICLOCAL->noisebuf;
		(*baudio->sound[6]).operate(
			(audiomain->palette)[4],
			voice,
			(*baudio->sound[6]).param,
			voice->locals[voice->index][6]);
	}

	if (baudio->mixflags & X_MULTI)
	{
		/*
		 * Operate LFO if we have a single LFO per synth.
		 */
		audiomain->palette[16]->specs->io[1].buf
			= audiomain->palette[16]->specs->io[2].buf
			= audiomain->palette[16]->specs->io[3].buf
			= audiomain->palette[16]->specs->io[4].buf
			= audiomain->palette[16]->specs->io[5].buf
			= audiomain->palette[16]->specs->io[6].buf
				= NULL;

		audiomain->palette[16]->specs->io[0].buf = SONICLOCAL->noisebuf;

		switch (baudio->mixflags & X_WAVE_MASK) {
			default:
			case X_TRI:
				audiomain->palette[16]->specs->io[1].buf = SONICLOCAL->lfoxbuf;
				break;
			case X_SAW:
				audiomain->palette[16]->specs->io[6].buf = SONICLOCAL->lfoxbuf;
				break;
			case X_RAMP:
				audiomain->palette[16]->specs->io[5].buf = SONICLOCAL->lfoxbuf;
				break;
			case X_SQUARE:
				audiomain->palette[16]->specs->io[2].buf = SONICLOCAL->lfoxbuf;
				break;
		}

		(*baudio->sound[7]).operate(
			(audiomain->palette)[16],
			voice,
			(*baudio->sound[7]).param,
			baudio->locals[voice->index][7]);

		bufadd(SONICLOCAL->lfoxbuf, 1.0, audiomain->samplecount);
	}

	if (baudio->mixflags & Y_MULTI)
	{
		audiomain->palette[16]->specs->io[1].buf
			= audiomain->palette[16]->specs->io[2].buf
			= audiomain->palette[16]->specs->io[3].buf
			= audiomain->palette[16]->specs->io[4].buf
			= audiomain->palette[16]->specs->io[5].buf
			= audiomain->palette[16]->specs->io[6].buf
				= NULL;

		audiomain->palette[16]->specs->io[0].buf = SONICLOCAL->noisebuf;

		switch (baudio->mixflags & Y_WAVE_MASK) {
			default:
			case Y_TRI:
				audiomain->palette[16]->specs->io[1].buf = SONICLOCAL->lfoybuf;
				break;
			case Y_SAW:
				audiomain->palette[16]->specs->io[6].buf = SONICLOCAL->lfoybuf;
				break;
			case Y_RAMP:
				audiomain->palette[16]->specs->io[5].buf = SONICLOCAL->lfoybuf;
				break;
			case Y_RAND:
				audiomain->palette[16]->specs->io[3].buf = SONICLOCAL->lfoybuf;
				break;
		}

		(*baudio->sound[8]).operate(
			(audiomain->palette)[16],
			voice,
			(*baudio->sound[8]).param,
			baudio->locals[voice->index][8]);

		bufadd(SONICLOCAL->lfoybuf, 1.0, audiomain->samplecount);
	}

	/*
	 * Run the ADSR. We need to review all the various trigger options here
	 * for keyboard or the two LFO retriggers.
	 */
	audiomain->palette[1]->specs->io[0].buf
		= SONICLOCAL->adsrbuf;

	if (voice->flags & (BRISTOL_KEYON|BRISTOL_KEYREON))
		SONICLOCAL->xtrigstate = SONICLOCAL->ytrigstate = 0;

	if (baudio->mixflags & X_TRIGGER)
	{
		if ((SONICLOCAL->xtrigstate < 0.5) &&
			(*(SONICLOCAL->lfoxbuf) > 0.5))
			voice->flags |= BRISTOL_KEYREON;
			SONICLOCAL->xtrigstate = *(SONICLOCAL->lfoxbuf);
	}

	if (baudio->mixflags & Y_TRIGGER)
	{
		if ((SONICLOCAL->ytrigstate < 0.5) &&
			(*(SONICLOCAL->lfoybuf) > 0.5))
			voice->flags |= BRISTOL_KEYREON;
			SONICLOCAL->ytrigstate = *(SONICLOCAL->lfoybuf);
	}

	(*baudio->sound[3]).operate(
		(audiomain->palette)[1],
		voice,
		(*baudio->sound[3]).param,
		voice->locals[voice->index][3]);

	/*
	 * See if the LFO have to be tempered
	 */
	switch (baudio->mixflags & X_MOD_MASK) {
		case X_MOD_ADSR:
			bufmerge(SONICLOCAL->lfoxbuf, 0.1,
				SONICLOCAL->scratch, 0.0, audiomain->samplecount);
			bristolbzero(SONICLOCAL->lfoxbuf, audiomain->segmentsize);
			audiomain->palette[2]->specs->io[0].buf = SONICLOCAL->scratch;
			audiomain->palette[2]->specs->io[1].buf = SONICLOCAL->adsrbuf;
			audiomain->palette[2]->specs->io[2].buf = SONICLOCAL->lfoxbuf;

			(*baudio->sound[5]).operate(
				(audiomain->palette)[2],
				voice,
				(*baudio->sound[5]).param,
				voice->locals[voice->index][5]);
			break;
		case X_MOD_WHEEL:
			bufmerge(SONICLOCAL->zerobuf, 0.0,
				SONICLOCAL->lfoxbuf, voice->baudio->contcontroller[1],
				audiomain->samplecount);
			break;
	}

	/*
	 * See if the LFO have to be tempered
	 */
	switch (baudio->mixflags & Y_MOD_MASK) {
		case Y_MOD_ADSR:
			bufmerge(SONICLOCAL->lfoybuf, 0.1,
				SONICLOCAL->scratch, 0.0, audiomain->samplecount);
			audiomain->palette[2]->specs->io[0].buf = SONICLOCAL->scratch;
			audiomain->palette[2]->specs->io[1].buf = SONICLOCAL->adsrbuf;
			audiomain->palette[2]->specs->io[2].buf = SONICLOCAL->lfoybuf;

			(*baudio->sound[5]).operate(
				(audiomain->palette)[2],
				voice,
				(*baudio->sound[5]).param,
				voice->locals[voice->index][5]);
			break;
		case Y_MOD_WHEEL:
			bufmerge(SONICLOCAL->zerobuf, 0.0,
				SONICLOCAL->lfoybuf, voice->baudio->contcontroller[1],
				audiomain->samplecount);
			break;
	}

	bufmerge(SONICLOCAL->lfoxbuf, 1.0 - SONICLOCAL->genXY,
		SONICLOCAL->lfoxybuf, 0.0,
		audiomain->samplecount);
	bufmerge(SONICLOCAL->lfoybuf, SONICLOCAL->genXY,
		SONICLOCAL->lfoxybuf, 1.0,
		audiomain->samplecount);
/* Multi Mods done */

/* OSC A */
	/*
	 * We need to do a couple of things here for the different OSC since one
	 * will track glide and the other will not. The first call does not use
	 * glide for Osc-A but does for Osc-B later. This dual use of the call is
	 * supported since only calling once with glide does not damage the glide
	 * parameters in the voice structure.
	 *
	 * We need to check for keyboard tracking, high (drone) or LFO settings.
	 */
	if (baudio->mixflags & A_KBD)
		fillFreqTable(baudio, voice, SONICLOCAL->freqbuf,
			audiomain->samplecount, 0);
	else {
		float freq = 10.0, *bufptr =  SONICLOCAL->freqbuf;
		int i = audiomain->samplecount;

		if (baudio->mixflags & A_LFO)
			freq = 1.0;

		for (; i > 0; i-=8)
		{
			*bufptr++ = freq;
			*bufptr++ = freq;
			*bufptr++ = freq;
			*bufptr++ = freq;
			*bufptr++ = freq;
			*bufptr++ = freq;
			*bufptr++ = freq;
			*bufptr++ = freq;
		}
	}

	/*
	 * Oscillator A
	 */
	audiomain->palette[8]->specs->io[0].buf = SONICLOCAL->freqbuf;
	audiomain->palette[8]->specs->io[1].buf = SONICLOCAL->zerobuf;
	audiomain->palette[8]->specs->io[2].buf = SONICLOCAL->oscabuf;

	/*
	 * Put in the mods. We will eventually use the switch to select between
	 * LFO, KBD, and KEY_LFO. Note: if genXY == 1.0 then we should route
	 * X->A and Y->B directly.
	bufmerge(SONICLOCAL->freqbuf, 1.0,
		SONICLOCAL->scratch, 0.0, audiomain->samplecount);
	 */
	if (SONICLOCAL->genXY == 1.0)
		bufmerge(SONICLOCAL->lfoxbuf, SONICLOCAL->moda_xy,
			SONICLOCAL->freqbuf, 1.0, audiomain->samplecount);
	else
		bufmerge(SONICLOCAL->lfoxybuf, SONICLOCAL->moda_xy,
			SONICLOCAL->freqbuf, 1.0, audiomain->samplecount);
	bufmerge(SONICLOCAL->adsrbuf, SONICLOCAL->moda_adsr,
		SONICLOCAL->freqbuf, 1.0, audiomain->samplecount);

	/*
	 * Operate this oscillator
	 */
	(*baudio->sound[0]).operate(
		(audiomain->palette)[8],
		voice,
		(*baudio->sound[0]).param,
		voice->locals[voice->index][0]);

	/*
	 * See if we need to put this into the direct output.
	 */
	if (SONICLOCAL->directA != 0)
		bufmerge(SONICLOCAL->oscabuf, SONICLOCAL->directA,
			baudio->leftbuf, 1.0, audiomain->samplecount);
/* OSC A done */

/* OSC B */
	/* Get a frequence table with glide added in */
	fillFreqTable(baudio, voice, SONICLOCAL->freqbuf,
		audiomain->samplecount, 1);

	/*
	 * Oscillator B
	 */
	audiomain->palette[8]->specs->io[0].buf
		= SONICLOCAL->freqbuf;
	audiomain->palette[8]->specs->io[1].buf
		= SONICLOCAL->scratch;
	audiomain->palette[8]->specs->io[2].buf
		= SONICLOCAL->oscbbuf;

	/*
	 * Put in the mods. Note: if genXY == 1.0 then we should route
	 * X->A and Y->B directly.
	 */
	if (SONICLOCAL->genXY == 1.0)
		bufmerge(SONICLOCAL->lfoybuf, SONICLOCAL->modb_xy,
			SONICLOCAL->freqbuf, 1.0, audiomain->samplecount);
	else
		bufmerge(SONICLOCAL->lfoxybuf, SONICLOCAL->modb_xy,
			SONICLOCAL->freqbuf, 1.0, audiomain->samplecount);
	bufmerge(SONICLOCAL->oscabuf, SONICLOCAL->modb_a,
		SONICLOCAL->freqbuf, 1.0, audiomain->samplecount);
	bufmerge(SONICLOCAL->lfoxybuf, SONICLOCAL->modb_pwm,
		SONICLOCAL->scratch, 0.0, audiomain->samplecount);

	/*
	 * Operate this oscillator
	 */
	(*baudio->sound[1]).operate(
		(audiomain->palette)[8],
		voice,
		(*baudio->sound[1]).param,
		voice->locals[voice->index][1]);

	/*
	 * See if we need to put this into the direct output.
	 */
	if (SONICLOCAL->directB != 0)
		bufmerge(SONICLOCAL->oscbbuf, SONICLOCAL->directB,
			baudio->leftbuf, 1.0, audiomain->samplecount);
/* OSC B done */

/* RING MOD */
	if (baudio->mixflags & RM_A)
		audiomain->palette[20]->specs->io[0].buf = SONICLOCAL->oscabuf;
	else
		audiomain->palette[20]->specs->io[0].buf = startbuf;
	if (baudio->mixflags & RM_XY) {
		audiomain->palette[20]->specs->io[1].buf = SONICLOCAL->scratch;
		bufmerge(SONICLOCAL->lfoxybuf, 12.0,
			SONICLOCAL->scratch, 0.0, audiomain->samplecount);
	} else
		audiomain->palette[20]->specs->io[1].buf = SONICLOCAL->oscbbuf;

	audiomain->palette[20]->specs->io[2].buf = SONICLOCAL->rmbuf;

	(*baudio->sound[2]).operate(
		(audiomain->palette)[20],
		voice,
		(*baudio->sound[2]).param,
		voice->locals[voice->index][2]);

	/*
	 * See if we need to put this into the direct output.
	 */
	if (SONICLOCAL->directRM != 0)
		bufmerge(SONICLOCAL->rmbuf, SONICLOCAL->directRM,
			baudio->leftbuf, 1.0, audiomain->samplecount);
/* RING MOD done */

/* MIXER */
	bufmerge(SONICLOCAL->oscbbuf, SONICLOCAL->genAB,
		SONICLOCAL->oscabuf, 1.0 - SONICLOCAL->genAB, audiomain->samplecount);

	bufmerge(SONICLOCAL->noisebuf, SONICLOCAL->mix_noise,
		SONICLOCAL->rmbuf, SONICLOCAL->mix_rm, audiomain->samplecount);
	bufmerge(SONICLOCAL->oscabuf, SONICLOCAL->mix_ab,
		SONICLOCAL->rmbuf, 1.0, audiomain->samplecount);
	bufmerge(startbuf, SONICLOCAL->mix_ext,
		SONICLOCAL->rmbuf, 1.0, audiomain->samplecount);
/* MIXER done */

/* FILTER */
	/* 
	 * If we have mods on the filter, apply them here.
	 */
	bufmerge(SONICLOCAL->adsrbuf, SONICLOCAL->fmodadsr,
		SONICLOCAL->scratch, 0.0, audiomain->samplecount);
	bufmerge(SONICLOCAL->lfoxybuf, SONICLOCAL->fmodxy,
		SONICLOCAL->scratch, 1.0, audiomain->samplecount);

	/*
	 * Run the mixed oscillators into the filter. Input and output buffers
	 * are the same.
	 */
	audiomain->palette[3]->specs->io[0].buf = SONICLOCAL->rmbuf;
	audiomain->palette[3]->specs->io[1].buf = SONICLOCAL->scratch;
	audiomain->palette[3]->specs->io[2].buf = SONICLOCAL->filtbuf;

	/* Filter input stage EQ. Brings signal in line with max filter output */
	bufmerge(SONICLOCAL->zerobuf, 0.0,
		SONICLOCAL->rmbuf, 96.0, audiomain->samplecount);

	(*baudio->sound[4]).operate(
		(audiomain->palette)[3],
		voice,
		(*baudio->sound[4]).param,
		voice->locals[voice->index][4]);
/* FILTER done */

/* AMP */
	if (baudio->mixflags & BYPASS)
		bufmerge(SONICLOCAL->filtbuf, 12.0,
			baudio->leftbuf, 1.0, audiomain->samplecount);
	else {
		audiomain->palette[2]->specs->io[0].buf = SONICLOCAL->filtbuf;
		audiomain->palette[2]->specs->io[1].buf = SONICLOCAL->adsrbuf;
		audiomain->palette[2]->specs->io[2].buf = baudio->leftbuf;

		(*baudio->sound[5]).operate(
			(audiomain->palette)[2],
			voice,
			(*baudio->sound[5]).param,
			voice->locals[voice->index][5]);
	}
/* AMP done */

	return(0);
}

int
operateSonic6Postops(audioMain *audiomain, Baudio *baudio,
bristolVoice *voice, register float *startbuf)
{
	bufmerge(baudio->leftbuf, SONICLOCAL->mastervolume,
		baudio->leftbuf, 0.0, audiomain->samplecount);

	return(0);
}

static int
destroyOneSonic6Voice(audioMain *audiomain, Baudio *baudio)
{
	printf("destroying Sonic-6\n");

	bristolfree(SONICLOCAL->oscabuf);
	bristolfree(SONICLOCAL->oscbbuf);
	bristolfree(SONICLOCAL->noisebuf);
	bristolfree(SONICLOCAL->freqbuf);
	bristolfree(SONICLOCAL->filtbuf);
	bristolfree(SONICLOCAL->rmbuf);
	bristolfree(SONICLOCAL->lfoxbuf);
	bristolfree(SONICLOCAL->lfoybuf);
	bristolfree(SONICLOCAL->lfoxybuf);
	bristolfree(SONICLOCAL->adsrbuf);
	bristolfree(SONICLOCAL->outbuf);
	bristolfree(SONICLOCAL->zerobuf);
	bristolfree(SONICLOCAL->scratch);

	return(0);
}

int
bristolSonic6Init(audioMain *audiomain, Baudio *baudio)
{
	printf("initialising Sonic-6\n");

	baudio->soundCount = 9; /* Number of operators in this voice (Sonic6) */

	/*
	 * Assign an array of sound pointers.
	 */
	baudio->sound = (bristolSound **)
		bristolmalloc0(sizeof(bristolOP *) * baudio->soundCount);
	baudio->effect = (bristolSound **)
		bristolmalloc0(sizeof(bristolOP *) * baudio->soundCount);

	/*
	 * Two VCO
	 * RingMod
	 * Noise
	 * Env
	 * AMP
	 * Filter
	 * Two LFO, may run to three if we put in a global LFO
	 *
	 * Possibly add a reverb?
	 */
	/* VCO */
	initSoundAlgo(	8,	0, baudio, audiomain, baudio->sound);
	initSoundAlgo(	8,	1, baudio, audiomain, baudio->sound);
	/* Ring Mod */
	initSoundAlgo(	20,	2, baudio, audiomain, baudio->sound);
	/* An ADSR */
	initSoundAlgo(	1,	3, baudio, audiomain, baudio->sound);
	/* A filter */
	initSoundAlgo(	3,	4, baudio, audiomain, baudio->sound);
	/* An amplifier */
	initSoundAlgo(	2,	5, baudio, audiomain, baudio->sound);
	/* An noise source */
	initSoundAlgo(	4,	6, baudio, audiomain, baudio->sound);
	/* LFO */
	initSoundAlgo(	16,	7, baudio, audiomain, baudio->sound);
	initSoundAlgo(	16,	8, baudio, audiomain, baudio->sound);

	baudio->param = sonic6GlobalController;
	baudio->destroy = destroyOneSonic6Voice;
	baudio->operate = operateOneSonic6Voice;
	baudio->preops = operateSonic6Preops;
	baudio->postops = operateSonic6Postops;

	/* We need to flag this for the parallel filters */
	baudio->mixlocals = bristolmalloc0(sizeof(bSonic6));

	/*
	 * Put in a reverb on our effects list.
	 */
	initSoundAlgo(22, 0, baudio, audiomain, baudio->effect);

	if (SONICLOCAL->oscabuf == (float *) NULL)
		SONICLOCAL->oscabuf = (float *) bristolmalloc0(audiomain->segmentsize);
	if (SONICLOCAL->oscbbuf == (float *) NULL)
		SONICLOCAL->oscbbuf = (float *) bristolmalloc0(audiomain->segmentsize);
	if (SONICLOCAL->noisebuf == (float *) NULL)
		SONICLOCAL->noisebuf = (float *) bristolmalloc0(audiomain->segmentsize);
	if (SONICLOCAL->freqbuf == (float *) NULL)
		SONICLOCAL->freqbuf = (float *) bristolmalloc0(audiomain->segmentsize);
	if (SONICLOCAL->filtbuf == (float *) NULL)
		SONICLOCAL->filtbuf = (float *) bristolmalloc0(audiomain->segmentsize);
	if (SONICLOCAL->rmbuf == (float *) NULL)
		SONICLOCAL->rmbuf = (float *) bristolmalloc0(audiomain->segmentsize);
	if (SONICLOCAL->lfoxbuf == (float *) NULL)
		SONICLOCAL->lfoxbuf = (float *) bristolmalloc0(audiomain->segmentsize);
	if (SONICLOCAL->lfoybuf == (float *) NULL)
		SONICLOCAL->lfoybuf = (float *) bristolmalloc0(audiomain->segmentsize);
	if (SONICLOCAL->lfoxybuf == (float *) NULL)
		SONICLOCAL->lfoxybuf = (float *) bristolmalloc0(audiomain->segmentsize);
	if (SONICLOCAL->adsrbuf == (float *) NULL)
		SONICLOCAL->adsrbuf = (float *) bristolmalloc0(audiomain->segmentsize);
	if (SONICLOCAL->outbuf == (float *) NULL)
		SONICLOCAL->outbuf = (float *) bristolmalloc0(audiomain->segmentsize);
	if (SONICLOCAL->zerobuf == (float *) NULL)
		SONICLOCAL->zerobuf = (float *) bristolmalloc0(audiomain->segmentsize);
	if (SONICLOCAL->scratch == (float *) NULL)
		SONICLOCAL->scratch = (float *) bristolmalloc0(audiomain->segmentsize);

	return(0);
}

