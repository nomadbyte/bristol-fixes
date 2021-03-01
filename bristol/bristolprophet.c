
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
#include "bristolprophet.h"

/*
 * Use of these prophet global buffers will be an issue with use of multiple
 * audio threads, unless we ensure a single thread deals with any given algo
 * type, since then they are only used sequencially.
 *
 * These should really be hidden in the pMods structure for multiple instances.
 */
float *freqbuf = (float *) NULL;
float *osc3buf = (float *) NULL;
float *wmodbuf = (float *) NULL;
float *pmodbuf = (float *) NULL;
float *adsrbuf = (float *) NULL;
float *filtbuf = (float *) NULL;
float *noisebuf = (float *) NULL;
float *oscbbuf = (float *) NULL;
float *oscabuf = (float *) NULL;
float *scratchbuf = (float *) NULL;

float *tribuf = (float *) NULL;
float *sqrbuf = (float *) NULL;
float *rampbuf = (float *) NULL;

extern int s440holder;

int
prophetController(Baudio *baudio, u_char operator, u_char controller,
float value)
{
	int ivalue = value * CONTROLLER_RANGE;

#ifdef DEBUG
	printf("bristolProphetControl(%i, %i, %f)\n", operator, controller, value);
#endif
	if (operator != 126)
		return(0);

	switch (controller) {
		case 0:
			baudio->glide = value * value * baudio->glidemax;
			break;
		case 1:
			if (ivalue == 0)
				baudio->mixflags &= ~BRISTOL_V_UNISON;
			else
				baudio->mixflags |= BRISTOL_V_UNISON;
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
				baudio->mixflags &= ~A_440;
			else {
				s440holder = 0;
				baudio->mixflags |= A_440;
			}
			break;
		case 4:
			/*
			 * Release all notes - issue for midi interface?
			 */
			break;
		case 5:
			/*
			 * Tune - issue for the GUI to normalise f on all oscillators.
			 */
			break;
		case 6:
			((pmods *) baudio->mixlocals)->pm_filtenv = value * 64;
			break;
		case 7:
			((pmods *) baudio->mixlocals)->pm_oscb = value * 64;
			break;
		case 8:
			if (ivalue == 0)
				baudio->mixflags &= ~P_PM_FREQ_A;
			else
				baudio->mixflags |= P_PM_FREQ_A;
			break;
		case 9:
			if (ivalue == 0)
				baudio->mixflags &= ~P_PM_PW_A;
			else
				baudio->mixflags |= P_PM_PW_A;
			break;
		case 10:
			if (ivalue == 0)
				baudio->mixflags &= ~P_PM_FILTER;
			else
				baudio->mixflags |= P_PM_FILTER;
			break;
		case 11:
			((pmods *) baudio->mixlocals)->modmix = value;
			break;
		case 12:
			if (ivalue == 0)
				baudio->mixflags &= ~P_WM_FREQ_A;
			else
				baudio->mixflags |= P_WM_FREQ_A;
			break;
		case 13:
			if (ivalue == 0)
				baudio->mixflags &= ~P_WM_FREQ_B;
			else
				baudio->mixflags |= P_WM_FREQ_B;
			break;
		case 14:
			if (ivalue == 0)
				baudio->mixflags &= ~P_WM_PW_A;
			else
				baudio->mixflags |= P_WM_PW_A;
			break;
		case 15:
			if (ivalue == 0)
				baudio->mixflags &= ~P_WM_PW_B;
			else
				baudio->mixflags |= P_WM_PW_B;
			break;
		case 16:
			if (ivalue == 0)
				baudio->mixflags &= ~P_WM_FILTER;
			else
				baudio->mixflags |= P_WM_FILTER;
			break;
		case 18:
			if (ivalue == 0)
				baudio->mixflags &= ~P_LFO_B;
			else
				baudio->mixflags |= P_LFO_B;
			break;
		case 19:
			if (ivalue == 0)
				baudio->mixflags &= ~P_KBD_B;
			else
				baudio->mixflags |= P_KBD_B;
			break;
		case 20:
			((pmods *) baudio->mixlocals)->mix_a = value * 96.0;
			break;
		case 21:
			((pmods *) baudio->mixlocals)->mix_b = value * 96.0;
			break;
		case 22:
			((pmods *) baudio->mixlocals)->mix_n = value * 96.0;
			break;
		case 23:
			((pmods *) baudio->mixlocals)->f_envlevel = value;
			break;
		case 24:
			if (ivalue == 0)
				baudio->mixflags &= ~P_LFO_RAMP;
			else
				baudio->mixflags |= P_LFO_RAMP;
			break;
		case 25:
			if (ivalue == 0)
				baudio->mixflags &= ~P_LFO_TRI;
			else
				baudio->mixflags |= P_LFO_TRI;
			break;
		case 26:
			if (ivalue == 0)
				baudio->mixflags &= ~P_LFO_SQR;
			else
				baudio->mixflags |= P_LFO_SQR;
			break;
		/*
		 * The following are from the Prophet52 only.
		 */
		case 30:
			/*
			 * The first param is rate, it should be X number of steps to 
			 * reach 0 to the depth, and has to be adjusted such that the 
			 * speed is constant over all depths.
			 */
			baudio->effect[0]->param->param[0].float_val =
				(0.1 + (value * value) * 20) * 1024 / baudio->samplerate;
			break;
		case 31:
			baudio->effect[0]->param->param[1].float_val = (value * value);
			break;
		case 32:
			baudio->effect[0]->param->param[2].float_val = (value * value);
			break;
		case 33:
			baudio->effect[0]->param->param[3].float_val = (value * value);
			break;
		case 34:
			((pmods *) baudio->mixlocals)->pan = value;
			break;
		case 35:
			/*
			 * When we go into Uni mode then configure 6 voices on this layer
			 * otherwise just three.
			 */
			if (ivalue == 0)
				baudio->voicecount = ((pmods *) baudio->mixlocals)->voicecount;
			else
				baudio->voicecount =
					((pmods *) baudio->mixlocals)->voicecount >> 1;
			break;
		case 36:
			((pmods *) baudio->mixlocals)->gain = value;
			break;
	}
	return(0);
}

/*
 * Preops will do noise, and one oscillator - the LFO.
 */
int
operateProphetPreops(audioMain *audiomain, Baudio *baudio,
bristolVoice *voice, register float *startbuf)
{
	register float *bufptr;
	register int samplecount = audiomain->samplecount;

#ifdef DEBUG
	if ((audiomain->debuglevel & BRISTOL_DEBUG_MASK) > BRISTOL_DEBUG5)
		printf("operateProphetPreops(%x, %x, %x) %i\n",
			baudio, voice, startbuf, baudio->cvoices);
#endif
	if (freqbuf == (float *) NULL)
		freqbuf = (float *) bristolmalloc0(audiomain->segmentsize);
	bufptr = freqbuf;

	if (noisebuf == (float *) NULL)
		noisebuf = (float *) bristolmalloc0(audiomain->segmentsize);
	if (osc3buf == (float *) NULL)
		osc3buf = (float *) bristolmalloc0(audiomain->segmentsize);
	if (wmodbuf == (float *) NULL)
		wmodbuf = (float *) bristolmalloc0(audiomain->segmentsize);
	if (sqrbuf == (float *) NULL)
		sqrbuf = (float *) bristolmalloc0(audiomain->segmentsize);
	if (rampbuf == (float *) NULL)
		rampbuf = (float *) bristolmalloc0(audiomain->segmentsize);
	if (tribuf == (float *) NULL)
		tribuf = (float *) bristolmalloc0(audiomain->segmentsize);

	bristolbzero(osc3buf, audiomain->segmentsize);
	bristolbzero(noisebuf, audiomain->segmentsize);
	bristolbzero(wmodbuf, audiomain->segmentsize);

	/*
	 * Third oscillator. We do this first, since it may be used to modulate
	 * the first two oscillators. No mods on LFO - wmodbuf is zeros at moment.
	audiomain->palette[(*baudio->sound[2]).index]->specs->io[0].buf = freqbuf;
	audiomain->palette[(*baudio->sound[2]).index]->specs->io[1].buf = wmodbuf;
	audiomain->palette[(*baudio->sound[2]).index]->specs->io[2].buf = osc3buf;
	 */
	audiomain->palette[(*baudio->sound[2]).index]->specs->io[0].buf = NULL;
	audiomain->palette[(*baudio->sound[2]).index]->specs->io[1].buf = tribuf;
	audiomain->palette[(*baudio->sound[2]).index]->specs->io[2].buf = sqrbuf;
	audiomain->palette[(*baudio->sound[2]).index]->specs->io[3].buf = NULL;
	audiomain->palette[(*baudio->sound[2]).index]->specs->io[4].buf = osc3buf;
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
	if (baudio->mixflags & (P_LFO_SQR|P_LFO_RAMP|P_LFO_TRI))
	{
		bristolbzero(osc3buf, audiomain->segmentsize);
		if (baudio->mixflags & P_LFO_TRI)
			bufmerge(tribuf, 12.0, osc3buf, 1.0, samplecount);
		if (baudio->mixflags & P_LFO_SQR)
			bufmerge(sqrbuf, 12.0, osc3buf, 1.0, samplecount);
		if (baudio->mixflags & P_LFO_RAMP)
			bufmerge(rampbuf, 12.0, osc3buf, 1.0, samplecount);
	} else
		bufmerge(tribuf, 0.0, osc3buf, 12.0, samplecount);
/* LFO OVER */

/* NOISE - This should go in a preops call.... */
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

	/*
	 * Mix osc3 and noise - these are both in the startbuf if we have
	 * them flagged to be mixed. These are the WHEEL MOD inputs.
	 */
	bufmerge(noisebuf,
		((pmods *) baudio->mixlocals)->modmix * baudio->contcontroller[1] * 0.2,
			wmodbuf, 0.0, samplecount);
	bufmerge(osc3buf,
		(1 - ((pmods *) baudio->mixlocals)->modmix) * baudio->contcontroller[1],
		wmodbuf, 1.0, samplecount);
	return(0);
}

int
operateOneProphet(audioMain *audiomain, Baudio *baudio,
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
	bristolbzero(pmodbuf, audiomain->segmentsize);
	bristolbzero(adsrbuf, audiomain->segmentsize);
	bristolbzero(filtbuf, audiomain->segmentsize);
	bristolbzero(oscbbuf, audiomain->segmentsize);
	bristolbzero(oscabuf, audiomain->segmentsize);
	bristolbzero(scratchbuf, audiomain->segmentsize);

/* ADSR - POLY MOD */
	/*
	 * Run the ADSR for the filter, reusing the startbuf. We need this for the
	 * poly mods.
	 */
	audiomain->palette[(*baudio->sound[3]).index]->specs->io[0].buf = adsrbuf;
	(*baudio->sound[3]).operate(
		(audiomain->palette)[1],
		voice,
		(*baudio->sound[3]).param,
		voice->locals[voice->index][3]);
/* ADSR - POLY MOD - OVER */

/* OSC B FIRST */
	/*
	 * Second oscillator
	 */
	audiomain->palette[(*baudio->sound[1]).index]->specs->io[0].buf
		= freqbuf;
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
	if (baudio->mixflags & P_KBD_B)
	{
		/*
		 * See if glide is in unison.
		 */
		if (baudio->mixflags & P_UNISON)
			fillFreqTable(baudio, voice, freqbuf, samplecount, 1);
		else
			fillFreqTable(baudio, voice, freqbuf, samplecount, 0);
	} else {
		bufptr = freqbuf;

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

	if (baudio->mixflags & P_LFO_B)
		bufmerge(freqbuf, 0.0, freqbuf, 0.25, samplecount);

	/*
	 * Put in any mods we want.
	 */
	if (baudio->mixflags & P_WM_FREQ_B)
		bufmerge(wmodbuf, 0.025, freqbuf, 1.0, samplecount);
	if (baudio->mixflags & P_WM_PW_B)
		bufmerge(wmodbuf, 16.0, scratchbuf, 0.0, samplecount);
	/*
	 * And finally operate this oscillator
	 */
	(*baudio->sound[1]).operate(
		(audiomain->palette)[8],
		voice,
		(*baudio->sound[1]).param,
		voice->locals[voice->index][1]);
/* OSC B -DONE */

	/*
	 * Put together our Poly Mod buffer.
	bufmerge(oscbbuf, ((pmods *) baudio->mixlocals)->pm_oscb, adsrbuf,
		((pmods *) baudio->mixlocals)->pm_filtenv, samplecount);
	 */

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
	 * Fill tmpbuf1 with our frequency information
	 */
	fillFreqTable(baudio, voice, freqbuf, samplecount, 1);
	/*
	 * If we have any mods on the oscillators, we need to put them in here.
	 * This should be under the control of polypressure and/or
	 * channelpressure?
	 */
	bzero(scratchbuf, audiomain->segmentsize);
	if (baudio->mixflags & P_PM_FREQ_A)
	{
		/*
		 * This needs to be fixed however I cannot test it right now:
		 *	The ADSR should be amplified in as a function of the voice frequency
		 *	so that it is constand for every note and can then stay in tune.
		 */
#warning adsr2osca needs amplification by frequency
		bufmerge(adsrbuf, 0.04 * ((pmods *) baudio->mixlocals)->pm_filtenv,
			freqbuf, 1.0, samplecount);
		bufmerge(oscbbuf, 0.002 * ((pmods *) baudio->mixlocals)->pm_oscb,
			freqbuf, 1.0, samplecount);
	}
	if (baudio->mixflags & P_PM_PW_A)
	{
		bufmerge(adsrbuf, ((pmods *) baudio->mixlocals)->pm_filtenv,
			scratchbuf, 0.0, samplecount);
		bufmerge(oscbbuf, ((pmods *) baudio->mixlocals)->pm_oscb,
			scratchbuf, 1.0, samplecount);
	}
	if (baudio->mixflags & P_WM_FREQ_A)
		bufmerge(wmodbuf, 0.025, freqbuf, 1.0, samplecount);
	if (baudio->mixflags & P_WM_PW_A)
		bufmerge(wmodbuf, 16.0, scratchbuf, (float) 0, samplecount);

	(*baudio->sound[0]).operate(
		(audiomain->palette)[8],
		voice,
		(*baudio->sound[0]).param,
		voice->locals[voice->index][0]);
/* OSC A - DONE */

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
	bzero(scratchbuf, audiomain->segmentsize);
	if (baudio->mixflags & P_PM_FILTER)
	{
		bufmerge(adsrbuf, ((pmods *) baudio->mixlocals)->f_envlevel
			* ((pmods *) baudio->mixlocals)->pm_filtenv * 0.05, scratchbuf,
			0.0, samplecount);
		bufmerge(oscbbuf, ((pmods *) baudio->mixlocals)->pm_oscb * 0.001,
			scratchbuf, 1.0, samplecount);
	}
	if (baudio->mixflags & P_WM_FILTER)
		bufmerge(wmodbuf, 0.1 * baudio->contcontroller[1], scratchbuf, 1.0,
			samplecount);
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

int
operateProphetPostops(audioMain *audiomain, Baudio *baudio,
bristolVoice *voice, register float *startbuf)
{
/* Panning - this is for the OBX only. If pan is zero we don't have to do */
/* anything. */
	register int samplecount = audiomain->samplecount;
	register float *lbuf = baudio->leftbuf;
	register float *rbuf = baudio->rightbuf;
	register float g;

	g = ((pmods *) baudio->mixlocals)->pan
		* ((pmods *) baudio->mixlocals)->gain;
	/*
	 * Gain into the rightbuf, adjust the leftbuf;
	 */
	for (;samplecount > 0; samplecount-=8)
	{
		*rbuf++ = *lbuf++ * g;
		*rbuf++ = *lbuf++ * g;
		*rbuf++ = *lbuf++ * g;
		*rbuf++ = *lbuf++ * g;
		*rbuf++ = *lbuf++ * g;
		*rbuf++ = *lbuf++ * g;
		*rbuf++ = *lbuf++ * g;
		*rbuf++ = *lbuf++ * g;
	}

	samplecount = audiomain->samplecount;
	g = (1.0f -  ((pmods *) baudio->mixlocals)->pan)
		* ((pmods *) baudio->mixlocals)->gain;
	lbuf = baudio->leftbuf;
	for (;samplecount > 0; samplecount-=8)
	{
		*lbuf++ *= g;
		*lbuf++ *= g;
		*lbuf++ *= g;
		*lbuf++ *= g;
		*lbuf++ *= g;
		*lbuf++ *= g;
		*lbuf++ *= g;
		*lbuf++ *= g;
	}
	return(0);
}

int
bristolProphetDestroy(audioMain *audiomain, Baudio *baudio)
{
printf("removing one prophet\n");
/*
	bristolfree(freqbuf);
	bristolfree(scratchbuf);
	bristolfree(pmodbuf);
	bristolfree(adsrbuf);
	bristolfree(filtbuf);
	bristolfree(oscbbuf);
	bristolfree(oscabuf);
*/
	return(0);
}

int
bristolProphetInit(audioMain *audiomain, Baudio *baudio)
{
printf("initialising one prophet\n");
	baudio->soundCount = 8; /* Number of operators in this voice (MM) */
	/*
	 * Assign an array of sound pointers.
	 */
	baudio->sound = (bristolSound **)
		bristolmalloc0(sizeof(bristolOP *) * baudio->soundCount);
	baudio->effect = (bristolSound **)
		bristolmalloc0(sizeof(bristolOP *) * baudio->soundCount);

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

	baudio->param = prophetController;
	baudio->destroy = bristolProphetDestroy;
	baudio->operate = operateOneProphet;
	baudio->preops = operateProphetPreops;
	baudio->postops = operateProphetPostops;

	if (freqbuf == 0)
		freqbuf = (float *) bristolmalloc0(audiomain->segmentsize);
	if (scratchbuf == 0)
		scratchbuf = (float *) bristolmalloc0(audiomain->segmentsize);
	if (pmodbuf == 0)
		pmodbuf = (float *) bristolmalloc0(audiomain->segmentsize);
	if (adsrbuf == 0)
		adsrbuf = (float *) bristolmalloc0(audiomain->segmentsize);
	if (filtbuf == 0)
		filtbuf = (float *) bristolmalloc0(audiomain->segmentsize);
	if (oscbbuf == 0)
		oscbbuf = (float *) bristolmalloc0(audiomain->segmentsize);
	if (oscabuf == 0)
		oscabuf = (float *) bristolmalloc0(audiomain->segmentsize);

	baudio->mixlocals = (float *) bristolmalloc0(sizeof(pmods));
	((pmods *) baudio->mixlocals)->voicecount = baudio->voicecount;
	/* Default center pan */
	((pmods *) baudio->mixlocals)->pan = 0.5;
	baudio->mixflags |= BRISTOL_STEREO;
	baudio->mixflags |= P_UNISON;
	return(0);
}

