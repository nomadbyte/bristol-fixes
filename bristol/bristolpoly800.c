
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

/* Korg Poly800 algo */

/*#define DEBUG */

#include "bristol.h"
#include "bristolmm.h"
#include "bristolpoly800.h"

/*
 * Use of these poly800 global buffers will be an issue with use of multiple
 * audio threads, unless we ensure a single thread deals with any given algo
 * type, since then they are only used sequentially.
 */

int
poly800Controller(Baudio *baudio, u_char operator, u_char controller, float value)
{
//	int ivalue = value * CONTROLLER_RANGE;

#ifdef DEBUG
	printf("bristolPoly800Control(%i, %i, %f)\n", operator, controller, value);
#endif

	/*
	 * These will be for our chorus.
	 */
	if (operator == 100)
	{
		if (baudio->effect[0] == NULL)
			return(0);

		switch (controller) {
			case 0:
				baudio->effect[0]->param->param[controller].float_val = value;
					(0.1 + (value * value) * 20) * 1024 / baudio->samplerate;
				break;
			case 1:
			case 2:
			case 3:
				baudio->effect[0]->param->param[controller].float_val = value;
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
		case 2:
			P800LOCAL->volume = value * 4;
			break;
		case 3:
			if (value < 0.5)
				P800LOCAL->bend = 1.0 - value;
			else
				P800LOCAL->bend = 1.0 + (value - 0.5) * 2;
			break;
		case 4:
			P800LOCAL->dco1pwm = value * 0.5;
			break;
		case 5:
			P800LOCAL->dco2pwm = value * 0.5;
			break;
		case 6:
			P800LOCAL->mod2vcodepth = 0.1 + value * 1.9;
			break;
		case 10:
			if (value == 0) {
				baudio->mixflags &= ~P800_DOUBLE;
				baudio->voicecount = P800LOCAL->voicecount;
			} else {
				baudio->mixflags |= P800_DOUBLE;
				baudio->voicecount = P800LOCAL->voicecount / 2;
			}
			break;
		case 11:
			if (value == 0)
				baudio->mixflags &= ~P800_LFOMULTI;
			else
				baudio->mixflags |= P800_LFOMULTI;
			break;
		case 12:
			if (value == 0)
				baudio->mixflags &= ~P800_FILTMULTI;
			else
				baudio->mixflags |= P800_FILTMULTI;
			break;
		case 13:
			if (value == 0)
				baudio->mixflags &= ~P800_SYNC;
			else
				baudio->mixflags |= P800_SYNC;
			break;
		case 20:
			if (value != 0) {
				baudio->mixflags &= ~P800_POLARITY;
				if (P800LOCAL->envamount < 0)
					P800LOCAL->envamount = -P800LOCAL->envamount;
			} else {
				baudio->mixflags |= P800_POLARITY;
				if (P800LOCAL->envamount > 0)
					P800LOCAL->envamount = -P800LOCAL->envamount;
			}
			break;
		case 21:
			if (baudio->mixflags & P800_POLARITY)
				P800LOCAL->envamount = -value * 2;
			else
				P800LOCAL->envamount = value * 2;
			break;
		case 22:
			P800LOCAL->vcomod = value * value * 4;
			break;
		case 23:
			P800LOCAL->vcfmod = value * value * 8;
			break;
		case 24:
			if ((P800LOCAL->lfodelay = value * baudio->samplerate * 15) == 0)
				P800LOCAL->lforate = 1;
			else
				P800LOCAL->lforate = 1 / (value * baudio->samplerate * 15);
			break;
	}
	return(0);
}

static float
fillLFOgainTable(float *buf, float current, float slope, float target, int i)
{
	for (; i > 0; i-=4)
	{
		if ((current += slope) > target) current = target; *buf++ = current;
		if ((current += slope) > target) current = target; *buf++ = current;
		if ((current += slope) > target) current = target; *buf++ = current;
		if ((current += slope) > target) current = target; *buf++ = current;
	}
	return(current);
}

int
poly800Preops(audioMain *audiomain, Baudio *baudio,
bristolVoice *voice, register float *startbuf)
{
	/*
	 * Preops will do the LFO and noise, unless multi.
	 */

	P800LOCAL->maxkey = voice->index;
	P800LOCAL->maxvoice = voice;

	if ((baudio->mixflags & P800_LFOMULTI) == 0)
	{
		/*
		 * This needs to be done legato style here.
		 */
		if ((voice->flags & (BRISTOL_KEYON|BRISTOL_KEYREON))
			&& (baudio->lvoices == 0))
		{
			/* Put in up to 10s of delay */
			P800LOCAL->lfodelays[0] = P800LOCAL->lfodelay;
			P800LOCAL->lfogains[0] = 0;
		}

		if ((P800LOCAL->lfodelays[0] -= audiomain->samplecount) <= 0)
		{
			P800LOCAL->lfodelays[0] = 0;

			/*
			 * Operate LFO if we have a single LFO per synth.
			 */
			audiomain->palette[B_LFO]->specs->io[1].buf
				= audiomain->palette[B_LFO]->specs->io[2].buf
				= audiomain->palette[B_LFO]->specs->io[3].buf
				= audiomain->palette[B_LFO]->specs->io[5].buf
				= audiomain->palette[B_LFO]->specs->io[6].buf
					= NULL;

			audiomain->palette[B_LFO]->specs->io[0].buf = P800LOCAL->zerobuf;
			audiomain->palette[B_LFO]->specs->io[4].buf = P800LOCAL->oscabuf;

			(*baudio->sound[8]).operate(
				(audiomain->palette)[B_LFO],
				voice,
				(*baudio->sound[8]).param,
				baudio->locals[0][8]);

			bufadd(P800LOCAL->oscabuf, 1.0, audiomain->samplecount);

			bristolbzero(P800LOCAL->lfobuf, audiomain->segmentsize);

			P800LOCAL->lfogains[0] =
				fillLFOgainTable(P800LOCAL->adsrbuf, P800LOCAL->lfogains[0],	
					P800LOCAL->lforate, 1.0, audiomain->samplecount);

			audiomain->palette[B_DCA]->specs->io[0].buf = P800LOCAL->oscabuf;
			audiomain->palette[B_DCA]->specs->io[1].buf = P800LOCAL->adsrbuf;
			audiomain->palette[B_DCA]->specs->io[2].buf = P800LOCAL->lfobuf;

			(*baudio->sound[7]).operate(
				(audiomain->palette)[B_DCA],
				voice,
				(*baudio->sound[7]).param,
				baudio->locals[0][7]);
		} else
			bristolbzero(P800LOCAL->lfobuf, audiomain->segmentsize);

		bristolbzero(P800LOCAL->noisebuf, audiomain->segmentsize);
		audiomain->palette[B_NOISE]->specs->io[0].buf = P800LOCAL->noisebuf;
		(*baudio->sound[3]).operate(
			(audiomain->palette)[B_NOISE],
			voice,
			(*baudio->sound[3]).param,
			voice->locals[0][3]);
	}

	/* We only need to do this once for all voices */
	if (baudio->contcontroller[1] < 0.5) {
		P800LOCAL->mod2vcf = 0;
		P800LOCAL->mod2vco = (0.5 - baudio->contcontroller[1]) * 
			P800LOCAL->mod2vcodepth;
	} else {
		P800LOCAL->mod2vco = 0;
		P800LOCAL->mod2vcf = (baudio->contcontroller[1] - 0.5) * 4;
	}

	return(0);
}

/*
 * This looks a little specific, it is used here to apply the normalised LFO
 * to the frequency buffer - as a multiplier. That maintains frequency mod over
 * different notes. It could be made a little more general such as all freq 
 * mods including ADSR for the prophet, etc
 */
static void
mult2buf(register float *d, register float *s, register float v, register int c)
{
	for (; c > 0; c-=8)
	{
		*d = (1.0 + *s++) * *d * v; d++;
		*d = (1.0 + *s++) * *d * v; d++;
		*d = (1.0 + *s++) * *d * v; d++;
		*d = (1.0 + *s++) * *d * v; d++;
		*d = (1.0 + *s++) * *d * v; d++;
		*d = (1.0 + *s++) * *d * v; d++;
		*d = (1.0 + *s++) * *d * v; d++;
		*d = (1.0 + *s++) * *d * v; d++;
	}
}

int
operateOnePoly800(audioMain *audiomain, Baudio *baudio,
bristolVoice *voice, register float *startbuf)
{
	unsigned int flags = 0;
/*printf("operateOnePoly800(%i, %x, %x)\n", voice->index, audiomain, baudio); */

	if (baudio->mixflags & P800_LFOMULTI)
	{
		/*
		 * This needs to be done legato style here.
		 */
		if (voice->flags & (BRISTOL_KEYON|BRISTOL_KEYREON))
		{
			/* Put in up to 10s of delay */
			P800LOCAL->lfodelays[voice->index] = P800LOCAL->lfodelay;
			P800LOCAL->lfogains[voice->index] = 0;
		}

		if ((P800LOCAL->lfodelays[voice->index] -= audiomain->samplecount) <= 0)
		{
			P800LOCAL->lfodelays[voice->index] = 0;

			/*
			 * Operate LFO if we have a single LFO per synth.
			 */
			audiomain->palette[B_LFO]->specs->io[1].buf
				= audiomain->palette[B_LFO]->specs->io[2].buf
				= audiomain->palette[B_LFO]->specs->io[3].buf
				= audiomain->palette[B_LFO]->specs->io[5].buf
				= audiomain->palette[B_LFO]->specs->io[6].buf
					= NULL;

			audiomain->palette[B_LFO]->specs->io[0].buf = P800LOCAL->zerobuf;
			audiomain->palette[B_LFO]->specs->io[4].buf = P800LOCAL->oscabuf;

			(*baudio->sound[8]).operate(
				(audiomain->palette)[B_LFO],
				voice,
				(*baudio->sound[8]).param,
				baudio->locals[voice->index][8]);

			bufadd(P800LOCAL->oscabuf, 1.0, audiomain->samplecount);

			bristolbzero(P800LOCAL->lfobuf, audiomain->segmentsize);

			P800LOCAL->lfogains[voice->index] =
				fillLFOgainTable(P800LOCAL->adsrbuf,
					P800LOCAL->lfogains[voice->index],	
					P800LOCAL->lforate, 1.0, audiomain->samplecount);

			audiomain->palette[B_DCA]->specs->io[0].buf = P800LOCAL->oscabuf;
			audiomain->palette[B_DCA]->specs->io[1].buf = P800LOCAL->adsrbuf;
			audiomain->palette[B_DCA]->specs->io[2].buf = P800LOCAL->lfobuf;

			(*baudio->sound[7]).operate(
				(audiomain->palette)[B_DCA],
				voice,
				(*baudio->sound[7]).param,
				baudio->locals[voice->index][7]);
		} else
			bristolbzero(P800LOCAL->lfobuf, audiomain->segmentsize);

		bristolbzero(P800LOCAL->noisebuf, audiomain->segmentsize);
		audiomain->palette[B_NOISE]->specs->io[0].buf = P800LOCAL->noisebuf;
		(*baudio->sound[3]).operate(
			(audiomain->palette)[B_NOISE],
			voice,
			(*baudio->sound[3]).param,
			voice->locals[voice->index][3]);
	}

/* Osc-1 */
	/*
	 * This will do 1 osc with env, unless multi when it does two. Will track
	 * the highest note for the filter cutoff point for when filter is not 
	 * MULTI.
	 */
	fillFreqBuf(baudio, voice, P800LOCAL->freqbuf, audiomain->samplecount, 1);

	bristolbzero(P800LOCAL->oscabuf, audiomain->segmentsize);
	bristolbzero(P800LOCAL->syncbuf, audiomain->segmentsize);

	audiomain->palette[B_NRO]->specs->io[0].buf = P800LOCAL->freqbuf;
	audiomain->palette[B_NRO]->specs->io[1].buf = P800LOCAL->oscabuf;
	audiomain->palette[B_NRO]->specs->io[2].buf = P800LOCAL->pwmbuf;
	audiomain->palette[B_NRO]->specs->io[3].buf = P800LOCAL->zerobuf; /* Sync */
	audiomain->palette[B_NRO]->specs->io[4].buf = P800LOCAL->syncbuf; /* Sout */

	bufmerge(P800LOCAL->lfobuf, P800LOCAL->dco1pwm,
		P800LOCAL->pwmbuf, 0.0, audiomain->samplecount);

	/* This seems odd, but if we have SYNC enabled only do LFO mods to DCO-2 */
	if ((baudio->mixflags & P800_SYNC) == 0)
	{
		bufmerge(P800LOCAL->lfobuf, P800LOCAL->vcomod,
			P800LOCAL->scratch, 0.0, audiomain->samplecount);
		mult2buf(P800LOCAL->freqbuf, P800LOCAL->scratch, 1.0,
			audiomain->samplecount);
	}

	bufmerge(P800LOCAL->lfobuf, P800LOCAL->mod2vco,
		P800LOCAL->scratch, 0.0, audiomain->samplecount);
	mult2buf(P800LOCAL->freqbuf, P800LOCAL->scratch, 1.0,
		audiomain->samplecount);

	(*baudio->sound[0]).operate(
		(audiomain->palette)[B_NRO],
		voice,
		(*baudio->sound[0]).param,
		voice->locals[voice->index][0]);

	audiomain->palette[B_ENV5S]->specs->io[0].buf = P800LOCAL->adsrbuf;
	(*baudio->sound[4]).operate(
		(audiomain->palette)[B_ENV5S],
		voice,
		(*baudio->sound[4]).param,
		voice->locals[voice->index][4]);

	audiomain->palette[B_DCA]->specs->io[0].buf = P800LOCAL->oscabuf;
	audiomain->palette[B_DCA]->specs->io[1].buf = P800LOCAL->adsrbuf;
	audiomain->palette[B_DCA]->specs->io[2].buf = P800LOCAL->outbuf;
	(*baudio->sound[7]).operate(
		(audiomain->palette)[B_DCA],
		voice,
		(*baudio->sound[7]).param,
		baudio->locals[voice->index][7]);

	flags = voice->flags & BRISTOL_KEYDONE;
/* Osc-1 DONE */

/* Osc-2 */
	if (baudio->mixflags & P800_DOUBLE)
	{
		bristolbzero(P800LOCAL->oscbbuf, audiomain->segmentsize);

		audiomain->palette[B_NRO]->specs->io[0].buf = P800LOCAL->freqbuf;
		audiomain->palette[B_NRO]->specs->io[1].buf = P800LOCAL->oscbbuf;
		audiomain->palette[B_NRO]->specs->io[2].buf = P800LOCAL->pwmbuf;

		if (baudio->mixflags & P800_SYNC) {
			/*
			 * If sync is enabled then the LFO was not fead into DCO1 but we
			 * do want it in DCO-2 for sync modulations. It is still possible
			 * to get vibrato on DCO1 with sync but it requires the joystick.
			 */
			bufmerge(P800LOCAL->lfobuf, P800LOCAL->vcomod,
				P800LOCAL->scratch, 0.0, audiomain->samplecount);
			mult2buf(P800LOCAL->freqbuf, P800LOCAL->scratch, 1.0,
				audiomain->samplecount);
			audiomain->palette[B_NRO]->specs->io[3].buf = P800LOCAL->syncbuf;
		} else
			audiomain->palette[B_NRO]->specs->io[3].buf = P800LOCAL->zerobuf;
		audiomain->palette[B_NRO]->specs->io[4].buf = NULL; /* S-out */

		(*baudio->sound[1]).operate(
			(audiomain->palette)[B_NRO],
			voice,
			(*baudio->sound[1]).param,
			voice->locals[voice->index][1]);

		audiomain->palette[B_ENV5S]->specs->io[0].buf = P800LOCAL->adsrbuf;
		(*baudio->sound[5]).operate(
			(audiomain->palette)[B_ENV5S],
			voice,
			(*baudio->sound[5]).param,
			voice->locals[voice->index][5]);

		/*
		 * We only want a key off event when both envs have terminated
		 */
		if (flags == 0)
			voice->flags &= ~BRISTOL_KEYDONE;

		bufmerge(P800LOCAL->lfobuf, P800LOCAL->dco2pwm,
			P800LOCAL->pwmbuf, 0.0, audiomain->samplecount);

		audiomain->palette[B_DCA]->specs->io[0].buf = P800LOCAL->oscbbuf;
		audiomain->palette[B_DCA]->specs->io[1].buf = P800LOCAL->adsrbuf;
		audiomain->palette[B_DCA]->specs->io[2].buf = P800LOCAL->outbuf;
		(*baudio->sound[7]).operate(
			(audiomain->palette)[B_DCA],
			voice,
			(*baudio->sound[7]).param,
			baudio->locals[voice->index][7]);
	}
/* Osc-2 DONE */

	bufmerge(P800LOCAL->outbuf, 7.0,
		P800LOCAL->outbuf, 1.0, audiomain->samplecount);

/* Env-3, filter (MULTI supported) */
	if (baudio->mixflags & P800_FILTMULTI) {
		flags = voice->flags;
		audiomain->palette[B_ENV5S]->specs->io[0].buf = P800LOCAL->adsrbuf;
		(*baudio->sound[6]).operate(
			(audiomain->palette)[B_ENV5S],
			voice,
			(*baudio->sound[6]).param,
			voice->locals[voice->index][6]);
		voice->flags = flags;

		bufmerge(P800LOCAL->adsrbuf, P800LOCAL->envamount,
			P800LOCAL->scratch, 0.0, audiomain->samplecount);
		bufmerge(P800LOCAL->lfobuf,
			P800LOCAL->vcfmod + P800LOCAL->mod2vcf,
			P800LOCAL->scratch, 1.0, audiomain->samplecount);

		/* Amp the noise into the output */
		audiomain->palette[B_DCA]->specs->io[0].buf = P800LOCAL->noisebuf;
		audiomain->palette[B_DCA]->specs->io[1].buf = P800LOCAL->adsrbuf;
		audiomain->palette[B_DCA]->specs->io[2].buf = P800LOCAL->outbuf;
		(*baudio->sound[7]).operate(
			(audiomain->palette)[B_DCA],
			voice,
			(*baudio->sound[7]).param,
			baudio->locals[voice->index][7]);

		audiomain->palette[B_FILTER]->specs->io[0].buf = P800LOCAL->outbuf;
		audiomain->palette[B_FILTER]->specs->io[1].buf = P800LOCAL->scratch;
		audiomain->palette[B_FILTER]->specs->io[2].buf = baudio->leftbuf;
		(*baudio->sound[2]).operate(
			(audiomain->palette)[B_FILTER],
			voice,
			(*baudio->sound[2]).param,
			voice->locals[voice->index][2]);

		bristolbzero(P800LOCAL->outbuf, audiomain->segmentsize);
	} else if (P800LOCAL->maxkey < voice->key.key) {
		P800LOCAL->maxkey = voice->key.key;
		P800LOCAL->maxvoice = voice;
	}

	return(0);
}

int
poly800PostOps(audioMain *audiomain, Baudio *baudio,
bristolVoice *voice, register float *startbuf)
{
	/*
	 * This will do the Filter, unless multi. If done here we should be
	 * careful with the envelope flags since this one does not control the
	 * voice.
	 */
/* Env-3, filter (MULTI supported) */
	/*
	 * This jumps and clicks too much. Consider just using voice-0 and doing
	 * some manipulations on the voice->key.key to change the keyboard tracking
	 */
	if ((baudio->mixflags & P800_FILTMULTI) == 0) {
		int flags = P800LOCAL->maxvoice->flags;

		/*
		 * We need to check our preOps to see if we have a legato ON event, if
		 * not then clear this flag. This should really just be lvoices - if 
		 * last was zero then this has to have a note on flag.
		 */
		if (baudio->lvoices != 0)
			voice->flags &= ~(BRISTOL_KEYON|BRISTOL_KEYREON);

		audiomain->palette[B_ENV5S]->specs->io[0].buf = P800LOCAL->adsrbuf;
		(*baudio->sound[6]).operate(
			(audiomain->palette)[B_ENV5S],
			P800LOCAL->maxvoice,
			(*baudio->sound[6]).param,
			P800LOCAL->maxvoice->locals[P800LOCAL->maxvoice->index][6]);
		P800LOCAL->maxvoice->flags = flags;

		bufmerge(P800LOCAL->adsrbuf, P800LOCAL->envamount,
			P800LOCAL->scratch, 0.0, audiomain->samplecount);
		bufmerge(P800LOCAL->lfobuf,
			P800LOCAL->vcfmod + P800LOCAL->mod2vcf,
			P800LOCAL->scratch, 1.0, audiomain->samplecount);

		/* Amp the noise into the output */
		audiomain->palette[B_DCA]->specs->io[0].buf = P800LOCAL->noisebuf;
		audiomain->palette[B_DCA]->specs->io[1].buf = P800LOCAL->adsrbuf;
		audiomain->palette[B_DCA]->specs->io[2].buf = P800LOCAL->outbuf;
		(*baudio->sound[7]).operate(
			(audiomain->palette)[B_DCA],
			P800LOCAL->maxvoice,
			(*baudio->sound[7]).param,
			baudio->locals[P800LOCAL->maxvoice->index][7]);

		audiomain->palette[B_FILTER]->specs->io[0].buf = P800LOCAL->outbuf;
		audiomain->palette[B_FILTER]->specs->io[1].buf = P800LOCAL->scratch;
		audiomain->palette[B_FILTER]->specs->io[2].buf = baudio->leftbuf;
		(*baudio->sound[2]).operate(
			(audiomain->palette)[B_FILTER],
			P800LOCAL->maxvoice,
			(*baudio->sound[2]).param,
			P800LOCAL->maxvoice->locals[P800LOCAL->maxvoice->index][2]);

		bristolbzero(P800LOCAL->outbuf, audiomain->segmentsize);
	}

	bufmerge(baudio->leftbuf, 0.0,
		baudio->leftbuf, P800LOCAL->volume, audiomain->samplecount);
	return(0);
}

int
static bristolPoly800Destroy(audioMain *audiomain, Baudio *baudio)
{
	printf("removing one poly800\n");

	bristolfree(P800LOCAL->freqbuf);
	bristolfree(P800LOCAL->oscabuf);
	bristolfree(P800LOCAL->oscbbuf);
	bristolfree(P800LOCAL->noisebuf);
	bristolfree(P800LOCAL->zerobuf);
	bristolfree(P800LOCAL->scratch);
	bristolfree(P800LOCAL->syncbuf);
	bristolfree(P800LOCAL->pwmbuf);
	bristolfree(P800LOCAL->lfobuf);
	bristolfree(P800LOCAL->filtbuf);
	bristolfree(P800LOCAL->adsrbuf);
	bristolfree(P800LOCAL->outbuf);

	return(0);
}

int
bristolPoly800Init(audioMain *audiomain, Baudio *baudio)
{
	printf("initialising poly800\n");

	baudio->soundCount = 9; /* Number of operators in this voice */
	/*
	 * Assign an array of sound pointers.
	 */
	baudio->sound = (bristolSound **)
		bristolmalloc0(sizeof(bristolOP *) * baudio->soundCount);
	baudio->effect = (bristolSound **)
		bristolmalloc0(sizeof(bristolOP *) * baudio->soundCount);

	/*
	 * Two oscillators
	 * Filter
	 * Noise
	 * Three Env
	 * LFO
	 * AMP
	 */
	/* Two oscillator */
	initSoundAlgo(B_NRO,	0, baudio, audiomain, baudio->sound);
	initSoundAlgo(B_NRO,	1, baudio, audiomain, baudio->sound);
	/* A filter */
	initSoundAlgo(B_FILTER, 2, baudio, audiomain, baudio->sound);
	/* An noise source */
	initSoundAlgo(B_NOISE,	3, baudio, audiomain, baudio->sound);
	/* Three ADSR with AMP */
	initSoundAlgo(B_ENV5S,	4, baudio, audiomain, baudio->sound);
	initSoundAlgo(B_ENV5S,	5, baudio, audiomain, baudio->sound);
	initSoundAlgo(B_ENV5S,	6, baudio, audiomain, baudio->sound);
	initSoundAlgo(B_DCA,	7, baudio, audiomain, baudio->sound);
	/* LFO */
	initSoundAlgo(B_LFO,	8, baudio, audiomain, baudio->sound);

	baudio->param = poly800Controller;
	baudio->destroy = bristolPoly800Destroy;
	baudio->operate = operateOnePoly800;
	baudio->preops = poly800Preops;
	baudio->postops = poly800PostOps;

	/*
	 * Put in a vibrachorus on our effects list.
	 */
	initSoundAlgo(12, 0, baudio, audiomain, baudio->effect);

	baudio->mixlocals = (float *) bristolmalloc0(sizeof(p800mods));
	((p800mods *) baudio->mixlocals)->voicecount = baudio->voicecount;

	if (P800LOCAL->freqbuf == NULL)
		P800LOCAL->freqbuf = (float *) bristolmalloc0(audiomain->segmentsize);
	if (P800LOCAL->oscabuf == NULL)
		P800LOCAL->oscabuf = (float *) bristolmalloc0(audiomain->segmentsize);
	if (P800LOCAL->oscbbuf == NULL)
		P800LOCAL->oscbbuf = (float *) bristolmalloc0(audiomain->segmentsize);
	if (P800LOCAL->adsrbuf == NULL)
		P800LOCAL->adsrbuf = (float *) bristolmalloc0(audiomain->segmentsize);
	if (P800LOCAL->outbuf == NULL)
		P800LOCAL->outbuf = (float *) bristolmalloc0(audiomain->segmentsize);
	if (P800LOCAL->zerobuf == NULL)
		P800LOCAL->zerobuf = (float *) bristolmalloc0(audiomain->segmentsize);
	if (P800LOCAL->scratch == NULL)
		P800LOCAL->scratch = (float *) bristolmalloc0(audiomain->segmentsize);
	if (P800LOCAL->syncbuf == NULL)
		P800LOCAL->syncbuf = (float *) bristolmalloc0(audiomain->segmentsize);
	if (P800LOCAL->pwmbuf == NULL)
		P800LOCAL->pwmbuf = (float *) bristolmalloc0(audiomain->segmentsize);
	if (P800LOCAL->lfobuf == NULL)
		P800LOCAL->lfobuf = (float *) bristolmalloc0(audiomain->segmentsize);
	if (P800LOCAL->noisebuf == NULL)
		P800LOCAL->noisebuf = (float *) bristolmalloc0(audiomain->segmentsize);
	if (P800LOCAL->filtbuf == NULL)
		P800LOCAL->filtbuf = (float *) bristolmalloc0(audiomain->segmentsize);

	return(0);
}

