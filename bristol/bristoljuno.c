
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
#include "bristoljuno.h"

/*
 * Use of these juno global buffers will be an issue with use of multiple
 * audio threads, unless we ensure a single thread deals with any given algo
 * type, since then they are only used sequentially.
 */
static float *freqbuf = (float *) NULL;
static float *pmodbuf = (float *) NULL;
static float *adsrbuf = (float *) NULL;
static float *filtbuf = (float *) NULL;
static float *oscbbuf = (float *) NULL;
static float *oscabuf = (float *) NULL;
static float *sbuf = (float *) NULL;

/*
 * These need to go into some local structure for multiple instances
 * of the juno - malloc()ed into the baudio->mixlocals.
 */
typedef struct jMods {
	float lfo_fgain;
	unsigned int flags;
	void *lfolocals;
	void *adsrlocals;
	int pwmod;
	float pwmodval;
	int envtype;
	float vcfmodval;
	int negenv;
	float env2vcf;
	float lfo2vcf;
	float key2vcf;
	float wheel2vcf;
	float level;
	float chgain;
	int chspeed;
} jmods;

extern int s440holder;

int
junoController(Baudio *baudio, u_char operator,
u_char controller, float value)
{
	int tval = value * C_RANGE_MIN_1;

#ifdef DEBUG
	printf("bristolJunoControl(%i, %i, %f)\n", operator, controller, value);
#endif

	/*
	 * These will be for our chorus.
	 */
	if (operator == 100)
	{
		switch (tval) {
			case 0:
				baudio->effect[0]->param->param[0].float_val = 0.2;
				baudio->effect[0]->param->param[1].float_val = 0.2;
				baudio->effect[0]->param->param[2].float_val = 0.2;
				baudio->effect[0]->param->param[3].float_val = 0.0;
				break;
			case 1:
				baudio->effect[0]->param->param[0].float_val = 0.005;
				baudio->effect[0]->param->param[1].float_val = 0.0168;
				baudio->effect[0]->param->param[2].float_val = 0.02;
				baudio->effect[0]->param->param[3].float_val = 0.65;
				break;
			case 2:
				baudio->effect[0]->param->param[0].float_val = 0.008;
				baudio->effect[0]->param->param[1].float_val = 0.016;
				baudio->effect[0]->param->param[2].float_val = 0.070;
				baudio->effect[0]->param->param[3].float_val = 0.65;
				break;
			case 3:
				baudio->effect[0]->param->param[0].float_val = 0.015;
				baudio->effect[0]->param->param[1].float_val = 0.015;
				baudio->effect[0]->param->param[2].float_val = 0.2;
				baudio->effect[0]->param->param[3].float_val = 0.65;
				break;
			case 4:
				baudio->effect[0]->param->param[0].float_val = 0.030;
				baudio->effect[0]->param->param[1].float_val = 0.014;
				baudio->effect[0]->param->param[2].float_val = 0.3;
				baudio->effect[0]->param->param[3].float_val = 0.65;
				break;
			case 5:
				baudio->effect[0]->param->param[0].float_val = 0.1;
				baudio->effect[0]->param->param[1].float_val = 0.013;
				baudio->effect[0]->param->param[2].float_val = 0.4;
				baudio->effect[0]->param->param[3].float_val = 0.65;
				break;
			case 6:
				baudio->effect[0]->param->param[0].float_val = 0.02;
				baudio->effect[0]->param->param[1].float_val = 0.012;
				baudio->effect[0]->param->param[2].float_val = 0.5;
				baudio->effect[0]->param->param[3].float_val = 0.75;
				break;
			case 7:
				baudio->effect[0]->param->param[0].float_val = 0.333;
				baudio->effect[0]->param->param[1].float_val = 0.011;
				baudio->effect[0]->param->param[2].float_val = 0.62;
				baudio->effect[0]->param->param[3].float_val = 0.75;
				break;
		}
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
			if (value == 0)
				((jmods *) baudio->mixlocals)->flags &= ~J_LFO_MAN;
			else
				((jmods *) baudio->mixlocals)->flags |= J_LFO_MAN;
			break;
		case 4:
			if (value == 0)
				((jmods *) baudio->mixlocals)->flags &= ~J_LFO_AUTO;
			else
				((jmods *) baudio->mixlocals)->flags |= J_LFO_AUTO;
			break;
		case 5:
			((jmods *) baudio->mixlocals)->lfo_fgain = value / 128;
			break;
		case 6:
			((jmods *) baudio->mixlocals)->pwmod =
				value * CONTROLLER_RANGE;
			break;
		case 7:
			((jmods *) baudio->mixlocals)->pwmodval = value;
			break;
		case 8:
			if (value == 0)
				((jmods *) baudio->mixlocals)->envtype = 0;
			else
				((jmods *) baudio->mixlocals)->envtype = 1;
			break;
		case 9:
			baudio->midi_pitch = value * 24;
			break;
		case 10:
			((jmods *) baudio->mixlocals)->vcfmodval = value;
			break;
		case 11:
			if (value == 0)
				((jmods *) baudio->mixlocals)->negenv = 0;
			else
				((jmods *) baudio->mixlocals)->negenv = 1;
			break;
		case 12:
			((jmods *) baudio->mixlocals)->env2vcf = value / 4;
			break;
		case 13:
			((jmods *) baudio->mixlocals)->lfo2vcf = value * 0.01;
			break;
		case 14:
			((jmods *) baudio->mixlocals)->key2vcf = value / 16;
			break;
		case 15:
			((jmods *) baudio->mixlocals)->level = value * 2;
			break;
		case 16:
			((jmods *) baudio->mixlocals)->wheel2vcf = value;
			break;
		case 17:
			if (value == 0)
				baudio->mixflags &= ~MASTER_ONOFF;
			else
				baudio->mixflags |= MASTER_ONOFF;
			break;
	}
	return(0);
}

int
junoPreops(audioMain *audiomain, Baudio *baudio,
bristolVoice *voice, register float *startbuf)
{
	register int samplecount = audiomain->samplecount, i;
	register float rate = baudio->contcontroller[1] / 2;
	int flags;

	bristolbzero(oscbbuf, audiomain->segmentsize);

	if (((jmods *) baudio->mixlocals)->lfolocals == 0)
	{
		((jmods *) baudio->mixlocals)->lfolocals =
			baudio->locals[voice->index][0];
		((jmods *) baudio->mixlocals)->adsrlocals = 
			voice->locals[voice->index][7];
	}

	flags = voice->flags;
	/*
	 * If we are configured for auto, only do autoops.
	 */
	if (((jmods *) baudio->mixlocals)->flags & J_LFO_AUTO)
	{
		/*
		 * If we are not active, and we get a key on, or reon, start the
		 * envelope for LFO delay.
		 *
		 * If we have not started, but we have a key start, that is great.
		 * If we have started, and we have a keyon/reon, remove it.
		 */
		if ((((jmods *) baudio->mixlocals)->flags & HAVE_STARTED)
			&& (voice->flags & (BRISTOL_KEYON|BRISTOL_KEYREON))
			&& (baudio->lvoices != 0))
			voice->flags &= ~(BRISTOL_KEYON|BRISTOL_KEYREON);

		if ((voice->flags & (BRISTOL_KEYON|BRISTOL_KEYREON))
			&& (baudio->lvoices == 0))
		{
/*printf("lfo auto on\n"); */
			((jmods *) baudio->mixlocals)->flags |= HAVE_STARTED;
		}

		/*
		 * We only accept key off if this is the last voice on the list.
		 */
		if ((((jmods *) baudio->mixlocals)->flags & HAVE_STARTED)
			&& (voice->flags & BRISTOL_KEYOFF))
		{
			if (baudio->lvoices > 1)
			{
				voice->flags &= ~BRISTOL_KEYOFF;
			} else {
/*printf("lfo auto off\n"); */
				((jmods *) baudio->mixlocals)->flags &= ~HAVE_STARTED;
			}
		}

/*printf("auto mode: %x, %x, %i\n", ((jmods *) baudio->mixlocals)->flags, */
/*voice->flags, voice->index); */
	} else if (((jmods *) baudio->mixlocals)->flags & J_LFO_MAN) {
/*printf("manual mode: %x\n", ((jmods *) baudio->mixlocals)->flags); */
		/*
		 * In man mode, if we have J_LFO_MAN and not started, then configure a
		 * keyon operation.
		 */
		if ((((jmods *) baudio->mixlocals)->flags & HAVE_STARTED) == 0)
		{
			((jmods *) baudio->mixlocals)->flags |= HAVE_STARTED;
/*printf("lfo manual on\n"); */
			voice->flags |= BRISTOL_KEYON;
		}
	} else {
		voice->flags |= BRISTOL_KEYOFF;
		((jmods *) baudio->mixlocals)->flags &= ~HAVE_STARTED;
/*printf("lfo manual off\n"); */
	}

	/*
	 * Put the correct buffer index into the oscillator freqbuf now.
	 */
	audiomain->palette[(*baudio->sound[0]).index]->specs->io[0].buf = startbuf;
	/*
	 * Fill a freq table for the LFO.
	 */
	for (i = 0; i < samplecount; i+=8)
	{
		*startbuf++ = 0.0025 + rate;
		*startbuf++ = 0.0025 + rate;
		*startbuf++ = 0.0025 + rate;
		*startbuf++ = 0.0025 + rate;
		*startbuf++ = 0.0025 + rate;
		*startbuf++ = 0.0025 + rate;
		*startbuf++ = 0.0025 + rate;
		*startbuf++ = 0.0025 + rate;
	}
	/*
	 * Run an ADSR for the LFO amplifier.
	 */
	audiomain->palette[(*baudio->sound[7]).index]->specs->io[0].buf = adsrbuf;
	(*baudio->sound[7]).operate(
		(audiomain->palette)[1],
		voice,
		(*baudio->sound[7]).param,
		((jmods *) baudio->mixlocals)->adsrlocals);

	bristolbzero(oscabuf, audiomain->segmentsize);
	/*
	 * Run the LFO into the oscbbuf
	 */
	audiomain->palette[(*baudio->sound[0]).index]->specs->io[1].buf = oscabuf;
	(*baudio->sound[0]).operate(
		(audiomain->palette)[0],
		voice,
		(*baudio->sound[0]).param,
		((jmods *) baudio->mixlocals)->lfolocals);

	/*
	 * And amplifly it 
	 */
	audiomain->palette[(*baudio->sound[8]).index]->specs->io[0].buf = oscabuf;
	audiomain->palette[(*baudio->sound[8]).index]->specs->io[1].buf = adsrbuf;
	audiomain->palette[(*baudio->sound[8]).index]->specs->io[2].buf = oscbbuf;
	(*baudio->sound[8]).operate(
		(audiomain->palette)[2],
		voice,
		(*baudio->sound[8]).param,
		baudio->locals[voice->index][8]);

/*printf("%f %f, %f %f\n", adsrbuf[0], adsrbuf[1], oscabuf[0], oscabuf[1]); */

	voice->flags = flags;

	return(0);
}

int
operateOneJuno(audioMain *audiomain, Baudio *baudio,
bristolVoice *voice, register float *startbuf)
{
	/*
	 * Master ON/OFF is an audiomain flags, but it is typically posted to the
	 * baudio structure.
	 */
	if ((voice->baudio->mixflags & MASTER_ONOFF) == 0)
	{
#ifdef DEBUG
		if ((audiomain->debuglevel & BRISTOL_DEBUG_MASK) > BRISTOL_DEBUG5)
			printf("Juno Master OFF\n");
#endif
		return(0);
	}

	bristolbzero(pmodbuf, audiomain->segmentsize);
	bristolbzero(filtbuf, audiomain->segmentsize);
	bristolbzero(sbuf, audiomain->segmentsize);
	bristolbzero(oscabuf, audiomain->segmentsize);

	/*
	 * Run the main ADSR.
	 */
	audiomain->palette[(*baudio->sound[4]).index]->specs->io[0].buf = adsrbuf;
	(*baudio->sound[4]).operate(
		(audiomain->palette)[1],
		voice,
		(*baudio->sound[4]).param,
		voice->locals[voice->index][4]);

	/*
	 * Build a frequency table for the DCO.
	 */
	fillFreqTable(baudio, voice, freqbuf, audiomain->samplecount, 1);
	bufmerge(oscbbuf, ((jmods *) baudio->mixlocals)->lfo_fgain
		+ (voice->press + voice->chanpressure),
		freqbuf, 1.0, audiomain->samplecount);

	/*
	 * Put in some PWM modulations. Range up to 500.
	 */
	switch (((jmods *) baudio->mixlocals)->pwmod) {
		default:
		{
			register int i, samplecount = audiomain->samplecount;
			register float value, *modbuf = pmodbuf;

			/*
			 * MAN
			 */
			value = ((jmods *) baudio->mixlocals)->pwmodval * 500;

			for (i = 0; i < samplecount; i+=8)
			{
				*modbuf++ = value;
				*modbuf++ = value;
				*modbuf++ = value;
				*modbuf++ = value;
				*modbuf++ = value;
				*modbuf++ = value;
				*modbuf++ = value;
				*modbuf++ = value;
			}
			break;
		}
		case 1:
		{
			register int i, samplecount = audiomain->samplecount;
			register float *modbuf = pmodbuf;

			/* LFO - needs normalising */
			bufmerge(oscbbuf, ((jmods *) baudio->mixlocals)->pwmodval * 3,
				pmodbuf, 0.0, audiomain->samplecount);
			break;
			for (i = 0; i < samplecount; i+=8)
			{
				*modbuf++ += 256;
				*modbuf++ += 256;
				*modbuf++ += 256;
				*modbuf++ += 256;
				*modbuf++ += 256;
				*modbuf++ += 256;
				*modbuf++ += 256;
				*modbuf++ += 256;
			}
			break;
		}
		case 2:
			/* env */
			bufmerge(adsrbuf, ((jmods *) baudio->mixlocals)->pwmodval * 40,
				pmodbuf, 0.0, audiomain->samplecount);
			break;
	}
	audiomain->palette[(*baudio->sound[1]).index]->specs->io[0].buf = freqbuf;
	audiomain->palette[(*baudio->sound[1]).index]->specs->io[1].buf = pmodbuf;
	audiomain->palette[(*baudio->sound[1]).index]->specs->io[2].buf = oscabuf;
	(*baudio->sound[1]).operate(
		(audiomain->palette)[11],
		voice,
		(*baudio->sound[1]).param,
		voice->locals[voice->index][1]);

	/* 
	 * Put the noise into the same oscillator buf.
	 */
	audiomain->palette[(*baudio->sound[6]).index]->specs->io[0].buf = oscabuf;
	(*baudio->sound[6]).operate(
		(audiomain->palette)[4],
		voice,
		(*baudio->sound[6]).param,
		voice->locals[voice->index][6]);

	/*
	 * Put the HPF filter in.
	 */
	audiomain->palette[(*baudio->sound[2]).index]->specs->io[0].buf = oscabuf;
	audiomain->palette[(*baudio->sound[2]).index]->specs->io[1].buf = sbuf;
	(*baudio->sound[2]).operate(
		(audiomain->palette)[10],
		voice,
		(*baudio->sound[2]).param,
		voice->locals[voice->index][2]);

	/* This is EQ to drive into the filter with a normalised gain */
	bufmerge(adsrbuf, 0, sbuf, 48.0, audiomain->samplecount);
	bristolbzero(pmodbuf, audiomain->segmentsize);

	/*
	 * Put all our VCF mods in here. They should be normalised to 1.0 range.
	 */
	if (((jmods *) baudio->mixlocals)->negenv)
	{
		register int i, samplecount = audiomain->samplecount;
		register float *modbuf = pmodbuf;

		for (i = 0; i < samplecount; i+=8)
		{
			*modbuf++ = 1;
			*modbuf++ = 1;
			*modbuf++ = 1;
			*modbuf++ = 1;
			*modbuf++ = 1;
			*modbuf++ = 1;
			*modbuf++ = 1;
			*modbuf++ = 1;
		}

		bufmerge(adsrbuf, -((jmods *) baudio->mixlocals)->env2vcf * 12.0,
			pmodbuf, 0.0, audiomain->samplecount);
	} else {
		bufmerge(adsrbuf, ((jmods *) baudio->mixlocals)->env2vcf * 12.0,
			pmodbuf, 0.0, audiomain->samplecount);
	}
	/*
	 * Add in the LFO
	 */
	bufmerge(oscbbuf, ((jmods *) baudio->mixlocals)->lfo2vcf,
		pmodbuf, 1.0, audiomain->samplecount);
	/*
	 * The keyboard
	bufmerge(oscbbuf, ((jmods *) baudio->mixlocals)->lfo2vcf,
		pmodbuf, 1.0, audiomain->samplecount);
	 */
	{
		register int i, samplecount = audiomain->samplecount;
		register float *modbuf = pmodbuf, value;

		value = baudio->pitchwheel * 8.0 *
			((jmods *) baudio->mixlocals)->wheel2vcf;

		for (i = 0; i < samplecount; i+=8)
		{
			*modbuf++ += value;
			*modbuf++ += value;
			*modbuf++ += value;
			*modbuf++ += value;
			*modbuf++ += value;
			*modbuf++ += value;
			*modbuf++ += value;
			*modbuf++ += value;
		}
	}
	/*
	 * Put the VCF filter in here.
	 */
	audiomain->palette[(*baudio->sound[3]).index]->specs->io[0].buf = sbuf;
	audiomain->palette[(*baudio->sound[3]).index]->specs->io[1].buf = pmodbuf;
	audiomain->palette[(*baudio->sound[3]).index]->specs->io[2].buf = filtbuf;
	(*baudio->sound[3]).operate(
		(audiomain->palette)[3],
		voice,
		(*baudio->sound[3]).param,
		voice->locals[voice->index][3]);

	/*
	 * And merge this into the outbuf via the amplifier.
	 */
	audiomain->palette[(*baudio->sound[5]).index]->specs->io[1].buf = adsrbuf;
	if (((jmods *) baudio->mixlocals)->envtype)
	{
		register int i, samplecount = audiomain->samplecount;
		register float *buf = adsrbuf, value, diff = 0;

		value = voice->velocity * ((jmods *) baudio->mixlocals)->level;

		if (voice->flags & BRISTOL_KEYDONE)
		{
			samplecount -= 16;
			diff = value / 16;
		}

		for (i = 0; i < samplecount; i+=8)
		{
			*buf++ = value;
			*buf++ = value;
			*buf++ = value;
			*buf++ = value;
			*buf++ = value;
			*buf++ = value;
			*buf++ = value;
			*buf++ = value;
		}
		if (voice->flags & BRISTOL_KEYDONE)
		{
			*buf++ = (value -= diff);
			*buf++ = (value -= diff);
			*buf++ = (value -= diff);
			*buf++ = (value -= diff);
			*buf++ = (value -= diff);
			*buf++ = (value -= diff);
			*buf++ = (value -= diff);
			*buf++ = (value -= diff);
			*buf++ = (value -= diff);
			*buf++ = (value -= diff);
			*buf++ = (value -= diff);
			*buf++ = (value -= diff);
			*buf++ = (value -= diff);
			*buf++ = (value -= diff);
			*buf++ = (value -= diff);
			*buf++ = (value -= diff);
		}
	} else {
		bufmerge(oscbbuf, 0.0,
			adsrbuf, ((jmods *) baudio->mixlocals)->level * voice->velocity,
			audiomain->samplecount);
	}
	audiomain->palette[(*baudio->sound[5]).index]->specs->io[0].buf = filtbuf;
	audiomain->palette[(*baudio->sound[5]).index]->specs->io[2].buf =
		baudio->leftbuf;

	(*baudio->sound[5]).operate(
		(audiomain->palette)[2],
		voice,
		(*baudio->sound[5]).param,
		voice->locals[voice->index][5]);

	return(0);
}

int
static bristolJunoDestroy(audioMain *audiomain, Baudio *baudio)
{
printf("removing one juno\n");
	return(0);
	bristolfree(freqbuf);
	bristolfree(sbuf);
	bristolfree(pmodbuf);
	bristolfree(adsrbuf);
	bristolfree(filtbuf);
	bristolfree(oscbbuf);
	bristolfree(oscabuf);

	return(0);
}

int
bristolJunoInit(audioMain *audiomain, Baudio *baudio)
{
printf("initialising one juno\n");
	baudio->soundCount = 9; /* Number of operators in this voice */
	/*
	 * Assign an array of sound pointers.
	 */
	baudio->sound = (bristolSound **)
		bristolmalloc0(sizeof(bristolOP *) * baudio->soundCount);
	baudio->effect = (bristolSound **)
		bristolmalloc0(sizeof(bristolOP *) * baudio->soundCount);

	/* LFO */
	initSoundAlgo(0, 0, baudio, audiomain, baudio->sound);
	/* Main oscillator */
	initSoundAlgo(11, 1, baudio, audiomain, baudio->sound);
	/* HPF */
	initSoundAlgo(10, 2, baudio, audiomain, baudio->sound);
	/* A filter */
	initSoundAlgo(3, 3, baudio, audiomain, baudio->sound);
	/* Another ADSR */
	initSoundAlgo(1, 4, baudio, audiomain, baudio->sound);
	/* An amplifier */
	initSoundAlgo(2, 5, baudio, audiomain, baudio->sound);
	/* An noise source */
	initSoundAlgo(4, 6, baudio, audiomain, baudio->sound);
	/* LFO ADSR */
	initSoundAlgo(1, 7, baudio, audiomain, baudio->sound);
	/* LFO amplifier */
	initSoundAlgo(2, 8, baudio, audiomain, baudio->sound);

	baudio->param = junoController;
	baudio->destroy = bristolJunoDestroy;
	baudio->operate = operateOneJuno;
	baudio->preops = junoPreops;

	/*
	 * Put in a vibrachorus on our effects list.
	 */
	initSoundAlgo(12, 0, baudio, audiomain, baudio->effect);

	if (freqbuf == NULL)
		freqbuf = (float *) bristolmalloc0(audiomain->segmentsize);
	if (sbuf == NULL)
		sbuf = (float *) bristolmalloc0(audiomain->segmentsize);
	if (pmodbuf == NULL)
		pmodbuf = (float *) bristolmalloc0(audiomain->segmentsize);
	if (adsrbuf == NULL)
		adsrbuf = (float *) bristolmalloc0(audiomain->segmentsize);
	if (filtbuf == NULL)
		filtbuf = (float *) bristolmalloc0(audiomain->segmentsize);
	if (oscbbuf == NULL)
		oscbbuf = (float *) bristolmalloc0(audiomain->segmentsize);
	if (oscabuf == NULL)
		oscabuf = (float *) bristolmalloc0(audiomain->segmentsize);

	baudio->mixlocals = (float *) bristolmalloc0(sizeof(jmods));

	return(0);
}

