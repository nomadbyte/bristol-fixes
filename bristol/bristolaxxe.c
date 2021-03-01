
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

/* ARP AXXE algorithm - we use two OSC, need one compound. FFS. */

/*#define DEBUG */

#include "bristol.h"
#include "bristolmm.h"
#include "bristolaxxe.h"

/*
 * Use of these axxe global buffers will be an issue with use of multiple
 * audio threads, unless we ensure a single thread deals with any given algo
 * type, since then they are only used sequentially.
 */
static float *freqbuf = (float *) NULL;
static float *filtbuf = (float *) NULL;
static float *noisebuf = (float *) NULL;
static float *adsrbuf = (float *) NULL;
static float *lfosh = (float *) NULL;
static float *lfosine = (float *) NULL;
static float *lfosquare = (float *) NULL;
static float *osc1buf = (float *) NULL;
static float *osc2buf = (float *) NULL;
static float *scratch = (float *) NULL;

int
axxeController(Baudio *baudio, u_char operator, u_char controller, float value)
{
	int ivalue = value * CONTROLLER_RANGE;

#ifdef DEBUG
	printf("bristolAxxeControl(%i, %i, %f)\n", operator, controller, value);
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
			((axxemods *) baudio->mixlocals)->o_f_sine = value;
			break;
		case 3:
			((axxemods *) baudio->mixlocals)->o_f_square = value;
			break;
		case 4:
			((axxemods *) baudio->mixlocals)->o_f_sh = value / 16;
			break;
		case 5:
			((axxemods *) baudio->mixlocals)->o_f_adsr = value;
			break;
		case 6:
			((axxemods *) baudio->mixlocals)->o_pwm_sine = value * 512;
			break;
		case 7:
			((axxemods *) baudio->mixlocals)->o_pwm_adsr = value * 100;
			break;
		case 8:
			((axxemods *) baudio->mixlocals)->m_noise = value;
			break;
		case 9:
			((axxemods *) baudio->mixlocals)->m_ramp = value;
			break;
		case 10:
			((axxemods *) baudio->mixlocals)->m_square = value * 96.0;
			break;
		case 11:
			((axxemods *) baudio->mixlocals)->f_sine = value;
			break;
		case 12:
			((axxemods *) baudio->mixlocals)->f_adsr = value;
			break;
		case 13:
			((axxemods *) baudio->mixlocals)->a_gain = value;
			break;
		case 14:
			((axxemods *) baudio->mixlocals)->a_adsr = value;
			break;
		case 15:
			if (baudio->voicecount == 1)
			{
				/* Multi Env trigger */
				if (ivalue == 0)
					baudio->mixflags &= ~BRISTOL_MULTITRIG;
				else
					baudio->mixflags |= BRISTOL_MULTITRIG;
			} else {
				/*
				 * Else multi lfo
				 */
				if (ivalue)
					baudio->mixflags &= ~A_LFO_MULTI;
				else
					baudio->mixflags |= A_LFO_MULTI;
			}
			break;
		case 16:
			/* PPC Flatten */
			if (ivalue)
				ivalue = -1;
			else
				ivalue = 0;
			baudio->gtune = 1.0
				+ (baudio->note_diff - 1) * (ivalue * 2);

			buildCurrentTable(baudio, baudio->gtune);
			alterAllNotes(baudio);
			break;
		case 17:
			/* PPC trill */
			if (ivalue)
				baudio->mixflags |= A_LFO_TRILL;
			else
				baudio->mixflags &= ~A_LFO_TRILL;
			break;
		case 18:
			/* PPC Sharpen */
			if (ivalue)
				ivalue = 1;
			else
				ivalue = 0;
			baudio->gtune = 1.0
				+ (baudio->note_diff - 1) * (ivalue * 2);

			buildCurrentTable(baudio, baudio->gtune);
			alterAllNotes(baudio);
			break;
		case 19:
			/* Auto Env repeat. */
			if (ivalue == 0) {
				((axxemods *) baudio->mixlocals)->lfoAuto = -1;
			} else {
				float low;

				/*
				 * We have been given a value between 0 and 1.0.
				 *	0 = 0.1Hz
				 *  1 = 10 Hz
				 *
				 * The delays are only going to be based on sample buffer
				 * delays, so find out the nearest number of buffers for any
				 * given rate.
				 *
				 * Buffer time = samplecount / samplerate.
				 *
				 *	Low = 0.1 / btime
				 *	High = 10 / btime (= 100 * Low)
				 *
				 * Rate is the value between these.
				 *
				 *	count = Low + 100 * Low  * (1 - value)
				low = 0.1 / ( ((float) audiomain->samplecount)
					/ ((float) audiomain->samplerate));
				 *
				 * Bums, don't have easy access to sc and sr.
				 */
				value = gainTable[
					(int) ((1 - value) * (CONTROLLER_RANGE - 1))].gain;
				low = 0.1 / ( ((float) 256)
					/ ((float) baudio->samplerate));
				((axxemods *) baudio->mixlocals)->lfoAuto =
					low + 100 * low * value;
			}
			break;
	}
	return(0);
}

int
axxePreops(audioMain *audiomain, Baudio *baudio,
bristolVoice *voice, register float *startbuf)
{
	if (((axxemods *) baudio->mixlocals)->lfolocals == 0)
	{
		((axxemods *) baudio->mixlocals)->lfolocals =
			baudio->locals[voice->index][2];
	}

/* NOISE */
	bristolbzero(noisebuf, audiomain->segmentsize);
	audiomain->palette[(*baudio->sound[6]).index]->specs->io[0].buf = noisebuf;
	(*baudio->sound[6]).operate(
		(audiomain->palette)[4],
		voice,
		(*baudio->sound[6]).param,
		voice->locals[voice->index][6]);
/* NOISE DONE */

/* LFO produces 3 outputs, sine, square and S/H */
	if ((baudio->mixflags & A_LFO_MULTI) == 0)
	{
		/*
		 * Operate LFO if we have one per voice.
		 */
		audiomain->palette[(*baudio->sound[2]).index]->specs->io[0].buf
			= noisebuf;
		audiomain->palette[(*baudio->sound[2]).index]->specs->io[4].buf
			= lfosine;
		audiomain->palette[(*baudio->sound[2]).index]->specs->io[2].buf
			= lfosquare;
		audiomain->palette[(*baudio->sound[2]).index]->specs->io[3].buf
			= lfosh;
		(*baudio->sound[2]).operate(
			(audiomain->palette)[16],
			voice,
			(*baudio->sound[2]).param,
			((axxemods *) baudio->mixlocals)->lfolocals);
	}

	if (((axxemods *) baudio->mixlocals)->lfoAuto >= 0)
	{
		if (++((axxemods *) baudio->mixlocals)->lfoAutoCount
			>= ((axxemods *) baudio->mixlocals)->lfoAuto)
			((axxemods *) baudio->mixlocals)->lfoAutoCount = 0;
	}
/* */
	return(0);
}

int
operateOneAxxe(audioMain *audiomain, Baudio *baudio,
bristolVoice *voice, register float *startbuf)
{
	axxemods *mods = (axxemods *) baudio->mixlocals;
	int sc = audiomain->samplecount, flags;

/*printf("operateOneAxxe(%i, %x, %x)\n", voice->index, audiomain, baudio); */

	bristolbzero(freqbuf, audiomain->segmentsize);
	bristolbzero(osc1buf, audiomain->segmentsize);
	bristolbzero(osc2buf, audiomain->segmentsize);
	bristolbzero(filtbuf, audiomain->segmentsize);
	bristolbzero(scratch, audiomain->segmentsize);
	/*
	 * There are a lot of mods in this synth, each operator will produce its
	 * output buffer, and then they will modulate eachother.
	 *
	 * Noise is done, run the LFO and ADSR.
	 */

/* LFO produces 3 outputs, sine, square and S/H */
	if (baudio->mixflags & A_LFO_MULTI)
	{
		/*
		 * Operate LFO if we have one per voice.
		 */
		audiomain->palette[(*baudio->sound[2]).index]->specs->io[0].buf
			= noisebuf;
		audiomain->palette[(*baudio->sound[2]).index]->specs->io[4].buf
			= lfosine;
		audiomain->palette[(*baudio->sound[2]).index]->specs->io[2].buf
			= lfosquare;
		audiomain->palette[(*baudio->sound[2]).index]->specs->io[3].buf
			= lfosh;
		(*baudio->sound[2]).operate(
			(audiomain->palette)[16],
			voice,
			(*baudio->sound[2]).param,
			baudio->locals[voice->index][2]);
	}

	if (mods->lfoAuto >= 0)
	{
		if (mods->lfoAutoCount == 0)
		{
			if ((voice->flags & (BRISTOL_KEYOFF|BRISTOL_KEYOFFING)) == 0)
				voice->flags |= BRISTOL_KEYREON;
/*			if ((voice->flags & (BRISTOL_KEYOFF|BRISTOL_KEYOFFING)) == 0) */
/*			{ */
/*				voice->flags |= BRISTOL_KEYREON; */
/*			} */
		}
	}
/* */

/* ADSR */
	/*
	 * Run the ADSR.
	 */
	audiomain->palette[(*baudio->sound[4]).index]->specs->io[0].buf = adsrbuf;
	(*baudio->sound[4]).operate(
		(audiomain->palette)[1],
		voice,
		(*baudio->sound[4]).param,
		voice->locals[voice->index][4]);
/* ADSR - OVER */

	/*
	 * We now have our mods, Noise, ADSR, S/H, Sine, Square.
	 *
	 * Run the oscillators.
	 */
/* OSC */
	audiomain->palette[(*baudio->sound[0]).index]->specs->io[0].buf = freqbuf;
	audiomain->palette[(*baudio->sound[0]).index]->specs->io[1].buf = 0;
	audiomain->palette[(*baudio->sound[0]).index]->specs->io[2].buf = osc1buf;

	fillFreqTable(baudio, voice, freqbuf, sc, 1);

	/*
	 * Put in any desired mods. There are two, MG and Wheel mod. Wheel mod is
	 * actually quite week, takes the LFO to freqbuf, same as MG without the
	 * env. Hm.
	 */
	if (baudio->mixflags & A_LFO_TRILL)
		bufmerge(lfosine, 0.2, freqbuf, 1.0, sc);
	if (mods->o_f_sine != 0.0f)
		bufmerge(lfosine, mods->o_f_sine, freqbuf, 1.0, sc);
	if (mods->o_f_square != 0.0f)
		bufmerge(lfosquare, mods->o_f_square, freqbuf, 1.0, sc);
	if (mods->o_f_sh != 0.0f)
		bufmerge(lfosh, mods->o_f_sh, freqbuf, 1.0, sc);
	if (mods->o_f_adsr != 0.0f)
		bufmerge(adsrbuf, mods->o_f_adsr, freqbuf, 1.0, sc);

	(*baudio->sound[0]).operate(
		(audiomain->palette)[8],
		voice,
		(*baudio->sound[0]).param,
		voice->locals[voice->index][0]);

	audiomain->palette[(*baudio->sound[1]).index]->specs->io[0].buf = freqbuf;
	audiomain->palette[(*baudio->sound[1]).index]->specs->io[1].buf = scratch;
	audiomain->palette[(*baudio->sound[1]).index]->specs->io[2].buf = osc2buf;

	if (mods->o_pwm_sine != 0.0f)
		bufmerge(lfosine, mods->o_pwm_sine, scratch, 0.0, sc);
	if (mods->o_pwm_adsr != 0.0f)
		bufmerge(adsrbuf, mods->o_pwm_adsr, scratch, 1.0, sc);

	/*
	 * This was called with sound 0 rather than 1, so had two different freq
	 * and mixed output buffers. Sounded like ring mod.
	 */
	(*baudio->sound[1]).operate(
		(audiomain->palette)[8],
		voice,
		(*baudio->sound[1]).param,
		voice->locals[voice->index][1]);
/* OSC done */

/* Mixer */
	bufmerge(noisebuf, mods->m_noise, osc1buf, mods->m_ramp, sc);
	bufmerge(osc2buf, mods->m_square, osc1buf, 96.0, sc);
/* */

/* Filter */
	audiomain->palette[(*baudio->sound[3]).index]->specs->io[0].buf
		= osc1buf;
	audiomain->palette[(*baudio->sound[3]).index]->specs->io[1].buf
		= scratch;
	audiomain->palette[(*baudio->sound[3]).index]->specs->io[2].buf
		= baudio->rightbuf;;

	bristolbzero(scratch, audiomain->segmentsize);
	if (mods->f_sine != 0.0f)
		bufmerge(lfosine, mods->f_sine, scratch, 0.0, sc);
	if (mods->f_adsr != 0.0f)
		bufmerge(adsrbuf, mods->f_adsr, scratch, 1.0, sc);

	(*baudio->sound[3]).operate(
		(audiomain->palette)[3],
		voice,
		(*baudio->sound[3]).param,
		voice->locals[voice->index][3]);
/* Filter DONE */

/* AMP */
	/*
	 * Reworked this for dual envelope mix - key grooming - reusing the lfosine
	 * buffer....
	bufmerge(lfosine, 0.0, adsrbuf, mods->a_adsr, sc);
	{
		register float gain = mods->a_gain, *ab = adsrbuf;
		register int i = 0;

		for (; i < sc; i += 8)
		{
			*ab++ += gain;
			*ab++ += gain;
			*ab++ += gain;
			*ab++ += gain;
			*ab++ += gain;
			*ab++ += gain;
			*ab++ += gain;
			*ab++ += gain;
		}
	}
	 */

	flags = voice->flags;
	audiomain->palette[(*baudio->sound[7]).index]->specs->io[0].buf = lfosine;
	(*baudio->sound[7]).operate(
		(audiomain->palette)[1],
		voice,
		(*baudio->sound[7]).param,
		voice->locals[voice->index][7]);
	voice->flags = flags;

	bufmerge(lfosine, mods->a_gain, adsrbuf, mods->a_adsr, sc);

	audiomain->palette[(*baudio->sound[5]).index]->specs->io[0].buf
		= baudio->rightbuf;
	audiomain->palette[(*baudio->sound[5]).index]->specs->io[1].buf
		= adsrbuf;
	audiomain->palette[(*baudio->sound[5]).index]->specs->io[2].buf =
		baudio->leftbuf;

	(*baudio->sound[5]).operate(
		(audiomain->palette)[2],
		voice,
		(*baudio->sound[5]).param,
		voice->locals[voice->index][5]);
/* AMP OVER */
	/*
	 * It is bad practice to use the rightbuf. It can be done, but must be
	 * zeroed - most efficiently in postOps but otherwise.....
	 */
	bristolbzero(baudio->rightbuf, audiomain->segmentsize);

	return(0);
}

static int
bristolAxxeDestroy(audioMain *audiomain, Baudio *baudio)
{
printf("removing one axxe\n");
	return(0);
	bristolfree(freqbuf);
	bristolfree(filtbuf);
	bristolfree(noisebuf);
	bristolfree(adsrbuf);
	bristolfree(lfosine);
	bristolfree(lfosquare);
	bristolfree(lfosh);
	bristolfree(osc1buf);
	bristolfree(osc1buf);

	return(0);
}

int
bristolAxxeInit(audioMain *audiomain, Baudio *baudio)
{
printf("initialising one axxe\n");
	baudio->soundCount = 8; /* Number of operators in this voice */
	/*
	 * Assign an array of sound pointers.
	 */
	baudio->sound = (bristolSound **)
		bristolmalloc0(sizeof(bristolOP *) * baudio->soundCount);
	baudio->effect = (bristolSound **)
		bristolmalloc0(sizeof(bristolOP *) * baudio->soundCount);

	/* Ramp oscillator */
	initSoundAlgo(8, 0, baudio, audiomain, baudio->sound);
	/* Square oscillator */
	initSoundAlgo(8, 1, baudio, audiomain, baudio->sound);
	/* LFO */
	initSoundAlgo(16, 2, baudio, audiomain, baudio->sound);
	/* A filter */
	initSoundAlgo(3, 3, baudio, audiomain, baudio->sound);
	/* Single ADSR */
	initSoundAlgo(1, 4, baudio, audiomain, baudio->sound);
	/* An amplifier */
	initSoundAlgo(2, 5, baudio, audiomain, baudio->sound);
	/* An noise source */
	initSoundAlgo(4, 6, baudio, audiomain, baudio->sound);
	/* A grooming env for direct gain to amp */
	initSoundAlgo(1, 7, baudio, audiomain, baudio->sound);

	baudio->param = axxeController;
	baudio->destroy = bristolAxxeDestroy;
	baudio->operate = operateOneAxxe;
	baudio->preops = axxePreops;

	if (freqbuf == NULL)
		freqbuf = (float *) bristolmalloc0(audiomain->segmentsize);
	if (filtbuf == NULL)
		filtbuf = (float *) bristolmalloc0(audiomain->segmentsize);
	if (noisebuf == NULL)
		noisebuf = (float *) bristolmalloc0(audiomain->segmentsize);
	if (adsrbuf == NULL)
		adsrbuf = (float *) bristolmalloc0(audiomain->segmentsize);
	if (lfosh == NULL)
		lfosh = (float *) bristolmalloc0(audiomain->segmentsize);
	if (lfosquare == NULL)
		lfosquare = (float *) bristolmalloc0(audiomain->segmentsize);
	if (lfosine == NULL)
		lfosine = (float *) bristolmalloc0(audiomain->segmentsize);
	if (osc1buf == NULL)
		osc1buf = (float *) bristolmalloc0(audiomain->segmentsize);
	if (osc2buf == NULL)
		osc2buf = (float *) bristolmalloc0(audiomain->segmentsize);
	if (scratch == NULL)
		scratch = (float *) bristolmalloc0(audiomain->segmentsize);

	baudio->mixlocals = (float *) bristolmalloc0(sizeof(axxemods));

	return(0);
}

