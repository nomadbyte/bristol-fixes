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
 * This will be a granular synthesiser. We need to build an efficient method to
 * generate a potentially very large number of grains, each of which will take
 *
 *	a gain curve, initially an inverted, normalised cosine or gaussian envelope
 *	a wave table
 *
 * The wavetable will be resampled to a given frequency at some extraction of
 * the given note, and its gain will be defined by a multiple of the resampled
 * gain curve. The multiple will be partially randomised by parameterisation.
 * We should allow for glissando of the waveforms. Grain time shall be from 2
 * to 100ms.
 *
 * Wave tables should be the usual suspects to start with.
 *
 * The result should go through an envelope generator and amp.
 */

/* #define DEBUG */

#include "bristol.h"
#include "granular.h"

static float *freqbuf = (float *) NULL;

static float *zerobuf = (float *) NULL;

static float *lfobuf = (float *) NULL;
static float *noisebuf = (float *) NULL;

static float *adsrbuf = (float *) NULL;

static float *bus1buf = (float *) NULL;
static float *bus2buf = (float *) NULL;
static float *bus3buf = (float *) NULL;
static float *bus4buf = (float *) NULL;
static float *bus5buf = (float *) NULL;
static float *bus6buf = (float *) NULL;
static float *bus7buf = (float *) NULL;
static float *bus8buf = (float *) NULL;
static float *bus9buf = (float *) NULL;

static float *busbuf[9];

extern int bristolGlobalController(struct BAudio *, u_char, u_char, float);
extern int buildCurrentTable(Baudio *, float);

int
granularGlobalController(Baudio *baudio, u_char operator,
u_char controller, float value)
{
	/*
	 * Granular global controller code. We will need to control the effects 
	 * chain and global tuning from here.
printf("granularGlobalController(%x, %i, %i, %f)\n",
baudio, controller, operator, value);
	 */

	/*
	 * 98 will be the dimension D
	 */
	if (operator == 98)
	{
		baudio->effect[1]->param->param[controller].float_val = value;
		return(0);
	}

	/*
	 * Reverb
	 */
	if (operator == 99)
	{
		if (controller == 0)
		{
			baudio->effect[0]->param->param[controller].float_val = value;
			baudio->effect[0]->param->param[controller].int_val = 1;
		} else
			baudio->effect[0]->param->param[controller].float_val = value;
/*
printf("F %f, C %f, W %f, G %f ? %f\n",
baudio->effect[0]->param->param[1].float_val,
baudio->effect[0]->param->param[2].float_val,
baudio->effect[0]->param->param[3].float_val,
baudio->effect[0]->param->param[4].float_val,
baudio->effect[0]->param->param[5].float_val);
*/

		return(0);
	}

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
		case 3:
			if (value == 0.0)
				baudio->mixflags &= ~KEY_TRACKING;
			else
				baudio->mixflags |= KEY_TRACKING;
			break;
		case 4:
			if (value == 0.0)
				baudio->mixflags &= ~MULTI_LFO;
			else
				baudio->mixflags |= MULTI_LFO;
			break;
		case 10:
			/*
			 * Mod routing Env1
			 */
			((granularmods *) baudio->mixlocals)->env1mod =
				value * CONTROLLER_RANGE;
printf("env 1 flags are %i\n", (int) (value * CONTROLLER_RANGE));
			break;
		case 11:
			/*
			 * Mod routing Env2
			 */
			((granularmods *) baudio->mixlocals)->env2mod =
				value * CONTROLLER_RANGE;
printf("env 2 flags are %i\n", (int) (value * CONTROLLER_RANGE));
			break;
		case 12:
			/*
			 * Mod routing Env3
			 */
			((granularmods *) baudio->mixlocals)->env3mod =
				value * CONTROLLER_RANGE;
printf("env 3 flags are %i\n", (int) (value * CONTROLLER_RANGE));
			break;
		case 13:
			/*
			 * Mod routing LFO
			 */
			((granularmods *) baudio->mixlocals)->lfomod =
				value * CONTROLLER_RANGE;
printf("lfo flags are %i\n", (int) (value * CONTROLLER_RANGE));
			break;
		case 14:
			/*
			 * Mod routing Noise
			 */
			((granularmods *) baudio->mixlocals)->noisemod =
				value * CONTROLLER_RANGE;
printf("noise flags are %i\n", (int) (value * CONTROLLER_RANGE));
			break;
	}
	return(0);
}

int
operateGranularPreops(audioMain *audiomain, Baudio *baudio,
bristolVoice *voice, register float *startbuf)
{
#ifdef DEBUG
	printf("operateGranularPreops(%x, %x, %x) %i\n",
		baudio, voice, startbuf, baudio->cvoices);
#endif

	if (lfobuf == NULL)
		return(0);

	if ((baudio->mixflags & MULTI_LFO) == 0)
	{
		audiomain->palette[(*baudio->sound[4]).index]->specs->io[0].buf
			= NULL;
		audiomain->palette[(*baudio->sound[4]).index]->specs->io[1].buf
			= lfobuf;
		audiomain->palette[(*baudio->sound[4]).index]->specs->io[2].buf
			= NULL;
		audiomain->palette[(*baudio->sound[4]).index]->specs->io[3].buf
			= NULL;
		audiomain->palette[(*baudio->sound[4]).index]->specs->io[4].buf
			= NULL;
		(*baudio->sound[4]).operate(
			(audiomain->palette)[16],
			voice,
			(*baudio->sound[4]).param,
			baudio->locals[voice->index][4]);

		bufmerge(zerobuf, 0.0,
			lfobuf, baudio->contcontroller[1], audiomain->samplecount);
	}

	return(0);
}

static void
modRoute(float *source, unsigned int flags, int sc)
{
	int i;

	for (i = 0; i < 9; i++)
		if ((flags & (1 << i)) != 0)
			bufmerge(source, 1.0, busbuf[i], 1.0, sc);
}

/*
 * Operate one granular voice.
 */
int
operateOneGranularVoice(audioMain *audiomain, Baudio *baudio,
bristolVoice *voice, register float *startbuf)
{
	int samplecount = audiomain->samplecount;
	int i;
	granularmods *mods = (granularmods *) baudio->mixlocals;

#ifdef DEBUG
	printf("operateOneGranularVoice(%x, %x, %x)\n", baudio, voice, startbuf);
#endif

	for (i = 0; i < 9; i++)
		bristolbzero(busbuf[i], audiomain->segmentsize);

	bristolbzero(startbuf, audiomain->segmentsize);

	if (lfobuf == NULL)
		return(0);

	if ((baudio->mixflags & MULTI_LFO) != 0)
	{
		audiomain->palette[(*baudio->sound[4]).index]->specs->io[0].buf
			= NULL;
		audiomain->palette[(*baudio->sound[4]).index]->specs->io[1].buf
			= lfobuf;
		audiomain->palette[(*baudio->sound[4]).index]->specs->io[2].buf
			= NULL;
		audiomain->palette[(*baudio->sound[4]).index]->specs->io[3].buf
			= NULL;
		audiomain->palette[(*baudio->sound[4]).index]->specs->io[4].buf
			= NULL;
		(*baudio->sound[4]).operate(
			(audiomain->palette)[16],
			voice,
			(*baudio->sound[4]).param,
			baudio->locals[voice->index][4]);

		bufmerge(zerobuf, 0.0,
			lfobuf, baudio->contcontroller[1], samplecount);
	}
	modRoute(lfobuf, mods->lfomod, samplecount);

	/* Noise source */
	bristolbzero(noisebuf, audiomain->segmentsize);
	audiomain->palette[(*baudio->sound[5]).index]->specs->io[0].buf = noisebuf;
	(*baudio->sound[5]).operate(
		(audiomain->palette)[4],
		voice,
		(*baudio->sound[5]).param,
		voice->locals[voice->index][5]);
	bufmerge(zerobuf, 0.0, noisebuf, 0.01, samplecount);
	modRoute(noisebuf, mods->noisemod, samplecount);

	/* ADSR 1, 2, 3 */
	audiomain->palette[(*baudio->sound[1]).index]->specs->io[0].buf = adsrbuf;
	(*baudio->sound[1]).operate(
		(audiomain->palette)[1],
		voice,
		(*baudio->sound[1]).param,
		voice->locals[voice->index][1]);
	modRoute(adsrbuf, mods->env1mod, samplecount);

	(*baudio->sound[2]).operate(
		(audiomain->palette)[1],
		voice,
		(*baudio->sound[2]).param,
		voice->locals[voice->index][2]);
	modRoute(adsrbuf, mods->env2mod, samplecount);

	(*baudio->sound[3]).operate(
		(audiomain->palette)[1],
		voice,
		(*baudio->sound[3]).param,
		voice->locals[voice->index][3]);
	modRoute(adsrbuf, mods->env3mod, samplecount);

	/*
	 * Merge the freqbuf into bus3buf for key tracking, which should be optional
	 */
	if (baudio->mixflags & KEY_TRACKING) {
		/*
		 * Fill the wavetable with the correct note value, accepting glissando.
		 */
		fillFreqTable(baudio, voice, freqbuf, samplecount, 1);
		bufmerge(freqbuf, 1.0, bus3buf, 1.0, samplecount);
	}

	/*
	 * Do the grains. We need to pass the mods, rbuf, lbuf
	 */
	audiomain->palette[(*baudio->sound[0]).index]->specs->io[0].buf = freqbuf;
	audiomain->palette[(*baudio->sound[0]).index]->specs->io[1].buf =
		baudio->leftbuf;
	audiomain->palette[(*baudio->sound[0]).index]->specs->io[2].buf =
		baudio->rightbuf;
	audiomain->palette[(*baudio->sound[0]).index]->specs->io[3].buf = bus1buf;
	audiomain->palette[(*baudio->sound[0]).index]->specs->io[4].buf = bus2buf;
	audiomain->palette[(*baudio->sound[0]).index]->specs->io[5].buf = bus3buf;
	audiomain->palette[(*baudio->sound[0]).index]->specs->io[6].buf = bus4buf;
	audiomain->palette[(*baudio->sound[0]).index]->specs->io[7].buf = bus5buf;
	audiomain->palette[(*baudio->sound[0]).index]->specs->io[8].buf = bus6buf;
	audiomain->palette[(*baudio->sound[0]).index]->specs->io[9].buf = bus7buf;
	audiomain->palette[(*baudio->sound[0]).index]->specs->io[10].buf = bus8buf;
	audiomain->palette[(*baudio->sound[0]).index]->specs->io[11].buf = bus9buf;

	(*baudio->sound[0]).operate(
		(audiomain->palette)[29],
		voice,
		(*baudio->sound[0]).param,
		voice->locals[voice->index][0]);

	return(0);
}

int
operateGranularPostops(audioMain *audiomain, Baudio *baudio,
bristolVoice *voice, register float *startbuf)
{
#ifdef DEBUG
	printf("operateGranularPostops(%x, %x, %x) %i\n",
		baudio, voice, startbuf, baudio->cvoices);
#endif

	if (baudio->mixlocals == NULL)
		/* ((void *) baudio->mixlocals) = voice->locals[voice->index]; */
		baudio->mixlocals = (float *) voice->locals[voice->index];

	return(0);
}

int
destroyOneGranularVoice(audioMain *audiomain, Baudio *baudio)
{
	baudio->mixlocals = NULL;
	return(0);
	if (freqbuf != NULL)
		bristolfree(freqbuf);
	freqbuf = NULL;

	if (baudio->mixlocals != NULL)
		bristolfree(baudio->mixlocals);

	baudio->mixlocals = NULL;

	return(0);
}

int
bristolGranularInit(audioMain *audiomain, Baudio *baudio)
{
	granularmods *mods;

printf("initialising one granular sound\n");

	baudio->soundCount = 6; /* Number of operators in this granular voice */

	/*
	 * Assign an array of sound pointers.
	 */
	baudio->sound = (bristolSound **)
		bristolmalloc0(sizeof(bristolOP *) * baudio->soundCount);
	baudio->effect = (bristolSound **)
		bristolmalloc0(sizeof(bristolOP *) * baudio->soundCount);

	/*
	 * We need 3 oscillators, two for the higher frequencies and one for the
	 * bass. Then we need an envelope and amplifier. After that we will start
	 * with one effect, the flanger and may add a reverb in depending on the 
	 * results.
	 *
	 * We will use the ARP oscillators (obviously), producing two waveforms
	 * each, a ramp and a square.
	 */
	initSoundAlgo( 29,	0, baudio, audiomain, baudio->sound);
	/* 3 ADSR */
	initSoundAlgo( 1,	1, baudio, audiomain, baudio->sound);
	initSoundAlgo( 1,	2, baudio, audiomain, baudio->sound);
	initSoundAlgo( 1,	3, baudio, audiomain, baudio->sound);
	/* LFO - preops or main ops? */
	initSoundAlgo(16,	4, baudio, audiomain, baudio->sound);
	/* Noise source */
	initSoundAlgo( 4,	5, baudio, audiomain, baudio->sound);

	/*
	 * Add in a flanger. We could add in a reverb before or after, or even
	 * a second flanger to space the sound out a little more.
	initSoundAlgo(	22, 0, baudio, audiomain, baudio->effect);
	initSoundAlgo(	12,	1, baudio, audiomain, baudio->effect);
	 */

	baudio->param = granularGlobalController;
	baudio->destroy = destroyOneGranularVoice;
	baudio->operate = operateOneGranularVoice;
	baudio->preops = operateGranularPreops;
	/*baudio->postops = operateGranularPostops; */

	/*
	 * Get some workspace
	 */
	if (freqbuf == (float *) NULL)
		freqbuf = (float *) bristolmalloc0(audiomain->segmentsize);
	if (zerobuf == (float *) NULL)
		zerobuf = (float *) bristolmalloc0(audiomain->segmentsize);
	if (lfobuf == (float *) NULL)
		lfobuf = (float *) bristolmalloc0(audiomain->segmentsize);
	if (adsrbuf == (float *) NULL)
		adsrbuf = (float *) bristolmalloc0(audiomain->segmentsize);
	if (noisebuf == (float *) NULL)
		noisebuf = (float *) bristolmalloc0(audiomain->segmentsize);
	if (bus1buf == (float *) NULL)
		bus1buf = (float *) bristolmalloc0(audiomain->segmentsize);
	if (bus2buf == (float *) NULL)
		bus2buf = (float *) bristolmalloc0(audiomain->segmentsize);
	if (bus3buf == (float *) NULL)
		bus3buf = (float *) bristolmalloc0(audiomain->segmentsize);
	if (bus4buf == (float *) NULL)
		bus4buf = (float *) bristolmalloc0(audiomain->segmentsize);
	if (bus5buf == (float *) NULL)
		bus5buf = (float *) bristolmalloc0(audiomain->segmentsize);
	if (bus6buf == (float *) NULL)
		bus6buf = (float *) bristolmalloc0(audiomain->segmentsize);
	if (bus7buf == (float *) NULL)
		bus7buf = (float *) bristolmalloc0(audiomain->segmentsize);
	if (bus8buf == (float *) NULL)
		bus8buf = (float *) bristolmalloc0(audiomain->segmentsize);
	if (bus9buf == (float *) NULL)
		bus9buf = (float *) bristolmalloc0(audiomain->segmentsize);

	busbuf[0] = bus1buf;
	busbuf[1] = bus2buf;
	busbuf[2] = bus3buf;
	busbuf[3] = bus4buf;
	busbuf[4] = bus5buf;
	busbuf[5] = bus6buf;
	busbuf[6] = bus7buf;
	busbuf[7] = bus8buf;
	busbuf[8] = bus9buf;

	mods = (granularmods *) bristolmalloc0(sizeof(granularmods));

	//printf("size is %i\n", sizeof(granularmods));

	baudio->mixlocals = (float *) mods;
	baudio->mixflags |= BRISTOL_STEREO;

printf("initialised one granular sound\n");

	return(0);
}

