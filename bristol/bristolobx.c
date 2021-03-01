
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
#include "bristolobx.h"

/*
 * Use of these obx global buffers will be an issue with use of multiple
 * audio threads, unless we ensure a single thread deals with any given algo
 * type, since then they are only used sequentially.
 *
 * (This file was copied from the prophet, hence shares much of the code)
 */
static float *freq = (float *) NULL;
static float *freqbuf = (float *) NULL;
static float *adsrbuf = (float *) NULL;
static float *filtbuf = (float *) NULL;
static float *noisebuf = (float *) NULL;
static float *oscbbuf = (float *) NULL;
static float *oscabuf = (float *) NULL;
static float *scratchbuf = (float *) NULL;

static float *lfosqr = (float *) NULL;
static float *lfo = (float *) NULL;
static float *lfosine = (float *) NULL;
static float *modsine = (float *) NULL;
static float *lfosh = (float *) NULL;

int
obxController(Baudio *baudio, u_char operator, u_char controller, float value)
{
	int ivalue = value * CONTROLLER_RANGE;

#ifdef DEBUG
	printf("bristolOBXControl(%i, %i, %f)\n", operator, controller, value);
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
		case 2: /* Global tune */
			baudio->gtune = 1.0
				+ (baudio->note_diff - 1)
				* (value * 2 - 1);

			buildCurrentTable(baudio, baudio->gtune);
			alterAllNotes(baudio);
			break;
		case 3: /* S/H */
			if (ivalue == 0)
				baudio->mixflags &= ~O_S_H;
			else
				baudio->mixflags |= O_S_H;
			break;
		case 4: /* Depth Mod 1 - float */
			((pmods *) baudio->mixlocals)->d_mod1 = value;
			break;
		case 5: /* Osc1 freq */
			if (ivalue == 0)
				baudio->mixflags &= ~O_FREQ_1;
			else
				baudio->mixflags |= O_FREQ_1;
			break;
		case 6: /* Osc2 freq */
			if (ivalue == 0)
				baudio->mixflags &= ~O_FREQ_2;
			else
				baudio->mixflags |= O_FREQ_2;
			break;
		case 7: /* Filt mod */
			if (ivalue == 0)
				baudio->mixflags &= ~O_FILT;
			else
				baudio->mixflags |= O_FILT;
			break;
		case 8: /* Depth Mod 2 - float */
			((pmods *) baudio->mixlocals)->d_mod2 = value;
			break;
		case 9: /* PWM Osc1 */
			if (ivalue == 0)
				baudio->mixflags &= ~O_PWM_1;
			else
				baudio->mixflags |= O_PWM_1;
			break;
		case 10: /* PWM Osc2 */
			if (ivalue == 0)
				baudio->mixflags &= ~O_PWM_2;
			else
				baudio->mixflags |= O_PWM_2;
			break;
		case 11: /* Crossmod A->B */
			if (ivalue == 0)
				baudio->mixflags &= ~O_XMOD;
			else
				baudio->mixflags |= O_XMOD;
			break;
		case 12: /* Osc1->Filt */
			if (ivalue == 0)
				baudio->mixflags &= ~O_F_OSC1;
			else
				baudio->mixflags |= O_F_OSC1;
			break;
		case 13: /* KBD->Filt */
			if (ivalue == 0)
				baudio->mixflags &= ~O_F_KBD;
			else
				baudio->mixflags |= O_F_KBD;
			break;
		case 14: /* Osc2->Filt */
			if (ivalue == 0)
				baudio->mixflags &= ~O_F_OSC2_1;
			else
				baudio->mixflags |= O_F_OSC2_1;
			break;
		case 15: /* Osc2->Filt */
			if (ivalue == 0)
				baudio->mixflags &= ~O_F_OSC2_2;
			else
				baudio->mixflags |= O_F_OSC2_2;
			break;
		case 16: /* Noise->Filt */
			if (ivalue == 0)
				baudio->mixflags &= ~O_F_NOISE1;
			else
				baudio->mixflags |= O_F_NOISE1;
			break;
		case 17: /* Noise->Filt */
			if (ivalue == 0)
				baudio->mixflags &= ~O_F_NOISE2;
			else
				baudio->mixflags |= O_F_NOISE2;
			break;
		case 18: /* Trem */
			if (ivalue == 0)
				baudio->mixflags &= ~O_TREM;
			else
				baudio->mixflags |= O_TREM;
			break;
		case 19: /* LFO Sine */
			if (ivalue == 0)
				baudio->mixflags &= ~O_LFO_SINE;
			else
				baudio->mixflags |= O_LFO_SINE;
			break;
		case 20: /* LFO Square */
			if (ivalue == 0)
				baudio->mixflags &= ~O_LFO_SQUARE;
			else
				baudio->mixflags |= O_LFO_SQUARE;
			break;
		case 21: /* S/H */
			if (ivalue == 0)
				baudio->mixflags &= ~O_LFO_SH;
			else
				baudio->mixflags |= O_LFO_SH;
			break;
		case 22: /* Multi LFO */
			if (ivalue == 0)
				baudio->mixflags &= ~O_MULTI_LFO;
			else
				baudio->mixflags |= O_MULTI_LFO;
			break;
		case 23: /* Envmods */
			if (ivalue == 0)
				baudio->mixflags &= ~O_ENV_MOD;
			else
				baudio->mixflags |= O_ENV_MOD;
			break;
		case 24: /* 4Pole */
			if (ivalue == 0)
				baudio->mixflags &= ~O_F_4POLE;
			else
				baudio->mixflags |= O_F_4POLE;
			break;
		case 25: /* Layer modes */
			if (ivalue == 0)
				baudio->mixflags &= ~O_MOD1_ENABLE;
			else
				baudio->mixflags |= O_MOD1_ENABLE;
			break;
		case 26: /* Mod LFO Depth */
			((pmods *) baudio->mixlocals)->wheelmod = value;
			break;
		case 27: /* Multi Mod LFO */
			if (ivalue == 0)
				baudio->mixflags &= ~O_MOD2_ENABLE;
			else
				baudio->mixflags |= O_MOD2_ENABLE;
			break;
		case 28: /* Pan */
			((pmods *) baudio->mixlocals)->pan = value;
			break;
		case 29: /* Modulate PWM */
			if (ivalue == 0)
				baudio->mixflags &= ~O_MOD_PWM;
			else
				baudio->mixflags |= O_MOD_PWM;
			break;
		case 30: /* Voice selections */
			if (value > 0.0)
				baudio->voicecount = ((pmods *) baudio->mixlocals)->voicecount;
			else
				baudio->voicecount =
						((pmods *) baudio->mixlocals)->voicecount >> 1;
			printf("voicecount now %i\n", baudio->voicecount);
			break;
	}
	return(0);
}

/*
 * Preops will do noise, and one oscillator - the LFO.
 */
int
operateOBXPreops(audioMain *audiomain, Baudio *baudio,
bristolVoice *voice, register float *startbuf)
{
	register int samplecount = audiomain->samplecount;

#ifdef DEBUG
	if ((audiomain->debuglevel & BRISTOL_DEBUG_MASK) > BRISTOL_DEBUG5)
		printf("operateOBXPreops(%x, %x, %x) %i\n",
			baudio, voice, startbuf, baudio->cvoices);
#endif

	if (freqbuf == (float *) NULL)
		freqbuf = (float *) bristolmalloc0(audiomain->segmentsize);

	if (noisebuf == (float *) NULL)
		noisebuf = (float *) bristolmalloc0(audiomain->segmentsize);

	bristolbzero(noisebuf, audiomain->segmentsize);

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

/* LFO */
	/*
	 * Third oscillator. We do this first, since it may be used to modulate
	 * the first two oscillators.
	 */
	if ((baudio->mixflags & O_MULTI_LFO) == 0)
	{
		audiomain->palette[(*baudio->sound[2]).index]->specs->io[0].buf
			= noisebuf;
		audiomain->palette[(*baudio->sound[2]).index]->specs->io[1].buf
			= lfo;
		audiomain->palette[(*baudio->sound[2]).index]->specs->io[2].buf
			= lfosqr;
		audiomain->palette[(*baudio->sound[2]).index]->specs->io[3].buf
			= lfosh;
		audiomain->palette[(*baudio->sound[2]).index]->specs->io[4].buf
			= lfosine;

		(*baudio->sound[2]).operate(
			(audiomain->palette)[16],
			voice,
			(*baudio->sound[2]).param,
			baudio->locals[voice->index][2]);

		/*
		 * See what LFO options to mix
		 */
		if (baudio->mixflags & (O_LFO_SINE|O_LFO_SQUARE|O_LFO_SH))
		{
			bristolbzero(lfo, audiomain->segmentsize);
			if (baudio->mixflags & O_LFO_SINE)
				bufmerge(lfosine, 1.0, lfo, 1.0, samplecount);
			if (baudio->mixflags & O_LFO_SQUARE)
				bufmerge(lfosqr, 1.0, lfo, 1.0, samplecount);
			if (baudio->mixflags & O_LFO_SH)
				bufmerge(lfosh, 0.1, lfo, 1.0, samplecount);
		}
	}
/* LFO OVER */

/* MOD LFO */
	audiomain->palette[(*baudio->sound[8]).index]->specs->io[0].buf = NULL;
	audiomain->palette[(*baudio->sound[8]).index]->specs->io[1].buf = NULL;
	audiomain->palette[(*baudio->sound[8]).index]->specs->io[2].buf = NULL;
	audiomain->palette[(*baudio->sound[8]).index]->specs->io[3].buf = NULL;
	audiomain->palette[(*baudio->sound[8]).index]->specs->io[4].buf = modsine;

	(*baudio->sound[8]).operate(
		(audiomain->palette)[16],
		voice,
		(*baudio->sound[8]).param,
		baudio->locals[voice->index][8]);
/* MODLFO OVER */

	/*
	 * We cannot do the mod routing yet as much of it is poly.
	 *
	 */
	return(0);
}

int
operateOneOBX(audioMain *audiomain, Baudio *baudio,
bristolVoice *voice, register float *startbuf)
{
	register int samplecount = audiomain->samplecount;

	/*
	 * We need to run through every bristolSound on the baudio sound chain.
	 * We need to pass the correct set of parameters to each operator, and
	 * ensure they get the correct local variable set.
	 */
	if (freqbuf == NULL)
		return(0);
	if (adsrbuf == NULL)
		return(0);

	bristolbzero(freqbuf, audiomain->segmentsize);
	bristolbzero(adsrbuf, audiomain->segmentsize);
	bristolbzero(filtbuf, audiomain->segmentsize);
	bristolbzero(oscbbuf, audiomain->segmentsize);
	bristolbzero(oscabuf, audiomain->segmentsize);

/* ADSR - POLY MOD */
	/*
	 * Run the ADSR for the filter.
	 */
	audiomain->palette[(*baudio->sound[3]).index]->specs->io[0].buf = adsrbuf;
	(*baudio->sound[3]).operate(
		(audiomain->palette)[1],
		voice,
		(*baudio->sound[3]).param,
		voice->locals[voice->index][3]);
/* ADSR - POLY MOD - OVER */

/* LFO */
	/*
	 * Third oscillator. We do this first, since it may be used to modulate
	 * the first two oscillators.
	 */

	if (baudio->mixflags & O_MULTI_LFO)
	{
		audiomain->palette[(*baudio->sound[2]).index]->specs->io[0].buf
			= noisebuf;
		audiomain->palette[(*baudio->sound[2]).index]->specs->io[1].buf
			= lfo;
		audiomain->palette[(*baudio->sound[2]).index]->specs->io[2].buf
			= lfosqr;
		audiomain->palette[(*baudio->sound[2]).index]->specs->io[3].buf
			= lfosh;
		audiomain->palette[(*baudio->sound[2]).index]->specs->io[4].buf
			= lfosine;

		(*baudio->sound[2]).operate(
			(audiomain->palette)[16],
			voice,
			(*baudio->sound[2]).param,
			baudio->locals[voice->index][2]);

		/*
		 * See what LFO options to mix
		 */
		if (baudio->mixflags & (O_LFO_SINE|O_LFO_SQUARE|O_LFO_SH))
		{
			bristolbzero(lfo, audiomain->segmentsize);
			if (baudio->mixflags & O_LFO_SINE)
				bufmerge(lfosine, 1.0, lfo, 1.0, samplecount);
			if (baudio->mixflags & O_LFO_SQUARE)
				bufmerge(lfosqr, 1.0, lfo, 1.0, samplecount);
			if (baudio->mixflags & O_LFO_SH)
				bufmerge(lfosh, 0.1, lfo, 1.0, samplecount);
		}
	}
/* LFO OVER */

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
	audiomain->palette[(*baudio->sound[0]).index]->specs->io[3].buf = 0;

	/*
	 * Fill tmpbuf1 with our frequency information
	 */
	fillFreqTable(baudio, voice, freq, samplecount, 1);

	/*
	 * If we have any mods on the oscillators, we need to put them in here.
	 * This should be under the control of polypressure and/or channelpressure?
	 */
	bzero(scratchbuf, audiomain->segmentsize);
	/*
	 * PUT IN MOD ROUTING. We have d_mod1 adjusting freq and d_mod2 adjusting
	 * PW
	 */
	bufmerge(freq, 1.0, freqbuf, 0.0, samplecount);
	if (baudio->mixflags & O_FREQ_1)
		bufmerge(lfo, ((pmods *) baudio->mixlocals)->d_mod1 * 0.4,
			freqbuf, 1.0, samplecount);
	if (baudio->mixflags & O_PWM_1)
		bufmerge(lfo, ((pmods *) baudio->mixlocals)->d_mod2 * 128,
			scratchbuf, 0.0, samplecount);

	if (baudio->mixflags & O_MOD_PWM)
		bufmerge(modsine,
			((pmods *) baudio->mixlocals)->wheelmod
				* baudio->contcontroller[1] * 512,
			scratchbuf, 1.0, samplecount);

	if (baudio->mixflags & O_MOD1_ENABLE)
	{
		/*
		 * Wheel mod....
		 */
		bufmerge(modsine,
			((pmods *) baudio->mixlocals)->wheelmod * baudio->contcontroller[1]
				* 0.6, freqbuf, 1.0, samplecount);
	}

	(*baudio->sound[0]).operate(
		(audiomain->palette)[8],
		voice,
		(*baudio->sound[0]).param,
		voice->locals[voice->index][0]);
/* OSC A - DONE */

/* OSC B */
	/*
	 * Second oscillator
	 */
	audiomain->palette[(*baudio->sound[1]).index]->specs->io[0].buf
		= freq;
	audiomain->palette[(*baudio->sound[1]).index]->specs->io[1].buf
		= scratchbuf;
	audiomain->palette[(*baudio->sound[1]).index]->specs->io[2].buf
		= oscbbuf;
	audiomain->palette[(*baudio->sound[1]).index]->specs->io[3].buf
		= oscabuf;
	/*
	 * Fill tmpbuf1 with our frequency information - should already have it
	 * done, including mods? The Second oscillator may not be doing glide, 
	 * controlled by the UNISON flag. If we do not have unison, then force 
	 * dfreq and cfreq together (temporarily?).
	fillFreqTable(baudio, voice, freqbuf, samplecount, 0);
	 */

	/* This should perhaps go into PWM and/or freq buf? */
	if (baudio->mixflags & O_ENV_MOD)
		bufmerge(adsrbuf, ((pmods *) baudio->mixlocals)->d_mod1,
			freq, 1.0, samplecount);

	/*
	 * Put in any mods we want.
	 */
	bzero(scratchbuf, audiomain->segmentsize);
	if (baudio->mixflags & O_FREQ_2)
		bufmerge(lfo, ((pmods *) baudio->mixlocals)->d_mod1 * 0.4,
			freq, 1.0, samplecount);
	if (baudio->mixflags & O_PWM_2)
		bufmerge(lfo, ((pmods *) baudio->mixlocals)->d_mod2 * 128,
			scratchbuf, 0.0, samplecount);
	if (baudio->mixflags & O_XMOD)
		bufmerge(oscabuf, 0.4, freq, 1.0, samplecount);

	if (baudio->mixflags & O_MOD_PWM)
		bufmerge(modsine,
			((pmods *) baudio->mixlocals)->wheelmod
				* baudio->contcontroller[1] * 512,
			scratchbuf, 1.0, samplecount);

	if (baudio->mixflags & O_MOD2_ENABLE)
	{
		/*
		 * Wheel mod....
		 */
		bufmerge(modsine,
			((pmods *) baudio->mixlocals)->wheelmod * baudio->contcontroller[1]
				* 0.6, freq, 1.0, samplecount);
	}

	/*
	 * And finally operate this oscillator
	 */
	(*baudio->sound[1]).operate(
		(audiomain->palette)[8],
		voice,
		(*baudio->sound[1]).param,
		voice->locals[voice->index][1]);
/* OSC B -DONE */

/* FILTER */
	/* 
	 * If we have mods on the filter, apply them here.
	 */
	bufmerge(adsrbuf, 1.5, scratchbuf, 0.0, samplecount);
	if (baudio->mixflags & O_FILT)
		bufmerge(lfo, ((pmods *) baudio->mixlocals)->d_mod1 * 1.5,
			scratchbuf, 1.0, samplecount);

	/*
	 * These should be radio buttons that can be deselected. Rather than that,
	 * as it is easier and more flexible, take different levels depending on
	 * what is selected
	 */
	if ((baudio->mixflags & O_F_OSC2_1) ||
		(baudio->mixflags & O_F_OSC2_2))
	{
		if (baudio->mixflags & O_F_OSC2_1)
		{
			if (baudio->mixflags & O_F_OSC2_2)
				bufmerge(oscabuf, 0.0, oscbbuf, 1.0, samplecount);
			else
				bufmerge(oscabuf, 0.0, oscbbuf, 0.25, samplecount);
		} else
			bufmerge(oscabuf, 0.0, oscbbuf, 0.5, samplecount);
	} else
		bufmerge(oscabuf, 0.0, oscbbuf, 0.0, samplecount);

	if (baudio->mixflags & O_F_OSC1)
		bufmerge(oscabuf, 0.0, oscabuf, 1.0, samplecount);
	else
		bufmerge(oscabuf, 0.0, oscabuf, 0.0, samplecount);

	if ((baudio->mixflags & O_F_NOISE1) ||
		(baudio->mixflags & O_F_NOISE2))
	{
		if (baudio->mixflags & O_F_NOISE1)
		{
			if (baudio->mixflags & O_F_NOISE2)
				bufmerge(noisebuf, 0.8, oscbbuf, 1.0, samplecount);
			else
				bufmerge(noisebuf, 0.2, oscbbuf, 1.0, samplecount);
		} else
			bufmerge(noisebuf, 0.4, oscbbuf, 1.0, samplecount);
	}

	/*
	 * Mix the oscillators together. Had to do the filter mods first though.
	 *
	 * This is mostly wrong. Most of the buttons taken as filter mods were
	 * actually mixing options into the filter.
	 */
	bufmerge(oscabuf, 64.0, oscbbuf, 64.0, samplecount);

	/*
	 * Run the mixed oscillators into the filter. Input and output buffers
	 * are the same.
	 */
	audiomain->palette[(*baudio->sound[4]).index]->specs->io[0].buf = oscbbuf;
	audiomain->palette[(*baudio->sound[4]).index]->specs->io[1].buf =scratchbuf;
	audiomain->palette[(*baudio->sound[4]).index]->specs->io[2].buf = filtbuf;

	(*baudio->sound[4]).operate(
		(audiomain->palette)[B_FILTER2],
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
	if (baudio->mixflags & O_TREM)
		bufmerge(lfo, ((pmods *) baudio->mixlocals)->d_mod2 * 0.4,
			adsrbuf, 1.0, samplecount);
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
operateOBXPostops(audioMain *audiomain, Baudio *baudio,
bristolVoice *voice, register float *startbuf)
{
/* Panning - this is for the OBX only. If pan is zero we don't have to do */
/* anything. */
	register int samplecount = audiomain->samplecount;
	register float *lbuf = baudio->leftbuf;
	register float *rbuf = baudio->rightbuf;
	register float g = ((pmods *) baudio->mixlocals)->pan;

	/*
	 * Gain into the rightbuf, adjust the leftbuf;
	 */
	for (;samplecount > 0; samplecount-=8)
	{
		*rbuf++ = *lbuf++ * g * 2;
		*rbuf++ = *lbuf++ * g * 2;
		*rbuf++ = *lbuf++ * g * 2;
		*rbuf++ = *lbuf++ * g * 2;
		*rbuf++ = *lbuf++ * g * 2;
		*rbuf++ = *lbuf++ * g * 2;
		*rbuf++ = *lbuf++ * g * 2;
		*rbuf++ = *lbuf++ * g * 2;
	}

	samplecount = audiomain->samplecount;
	g = (1.0f - g) * 2;
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
bristolOBXDestroy(audioMain *audiomain, Baudio *baudio)
{
	printf("removing one obx\n");

	/*
	 * We need to leave these, we may have multiple invocations running
	 */
	return(0);

/*
 * This ought to be fixed, these should be removed but since they are common
 * to the synth then that will break with multiple invocations such as the
 * OB-Xa and Prophet-10
 */
	if (freqbuf != NULL) {
		bristolfree(freqbuf);
		freqbuf = NULL;
	}
	if (freq != NULL) {
		bristolfree(freq);
		freq = NULL;
	}
    if (scratchbuf != NULL) {
		bristolfree(scratchbuf);
		scratchbuf = NULL;
	}
    if (adsrbuf != NULL) {
		bristolfree(adsrbuf);
		adsrbuf = NULL;
	}
    if (filtbuf != NULL) {
		bristolfree(filtbuf);
		filtbuf = NULL;
	}
    if (oscbbuf != NULL) {
		bristolfree(oscbbuf);
		oscbbuf = NULL;
	}
    if (oscabuf != NULL) {
		bristolfree(oscabuf);
		oscabuf = NULL;
	}
	/* lots of lfo bufs missed out here.... */
	return(0);
}

int
bristolOBXInit(audioMain *audiomain, Baudio *baudio)
{
	printf("initialising one obx\n");
	/*
	 * The OBX is relatively straightforward in terms of operators, the real
	 * strength of the synth is the routing that is used to beef up the sound.
	 *
	 * We will need the following and will use those from the prophet:
	 *	LFO
	 *	Two DCO
	 *	Filter
	 *	Two ENV
	 *	Amplifier
	 */
	baudio->soundCount = 9; /* Number of operators in this voice (MM) */
	/*
	 * Assign an array of sound pointers.
	 */
	baudio->sound = (bristolSound **)
		bristolmalloc0(sizeof(bristolOP *) * baudio->soundCount);
	baudio->effect = (bristolSound **)
		bristolmalloc0(sizeof(bristolOP *) * baudio->soundCount);

	/*
	 * Two OSC:
	 */
	initSoundAlgo(8, 0, baudio, audiomain, baudio->sound);
	initSoundAlgo(8, 1, baudio, audiomain, baudio->sound);
	/* LFO */
	initSoundAlgo(16, 2, baudio, audiomain, baudio->sound);
	/* An ADSR */
	initSoundAlgo(1, 3, baudio, audiomain, baudio->sound);
	/* A filter */
	initSoundAlgo(B_FILTER2, 4, baudio, audiomain, baudio->sound);
	/* Another ADSR */
	initSoundAlgo(1, 5, baudio, audiomain, baudio->sound);
	/* An amplifier */
	initSoundAlgo(2, 6, baudio, audiomain, baudio->sound);
	/* An noise source */
	initSoundAlgo(4, 7, baudio, audiomain, baudio->sound);
	/* Second LFO for mods */
	initSoundAlgo(16, 8, baudio, audiomain, baudio->sound);

	baudio->param = obxController;
	baudio->destroy = bristolOBXDestroy;
	baudio->operate = operateOneOBX;
	baudio->preops = operateOBXPreops;
	baudio->postops = operateOBXPostops;

	if (freq == 0)
		freq = (float *) bristolmalloc0(audiomain->segmentsize);
	if (freqbuf == 0)
		freqbuf = (float *) bristolmalloc0(audiomain->segmentsize);
	if (scratchbuf == 0)
		scratchbuf = (float *) bristolmalloc0(audiomain->segmentsize);
	if (adsrbuf == 0)
		adsrbuf = (float *) bristolmalloc0(audiomain->segmentsize);
	if (filtbuf == 0)
		filtbuf = (float *) bristolmalloc0(audiomain->segmentsize);
	if (oscbbuf == 0)
		oscbbuf = (float *) bristolmalloc0(audiomain->segmentsize);
	if (oscabuf == 0)
		oscabuf = (float *) bristolmalloc0(audiomain->segmentsize);

	if (lfo == 0)
		lfo = (float *) bristolmalloc0(audiomain->segmentsize);
	if (modsine == 0)
		modsine = (float *) bristolmalloc0(audiomain->segmentsize);
	if (lfosine == 0)
		lfosine = (float *) bristolmalloc0(audiomain->segmentsize);
	if (lfosqr == 0)
		lfosqr = (float *) bristolmalloc0(audiomain->segmentsize);
	if (lfosh == 0)
		lfosh = (float *) bristolmalloc0(audiomain->segmentsize);

	baudio->mixlocals = (float *) bristolmalloc0(sizeof(pmods));
	/*
	 * For the OB-X this should be central panning as it is a mono synth. For
	 * the OB-Xa this will be configured from the GUI
	 */
	((pmods *) baudio->mixlocals)->pan = 0.5f;
	((pmods *) baudio->mixlocals)->voicecount = baudio->voicecount;
	baudio->mixflags |= BRISTOL_STEREO;
	baudio->mixflags |= O_UNISON;
	return(0);
}

