
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
#include "roadrunner.h"

static float *freqbuf = (float *) NULL;
static float *modbuf = (float *) NULL;
static float *sawbuf = (float *) NULL;
static float *tribuf = (float *) NULL;
static float *sqrbuf = (float *) NULL;
static float *lfobuf = (float *) NULL;

extern int bristolGlobalController(struct BAudio *, u_char, u_char, float);
extern int buildCurrentTable(Baudio *, float);

int
roadrunnerGlobalController(Baudio *baudio, u_char operator,
u_char controller, float value)
{
	/*
	 * Roadrunner global controller code. We will need to control the effects 
	 * chain and global tuning from here.
printf("roadrunnerGlobalController(%x, %i, %i, %f)\n",
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
			((roadrunnermods *) baudio->mixlocals)->pwm = value * 128;
			break;
		case 4:
			if (value == 0.0)
				baudio->mixflags &= ~MULTI_LFO;
			else
				baudio->mixflags |= MULTI_LFO;
			break;
		case 5:
			((roadrunnermods *) baudio->mixlocals)->lowsplit =
				value * CONTROLLER_RANGE;
			break;
		case 6:
			((roadrunnermods *) baudio->mixlocals)->highsplit =
				value * CONTROLLER_RANGE;
			break;
		case 7:
			((roadrunnermods *) baudio->mixlocals)->vibrato = value * 0.25;
			break;
		case 8:
			((roadrunnermods *) baudio->mixlocals)->tremolo = value;
			break;
		case 10:
			if (value == 0)
				baudio->mixflags &= ~RR_OSC1_SAW;
			else
				baudio->mixflags |= RR_OSC1_SAW;
			break;
		case 11:
			if (value == 0)
				baudio->mixflags &= ~RR_OSC1_SQR;
			else
				baudio->mixflags |= RR_OSC1_SQR;
			break;
		case 12:
			if (value == 0)
				baudio->mixflags &= ~RR_OSC1_TRI;
			else
				baudio->mixflags |= RR_OSC1_TRI;
			break;
		case 20:
			if (value == 0)
				baudio->mixflags &= ~RR_OSC2_SAW;
			else
				baudio->mixflags |= RR_OSC2_SAW;
			break;
		case 21:
			if (value == 0)
				baudio->mixflags &= ~RR_OSC2_SQR;
			else
				baudio->mixflags |= RR_OSC2_SQR;
			break;
		case 22:
			if (value == 0)
				baudio->mixflags &= ~RR_OSC2_TRI;
			else
				baudio->mixflags |= RR_OSC2_TRI;
			break;
		case 30:
			if (value == 0)
				baudio->mixflags &= ~RR_OSC3_SAW;
			else
				baudio->mixflags |= RR_OSC3_SAW;
			break;
		case 31:
			if (value == 0)
				baudio->mixflags &= ~RR_OSC3_SQR;
			else
				baudio->mixflags |= RR_OSC3_SQR;
			break;
		case 32:
			if (value == 0)
				baudio->mixflags &= ~RR_OSC3_TRI;
			else
				baudio->mixflags |= RR_OSC3_TRI;
			break;
	}

	return(0);
}

int
operateRoadrunnerPreops(audioMain *audiomain, Baudio *baudio,
bristolVoice *voice, register float *startbuf)
{
	roadrunnermods *mods = (roadrunnermods *) baudio->mixlocals;

#ifdef DEBUG
	printf("operateRoadrunnerPreops(%x, %x, %x) %i\n",
		baudio, voice, startbuf, baudio->cvoices);
#endif

	if ((baudio->mixflags & MULTI_LFO) == 0)
	{
		audiomain->palette[(*baudio->sound[5]).index]->specs->io[0].buf = NULL;
		audiomain->palette[(*baudio->sound[5]).index]->specs->io[1].buf
			= lfobuf;
		audiomain->palette[(*baudio->sound[5]).index]->specs->io[2].buf = NULL;
		audiomain->palette[(*baudio->sound[5]).index]->specs->io[3].buf = NULL;
		audiomain->palette[(*baudio->sound[5]).index]->specs->io[4].buf = NULL;
		(*baudio->sound[5]).operate(
			(audiomain->palette)[16],
			voice,
			(*baudio->sound[5]).param,
			baudio->locals[voice->index][5]);

		bufmerge(lfobuf, mods->pwm, modbuf, 0.0, audiomain->samplecount);
	}

	return(0);
}

/*
 * Operate one roadrunner voice.
 */
int
operateOneRoadrunnerVoice(audioMain *audiomain, Baudio *baudio,
bristolVoice *voice, register float *startbuf)
{
	int samplecount = audiomain->samplecount;
	roadrunnermods *mods = (roadrunnermods *) baudio->mixlocals;

#ifdef DEBUG
	printf("operateOneRoadrunnerVoice(%x, %x, %x)\n", baudio, voice, startbuf);
#endif

	bristolbzero(startbuf, audiomain->segmentsize);

	if (baudio->mixflags & MULTI_LFO)
	{
		audiomain->palette[(*baudio->sound[5]).index]->specs->io[0].buf = NULL;
		audiomain->palette[(*baudio->sound[5]).index]->specs->io[1].buf
			= lfobuf;
		audiomain->palette[(*baudio->sound[5]).index]->specs->io[2].buf = NULL;
		audiomain->palette[(*baudio->sound[5]).index]->specs->io[3].buf = NULL;
		audiomain->palette[(*baudio->sound[5]).index]->specs->io[4].buf = NULL;
		(*baudio->sound[5]).operate(
			(audiomain->palette)[16],
			voice,
			(*baudio->sound[5]).param,
			baudio->locals[voice->index][5]);

		bufmerge(lfobuf, mods->pwm, modbuf, 0.0, samplecount);
	}

	/*
	 * Fill the wavetable with the correct note value
	 */
	fillFreqTable(baudio, voice, freqbuf, samplecount, 0);
	bufmerge(lfobuf, mods->vibrato, freqbuf, 1.0, samplecount);

	audiomain->palette[(*baudio->sound[0]).index]->specs->io[0].buf = freqbuf;
	audiomain->palette[(*baudio->sound[0]).index]->specs->io[1].buf = modbuf;
	audiomain->palette[(*baudio->sound[0]).index]->specs->io[2].buf = sawbuf;
	audiomain->palette[(*baudio->sound[0]).index]->specs->io[3].buf = NULL;
	audiomain->palette[(*baudio->sound[0]).index]->specs->io[4].buf = sqrbuf;
	/* Going to take sine rather than the preferable tri due to noise bug */
	audiomain->palette[(*baudio->sound[0]).index]->specs->io[5].buf = tribuf;
	audiomain->palette[(*baudio->sound[0]).index]->specs->io[6].buf = NULL;

	/*
	 * We will use the same lot of buffers each time around so now we can just
	 * roll off the oscillators.
	 */
	if (voice->key.key >= mods->highsplit)
	{
		bristolbzero(sawbuf, audiomain->segmentsize);
		bristolbzero(sqrbuf, audiomain->segmentsize);
		bristolbzero(tribuf, audiomain->segmentsize);

		(*baudio->sound[0]).operate(
			(audiomain->palette)[19],
			voice,
			(*baudio->sound[0]).param,
			voice->locals[voice->index][0]);

		if (baudio->mixflags & RR_OSC1_SAW)
			bufmerge(sawbuf, 1.0, baudio->rightbuf, 1.0, samplecount);
		if (baudio->mixflags & RR_OSC1_SQR)
			bufmerge(sqrbuf, 1.0, baudio->rightbuf, 1.0, samplecount);
		if (baudio->mixflags & RR_OSC1_TRI)
			bufmerge(tribuf, 1.0, baudio->rightbuf, 1.0, samplecount);

		bristolbzero(sawbuf, audiomain->segmentsize);
		bristolbzero(sqrbuf, audiomain->segmentsize);
		bristolbzero(tribuf, audiomain->segmentsize);

		(*baudio->sound[1]).operate(
			(audiomain->palette)[19],
			voice,
			(*baudio->sound[1]).param,
			voice->locals[voice->index][1]);

		if (baudio->mixflags & RR_OSC2_SAW)
			bufmerge(sawbuf, 12.0, baudio->rightbuf, 1.0, samplecount);
		if (baudio->mixflags & RR_OSC2_SQR)
			bufmerge(sqrbuf, 12.0, baudio->rightbuf, 1.0, samplecount);
		if (baudio->mixflags & RR_OSC2_TRI)
			bufmerge(tribuf, 12.0, baudio->rightbuf, 1.0, samplecount);
	}


	/*
	 * This one will depend on note number.
	 */
	if (voice->key.key <= mods->lowsplit)
	{
		bristolbzero(sawbuf, audiomain->segmentsize);
		bristolbzero(sqrbuf, audiomain->segmentsize);
		bristolbzero(tribuf, audiomain->segmentsize);

		(*baudio->sound[2]).operate(
			(audiomain->palette)[19],
			voice,
			(*baudio->sound[2]).param,
			voice->locals[voice->index][2]);


		if (baudio->mixflags & RR_OSC3_SAW)
			bufmerge(sawbuf, 12.0, baudio->rightbuf, 1.0, samplecount);
		if (baudio->mixflags & RR_OSC3_SQR)
			bufmerge(sqrbuf, 12.0, baudio->rightbuf, 1.0, samplecount);
		if (baudio->mixflags & RR_OSC3_TRI)
			bufmerge(tribuf, 12.0, baudio->rightbuf, 1.0, samplecount);
	}

	/* ADSR */
	audiomain->palette[(*baudio->sound[3]).index]->specs->io[0].buf = freqbuf;
	(*baudio->sound[3]).operate(
		(audiomain->palette)[1],
		voice,
		(*baudio->sound[3]).param,
		voice->locals[voice->index][3]);

	/* AMP */
	audiomain->palette[(*baudio->sound[4]).index]->specs->io[0].buf
		= baudio->rightbuf;
	audiomain->palette[(*baudio->sound[4]).index]->specs->io[1].buf
		= freqbuf;
	audiomain->palette[(*baudio->sound[4]).index]->specs->io[2].buf
		= baudio->leftbuf;

	bufmerge(lfobuf, mods->tremolo, freqbuf, 1.0, samplecount);

	(*baudio->sound[4]).operate(
		(audiomain->palette)[2],
		voice,
		(*baudio->sound[4]).param,
		voice->locals[voice->index][4]);

	bristolbzero(baudio->rightbuf, audiomain->segmentsize);

	return(0);
}

int
operateRoadrunnerPostops(audioMain *audiomain, Baudio *baudio,
bristolVoice *voice, register float *startbuf)
{
#ifdef DEBUG
	printf("operateRoadrunnerPostops(%x, %x, %x) %i\n",
		baudio, voice, startbuf, baudio->cvoices);
#endif

	/*
	if (baudio->mixlocals == NULL)
		baudio->mixlocals = (float *) voice->locals[voice->index];
	*/

	bristolbzero(baudio->rightbuf, audiomain->segmentsize);
	return(0);
}

int
destroyOneRoadrunnerVoice(audioMain *audiomain, Baudio *baudio)
{
	return(0);
}

int
bristolRoadrunnerInit(audioMain *audiomain, Baudio *baudio)
{
printf("initialising one roadrunner sound\n");

	baudio->soundCount = 6; /* Number of operators in this roadrunner voice */

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
	initSoundAlgo( 19,	0, baudio, audiomain, baudio->sound);
	initSoundAlgo( 19,	1, baudio, audiomain, baudio->sound);
	initSoundAlgo( 19,	2, baudio, audiomain, baudio->sound);
	/* ADSR and an amp */
	initSoundAlgo( 1,	3, baudio, audiomain, baudio->sound);
	initSoundAlgo( 2,	4, baudio, audiomain, baudio->sound);
	/* LFO - preops or main ops? */
	initSoundAlgo( 16,	5, baudio, audiomain, baudio->sound);

	/*
	 * Add in a flanger. We could add in a reverb before or after, or even
	 * a second flanger to space the sound out a little more.
	 */
	initSoundAlgo(	22, 0, baudio, audiomain, baudio->effect);
	initSoundAlgo(	12,	1, baudio, audiomain, baudio->effect);

	baudio->param = roadrunnerGlobalController;
	baudio->destroy = destroyOneRoadrunnerVoice;
	baudio->operate = operateOneRoadrunnerVoice;
	baudio->preops = operateRoadrunnerPreops;
	/*baudio->postops = operateRoadrunnerPostops; */

	/*
	 * Get some workspace
	 */
	if (freqbuf == (float *) NULL)
		freqbuf = (float *) bristolmalloc0(audiomain->segmentsize);
	if (modbuf == (float *) NULL)
		modbuf = (float *) bristolmalloc0(audiomain->segmentsize);
	if (sawbuf == (float *) NULL)
		sawbuf = (float *) bristolmalloc0(audiomain->segmentsize);
	if (tribuf == (float *) NULL)
		tribuf = (float *) bristolmalloc0(audiomain->segmentsize);
	if (sqrbuf == (float *) NULL)
		sqrbuf = (float *) bristolmalloc0(audiomain->segmentsize);
	if (lfobuf == (float *) NULL)
		lfobuf = (float *) bristolmalloc0(audiomain->segmentsize);

	baudio->mixlocals = (float *) bristolmalloc0(sizeof(roadrunnermods));

	return(0);
}

