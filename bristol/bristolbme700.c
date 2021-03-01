
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

/* Bme700 algo */

/*#define DEBUG */

#include "bristol.h"
#include "bristolmm.h"
#include "bristolbme700.h"

#define mixscaler 32.0
#define f_mm 3.0

/*
 * Use of these bme700 global buffers will be an issue with use of multiple
 * audio threads, unless we ensure a single thread deals with any given algo
 * type, since then they are only used sequentially.
 */

int
bme700Controller(Baudio *baudio, u_char operator, u_char controller, float value)
{
//	int ivalue = value * CONTROLLER_RANGE;

#ifdef DEBUG
	printf("bristolBme700Control(%i, %i, %f)\n", operator, controller, value);
#endif

	if (operator != 126)
		return(0);

	switch (controller) {
		case 0:
			((bme700mods *) baudio->mixlocals)->glide = value * value;

			if (baudio->mixflags & BME700_GLIDE)
				baudio->glide = ((bme700mods *) baudio->mixlocals)->glide
					* baudio->glidemax;
			else
				baudio->glide = 0;
			break;
		case 1:
			baudio->gtune = 1.0
				+ (baudio->note_diff - 1)
				* (value * 2 - 1);

			buildCurrentTable(baudio, baudio->gtune);
			alterAllNotes(baudio);
			break;
		case 2:
			BME700LOCAL->volume = value * 0.025625; // 0.03125;
			break;
		case 3:
			BME700LOCAL->oscmix = value * mixscaler;
			break;
		case 4:
			BME700LOCAL->resmix = value * mixscaler;
			break;
		case 5:
			BME700LOCAL->envmix1 = value;
			break;
		case 6:
			BME700LOCAL->envmix2 = value;
			break;
		case 7:
			if (value == 0)
				baudio->mixflags &= ~BME700_LFO1_MULTI;
			else
				baudio->mixflags |= BME700_LFO1_MULTI;
			break;
		case 8:
			if (value == 0)
				baudio->mixflags &= ~BME700_LFO2_MULTI;
			else
				baudio->mixflags |= BME700_LFO2_MULTI;
			break;
		case 9:
			if (value == 0)
				baudio->mixflags &= ~BME700_NOISE_MULTI;
			else
				baudio->mixflags |= BME700_NOISE_MULTI;
			break;

		case 10: /* inversed parameter */
			if (value == 0)
				baudio->mixflags |= BME700_VIBRA_ENV;
			else
				baudio->mixflags &= ~BME700_VIBRA_ENV;
			break;
		case 11:
			if (value == 0)
				baudio->mixflags &= ~BME700_VIBRA_TRI1;
			else
				baudio->mixflags |= BME700_VIBRA_TRI1;
			break;
		case 12:
			if (value == 0)
				baudio->mixflags &= ~BME700_VIBRA_TRI2;
			else
				baudio->mixflags |= BME700_VIBRA_TRI2;
			break;
		case 13:
			if (value == 0)
				baudio->mixflags &= ~BME700_PWM_TRI;
			else
				baudio->mixflags |= BME700_PWM_TRI;
			break;
		case 14: /* inversed parameter */
			if (value == 0)
				baudio->mixflags |= BME700_PWM_DOUBLE;
			else
				baudio->mixflags &= ~BME700_PWM_DOUBLE;
			break;

		case 15:
			if (value == 0)
				baudio->mixflags &= ~BME700_GLIDE;
			else
				baudio->mixflags |= BME700_GLIDE;

			if (baudio->mixflags & BME700_GLIDE)
				baudio->glide = ((bme700mods *) baudio->mixlocals)->glide
					* baudio->glidemax;
			else
				baudio->glide = 0;
			break;

		case 16:
			if (value == 0)
				baudio->mixflags &= ~BME700_PWM_MAN;
			else
				baudio->mixflags |= BME700_PWM_MAN;
			break;
		case 17:
			if (value == 0)
				baudio->mixflags &= ~BME700_PWM_ENV;
			else
				baudio->mixflags |= BME700_PWM_ENV;
			break;

		case 20:
			if (value == 0)
				baudio->mixflags &= ~BME700_FILT_TRI;
			else
				baudio->mixflags |= BME700_FILT_TRI;
			break;
		case 21:
			if (value == 0)
				baudio->mixflags &= ~BME700_FILT_MOD1;
			else
				baudio->mixflags |= BME700_FILT_MOD1;
			break;
		case 22:
			if (value == 0)
				baudio->mixflags &= ~BME700_FILT_KBD;
			else
				baudio->mixflags |= BME700_FILT_KBD;
			break;

		case 23:
			if (value == 0)
				baudio->mixflags &= ~BME700_TREM_MOD1;
			else
				baudio->mixflags |= BME700_TREM_MOD1;
			break;
		case 24:
			if (value == 0)
				baudio->mixflags &= ~BME700_TREM_TRI;
			else
				baudio->mixflags |= BME700_TREM_TRI;
			break;

		case 25:
			BME700LOCAL->f_modmix = value * f_mm;
			break;
		case 26:
			BME700LOCAL->a_modmix = value;
			break;

		case 27:
			BME700LOCAL->vibramix = value;
			break;
		case 28:
			BME700LOCAL->vibra = value * value * 96;
			break;

		case 29:
			BME700LOCAL->pwm = value * 0.5; //0.0825;
			break;
	}
	return(0);
}

/*
 * PreOps will do LFO and may drop the option for multi here? Alternatively
 * always have multi and let uni be monophonic?
 */
int
bme700Preops(audioMain *audiomain, Baudio *baudio,
bristolVoice *voice, register float *startbuf)
{
	if ((baudio->mixflags & BME700_LFO1_MULTI) == 0)
	{
			/*
			 * Operate LFO if we have a single LFO per synth.
			 */
			audiomain->palette[B_LFO]->specs->io[0].buf = BME700LOCAL->zerobuf;

			audiomain->palette[B_LFO]->specs->io[1].buf
				= BME700LOCAL->lfo1_tri;
			audiomain->palette[B_LFO]->specs->io[2].buf
				= BME700LOCAL->lfo1_sqr;

			audiomain->palette[B_LFO]->specs->io[3].buf
				= audiomain->palette[B_LFO]->specs->io[4].buf
				= audiomain->palette[B_LFO]->specs->io[5].buf
				= audiomain->palette[B_LFO]->specs->io[6].buf
					= NULL;

			(*baudio->sound[0]).operate(
				(audiomain->palette)[B_LFO],
				voice,
				(*baudio->sound[0]).param,
				baudio->locals[voice->index][0]);
	}

	if ((baudio->mixflags & BME700_LFO2_MULTI) == 0)
	{
			/*
			 * Operate LFO if we have a single LFO per synth.
			 */
			audiomain->palette[B_LFO]->specs->io[0].buf = BME700LOCAL->zerobuf;

			audiomain->palette[B_LFO]->specs->io[1].buf
				= BME700LOCAL->lfo2_tri;
			audiomain->palette[B_LFO]->specs->io[2].buf
				= BME700LOCAL->lfo2_sqr;

			audiomain->palette[B_LFO]->specs->io[3].buf
				= audiomain->palette[B_LFO]->specs->io[4].buf
				= audiomain->palette[B_LFO]->specs->io[5].buf
				= audiomain->palette[B_LFO]->specs->io[6].buf
					= NULL;

			(*baudio->sound[1]).operate(
				(audiomain->palette)[B_LFO],
				voice,
				(*baudio->sound[1]).param,
				baudio->locals[voice->index][1]);
	}

	if ((baudio->mixflags & BME700_NOISE_MULTI) == 0)
	{
		bristolbzero(BME700LOCAL->noisebuf, audiomain->segmentsize);
		audiomain->palette[B_NOISE]->specs->io[0].buf = BME700LOCAL->noisebuf;
		(*baudio->sound[5]).operate(
			(audiomain->palette)[B_NOISE],
			voice,
			(*baudio->sound[5]).param,
			voice->locals[voice->index][5]);
	}

	return(0);
}

int
operateOneBme700(audioMain *audiomain, Baudio *baudio,
bristolVoice *voice, register float *startbuf)
{
	int flags;
/*printf("operateOneBme700(%i, %x, %x)\n", voice->index, audiomain, baudio); */

/* LFO/Noise Done */
	if (baudio->mixflags & BME700_LFO1_MULTI)
	{
			/*
			 * Operate LFO if we have a single LFO per synth.
			 */
			audiomain->palette[B_LFO]->specs->io[0].buf = BME700LOCAL->zerobuf;

			audiomain->palette[B_LFO]->specs->io[1].buf = BME700LOCAL->lfo1_tri;
			audiomain->palette[B_LFO]->specs->io[2].buf = BME700LOCAL->lfo1_sqr;

			audiomain->palette[B_LFO]->specs->io[3].buf
				= audiomain->palette[B_LFO]->specs->io[4].buf
				= audiomain->palette[B_LFO]->specs->io[5].buf
				= audiomain->palette[B_LFO]->specs->io[6].buf
					= NULL;

			(*baudio->sound[0]).operate(
				(audiomain->palette)[B_LFO],
				voice,
				(*baudio->sound[0]).param,
				baudio->locals[voice->index][0]);
	}

	if (baudio->mixflags & BME700_LFO2_MULTI)
	{
			/*
			 * Operate LFO if we have a single LFO per synth.
			 */
			audiomain->palette[B_LFO]->specs->io[0].buf = BME700LOCAL->zerobuf;

			audiomain->palette[B_LFO]->specs->io[1].buf = BME700LOCAL->lfo2_tri;
			audiomain->palette[B_LFO]->specs->io[2].buf = BME700LOCAL->lfo2_sqr;

			audiomain->palette[B_LFO]->specs->io[3].buf
				= audiomain->palette[B_LFO]->specs->io[4].buf
				= audiomain->palette[B_LFO]->specs->io[5].buf
				= audiomain->palette[B_LFO]->specs->io[6].buf
					= NULL;

			(*baudio->sound[1]).operate(
				(audiomain->palette)[B_LFO],
				voice,
				(*baudio->sound[1]).param,
				baudio->locals[voice->index][1]);
	}

	if (baudio->mixflags & BME700_NOISE_MULTI)
	{
		bristolbzero(BME700LOCAL->noisebuf, audiomain->segmentsize);
		audiomain->palette[B_NOISE]->specs->io[0].buf = BME700LOCAL->noisebuf;
		(*baudio->sound[5]).operate(
			(audiomain->palette)[B_NOISE],
			voice,
			(*baudio->sound[5]).param,
			voice->locals[voice->index][5]);
	}
/* LFO/Noise Done */

/* Envelopes */
	audiomain->palette[B_ENV]->specs->io[0].buf = BME700LOCAL->adsr1buf;
	(*baudio->sound[6]).operate(
		(audiomain->palette)[B_ENV],
		voice,
		(*baudio->sound[6]).param,
		voice->locals[voice->index][6]);
	flags = voice->flags;

	audiomain->palette[B_ENV]->specs->io[0].buf = BME700LOCAL->adsr2buf;
	(*baudio->sound[7]).operate(
		(audiomain->palette)[B_ENV],
		voice,
		(*baudio->sound[7]).param,
		voice->locals[voice->index][7]);
	if ((flags & BRISTOL_KEYDONE) == 0)
		voice->flags &= ~BRISTOL_KEYDONE;
/* Envelopes Done */

/* Osc */
	/*
	 * This will do 1 osc with env, unless multi when it does two. Will track
	 * the highest note for the filter cutoff point for when filter is not 
	 * MULTI.
	 */
	fillFreqBuf(baudio, voice, BME700LOCAL->freqbuf, audiomain->samplecount, 1);

	/* Mods - vibra - merge is wrong, it should be mult */
	if (baudio->mixflags & BME700_VIBRA_ENV) {
		if (baudio->mixflags & BME700_VIBRA_TRI2) {
			bufmerge(BME700LOCAL->adsr1buf,
				(1.0 - BME700LOCAL->vibramix) * BME700LOCAL->vibra,
				BME700LOCAL->freqbuf, 1.0, audiomain->samplecount);
			bufmerge(BME700LOCAL->lfo2_tri,
				BME700LOCAL->vibramix * BME700LOCAL->vibra,
				BME700LOCAL->freqbuf, 1.0, audiomain->samplecount);
		} else {
			bufmerge(BME700LOCAL->adsr1buf,
				(1.0 - BME700LOCAL->vibramix) * BME700LOCAL->vibra,
				BME700LOCAL->freqbuf, 1.0, audiomain->samplecount);
			bufmerge(BME700LOCAL->lfo2_sqr,
				BME700LOCAL->vibramix * BME700LOCAL->vibra,
				BME700LOCAL->freqbuf, 1.0, audiomain->samplecount);
		}
	} else {
		if (baudio->mixflags & BME700_VIBRA_TRI1) {
			if (baudio->mixflags & BME700_VIBRA_TRI2) {
				bufmerge(BME700LOCAL->lfo1_tri,
					(1.0 - BME700LOCAL->vibramix) * BME700LOCAL->vibra,
					BME700LOCAL->freqbuf, 1.0, audiomain->samplecount);
				bufmerge(BME700LOCAL->lfo2_tri,
					BME700LOCAL->vibramix * BME700LOCAL->vibra,
						BME700LOCAL->freqbuf, 1.0, audiomain->samplecount);
			} else {
				bufmerge(BME700LOCAL->lfo1_tri,
					(1.0 - BME700LOCAL->vibramix) * BME700LOCAL->vibra,
					BME700LOCAL->freqbuf, 1.0, audiomain->samplecount);
				bufmerge(BME700LOCAL->lfo2_sqr,
					BME700LOCAL->vibramix * BME700LOCAL->vibra,
					BME700LOCAL->freqbuf, 1.0, audiomain->samplecount);
			}
		} else {
			if (baudio->mixflags & BME700_VIBRA_TRI2) {
				bufmerge(BME700LOCAL->lfo1_sqr,
					(1.0 - BME700LOCAL->vibramix) * BME700LOCAL->vibra,
					BME700LOCAL->freqbuf, 1.0, audiomain->samplecount);
				bufmerge(BME700LOCAL->lfo2_tri,
					BME700LOCAL->vibramix * BME700LOCAL->vibra,
						BME700LOCAL->freqbuf, 1.0, audiomain->samplecount);
			} else {
				bufmerge(BME700LOCAL->lfo1_sqr,
					(1.0 - BME700LOCAL->vibramix) * BME700LOCAL->vibra,
					BME700LOCAL->freqbuf, 1.0, audiomain->samplecount);
				bufmerge(BME700LOCAL->lfo2_sqr,
					BME700LOCAL->vibramix * BME700LOCAL->vibra,
					BME700LOCAL->freqbuf, 1.0, audiomain->samplecount);
			}
		}
	}

	bristolbzero(BME700LOCAL->oscbuf, audiomain->segmentsize);

	audiomain->palette[B_NRO]->specs->io[0].buf = BME700LOCAL->freqbuf;
	audiomain->palette[B_NRO]->specs->io[1].buf = BME700LOCAL->oscbuf;
	audiomain->palette[B_NRO]->specs->io[3].buf = BME700LOCAL->zerobuf;
	audiomain->palette[B_NRO]->specs->io[4].buf = NULL; /* no sync required */

	if (baudio->mixflags & BME700_PWM_MAN)
		audiomain->palette[B_NRO]->specs->io[2].buf = BME700LOCAL->zerobuf;
	else {
		audiomain->palette[B_NRO]->specs->io[2].buf = BME700LOCAL->pwmbuf;

		if (baudio->mixflags & BME700_PWM_ENV)
			bufmerge(BME700LOCAL->adsr1buf, BME700LOCAL->pwm,
				BME700LOCAL->pwmbuf, 0.0, audiomain->samplecount);
		else {
			if (baudio->mixflags & BME700_PWM_TRI)
			{
				bufmerge(BME700LOCAL->lfo1_tri, BME700LOCAL->pwm,
					BME700LOCAL->pwmbuf, 0.0, audiomain->samplecount);
				if (baudio->mixflags & BME700_PWM_DOUBLE)
					bufmerge(BME700LOCAL->lfo2_tri, BME700LOCAL->pwm,
						BME700LOCAL->pwmbuf, 1.0, audiomain->samplecount);
			} else {
				bufmerge(BME700LOCAL->lfo1_sqr, BME700LOCAL->pwm,
					BME700LOCAL->pwmbuf, 0.0, audiomain->samplecount);
				if (baudio->mixflags & BME700_PWM_DOUBLE)
					bufmerge(BME700LOCAL->lfo2_sqr, BME700LOCAL->pwm,
						BME700LOCAL->pwmbuf, 1.0, audiomain->samplecount);
			}
		}
	}

	(*baudio->sound[2]).operate(
		(audiomain->palette)[B_NRO],
		voice,
		(*baudio->sound[2]).param,
		voice->locals[voice->index][2]);
/* Osc DONE */

	bufmerge(BME700LOCAL->noisebuf, (mixscaler - BME700LOCAL->oscmix) * 32,
		BME700LOCAL->oscbuf, BME700LOCAL->oscmix, audiomain->samplecount);

/* resonator */
	bristolbzero(BME700LOCAL->resbuf, audiomain->segmentsize);

	audiomain->palette[B_FILTER2]->specs->io[0].buf = BME700LOCAL->oscbuf;
	audiomain->palette[B_FILTER2]->specs->io[1].buf = BME700LOCAL->zerobuf;
	audiomain->palette[B_FILTER2]->specs->io[2].buf = BME700LOCAL->resbuf;
	(*baudio->sound[3]).operate(
		(audiomain->palette)[B_FILTER2],
		voice,
		(*baudio->sound[3]).param,
		voice->locals[voice->index][3]);
/* resonator done */

/* filter */
	bristolbzero(BME700LOCAL->filtbuf, audiomain->segmentsize);

	bufmerge(BME700LOCAL->adsr1buf,
		(1.0 - BME700LOCAL->envmix1) * BME700LOCAL->f_modmix,
		BME700LOCAL->scratch, 0.0, audiomain->samplecount);
	bufmerge(BME700LOCAL->adsr2buf,
		BME700LOCAL->envmix1 * BME700LOCAL->f_modmix,
		BME700LOCAL->scratch, 1.0, audiomain->samplecount);

	if ((baudio->mixflags & BME700_FILT_KBD) == 0) {
		if (baudio->mixflags & BME700_FILT_MOD1) {
			if (baudio->mixflags & BME700_FILT_TRI)
				bufmerge(BME700LOCAL->lfo1_tri, f_mm - BME700LOCAL->f_modmix,
					BME700LOCAL->scratch, 1.0, audiomain->samplecount);
			else
				bufmerge(BME700LOCAL->lfo1_sqr, f_mm - BME700LOCAL->f_modmix,
					BME700LOCAL->scratch, 1.0, audiomain->samplecount);
		} else {
			if (baudio->mixflags & BME700_FILT_TRI)
				bufmerge(BME700LOCAL->lfo2_tri, f_mm - BME700LOCAL->f_modmix,
					BME700LOCAL->scratch, 1.0, audiomain->samplecount);
			else
				bufmerge(BME700LOCAL->lfo2_sqr, f_mm - BME700LOCAL->f_modmix,
					BME700LOCAL->scratch, 1.0, audiomain->samplecount);
		}
	}

	audiomain->palette[B_FILTER2]->specs->io[0].buf = BME700LOCAL->oscbuf;
	audiomain->palette[B_FILTER2]->specs->io[1].buf = BME700LOCAL->scratch;
	audiomain->palette[B_FILTER2]->specs->io[2].buf = BME700LOCAL->filtbuf;
	(*baudio->sound[4]).operate(
		(audiomain->palette)[B_FILTER2],
		voice,
		(*baudio->sound[4]).param,
		voice->locals[voice->index][4]);
/* filter done */

	bufmerge(BME700LOCAL->resbuf, BME700LOCAL->resmix,
		BME700LOCAL->filtbuf, mixscaler - BME700LOCAL->resmix,
			audiomain->samplecount);

/* Output stage */
	bufmerge(BME700LOCAL->adsr1buf,
		(1.0 - BME700LOCAL->envmix2) * BME700LOCAL->a_modmix,
		BME700LOCAL->scratch, 0.0, audiomain->samplecount);
	bufmerge(BME700LOCAL->adsr2buf,
		BME700LOCAL->envmix2 * BME700LOCAL->a_modmix,
		BME700LOCAL->scratch, 1.0, audiomain->samplecount);

	if (baudio->mixflags & BME700_TREM_MOD1) {
		if (baudio->mixflags & BME700_TREM_TRI)
			bufmerge(BME700LOCAL->lfo1_tri, (1.0 - BME700LOCAL->a_modmix) * 4,
				BME700LOCAL->scratch, 1.0, audiomain->samplecount);
		else
			bufmerge(BME700LOCAL->lfo1_sqr, (1.0 - BME700LOCAL->a_modmix) * 4,
				BME700LOCAL->scratch, 1.0, audiomain->samplecount);
	} else {
		if (baudio->mixflags & BME700_TREM_TRI)
			bufmerge(BME700LOCAL->lfo2_tri, (1.0 - BME700LOCAL->a_modmix) * 4,
				BME700LOCAL->scratch, 1.0, audiomain->samplecount);
		else
			bufmerge(BME700LOCAL->lfo2_sqr, (1.0 - BME700LOCAL->a_modmix) * 4,
				BME700LOCAL->scratch, 1.0, audiomain->samplecount);
	}

	audiomain->palette[B_DCA]->specs->io[0].buf = BME700LOCAL->filtbuf;
	audiomain->palette[B_DCA]->specs->io[1].buf = BME700LOCAL->scratch;
	audiomain->palette[B_DCA]->specs->io[2].buf = baudio->leftbuf;
	(*baudio->sound[8]).operate(
		(audiomain->palette)[B_DCA],
		voice,
		(*baudio->sound[8]).param,
		baudio->locals[voice->index][8]);
/* DONE */

	return(0);
}

int
bme700PostOps(audioMain *audiomain, Baudio *baudio,
bristolVoice *voice, register float *startbuf)
{
	bufmerge(baudio->leftbuf, 0.0,
		baudio->leftbuf, BME700LOCAL->volume, audiomain->samplecount);
//printf("%f %f\n", BME700LOCAL->volume, baudio->leftbuf[0]);

	return(0);
}

int
static bristolBme700Destroy(audioMain *audiomain, Baudio *baudio)
{
	printf("removing one bme700\n");

	bristolfree(BME700LOCAL->freqbuf);
	bristolfree(BME700LOCAL->lfo1_tri);
	bristolfree(BME700LOCAL->lfo1_sqr);
	bristolfree(BME700LOCAL->lfo2_tri);
	bristolfree(BME700LOCAL->lfo2_sqr);
	bristolfree(BME700LOCAL->noisebuf);
	bristolfree(BME700LOCAL->outbuf);

	bristolfree(BME700LOCAL->oscbuf);
	bristolfree(BME700LOCAL->zerobuf);
	bristolfree(BME700LOCAL->scratch);
	bristolfree(BME700LOCAL->pwmbuf);
	bristolfree(BME700LOCAL->filtbuf);
	bristolfree(BME700LOCAL->resbuf);
	bristolfree(BME700LOCAL->adsr1buf);
	bristolfree(BME700LOCAL->adsr2buf);

	return(0);
}

int
bristolBme700Init(audioMain *audiomain, Baudio *baudio)
{
	printf("initialising bme700\n");

	baudio->soundCount = 9; /* Number of operators in this voice */

	/*
	 * Assign an array of sound pointers.
	 */
	baudio->sound = (bristolSound **)
		bristolmalloc0(sizeof(bristolOP *) * baudio->soundCount);
	baudio->effect = (bristolSound **)
		bristolmalloc0(sizeof(bristolOP *) * baudio->soundCount);

	/*
	 * two LFO
	 * NRO - needs work for morphing - Done, test
	 * two filter - need work for 12/24 - Done
	 * two env
	 * amp
	 * noise
	 */
	/* Two LFO */
	initSoundAlgo(B_LFO,	0, baudio, audiomain, baudio->sound);
	initSoundAlgo(B_LFO,	1, baudio, audiomain, baudio->sound);
	/* Non resampling oscillator */
	initSoundAlgo(B_NRO,	2, baudio, audiomain, baudio->sound);
	/* Two filters */
	initSoundAlgo(B_FILTER2, 3, baudio, audiomain, baudio->sound);
	initSoundAlgo(B_FILTER2, 4, baudio, audiomain, baudio->sound);
	/* An noise source */
	initSoundAlgo(B_NOISE,	5, baudio, audiomain, baudio->sound);
	/* Two ADSR with AMP */
	initSoundAlgo(B_ENV,	6, baudio, audiomain, baudio->sound);
	initSoundAlgo(B_ENV,	7, baudio, audiomain, baudio->sound);
	initSoundAlgo(B_DCA,	8, baudio, audiomain, baudio->sound);

	baudio->param = bme700Controller;
	baudio->destroy = bristolBme700Destroy;
	baudio->operate = operateOneBme700;
	baudio->preops = bme700Preops;
	baudio->postops = bme700PostOps;

	/*
	 * Put nothing on our effects list.
	initSoundAlgo(12, 0, baudio, audiomain, baudio->effect);
	 */

	baudio->mixlocals = (float *) bristolmalloc0(sizeof(bme700mods));

	BME700LOCAL->freqbuf = (float *) bristolmalloc0(audiomain->segmentsize);
	BME700LOCAL->lfo1_tri = (float *) bristolmalloc0(audiomain->segmentsize);
	BME700LOCAL->lfo1_sqr = (float *) bristolmalloc0(audiomain->segmentsize);
	BME700LOCAL->lfo2_tri = (float *) bristolmalloc0(audiomain->segmentsize);
	BME700LOCAL->lfo2_sqr = (float *) bristolmalloc0(audiomain->segmentsize);
	BME700LOCAL->oscbuf = (float *) bristolmalloc0(audiomain->segmentsize);
	BME700LOCAL->adsr1buf = (float *) bristolmalloc0(audiomain->segmentsize);
	BME700LOCAL->adsr2buf = (float *) bristolmalloc0(audiomain->segmentsize);
	BME700LOCAL->outbuf = (float *) bristolmalloc0(audiomain->segmentsize);
	BME700LOCAL->zerobuf = (float *) bristolmalloc0(audiomain->segmentsize);
	BME700LOCAL->scratch = (float *) bristolmalloc0(audiomain->segmentsize);
	BME700LOCAL->pwmbuf = (float *) bristolmalloc0(audiomain->segmentsize);
	BME700LOCAL->noisebuf = (float *) bristolmalloc0(audiomain->segmentsize);
	BME700LOCAL->filtbuf = (float *) bristolmalloc0(audiomain->segmentsize);
	BME700LOCAL->resbuf = (float *) bristolmalloc0(audiomain->segmentsize);

	return(0);
}

