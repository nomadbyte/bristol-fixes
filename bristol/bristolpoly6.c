
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

/* Korg Poly6 algo */

/*#define DEBUG */

#include "bristol.h"
#include "bristolmm.h"
#include "bristolpoly6.h"

/*
 * Use of these poly6 global buffers will be an issue with use of multiple
 * audio threads, unless we ensure a single thread deals with any given algo
 * type, since then they are only used sequentially.
 */
static float *freqbuf = (float *) NULL;
static float *filtbuf = (float *) NULL;
static float *noisebuf = (float *) NULL;
static float *pmodbuf = (float *) NULL;
static float *adsrbuf = (float *) NULL;
static float *oscabuf = (float *) NULL;
static float *mgbuf = (float *) NULL;
static float *lfobuf = (float *) NULL;

int
poly6Controller(Baudio *baudio, u_char operator, u_char controller, float value)
{
	int ivalue = value * CONTROLLER_RANGE;

#ifdef DEBUG
	printf("bristolPoly6Control(%i, %i, %f)\n", operator, controller, value);
#endif

	/*
	 * These will be for our chorus.
	 */
	if (operator == 100)
	{
		if (controller == 1) {
			baudio->effect[0]->param->param[3].float_val = value;
		} else if (controller == 0) {
			value = 0.1 + value * C_RANGE_MIN_1 / 3 * 0.9;
			baudio->effect[0]->param->param[0].float_val =
				(0.1 + (value * value) * 8) * 1024 / baudio->samplerate;
			baudio->effect[0]->param->param[2].float_val = value * 0.2;
			baudio->effect[0]->param->param[1].float_val = value * 0.2;
		}
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
		case 2: /* Bend depth */
			break;
		case 3:
			break;
		case 4:
			((p6mods *) baudio->mixlocals)->pwmlevel = value * 300;
			break;
		case 5:
			if (ivalue == 0)
				baudio->mixflags &= ~P6_PWM;
			else
				baudio->mixflags |= P6_PWM;
			break;
		case 6:
			baudio->mixflags &= ~(P6_M_VCO|P6_M_VCA|P6_M_VCF);
			switch (ivalue) {
				case 0:
					baudio->mixflags |= P6_M_VCA;
					break;
				case 1:
					baudio->mixflags |= P6_M_VCF;
					break;
				case 2:
					baudio->mixflags |= P6_M_VCO;
					break;
			}
			break;
		case 7:
			if (ivalue)
				baudio->mixflags |= BRISTOL_V_UNISON;
			break;
		case 8:
			if (ivalue)
				baudio->mixflags &= ~BRISTOL_V_UNISON;
			break;
	}
	return(0);
}

int
poly6Preops(audioMain *audiomain, Baudio *baudio,
bristolVoice *voice, register float *startbuf)
{
	register int samplecount = audiomain->samplecount, i;
	register float rate = baudio->contcontroller[1] / 2;
	int flags;

	if (((p6mods *) baudio->mixlocals)->lfolocals == 0)
	{
		((p6mods *) baudio->mixlocals)->lfolocals =
			baudio->locals[voice->index][2];
		((p6mods *) baudio->mixlocals)->lfo2locals =
			baudio->locals[voice->index][10];
		((p6mods *) baudio->mixlocals)->adsrlocals = 
			voice->locals[voice->index][8];
	}

	flags = voice->flags;

	/*
	 * If we are configured for auto, only do autoops.
	 */
	/*
	 * If we are not active, and we get a key on, or reon, start the
	 * envelope for LFO delay.
	 *
	 * If we have not started, but we have a key start, that is great.
	 * If we have started, and we have a keyon/reon, remove it.
	 */
	if ((((p6mods *) baudio->mixlocals)->flags & HAVE_STARTED)
		&& (voice->flags & (BRISTOL_KEYON|BRISTOL_KEYREON))
		&& (baudio->lvoices != 0))
		voice->flags &= ~(BRISTOL_KEYON|BRISTOL_KEYREON);

	if ((voice->flags & (BRISTOL_KEYON|BRISTOL_KEYREON))
		&& (baudio->lvoices == 0))
	{
/*printf("lfo auto on\n"); */
		((p6mods *) baudio->mixlocals)->flags |= HAVE_STARTED;
	}

	/*
	 * We only accept key off if this is the last voice on the list.
	 */
	if ((((p6mods *) baudio->mixlocals)->flags & HAVE_STARTED)
		&& (voice->flags & BRISTOL_KEYOFF))
	{
		if (baudio->lvoices > 1)
		{
			voice->flags &= ~BRISTOL_KEYOFF;
		} else {
/*printf("lfo auto off\n"); */
			((p6mods *) baudio->mixlocals)->flags &= ~HAVE_STARTED;
		}
	}

/*printf("auto mode: %x, %x, %i\n", ((p6mods *) baudio->mixlocals)->flags, */
/*voice->flags, voice->index); */

	/*
	 * Fill a freq table for the LFO.
	 */
	for (i = 0; i < samplecount; i+=8)
	{
		*startbuf++ = 0.025 + rate;
		*startbuf++ = 0.025 + rate;
		*startbuf++ = 0.025 + rate;
		*startbuf++ = 0.025 + rate;
		*startbuf++ = 0.025 + rate;
		*startbuf++ = 0.025 + rate;
		*startbuf++ = 0.025 + rate;
		*startbuf++ = 0.025 + rate;
	}
	/*
	 * Run an ADSR for the LFO amplifier.
	 */
	audiomain->palette[(*baudio->sound[8]).index]->specs->io[0].buf = adsrbuf;
	(*baudio->sound[8]).operate(
		(audiomain->palette)[1],
		voice,
		(*baudio->sound[8]).param,
		((p6mods *) baudio->mixlocals)->adsrlocals);

	bristolbzero(oscabuf, audiomain->segmentsize);
	bristolbzero(lfobuf, audiomain->segmentsize);
	bristolbzero(mgbuf, audiomain->segmentsize);

	/*
	 * Run the LFO
	 */
	audiomain->palette[(*baudio->sound[2]).index]->specs->io[1].buf = lfobuf;
	(*baudio->sound[2]).operate(
		(audiomain->palette)[16],
		voice,
		(*baudio->sound[2]).param,
		((p6mods *) baudio->mixlocals)->lfolocals);

	/*
	 * And amplifly it for the delay.
	 */
	audiomain->palette[(*baudio->sound[9]).index]->specs->io[0].buf = lfobuf;
	audiomain->palette[(*baudio->sound[9]).index]->specs->io[1].buf = adsrbuf;
	audiomain->palette[(*baudio->sound[9]).index]->specs->io[2].buf = mgbuf;
	(*baudio->sound[9]).operate(
		(audiomain->palette)[2],
		voice,
		(*baudio->sound[9]).param,
		baudio->locals[voice->index][9]);

	voice->flags = flags;

	bristolbzero(noisebuf, audiomain->segmentsize);
	audiomain->palette[(*baudio->sound[7]).index]->specs->io[0].buf = noisebuf;
	(*baudio->sound[7]).operate(
		(audiomain->palette)[4],
		voice,
		(*baudio->sound[7]).param,
		voice->locals[voice->index][7]);

	bristolbzero(pmodbuf, audiomain->segmentsize);

	/*
	 * Run the PWM LFO
	 */
	audiomain->palette[(*baudio->sound[10]).index]->specs->io[1].buf = pmodbuf;
	(*baudio->sound[10]).operate(
		(audiomain->palette)[16],
		voice,
		(*baudio->sound[10]).param,
		((p6mods *) baudio->mixlocals)->lfo2locals);
	bufmerge(noisebuf, 0.0, pmodbuf,
		((p6mods *) baudio->mixlocals)->pwmlevel, audiomain->samplecount);
	return(0);
}

int
operateOnePoly6(audioMain *audiomain, Baudio *baudio,
bristolVoice *voice, register float *startbuf)
{
/*printf("operateOnePoly6(%i, %x, %x)\n", voice->index, audiomain, baudio); */

	/*
	 * This should be straightforward, the mods are not excessive.
	 *	Do the oscillators, there are two representing one and its subosc.
	 *	Run filter ADSR and filter,
	 *	Run amp ADSR and AMP.
	 */
	bristolbzero(oscabuf, audiomain->segmentsize);
	bristolbzero(freqbuf, audiomain->segmentsize);
	bristolbzero(filtbuf, audiomain->segmentsize);

/* OSC */
	audiomain->palette[(*baudio->sound[0]).index]->specs->io[0].buf = freqbuf;
	if (baudio->mixflags & P6_PWM)
		audiomain->palette[(*baudio->sound[0]).index]->specs->io[1].buf
			= pmodbuf;
	else
		audiomain->palette[(*baudio->sound[0]).index]->specs->io[1].buf
			= baudio->rightbuf;
	audiomain->palette[(*baudio->sound[0]).index]->specs->io[2].buf = oscabuf;

	fillFreqTable(baudio, voice, freqbuf, audiomain->samplecount, 1);

	/*
	 * Put in any desired mods. There are two, MG and Wheel mod. Wheel mod is
	 * actually quite week, takes the LFO to freqbuf, same as MG without the
	 * env. Hm.
	 */
	if (baudio->mixflags & P6_M_VCO)
		bufmerge(mgbuf, baudio->contcontroller[1] * 1.0,
			freqbuf, 1.0, audiomain->samplecount);

	(*baudio->sound[0]).operate(
		(audiomain->palette)[8],
		voice,
		(*baudio->sound[0]).param,
		voice->locals[voice->index][0]);
/* OSC done */

/* SUBOSC */
	audiomain->palette[(*baudio->sound[1]).index]->specs->io[0].buf = freqbuf;
	audiomain->palette[(*baudio->sound[1]).index]->specs->io[1].buf
		= baudio->rightbuf;
	audiomain->palette[(*baudio->sound[1]).index]->specs->io[2].buf = oscabuf;

	(*baudio->sound[1]).operate(
		(audiomain->palette)[8],
		voice,
		(*baudio->sound[1]).param,
		voice->locals[voice->index][1]);
/* SUBOSC DONE */

/* NOISE */
	bufmerge(noisebuf, 48.0, oscabuf, 96.0, audiomain->samplecount);
/* NOISE DONE */

/* FILTER */
	/*
	 * Run the ADSR for the filter.
	 */
	audiomain->palette[(*baudio->sound[4]).index]->specs->io[0].buf = adsrbuf;
	(*baudio->sound[4]).operate(
		(audiomain->palette)[1],
		voice,
		(*baudio->sound[4]).param,
		voice->locals[voice->index][4]);

	audiomain->palette[(*baudio->sound[3]).index]->specs->io[0].buf
		= oscabuf;
	audiomain->palette[(*baudio->sound[3]).index]->specs->io[1].buf
		= adsrbuf;
	audiomain->palette[(*baudio->sound[3]).index]->specs->io[2].buf
		= filtbuf;

	if (baudio->mixflags & P6_M_VCF)
		bufmerge(mgbuf, baudio->contcontroller[1],
			adsrbuf, 1.0, audiomain->samplecount);

	(*baudio->sound[3]).operate(
		(audiomain->palette)[3],
		voice,
		(*baudio->sound[3]).param,
		voice->locals[voice->index][3]);
	/* Clear any voice off flags from the filter env. */
	voice->flags &= ~BRISTOL_KEYDONE;
/* FILTER DONE */

/* ADSR - ENV MOD */
	/*
	 * Run the ADSR for the amp.
	 */
	audiomain->palette[(*baudio->sound[6]).index]->specs->io[0].buf = adsrbuf;
	(*baudio->sound[6]).operate(
		(audiomain->palette)[1],
		voice,
		(*baudio->sound[6]).param,
		voice->locals[voice->index][6]);
/* ADSR - ENV - OVER */

	if (baudio->mixflags & P6_M_VCA)
		bufmerge(mgbuf, baudio->contcontroller[1] * 0.20,
			adsrbuf, 0.5, audiomain->samplecount);
	else
		bufmerge(mgbuf, baudio->contcontroller[1] * 0.0,
			adsrbuf, 0.5, audiomain->samplecount);

/* AMP */
	audiomain->palette[(*baudio->sound[5]).index]->specs->io[0].buf = filtbuf;
	audiomain->palette[(*baudio->sound[5]).index]->specs->io[1].buf = adsrbuf;
	audiomain->palette[(*baudio->sound[5]).index]->specs->io[2].buf
		= baudio->leftbuf;
	(*baudio->sound[5]).operate(
		(audiomain->palette)[2],
		voice,
		(*baudio->sound[5]).param,
		voice->locals[voice->index][5]);
/* AMP DONE */
	return(0);
}

int
static bristolPoly6Destroy(audioMain *audiomain, Baudio *baudio)
{
printf("removing one poly6\n");
	return(0);
	bristolfree(freqbuf);
	bristolfree(filtbuf);
	bristolfree(noisebuf);
	bristolfree(pmodbuf);
	bristolfree(adsrbuf);
	bristolfree(oscabuf);
	bristolfree(lfobuf);
	bristolfree(mgbuf);
	return(0);
}

int
bristolPoly6Init(audioMain *audiomain, Baudio *baudio)
{
printf("initialising one poly6\n");
	baudio->soundCount = 11; /* Number of operators in this voice */
	/*
	 * Assign an array of sound pointers.
	 */
	baudio->sound = (bristolSound **)
		bristolmalloc0(sizeof(bristolOP *) * baudio->soundCount);
	baudio->effect = (bristolSound **)
		bristolmalloc0(sizeof(bristolOP *) * baudio->soundCount);

	/* Main oscillator */
	initSoundAlgo(8, 0, baudio, audiomain, baudio->sound);
	/* Sub oscillator */
	initSoundAlgo(8, 1, baudio, audiomain, baudio->sound);
	/* LFO */
	initSoundAlgo(16, 2, baudio, audiomain, baudio->sound);
	/* A filter - 27 is the AKS filter */
	initSoundAlgo(3, 3, baudio, audiomain, baudio->sound);
	/* Filter ADSR */
	initSoundAlgo(1, 4, baudio, audiomain, baudio->sound);
	/* An amplifier */
	initSoundAlgo(2, 5, baudio, audiomain, baudio->sound);
	/* AMP ADSR */
	initSoundAlgo(1, 6, baudio, audiomain, baudio->sound);
	/* An noise source */
	initSoundAlgo(4, 7, baudio, audiomain, baudio->sound);
	/* LFO ADSR */
	initSoundAlgo(1, 8, baudio, audiomain, baudio->sound);
	/* LFO amplifier */
	initSoundAlgo(2, 9, baudio, audiomain, baudio->sound);
	/* LFO for PWM */
	initSoundAlgo(16, 10, baudio, audiomain, baudio->sound);

	baudio->param = poly6Controller;
	baudio->destroy = bristolPoly6Destroy;
	baudio->operate = operateOnePoly6;
	baudio->preops = poly6Preops;

	/*
	 * Put in a vibrachorus on our effects list.
	 */
	initSoundAlgo(12, 0, baudio, audiomain, baudio->effect);

	if (freqbuf == NULL)
		freqbuf = (float *) bristolmalloc0(audiomain->segmentsize);
	if (filtbuf == NULL)
		filtbuf = (float *) bristolmalloc0(audiomain->segmentsize);
	if (noisebuf == NULL)
		noisebuf = (float *) bristolmalloc0(audiomain->segmentsize);
	if (pmodbuf == NULL)
		pmodbuf = (float *) bristolmalloc0(audiomain->segmentsize);
	if (adsrbuf == NULL)
		adsrbuf = (float *) bristolmalloc0(audiomain->segmentsize);
	if (oscabuf == NULL)
		oscabuf = (float *) bristolmalloc0(audiomain->segmentsize);
	if (mgbuf == NULL)
		mgbuf = (float *) bristolmalloc0(audiomain->segmentsize);
	if (lfobuf == NULL)
		lfobuf = (float *) bristolmalloc0(audiomain->segmentsize);

	baudio->mixflags |= BRISTOL_STEREO;
	baudio->mixlocals = (float *) bristolmalloc0(sizeof(p6mods));
	return(0);
}

