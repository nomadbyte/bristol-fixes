
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
#include "bristolprophet1.h"

/*
 * Use of these pro1 global buffers will be an issue with use of multiple
 * audio threads, unless we ensure a single thread deals with any given algo
 * type, since then they are only used sequencially.
 *
 * These should really be hidden in the pMods structure for multiple instances.
 */
static float *freqbuf = (float *) NULL;
static float *tfreqbuf = (float *) NULL;
static float *lfobuf = (float *) NULL;
static float *wmodbuf = (float *) NULL;
static float *dmodbuf = (float *) NULL;
static float *adsrbuf = (float *) NULL;
static float *filtbuf = (float *) NULL;
static float *noisebuf = (float *) NULL;
static float *oscbbuf = (float *) NULL;
static float *oscabuf = (float *) NULL;
static float *scratchbuf = (float *) NULL;

static float *tribuf = (float *) NULL;
static float *sqrbuf = (float *) NULL;
static float *rampbuf = (float *) NULL;

extern int s440holder;

int
pro1Controller(Baudio *baudio, u_char operator, u_char controller,
float value)
{
	int ivalue = value * CONTROLLER_RANGE;

#ifdef DEBUG
	printf("bristolPro1Control(%i, %i, %f) %i\n", operator, controller, value,
		ivalue);
#endif

	if (operator != 126)
		return(0);

	switch (controller) {
		case 0:
			baudio->glide = value * value * baudio->glidemax;
			break;
		case 2:
			baudio->gtune = 1.0
				+ (baudio->note_diff - 1)
				* (value * 2 - 1);

			buildCurrentTable(baudio, baudio->gtune);
			alterAllNotes(baudio);
			break;
		case 3:
			if (ivalue == 0)
				baudio->mixflags &= ~P1_FE_WHEEL;
			else
				baudio->mixflags |= P1_FE_WHEEL;
			break;
		case 4:
			((pmods *) baudio->mixlocals)->oscbmod = value;
			break;
		case 5:
			if (ivalue == 0)
				baudio->mixflags &= ~P1_OSCB_WHEEL;
			else
				baudio->mixflags |= P1_OSCB_WHEEL;
			break;
		case 6:
			((pmods *) baudio->mixlocals)->lfomod = value;
			break;
		case 7:
			if (ivalue == 0)
				baudio->mixflags &= ~P1_LFO_WHEEL;
			else
				baudio->mixflags |= P1_LFO_WHEEL;
			break;
		case 8:
			if (ivalue == 0)
				baudio->mixflags &= ~BRISTOL_GLIDE_AUTO;
			else
				baudio->mixflags |= BRISTOL_GLIDE_AUTO;
			break;
		case 9:
			if (ivalue == 0)
				baudio->mixflags &= ~P1_REPEAT;
			else
				baudio->mixflags |= P1_REPEAT;
			break;

		case 10:
			baudio->mixflags &= ~(P1_OSCA_F_WM|P1_OSCA_F_DM);
			if (ivalue == 2)
				baudio->mixflags |= P1_OSCA_F_WM;
			else if (ivalue == 0)
				baudio->mixflags |= P1_OSCA_F_DM;
			break;
		case 11:
			baudio->mixflags &= ~(P1_OSCA_PW_WM|P1_OSCA_PW_DM);
			if (ivalue == 2)
				baudio->mixflags |= P1_OSCA_PW_WM;
			else if (ivalue == 0)
				baudio->mixflags |= P1_OSCA_PW_DM;
			break;
		case 12:
			baudio->mixflags &= ~(P1_OSCB_F_WM|P1_OSCB_F_DM);
			if (ivalue == 2)
				baudio->mixflags |= P1_OSCB_F_WM;
			else if (ivalue == 0)
				baudio->mixflags |= P1_OSCB_F_DM;
			break;
		case 13:
			baudio->mixflags &= ~(P1_OSCB_PW_WM|P1_OSCB_PW_DM);
			if (ivalue == 2)
				baudio->mixflags |= P1_OSCB_PW_WM;
			else if (ivalue == 0)
				baudio->mixflags |= P1_OSCB_PW_DM;
			break;
		case 14:
			baudio->mixflags &= ~(P1_FILT_WM|P1_FILT_DM);
			if (ivalue == 2)
				baudio->mixflags |= P1_FILT_WM;
			else if (ivalue == 0)
				baudio->mixflags |= P1_FILT_DM;
			break;

		case 18:
			if (ivalue == 0)
				baudio->mixflags &= ~P1_LFO_B;
			else
				baudio->mixflags |= P1_LFO_B;
			break;
		case 19:
			if (ivalue == 0)
				baudio->mixflags &= ~P1_KBD_B;
			else
				baudio->mixflags |= P1_KBD_B;
			break;

		case 20:
			((pmods *) baudio->mixlocals)->mix_a = value * 32;
			break;
		case 21:
			((pmods *) baudio->mixlocals)->mix_b = value * 32;
			break;
		case 22:
			((pmods *) baudio->mixlocals)->mix_n = value * 32;
			break;
		case 23:
			((pmods *) baudio->mixlocals)->f_envlevel = value;
			break;
		case 24:
			if (ivalue == 0)
				baudio->mixflags &= ~P1_LFO_RAMP;
			else
				baudio->mixflags |= P1_LFO_RAMP;
			break;
		case 25:
			if (ivalue == 0)
				baudio->mixflags &= ~P1_LFO_TRI;
			else
				baudio->mixflags |= P1_LFO_TRI;
			break;
		case 26:
			if (ivalue == 0)
				baudio->mixflags &= ~P1_LFO_SQR;
			else
				baudio->mixflags |= P1_LFO_SQR;
			break;
		case 27:
			if (ivalue == 0)
				baudio->mixflags &= ~BRISTOL_MULTITRIG;
			else
				baudio->mixflags |= BRISTOL_MULTITRIG;
		case 28:
			if (ivalue == 0)
				baudio->mixflags &= ~P1_DRONE;
			else
				baudio->mixflags |= P1_DRONE;
			break;
	}
	return(0);
}

/*
 * Preops will do noise, and one oscillator - the LFO.
 */
int
operatePro1Preops(audioMain *audiomain, Baudio *baudio,
bristolVoice *voice, register float *startbuf)
{
	register int samplecount = audiomain->samplecount;

#ifdef DEBUG
	if ((audiomain->debuglevel & BRISTOL_DEBUG_MASK) > BRISTOL_DEBUG5)
		printf("operatePro1Preops(%x, %x, %x) %i\n",
			baudio, voice, startbuf, baudio->cvoices);
#endif

	bristolbzero(lfobuf, audiomain->segmentsize);
	bristolbzero(noisebuf, audiomain->segmentsize);
	bristolbzero(wmodbuf, audiomain->segmentsize);

	/*
	 * Third oscillator. We do this first, since it may be used to modulate
	 * the first two oscillators. No mods on LFO - wmodbuf is zeros at moment.
	 */
	audiomain->palette[(*baudio->sound[2]).index]->specs->io[0].buf = NULL;
	audiomain->palette[(*baudio->sound[2]).index]->specs->io[1].buf = tribuf;
	audiomain->palette[(*baudio->sound[2]).index]->specs->io[2].buf = sqrbuf;
	audiomain->palette[(*baudio->sound[2]).index]->specs->io[3].buf = NULL;
	audiomain->palette[(*baudio->sound[2]).index]->specs->io[4].buf = lfobuf;
	audiomain->palette[(*baudio->sound[2]).index]->specs->io[5].buf = rampbuf;

	/*
	 * Operate oscillator 3. LFO...
	 */
	(*baudio->sound[2]).operate(
		(audiomain->palette)[16],
		voice,
		(*baudio->sound[2]).param,
		baudio->locals[voice->index][2]);

	/* Now mix according to the LFO flags: */
	if (baudio->mixflags & (P1_LFO_SQR|P1_LFO_RAMP|P1_LFO_TRI))
	{
		bristolbzero(lfobuf, audiomain->segmentsize);
		if (baudio->mixflags & P1_LFO_TRI)
			bufmerge(tribuf, 12.0, lfobuf, 1.0, samplecount);
		if (baudio->mixflags & P1_LFO_SQR)
			bufmerge(sqrbuf, 12.0, lfobuf, 1.0, samplecount);
		if (baudio->mixflags & P1_LFO_RAMP)
			bufmerge(rampbuf, 12.0, lfobuf, 1.0, samplecount);
	} else
		/*
		 * If there were no flags then put a bit more gain into the LFO buffer
		 * which contains a sine wave.
		 */
		bufmerge(tribuf, 0.0, lfobuf, 12.0, samplecount);
/* LFO OVER */

	return(0);
}

int
operateOnePro1(audioMain *audiomain, Baudio *baudio,
bristolVoice *voice, register float *startbuf)
{
	register int samplecount = audiomain->samplecount, i;
	register float *bufptr;

	/*
	 * We need to run through every bristolSound on the baudio sound chain.
	 * We need to pass the correct set of parameters to each operator, and
	 * ensure they get the correct local variable set.
	 */
	bufptr = freqbuf;

	bristolbzero(freqbuf, audiomain->segmentsize);
	bristolbzero(dmodbuf, audiomain->segmentsize);
	bristolbzero(wmodbuf, audiomain->segmentsize);
	bristolbzero(adsrbuf, audiomain->segmentsize);
	bristolbzero(filtbuf, audiomain->segmentsize);
	bristolbzero(oscabuf, audiomain->segmentsize);

/* NOISE - This should go in a poly call.... */
	/*
	 * Noise - we do this early for the same reason as oscillator 3
	 */
	audiomain->palette[(*baudio->sound[7]).index]->specs->io[0].buf = noisebuf;
	(*baudio->sound[7]).operate(
		(audiomain->palette)[4],
		voice,
		(*baudio->sound[7]).param,
		voice->locals[voice->index][7]);
/* NOISE OVER */

	if ((baudio->mixflags & BRISTOL_MULTITRIG) == 0)
		voice->flags &= ~BRISTOL_KEYREON;
	if (baudio->mixflags & P1_DRONE)
		voice->flags &= ~BRISTOL_KEYOFF;

/* ADSR - POLY MOD */
	/*
	 * Run the ADSR for the filter, reusing the startbuf. We need this for the
	 * mods routing.
	 */
	audiomain->palette[(*baudio->sound[3]).index]->specs->io[0].buf = adsrbuf;
	(*baudio->sound[3]).operate(
		(audiomain->palette)[1],
		voice,
		(*baudio->sound[3]).param,
		voice->locals[voice->index][3]);
/* ADSR - POLY MOD - OVER */

/* Mod bus routing */
	/*
	 * We have two mod busses, one is direct and the other is controlled by
	 * the wheel mod. We need to add flags before this can be done here but
	 * we can just fill the buffers for now (testing):
	 */
	if (baudio->mixflags & P1_FE_WHEEL)
		bufmerge(adsrbuf, baudio->contcontroller[1], wmodbuf, 0.0, samplecount);
	else
		bufmerge(adsrbuf, 1.0, dmodbuf, 0.0, samplecount);

	if (baudio->mixflags & P1_LFO_WHEEL)
		bufmerge(lfobuf,
			((pmods *) baudio->mixlocals)->lfomod * baudio->contcontroller[1],
			wmodbuf, 1.0, samplecount);
	else
		bufmerge(lfobuf, ((pmods *) baudio->mixlocals)->lfomod * 1.0,
			dmodbuf, 1.0, samplecount);

	if (baudio->voicecount == 1) {
		if (baudio->mixflags & P1_OSCB_WHEEL)
			bufmerge(oscbbuf, ((pmods *) baudio->mixlocals)->oscbmod
					* baudio->contcontroller[1], wmodbuf, 1.0, samplecount);
		else
			bufmerge(oscbbuf, ((pmods *) baudio->mixlocals)->oscbmod,
				dmodbuf, 1.0, samplecount);
	}
/* Mod bus routing done */

/* OSC B FIRST */
	/*
	 * This has to be done late since we may have to mod itself. Note that we
	 * actually need an lfobuf per voice if we want to have osc B mod itself
	 * in poly mode.
	 */
	bristolbzero(oscbbuf, audiomain->segmentsize);

	/*
	 * Second oscillator
	 */
	audiomain->palette[(*baudio->sound[1]).index]->specs->io[0].buf
		= tfreqbuf;
	audiomain->palette[(*baudio->sound[1]).index]->specs->io[1].buf
		= scratchbuf;
	audiomain->palette[(*baudio->sound[1]).index]->specs->io[2].buf
		= oscbbuf;

	/*
	 * Fill tmpbuf1 with our frequency information - should already have it
	 * done, including mods? The Second oscillator may not be doing glide, 
	 * controlled by the UNISON flag. If we do not have unison, then force 
	 * dfreq and cfreq together (temporarily?).
	 */
	fillFreqTable(baudio, voice, freqbuf, samplecount, 1);
	if (baudio->mixflags & P1_KBD_B)
		/*
		 * See if glide is in unison.
		 */
		bufmerge(freqbuf, 1.0, tfreqbuf, 0.0, samplecount);
	else {
		bufptr = tfreqbuf;

		for (i = 0; i < samplecount; i+=8)
		{
			/**bufptr++ = 0.1 + baudio->contcontroller[1] / 2; */
			*bufptr++ = 0.1;
			*bufptr++ = 0.1;
			*bufptr++ = 0.1;
			*bufptr++ = 0.1;
			*bufptr++ = 0.1;
			*bufptr++ = 0.1;
			*bufptr++ = 0.1;
			*bufptr++ = 0.1;
		}
	}

	if (baudio->mixflags & P1_LFO_B)
		bufmerge(tfreqbuf, 0.0, tfreqbuf, 0.25, samplecount);

	bristolbzero(scratchbuf, audiomain->segmentsize);

	/*
	 * This needs to be fixed however I cannot test it right now:
	 *	The ADSR should be amplified in as a function of the voice frequency
	 *	so that it is constand for every note and can then stay in tune.
	 */
#warning osc mod needs amplification by frequency
	if (baudio->mixflags & P1_OSCB_F_WM)
		bufmerge(wmodbuf, 1.0, tfreqbuf, 1.0, samplecount);
	else if (baudio->mixflags & P1_OSCB_F_DM)
		bufmerge(dmodbuf, 1.0, tfreqbuf, 1.0, samplecount);

	if (baudio->mixflags & P1_OSCB_PW_WM)
		bufmerge(wmodbuf, 24.0, scratchbuf, 0.0, samplecount);
	else if (baudio->mixflags & P1_OSCB_PW_DM)
		bufmerge(dmodbuf, 24.0, scratchbuf, 0.0, samplecount);

	/*
	 * And finally operate this oscillator
	 */
	(*baudio->sound[1]).operate(
		(audiomain->palette)[8],
		voice,
		(*baudio->sound[1]).param,
		voice->locals[voice->index][1]);
/* OSC B -DONE */

/* More mod routing for OSC-B */
	if (baudio->voicecount > 1) {
		if (baudio->mixflags & P1_OSCB_WHEEL)
			bufmerge(oscbbuf, ((pmods *) baudio->mixlocals)->oscbmod
					* baudio->contcontroller[1], wmodbuf, 1.0, samplecount);
		else
			bufmerge(oscbbuf, ((pmods *) baudio->mixlocals)->oscbmod,
				dmodbuf, 1.0, samplecount);
	}
/* Done rest of mod source routing */

/* OSC A */
	/*
	 * First insert our buffer pointers, we do this by inserting the buffers
	 * into the existing opspec structures.
	 */
	audiomain->palette[(*baudio->sound[0]).index]->specs->io[0].buf
		= freqbuf;
	audiomain->palette[(*baudio->sound[0]).index]->specs->io[1].buf
		= scratchbuf;
	audiomain->palette[(*baudio->sound[0]).index]->specs->io[2].buf
		= oscabuf;
	audiomain->palette[(*baudio->sound[0]).index]->specs->io[3].buf
		= oscbbuf;

	/*
	 * If we have any mods on the oscillators, we need to put them in here.
	 * This should be under the control of polypressure and/or
	 * channelpressure?
	 */
	if (baudio->mixflags & P1_OSCA_F_WM)
		bufmerge(wmodbuf, 1.0, freqbuf, 1.0, samplecount);
	else if (baudio->mixflags & P1_OSCA_F_DM)
		bufmerge(dmodbuf, 1.0, freqbuf, 1.0, samplecount);

	bristolbzero(scratchbuf, audiomain->segmentsize);

	if (baudio->mixflags & P1_OSCA_PW_WM)
		bufmerge(wmodbuf, 24.0, scratchbuf, 0.0, samplecount);
	else if (baudio->mixflags & P1_OSCA_PW_DM)
		bufmerge(dmodbuf, 24.0, scratchbuf, 0.0, samplecount);

	(*baudio->sound[0]).operate(
		(audiomain->palette)[8],
		voice,
		(*baudio->sound[0]).param,
		voice->locals[voice->index][0]);
/* OSC A - DONE */

	/* The mixer will normalise the signals so that the filter is driven */
	bufmerge(oscabuf, ((pmods *) baudio->mixlocals)->mix_a,
		freqbuf, 0.0, samplecount);
	bufmerge(oscbbuf, ((pmods *) baudio->mixlocals)->mix_b,
		freqbuf, 1.0, samplecount);
	bufmerge(noisebuf, ((pmods *) baudio->mixlocals)->mix_n,
		freqbuf, 1.0, samplecount);

/* FILTER */
	/* 
	 * If we have mods on the filter, apply them here. For the Xa we need to
	 * consider the 4pole option for secondary filtering.
	 */
	bristolbzero(scratchbuf, audiomain->segmentsize);

	if (baudio->mixflags & P1_FILT_WM)
		bufmerge(wmodbuf, 0.5 * ((pmods *) baudio->mixlocals)->f_envlevel,
			scratchbuf, 0.0, samplecount);
	else if (baudio->mixflags & P1_FILT_DM)
		bufmerge(dmodbuf, ((pmods *) baudio->mixlocals)->f_envlevel,
			scratchbuf, 0.0, samplecount);

	/*
	 * Run the mixed oscillators into the filter. Input and output buffers
	 * are the same.
	 */
	audiomain->palette[(*baudio->sound[4]).index]->specs->io[0].buf = freqbuf;
	audiomain->palette[(*baudio->sound[4]).index]->specs->io[1].buf =scratchbuf;
	audiomain->palette[(*baudio->sound[4]).index]->specs->io[2].buf = filtbuf;
	(*baudio->sound[4]).operate(
		(audiomain->palette)[3],
		voice,
		(*baudio->sound[4]).param,
		voice->locals[voice->index][4]);
/* FILTER - DONE */

/* FINAL STAGE */
	/*
	 * Run the ADSR for the final stage amplifier, reusing the startbuf.
	 */
	if (baudio->mixflags & P1_REPEAT)
	{
		if ((((pmods *) baudio->mixlocals)->lfohist > 0) & (lfobuf[0] <= 0))
			voice->flags |= BRISTOL_KEYREON;
	}
	((pmods *) baudio->mixlocals)->lfohist = lfobuf[0];

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

/* FINAL STAGE - DONE */
	return(0);
}

/* Not used */
int
operatePro1Postops(audioMain *audiomain, Baudio *baudio,
bristolVoice *voice, register float *startbuf)
{
	return(0);
}

int
bristolPro1Destroy(audioMain *audiomain, Baudio *baudio)
{
printf("removing one pro1\n");
/*
	bristolfree(freqbuf);
	bristolfree(tfreqbuf);
	bristolfree(scratchbuf);
	bristolfree(dmodbuf);
	bristolfree(adsrbuf);
	bristolfree(filtbuf);
	bristolfree(oscbbuf);
	bristolfree(oscabuf);
*/
	return(0);
}

int
bristolPro1Init(audioMain *audiomain, Baudio *baudio)
{
	printf("initialising one pro1\n");

	baudio->soundCount = 8; /* Number of operators in this voice (MM) */

	/*
	 * Assign an array of sound pointers.
	 */
	baudio->sound = (bristolSound **)
		bristolmalloc0(sizeof(bristolOP *) * baudio->soundCount);
	baudio->effect = (bristolSound **)
		bristolmalloc0(sizeof(bristolOP *) * baudio->soundCount);

	/*
	 * Two main oscillators plus LFO, two envelopes, one filter, one amp, noise
	 */
	initSoundAlgo(8, 0, baudio, audiomain, baudio->sound);
	initSoundAlgo(8, 1, baudio, audiomain, baudio->sound);
	initSoundAlgo(16, 2, baudio, audiomain, baudio->sound);
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

	baudio->param = pro1Controller;
	baudio->destroy = bristolPro1Destroy;
	baudio->operate = operateOnePro1;
	baudio->preops = operatePro1Preops;
/* 	baudio->postops = operatePro1Postops; */

	if (freqbuf == 0)
		freqbuf = (float *) bristolmalloc0(audiomain->segmentsize);
	if (tfreqbuf == 0)
		tfreqbuf = (float *) bristolmalloc0(audiomain->segmentsize);
	if (scratchbuf == 0)
		scratchbuf = (float *) bristolmalloc0(audiomain->segmentsize);
	if (dmodbuf == 0)
		dmodbuf = (float *) bristolmalloc0(audiomain->segmentsize);
	if (adsrbuf == 0)
		adsrbuf = (float *) bristolmalloc0(audiomain->segmentsize);
	if (filtbuf == 0)
		filtbuf = (float *) bristolmalloc0(audiomain->segmentsize);
	if (oscbbuf == 0)
		oscbbuf = (float *) bristolmalloc0(audiomain->segmentsize);
	if (oscabuf == 0)
		oscabuf = (float *) bristolmalloc0(audiomain->segmentsize);
	if (noisebuf == (float *) NULL)
		noisebuf = (float *) bristolmalloc0(audiomain->segmentsize);
	if (lfobuf == (float *) NULL)
		lfobuf = (float *) bristolmalloc0(audiomain->segmentsize);
	if (wmodbuf == (float *) NULL)
		wmodbuf = (float *) bristolmalloc0(audiomain->segmentsize);
	if (sqrbuf == (float *) NULL)
		sqrbuf = (float *) bristolmalloc0(audiomain->segmentsize);
	if (rampbuf == (float *) NULL)
		rampbuf = (float *) bristolmalloc0(audiomain->segmentsize);
	if (tribuf == (float *) NULL)
		tribuf = (float *) bristolmalloc0(audiomain->segmentsize);

	baudio->mixlocals = (float *) bristolmalloc0(sizeof(pmods));
	return(0);
}

