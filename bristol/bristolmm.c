
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
#include "bristolmidi.h"

static float *freqbuf = (float *) NULL;
static float *modbuf = (float *) NULL;
static float *osc3buf = (float *) NULL;
static float *noisebuf = (float *) NULL;
static float *scratch = (float *) NULL;

extern int s440holder;

int
bristolGlobalController(Baudio *baudio, u_char controller,
u_char operator, float value)
{
#ifdef DEBUG
	printf("bristolGlobalControl(%i, %i, %f)\n", controller, operator, value);
#endif

	if (controller == 8)
		baudio->extgain = value;

	if (controller == 9)
	{
		if (baudio->glidemax == 0)
			/* Take the default glide value */
			baudio->glide = value * value * 5.0;
		else
			baudio->glide = value * value * baudio->glidemax;
	}

	if (controller == 10)
	{
		if (operator == 0) {
			baudio->gtune = 1.0
				+ (baudio->note_diff - 1)
				* (value * 2 - 1);

			buildCurrentTable(baudio, baudio->gtune);
			alterAllNotes(baudio);
		} else {
			baudio->midi_pitch = value * 24;
		}
	}

#warning fill this out with a structure for osc3 and noise mxing, etc.
	if (controller == 11)
		((mMods *) baudio->mixlocals)->mixmod = value;

	if (controller == 12)
	{
		int ivalue = value * CONTROLLER_RANGE;

		/*
		 * These are flags that control the sequencing of the audioEngine.
		 */
		switch (operator)
		{
			/*
			 * Four modifier options.
			 */
			case 0:
				if (ivalue == 0)
					baudio->mixflags &= ~OSC3_TRACKING;
				else
					baudio->mixflags |= OSC3_TRACKING;
				break;
			case 1:
				if (ivalue == 0)
					baudio->mixflags &= ~OSC_1_MOD;
				else
					baudio->mixflags |= OSC_1_MOD;
				break;
			case 2:
				if (ivalue == 0)
					baudio->mixflags &= ~FILTER_MOD;
				else
					baudio->mixflags |= FILTER_MOD;
				break;
			case 3:
				if (ivalue == 0)
					baudio->mixflags &= ~FILTER_TRACKING;
				else
					baudio->mixflags |= FILTER_TRACKING;
				break;
			case 4:
				if (ivalue == 0)
					baudio->mixflags &= ~OSC_2_MOD;
				else
					baudio->mixflags |= OSC_2_MOD;
				break;

			/*
			 * A global option - this must move to audiomain via bristolsystem()
			 */
			case 6:
				if (ivalue == 0)
					baudio->mixflags &= ~A_440;
				else {
					s440holder = 0;
					baudio->mixflags |= A_440;
				}
				break;

			/*
			 * Master enable.
			 */
			case 7:
				if (ivalue == 0)
					baudio->mixflags &= ~MASTER_ONOFF;
				else
					baudio->mixflags |= MASTER_ONOFF;
				break;

			/*
			 * Five mixer options.
			 */
			case 10:
				if (ivalue == 0)
					baudio->mixflags &= ~MIX_OSC1;
				else
					baudio->mixflags |= MIX_OSC1;
				break;
			case 11:
				if (ivalue == 0)
					baudio->mixflags &= ~MIX_OSC2;
				else
					baudio->mixflags |= MIX_OSC2;
				break;
			case 12:
				if (ivalue == 0)
					baudio->mixflags &= ~MIX_OSC3;
				else
					baudio->mixflags |= MIX_OSC3;
				break;
			case 13:
				if (ivalue == 0)
					baudio->mixflags &= ~MIX_EXT;
				else
					baudio->mixflags |= MIX_EXT;
				break;
			case 14:
				if (ivalue == 0)
					baudio->mixflags &= ~MIX_NOISE;
				else
					baudio->mixflags |= MIX_NOISE;
				break;
			case 15:
				if (ivalue == 0)
					baudio->mixflags &= ~BRISTOL_MULTITRIG;
				else
					baudio->mixflags |= BRISTOL_MULTITRIG;
				break;
			case 16:
				((mMods *) baudio->mixlocals)->mixosc3 = value;
				break;
			case 17:
				((mMods *) baudio->mixlocals)->mixnoise = value;
				break;
		}
	}

	/*
	 * Memory management controls for bristolSound memories.
	 */
	if (controller == 13)
	{
	}
	return(0);
}

int
operateOneMMVoiceFinal(audioMain *audiomain, Baudio *baudio,
bristolVoice *voice, register float *startbuf)
{
	register int samplecount = audiomain->samplecount, i;
	register float *adsrbuf, *filtbuf;

	/*
	 * Master ON/OFF is an audiomain flags, but it is typically posted to the
	 * baudio structure.
	 */
	if ((voice->baudio->mixflags & MASTER_ONOFF) == 0)
	{
#ifdef DEBUG
		if ((audiomain->debuglevel & BRISTOL_DEBUG_MASK) > BRISTOL_DEBUG5)
			printf("Mini Master OFF\n");
#endif
		return(0);
	}

	/*
	 * We need to have an external source gain change here.
	if (baudio->mixflags & MIX_EXT)
		extsig = baudio->extgain;

	for (i = 0; i < samplecount; i += 8)
	{
		*extmult++ *= extsig;
		*extmult++ *= extsig;
		*extmult++ *= extsig;
		*extmult++ *= extsig;
		*extmult++ *= extsig;
		*extmult++ *= extsig;
		*extmult++ *= extsig;
		*extmult++ *= extsig;
	}
	 */

	adsrbuf = freqbuf; /* Reusing this for different purposes */
	filtbuf = noisebuf; /* Reusing this for different purposes */

	bristolbzero(freqbuf, audiomain->segmentsize);
	bristolbzero(osc3buf, audiomain->segmentsize);
	bristolbzero(noisebuf, audiomain->segmentsize);
	bristolbzero(scratch, audiomain->segmentsize);

	/*
	 * Third oscillator. We do this first, since it may be used to modulate
	 * the first two oscillators.
	 */
	audiomain->palette[(*baudio->sound[2]).index]->specs->io[0].buf = freqbuf;
	audiomain->palette[(*baudio->sound[2]).index]->specs->io[1].buf = osc3buf;

	/*
	 * Fill the wavetable with the correct note value, which may be no kbd
	 * tracking enabled.
	 */
	if ((baudio->mixflags & OSC3_TRACKING) == 0)
	{
		register float *bufptr = freqbuf;
		/*
		 * If we do not have keyboard tracking of Osc-3, then put in a LFO
		 * frequency. This is about 0.1 Hz, although we should consider
		 * having an alternative LFO, since this method plays each sample
		 * about 400 times.
		 *
		 * Frequency of OSC-3 without keyboard control is under management of
		 * midi continuous controller 1.
		 */
		for (i = 0; i < samplecount; i+=8)
		{
			*bufptr++ = 0.001 + baudio->contcontroller[1] * 0.03125;
			*bufptr++ = 0.001 + baudio->contcontroller[1] * 0.03125;
			*bufptr++ = 0.001 + baudio->contcontroller[1] * 0.03125;
			*bufptr++ = 0.001 + baudio->contcontroller[1] * 0.03125;
			*bufptr++ = 0.001 + baudio->contcontroller[1] * 0.03125;
			*bufptr++ = 0.001 + baudio->contcontroller[1] * 0.03125;
			*bufptr++ = 0.001 + baudio->contcontroller[1] * 0.03125;
			*bufptr++ = 0.001 + baudio->contcontroller[1] * 0.03125;
		}
	} else
		fillFreqTable(baudio, voice, freqbuf, samplecount, 1);

	/*
	 * And finally operate oscillator 3.
	 */
	(*baudio->sound[2]).operate(
		(audiomain->palette)[0],
		voice,
		(*baudio->sound[2]).param,
		baudio->locals[voice->index][2]);

	/*
	 * If we are mixing the osc3 source, put it into the scratch (along with
	 * any existing ext source signal).
	 */
#warning correct the gain to bein the mix, not in the osc
	if (baudio->mixflags & MIX_OSC3)
		bufmerge(osc3buf, 1.0, scratch, 1.0, samplecount);

	/*
	 * Noise - we do this early for the same reason as oscillator 3
	 */
	audiomain->palette[(*baudio->sound[7]).index]->specs->io[0].buf = noisebuf;
	(*baudio->sound[7]).operate(
		(audiomain->palette)[4],
		voice,
		(*baudio->sound[7]).param,
		voice->locals[voice->index][7]);

	/*
	 * If we are mixing the noise source, put it into the scratch (along with
	 * any existing ext source signal).
	 */
	if (baudio->mixflags & MIX_NOISE)
		bufmerge(noisebuf, 1.0, scratch, 1.0, samplecount);

	/*
	 * Mix osc3 and noise - these are both in the scratch if we have
	 * them flagged to be mixed.
#warning convert noise mod to be S&H via OSC3
	bufmerge(noisebuf,
		((mMods *) baudio->mixlocals)->mixmod * baudio->contcontroller[1],
		modbuf, 0.0, samplecount);
	 */
	for (i = 0; i < samplecount; i+=8)
	{
		if ((((mMods *) baudio->mixlocals)->lsample < 0) && (osc3buf[i] >= 0))
			((mMods *) baudio->mixlocals)->csample = noisebuf[i];

		((mMods *) baudio->mixlocals)->lsample = osc3buf[i];

		modbuf[i] = ((mMods *) baudio->mixlocals)->csample
			* ((mMods *) baudio->mixlocals)->mixmod;
		modbuf[i+1] = ((mMods *) baudio->mixlocals)->csample
			* ((mMods *) baudio->mixlocals)->mixmod;
		modbuf[i+2] = ((mMods *) baudio->mixlocals)->csample
			* ((mMods *) baudio->mixlocals)->mixmod;
		modbuf[i+3] = ((mMods *) baudio->mixlocals)->csample
			* ((mMods *) baudio->mixlocals)->mixmod;
		modbuf[i+4] = ((mMods *) baudio->mixlocals)->csample
			* ((mMods *) baudio->mixlocals)->mixmod;
		modbuf[i+5] = ((mMods *) baudio->mixlocals)->csample
			* ((mMods *) baudio->mixlocals)->mixmod;
		modbuf[i+6] = ((mMods *) baudio->mixlocals)->csample
			* ((mMods *) baudio->mixlocals)->mixmod;
		modbuf[i+7] = ((mMods *) baudio->mixlocals)->csample
			* ((mMods *) baudio->mixlocals)->mixmod;
	}

	bufmerge(osc3buf,
		(1 - ((mMods *) baudio->mixlocals)->mixmod) * baudio->contcontroller[1],
		modbuf, baudio->contcontroller[1], samplecount);

	/*
	 * We need to run through every bristolSound on the baudio sound chain.
	 * We need to pass the correct set of parameters to each operator, and
	 * ensure they get the correct local variable set.
	 *
	 * First insert our buffer pointers, we do this by inserting the buffers
	 * into the existing opspec structures.
	 */
	fillFreqTable(baudio, voice, freqbuf, samplecount, 1);
	if (baudio->mixflags & MIX_OSC1)
	{
		audiomain->palette[(*baudio->sound[0]).index]->specs->io[0].buf
			= freqbuf;
		audiomain->palette[(*baudio->sound[0]).index]->specs->io[1].buf
			= scratch;
		/*
		 * Fill tmpbuf1 with our frequency information
		 */
		/*
		 * If we have any mods on the oscillators, we need to put them in here.
		 * This should be under the control of polypressure and/or
		 * channelpressure?
		 */
		if (baudio->mixflags & OSC_1_MOD)
			bufmerge(modbuf,
				0.5 + (voice->press + voice->chanpressure),
				freqbuf, 1.0, samplecount);

		(*baudio->sound[0]).operate(
			(audiomain->palette)[0],
			voice,
			(*baudio->sound[0]).param,
			voice->locals[voice->index][0]);
	}

	/*
	 * Second oscillator
	 */
	if (baudio->mixflags & MIX_OSC2)
	{
		audiomain->palette[(*baudio->sound[1]).index]->specs->io[0].buf
			= freqbuf;
		audiomain->palette[(*baudio->sound[1]).index]->specs->io[1].buf
			= scratch;
		/*
		 * Fill tmpbuf1 with our frequency information - should already have it
		 * done, including mods?
		 */
		fillFreqTable(baudio, voice, freqbuf, samplecount, 1);
		/*
		 * Put in any mods we want.
		 */
		if (baudio->mixflags & OSC_2_MOD)
			bufmerge(modbuf,
				0.5 + (voice->press + voice->chanpressure),
				freqbuf, 1.0, samplecount);

		/*
		 * And finally operate this oscillator
		 */
		(*baudio->sound[1]).operate(
			(audiomain->palette)[0],
			voice,
			(*baudio->sound[1]).param,
			voice->locals[voice->index][1]);
	}

	if ((baudio->mixflags & BRISTOL_MULTITRIG) == 0)
		voice->flags &= ~BRISTOL_KEYREON;

	/*
	 * Run the ADSR for the filter, reusing the scratch.
	 */
	audiomain->palette[(*baudio->sound[3]).index]->specs->io[0].buf = adsrbuf;
	(*baudio->sound[3]).operate(
		(audiomain->palette)[1],
		voice,
		(*baudio->sound[3]).param,
		voice->locals[voice->index][3]);

	/* 
	 * If we have mods on the filter, apply them here.
	 */
	if (baudio->mixflags & FILTER_MOD)
	{
		/*
		 * Rather than just merge, we are going to adjust osc3 to be DC
		 * to reduce clipping the output signal.
			bufmerge(osc3buf, 0.01, adsrbuf, 1.0, samplecount);
		for (i = 0; i < samplecount; i+=8)
		{
			*bufptr++ += 0.05 * (*osc3b++ + 12);
			*bufptr++ += 0.05 * (*osc3b++ + 12);
			*bufptr++ += 0.05 * (*osc3b++ + 12);
			*bufptr++ += 0.05 * (*osc3b++ + 12);
			*bufptr++ += 0.05 * (*osc3b++ + 12);
			*bufptr++ += 0.05 * (*osc3b++ + 12);
			*bufptr++ += 0.05 * (*osc3b++ + 12);
			*bufptr++ += 0.05 * (*osc3b++ + 12);
		}
		 */
		bufmerge(modbuf, 0.5, adsrbuf, 1.0, samplecount);
	}

	if (baudio->mixflags & MIX_EXT)
		bufmerge(startbuf, baudio->extgain * 2, scratch, 1.0, samplecount);
	bufmerge(scratch, 256.0, scratch, 1.0, samplecount);

	/*
	 * Run the mixed oscillators into the filter. Input and output buffers
	 * are the same.
	 */
	bristolbzero(filtbuf, audiomain->segmentsize);
	audiomain->palette[(*baudio->sound[4]).index]->specs->io[0].buf = scratch;
	audiomain->palette[(*baudio->sound[4]).index]->specs->io[1].buf = adsrbuf;
	audiomain->palette[(*baudio->sound[4]).index]->specs->io[2].buf = filtbuf;
	(*baudio->sound[4]).operate(
		(audiomain->palette)[3],
		voice,
		(*baudio->sound[4]).param,
		voice->locals[voice->index][4]);

	/*
	 * Run the ADSR for the final stage amplifier, reusing the scratch.
	 */
	audiomain->palette[(*baudio->sound[5]).index]->specs->io[0].buf = adsrbuf;
	(*baudio->sound[5]).operate(
		(audiomain->palette)[1],
		voice,
		(*baudio->sound[5]).param,
		voice->locals[voice->index][5]);

	/*
	 * Run the mixed oscillators into the amplifier. Input and output buffers
	 * are the same.
	 */
	audiomain->palette[(*baudio->sound[6]).index]->specs->io[0].buf = filtbuf;
	audiomain->palette[(*baudio->sound[6]).index]->specs->io[1].buf = adsrbuf;
	audiomain->palette[(*baudio->sound[6]).index]->specs->io[2].buf =
		baudio->leftbuf;

	(*baudio->sound[6]).operate(
		(audiomain->palette)[2],
		voice,
		(*baudio->sound[6]).param,
		voice->locals[voice->index][6]);
	return(0);
}

static int
destroyOneMMVoice(audioMain *audiomain, Baudio *baudio)
{
printf("removing one mini\n");

	/*
	 * We need to leave these, we may have multiple invocations running
	 */
	return(0);

	if (freqbuf != NULL)
		bristolfree(freqbuf);
	if (modbuf != NULL)
		bristolfree(modbuf);
	if (osc3buf != NULL)
		bristolfree(osc3buf);
	if (noisebuf != NULL)
		bristolfree(noisebuf);
	if (scratch != NULL)
		bristolfree(scratch);
	freqbuf = NULL;
	modbuf = NULL;
	osc3buf = NULL;
	noisebuf = NULL;
	scratch = NULL;
	return(0);
}

int
bristolMMInit(audioMain *audiomain, Baudio *baudio)
{
printf("initialising one bristolmini\n");
	baudio->soundCount = 8; /* Number of operators in this voice (MM) */

	/*
	 * Assign an array of sound pointers.
	 */
	baudio->sound = (bristolSound **)
		bristolmalloc0(sizeof(bristolOP *) * baudio->soundCount);
	baudio->effect = (bristolSound **)
		bristolmalloc0(sizeof(bristolOP *) * baudio->soundCount);

	initSoundAlgo(0, 0, baudio, audiomain, baudio->sound);
	initSoundAlgo(0, 1, baudio, audiomain, baudio->sound);
	initSoundAlgo(0, 2, baudio, audiomain, baudio->sound);
	/* An ADSR */
	initSoundAlgo(1, 3, baudio, audiomain, baudio->sound);
	/* A filter */
	initSoundAlgo(3, 4, baudio, audiomain, baudio->sound);
	/* Another ADSR */
	initSoundAlgo(1, 5, baudio, audiomain, baudio->sound);
	/* An amplifier */
	initSoundAlgo(2, 6, baudio, audiomain, baudio->sound);
	/* An noise source */
	initSoundAlgo(4, 7, baudio, audiomain, baudio->sound);

	baudio->param = bristolGlobalController;
	baudio->destroy = destroyOneMMVoice;
	baudio->operate = operateOneMMVoiceFinal;

	if (freqbuf == (float *) NULL)
		freqbuf = (float *) bristolmalloc0(audiomain->segmentsize);
	if (modbuf == (float *) NULL)
		modbuf = (float *) bristolmalloc0(audiomain->segmentsize);
	if (osc3buf == (float *) NULL)
		osc3buf = (float *) bristolmalloc0(audiomain->segmentsize);
	if (noisebuf == (float *) NULL)
		noisebuf = (float *) bristolmalloc0(audiomain->segmentsize);
	if (scratch == (float *) NULL)
		scratch = (float *) bristolmalloc0(audiomain->segmentsize);

//	baudio->mixlocals = bristolmalloc0(sizeof(float));
	baudio->mixlocals = (float *) bristolmalloc0(sizeof(mMods));

	return(0);
}

