
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

/* REALISTIC/Moog MG1 Concertmate algorithm */

/*#define DEBUG */

#include "bristol.h"
#include "bristolmm.h"
#include "bristolmg1.h"

/*
 * Use of these mg1 global buffers will be an issue with use of multiple
 * audio threads, unless we ensure a single thread deals with any given algo
 * type, since then they are only used sequentially.
 */
static float *freqbuf = (float *) NULL;
static float *pfreqbuf = (float *) NULL;
static float *filtbuf = (float *) NULL;
static float *noisebuf = (float *) NULL;
static float *padsrbuf = (float *) NULL;
static float *adsrbuf = (float *) NULL;
static float *lfosh = (float *) NULL;
static float *lforamp = (float *) NULL;
static float *lfosquare = (float *) NULL;
static float *osc1buf = (float *) NULL;
static float *osc2buf = (float *) NULL;
static float *oscpbuf = (float *) NULL;
static float *scratch = (float *) NULL;
static float *zerobuf = (float *) NULL;

static float *modbuf;

int
mg1Controller(Baudio *baudio, u_char operator, u_char controller, float value)
{
	int ivalue = value * CONTROLLER_RANGE;

#ifdef DEBUG
	printf("bristolMg1Control(%i, %i, %f)\n", operator, controller, value);
#endif

	if (operator != 126)
		return(0);

	switch (controller) {
		case 0:
			baudio->glide = value * value * baudio->glidemax;
			break;
		case 1:
			/*
			 * We should not use this, there are separate tuning for the
			 * Mono and Poly sections.
			 */
			baudio->gtune = 1.0
				+ (baudio->note_diff - 1)
				* (value * 2 - 1);

			buildCurrentTable(baudio, baudio->gtune);
			alterAllNotes(baudio);
			break;
		case 2:
			/* Osc-1 gain */
			((mg1mods *) baudio->mixlocals)->osc1gain = value * 64;
			break;
		case 3:
			/* Osc-2 gain */
			((mg1mods *) baudio->mixlocals)->osc2gain = value * 64;
			break;
		case 4:
			/* Tone mod */
			((mg1mods *) baudio->mixlocals)->tonemod = value * value;
			break;
		case 5:
			/* Filter mod */
			((mg1mods *) baudio->mixlocals)->filtermod = value * value;
			break;
		case 6:
			/* LFO Select */
			((mg1mods *) baudio->mixlocals)->lfoselect = ivalue;
			break;
		case 7:
			/* Noise gain */
			((mg1mods *) baudio->mixlocals)->noisegain = value * 32;
			break;
		case 8:
			/* Master volume */
			((mg1mods *) baudio->mixlocals)->master = value;
			break;
		case 9:
			/* Trigger */
			baudio->mixflags &= ~M_TRIG;
			if (value == 0)
				baudio->mixflags |= M_TRIG;
			break;
		case 10:
			/* Gate or env */
printf("gate is %i\n", ivalue);
			baudio->mixflags &= ~M_GATE;
			if (ivalue == 1)
				baudio->mixflags |= M_GATE;
			((mg1mods *) baudio->mixlocals)->gated = ivalue;
			break;
		case 11:
			/* Filter envelope modulation amount */
			((mg1mods *) baudio->mixlocals)->filterenv = value;
			break;
	}
	return(0);
}

int
mg1Preops(audioMain *audiomain, Baudio *baudio,
bristolVoice *voice, register float *startbuf)
{
	((mg1mods *) baudio->mixlocals)->vprocessed = 0;

	/* Ring mod */
	audiomain->palette[(*baudio->sound[9]).index]->specs->io[0].buf = osc1buf;
	audiomain->palette[(*baudio->sound[9]).index]->specs->io[1].buf = osc2buf;
	audiomain->palette[(*baudio->sound[9]).index]->specs->io[2].buf = scratch;
	(*baudio->sound[9]).operate(
		(audiomain->palette)[20],
		voice,
		(*baudio->sound[9]).param,
		voice->locals[voice->index][9]);
	bristolbzero(osc1buf, audiomain->segmentsize);
	bristolbzero(osc2buf, audiomain->segmentsize);

	if (((mg1mods *) baudio->mixlocals)->lfolocals == 0)
	{
		((mg1mods *) baudio->mixlocals)->lfolocals =
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

/* LFO produces 3 outputs, tri, square and S/H */
	/*
	 * Operate LFO if we have one per voice.
	 */
	audiomain->palette[(*baudio->sound[2]).index]->specs->io[0].buf
		= noisebuf;
	audiomain->palette[(*baudio->sound[2]).index]->specs->io[1].buf
		= lforamp;
	audiomain->palette[(*baudio->sound[2]).index]->specs->io[2].buf
		= lfosquare;
	audiomain->palette[(*baudio->sound[2]).index]->specs->io[3].buf
		= lfosh;
	(*baudio->sound[2]).operate(
		(audiomain->palette)[16],
		voice,
		(*baudio->sound[2]).param,
		((mg1mods *) baudio->mixlocals)->lfolocals);

	switch (((mg1mods *) baudio->mixlocals)->lfoselect) {
		case 1:
			modbuf = lfosquare;
			break;
		case 2:
		default:
			modbuf = lforamp;
			break;
		case 0:
			modbuf = lfosh;
			break;
	}

	return(0);
}

/*
 * This is only going to be the poly organ circuits.
 */
int
operateOneMg1(audioMain *audiomain, Baudio *baudio,
bristolVoice *voice, register float *startbuf)
{
	int sc = audiomain->samplecount;

	((mg1mods *) baudio->mixlocals)->vprocessed++;

	bristolbzero(freqbuf, audiomain->segmentsize);
	bristolbzero(filtbuf, audiomain->segmentsize);
	bristolbzero(oscpbuf, audiomain->segmentsize);

/* ADSR */
	/*
	 * Run the polyphonic grooming ADSR.
	 */
	audiomain->palette[(*baudio->sound[8]).index]->specs->io[0].buf = padsrbuf;
	(*baudio->sound[8]).operate(
		(audiomain->palette)[1],
		voice,
		(*baudio->sound[8]).param,
		voice->locals[voice->index][8]);
/* ADSR - OVER */

	/*
	 * Run the oscillator.
	 */
/* OSC */
	audiomain->palette[(*baudio->sound[7]).index]->specs->io[0].buf = freqbuf;
	audiomain->palette[(*baudio->sound[7]).index]->specs->io[1].buf = zerobuf;
	audiomain->palette[(*baudio->sound[7]).index]->specs->io[2].buf = oscpbuf;

	/* Fill the frequency table without glide */
	fillFreqTable(baudio, voice, freqbuf, sc, 0);

	(*baudio->sound[7]).operate(
		(audiomain->palette)[8],
		voice,
		(*baudio->sound[7]).param,
		voice->locals[voice->index][7]);
/* OSC done */

	bufmerge(zerobuf, 0.0, oscpbuf, 12.0, audiomain->samplecount);

/* AMP */
	audiomain->palette[(*baudio->sound[5]).index]->specs->io[0].buf
		= oscpbuf;
	audiomain->palette[(*baudio->sound[5]).index]->specs->io[1].buf
		= padsrbuf;
	audiomain->palette[(*baudio->sound[5]).index]->specs->io[2].buf
		= scratch;

	(*baudio->sound[5]).operate(
		(audiomain->palette)[2],
		voice,
		(*baudio->sound[5]).param,
		voice->locals[voice->index][5]);
/* AMP OVER */

	return(0);
}

int
mg1Postops(audioMain *audiomain, Baudio *baudio,
bristolVoice *voice, register float *startbuf)
{
	int kflags = 0;
	mg1mods *mods = (mg1mods *) baudio->mixlocals;
	/* Scratch is the poly section */

	/* Fill the frequency table with glide */
	fillFreqTable(baudio, voice, freqbuf, audiomain->samplecount, 1);
	/* Tone mods */
	bufmerge(modbuf, mods->tonemod, freqbuf, 1.0, audiomain->samplecount);

	/*
	 * We have our mods and all that, now I want to run one of the mono
	 * stuff.
	 */
	audiomain->palette[(*baudio->sound[0]).index]->specs->io[0].buf = freqbuf;
	audiomain->palette[(*baudio->sound[0]).index]->specs->io[1].buf = zerobuf;
	audiomain->palette[(*baudio->sound[0]).index]->specs->io[2].buf = osc1buf;
	audiomain->palette[(*baudio->sound[0]).index]->specs->io[3].buf = osc1buf;

	(*baudio->sound[0]).operate(
		(audiomain->palette)[8],
		voice,
		(*baudio->sound[0]).param,
		voice->locals[voice->index][0]);

	audiomain->palette[(*baudio->sound[0]).index]->specs->io[2].buf = osc2buf;

	(*baudio->sound[1]).operate(
		(audiomain->palette)[8],
		voice,
		(*baudio->sound[1]).param,
		voice->locals[voice->index][1]);

/* ADSR */
	/*
	 * See if we should recognise key off events.
	 */
	if (mods->gated == 0)
		voice->flags &= ~(BRISTOL_KEYOFF|BRISTOL_KEYOFFING);

	if ((mods->arAutoTransit < 0.0) && (*lfosquare > 0) &&
		((voice->flags & (BRISTOL_KEYOFF|BRISTOL_KEYOFFING)) == 0))
			kflags |= BRISTOL_KEYREON;
	else if ((mods->arAutoTransit > 0.0) && (*lfosquare < 0) &&
		((voice->flags & (BRISTOL_KEYOFF|BRISTOL_KEYOFFING)) == 0))
			kflags |= BRISTOL_KEYOFF;

	mods->arAutoTransit  = *lfosquare;

	if ((baudio->mixflags & M_TRIG) != 0)
		voice->flags |= kflags;
	/*
	 * At this point we have the mix done. Now pump it through the filter and
	 * envelope.
	 */
	audiomain->palette[(*baudio->sound[4]).index]->specs->io[0].buf = padsrbuf;
	(*baudio->sound[4]).operate(
		(audiomain->palette)[1],
		voice,
		(*baudio->sound[4]).param,
		voice->locals[voice->index][4]);

	/*
	 * If we are using trigger then we do not want this voice to go off
	 */
	if (baudio->mixflags & M_TRIG)
		voice->flags &= ~(BRISTOL_KEYOFF|BRISTOL_KEYDONE);
/* ADSR - OVER */

	/* 
	 * We first need to see if we have the oscillators enveloped or gated.
	 */
	bufmerge(osc1buf, mods->osc1gain, scratch, 1.0, audiomain->samplecount);
	bufmerge(osc2buf, mods->osc2gain, scratch, 1.0, audiomain->samplecount);
	bufmerge(noisebuf, mods->noisegain, scratch, 1.0, audiomain->samplecount);

/* Filter */
	/*
	 * We now have ringmod, poly, osc1+2, noise in the scratch buffer. Filter
	 * it.
	 */
	bufmerge(padsrbuf, mods->filterenv,
		modbuf, mods->filtermod, audiomain->samplecount);
	audiomain->palette[(*baudio->sound[3]).index]->specs->io[0].buf
		= scratch;
	audiomain->palette[(*baudio->sound[3]).index]->specs->io[1].buf
		= modbuf;
	audiomain->palette[(*baudio->sound[3]).index]->specs->io[2].buf
		= filtbuf;

	(*baudio->sound[3]).operate(
		(audiomain->palette)[3],
		voice,
		(*baudio->sound[3]).param,
		voice->locals[voice->index][3]);
/* Filter DONE */

/* Mono amp */
	if ((baudio->mixflags & M_GATE) == 0) {
		audiomain->palette[(*baudio->sound[5]).index]->specs->io[0].buf
			= filtbuf;
		audiomain->palette[(*baudio->sound[5]).index]->specs->io[1].buf
			= padsrbuf;
		audiomain->palette[(*baudio->sound[5]).index]->specs->io[2].buf
			= baudio->leftbuf;

		(*baudio->sound[5]).operate(
			(audiomain->palette)[2],
			voice,
			(*baudio->sound[5]).param,
			voice->locals[voice->index][5]);
	} else {
		bufmerge(filtbuf, 12.0, baudio->leftbuf, 1.0, audiomain->samplecount);
	}
/* AMP OVER */

	bufmerge(zerobuf, 0.0,
		baudio->leftbuf, mods->master,
		audiomain->samplecount);

	return(0);
}

static int
bristolMg1Destroy(audioMain *audiomain, Baudio *baudio)
{
printf("removing one mg1\n");
	return(0);
	bristolfree(freqbuf);
	bristolfree(filtbuf);
	bristolfree(noisebuf);
	bristolfree(adsrbuf);
	bristolfree(lforamp);
	bristolfree(lfosquare);
	bristolfree(lfosh);
	bristolfree(osc1buf);
	bristolfree(osc1buf);

	return(0);
}

int
bristolMg1Init(audioMain *audiomain, Baudio *baudio)
{
printf("initialising one mg1: %i\n", baudio->voicecount);
	/*
	 * The poly section is going to consist of one oscillator, probably the
	 * arpdcp, and an envelope. The envelope will simply be used for note
	 * grooming, fairly sharp attach and decay so that clicks are removed if
	 * a note is played when the mono envelope is fully open. It will be 
	 * operated in the poly operate() code.
	 *
	 * The mono section will consist of an LFO and Noise source modulators,
	 * two oscillators (these have to sync), an envelope, filter, RM. These
	 * will operate partially in the preops() and partially in the postops()
	 * and as such will only operate once per cycle to be monophonic.
	 *
	 * 2 + 2 + 2 + 2 + 1 + 1.
	 */
	baudio->soundCount = 10; /* Number of operators in this voice */

	/*
	 * Assign an array of sound pointers.
	 */
	baudio->sound = (bristolSound **)
		bristolmalloc0(sizeof(bristolOP *) * baudio->soundCount);
	baudio->effect = (bristolSound **)
		bristolmalloc0(sizeof(bristolOP *) * baudio->soundCount);

	/* oscillator 1 */
	initSoundAlgo(8,  0, baudio, audiomain, baudio->sound);
	/* oscillator 2 */
	initSoundAlgo(8,  1, baudio, audiomain, baudio->sound);
	/* LFO */
	initSoundAlgo(16, 2, baudio, audiomain, baudio->sound);
	/* A filter */
	initSoundAlgo(3,  3, baudio, audiomain, baudio->sound);
	/* Single ADSR */
	initSoundAlgo(1,  4, baudio, audiomain, baudio->sound);
	/* An amplifier */
	initSoundAlgo(2,  5, baudio, audiomain, baudio->sound);
	/* An noise source */
	initSoundAlgo(4,  6, baudio, audiomain, baudio->sound);
	/* Third osc for poly operation */
	initSoundAlgo(8,  7, baudio, audiomain, baudio->sound);
	/* Second ADSR for note grooming */
	initSoundAlgo(1,  8, baudio, audiomain, baudio->sound);
	/* Ring Modulator */
	initSoundAlgo(20, 9, baudio, audiomain, baudio->sound);

	baudio->param = mg1Controller;
	baudio->destroy = bristolMg1Destroy;
	baudio->operate = operateOneMg1;
	baudio->preops = mg1Preops;
	baudio->postops = mg1Postops;

	if (pfreqbuf == NULL)
		pfreqbuf = (float *) bristolmalloc0(audiomain->segmentsize);
	if (freqbuf == NULL)
		freqbuf = (float *) bristolmalloc0(audiomain->segmentsize);
	if (filtbuf == NULL)
		filtbuf = (float *) bristolmalloc0(audiomain->segmentsize);
	if (noisebuf == NULL)
		noisebuf = (float *) bristolmalloc0(audiomain->segmentsize);
	if (padsrbuf == NULL)
		padsrbuf = (float *) bristolmalloc0(audiomain->segmentsize);
	if (adsrbuf == NULL)
		adsrbuf = (float *) bristolmalloc0(audiomain->segmentsize);
	if (lfosh == NULL)
		lfosh = (float *) bristolmalloc0(audiomain->segmentsize);
	if (lfosquare == NULL)
		lfosquare = (float *) bristolmalloc0(audiomain->segmentsize);
	if (lforamp == NULL)
		lforamp = (float *) bristolmalloc0(audiomain->segmentsize);
	if (osc1buf == NULL)
		osc1buf = (float *) bristolmalloc0(audiomain->segmentsize);
	if (osc2buf == NULL)
		osc2buf = (float *) bristolmalloc0(audiomain->segmentsize);
	if (oscpbuf == NULL)
		oscpbuf = (float *) bristolmalloc0(audiomain->segmentsize);
	if (scratch == NULL)
		scratch = (float *) bristolmalloc0(audiomain->segmentsize);
	if (zerobuf == NULL)
		zerobuf = (float *) bristolmalloc0(audiomain->segmentsize);

	baudio->mixlocals = (float *) bristolmalloc0(sizeof(mg1mods));

	return(0);
}

