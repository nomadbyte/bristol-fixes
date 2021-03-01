
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
 * Organ and string grooming envelopes affect each other. Fixed
 *
 * Organ mod and spacialisation seem too similar. Fixed
 *
 * Touch sense synth broken. Fixed
 *
 * Mono Synth sometimes? Fixed
 *
 * Glide goes wild with pitch bend. Fixed
 */
/*#define DEBUG */

#include "bristol.h"
#include "bristoltrilogy.h"
#include <math.h>

extern int buildCurrentTable(Baudio *, float);

static int
trilogyGlobalController(Baudio *baudio, u_char controller,
u_char operator, float value)
{
	int ivalue = value * CONTROLLER_RANGE;

#ifdef DEBUG
	printf("trilogyGlobalControl(%i, %i, %f)\n", controller, operator, value);
#endif

	if (controller != 126)
		return(0);

	switch (operator) {
		case 0:
			baudio->glide = value * value * baudio->glidemax;
			break;
		case 1:
			TRILOGYLCL->mastervolume = value * 128;
			break;
		case 2:
			baudio->gtune = 1.0
				+ (baudio->note_diff - 1)
				* (value * 2 - 1);

			buildCurrentTable(baudio, baudio->gtune);
			alterAllNotes(baudio);
			break;
		case 3:
			TRILOGYLCL->panorgan = value;
			break;
		case 4:
			TRILOGYLCL->spaceorgan = value;
			break;
		case 5:
			TRILOGYLCL->mixorgan = value * 6;
			break;
		case 6:
			TRILOGYLCL->panstring = value;
			break;
		case 7:
			TRILOGYLCL->spacestring = value;
			break;
		case 8:
			TRILOGYLCL->mixstring = value * 2.8f;
			break;
		case 9:
			TRILOGYLCL->mixsynth = value * 1.5f;
			break;
		case 10:
			TRILOGYLCL->env2filter = (value - 0.5) * 4;
			break;
		case 11:
			/* Sync */
			if (value == 0)
				baudio->mixflags &= ~VCO_SYNC;
			else
				baudio->mixflags |= VCO_SYNC;
			break;
		case 12:
			/* Trill */
			if (value == 0)
				baudio->mixflags &= ~VCO_TRILL;
			else
				baudio->mixflags |= VCO_TRILL;
			break;
		case 13:
			/* Alternate..... */
			if (value == 0)
				baudio->mixflags &= ~VCO_ALTERNATE;
			else
				baudio->mixflags |= VCO_ALTERNATE;
			break;
		case 14:
			if (value == 0)
				baudio->mixflags &= ~VCO_LEGATO;
			else
				baudio->mixflags |= VCO_LEGATO;
			break;
		case 15:
			TRILOGYLCL->lfoRouting = value;
			if (value < 0.5) {
				TRILOGYLCL->lfoRouteO = (0.5 - value) * 2;
				TRILOGYLCL->lfoRouteF = value * 2;
				TRILOGYLCL->lfoRouteA = 0;
			} else {
				TRILOGYLCL->lfoRouteO = 0;
				TRILOGYLCL->lfoRouteF = (1.0 - value) * 2;
				TRILOGYLCL->lfoRouteA = (value - 0.5) * 2;
			}
			break;
		case 16:
			/* UniMulti */
			if (value == 0)
				baudio->mixflags &= ~MULTI_LFO;
			else
				baudio->mixflags |= MULTI_LFO;
			break;
		case 17:
			/* LFO Waveform */
			baudio->mixflags &= ~LFO_WAVE_MASK;
			baudio->mixflags |= LFO_TRI << ivalue;
			break;
		case 18:
			if ((TRILOGYLCL->lfoSlopeV = value * baudio->samplerate * 10) <= 0)
				TRILOGYLCL->lfoSlopeV= baudio->samplecount;

			TRILOGYLCL->lfoSlope = TRILOGYLCL->lfoGain / TRILOGYLCL->lfoSlopeV;
			break;
		case 19:
			TRILOGYLCL->lfoDelay = value * value * baudio->samplerate * 10;
			break;
		case 20:
			TRILOGYLCL->lfoGain = value;
			TRILOGYLCL->lfoSlope = TRILOGYLCL->lfoGain / TRILOGYLCL->lfoSlopeV;
			break;
		case 21:
			TRILOGYLCL->pansynth = value;
			break;
		case 22:
			/* We need to convert glideDepth into a multiplier/divider */
			TRILOGYLCL->glideDepth = 1.0 + value * value * 7;
			break;
		case 23:
			if ((TRILOGYLCL->glideRate = value*value*baudio->samplerate * 10)
				< baudio->samplecount)
				TRILOGYLCL->glideRate = baudio->samplecount - 1;
			break;
		case 24:
			/* Legato style? Perhaps use normal glide */
			if (ivalue == 0)
				baudio->mixflags |= GLIDE_LEGATO;
			else
				baudio->mixflags &= ~GLIDE_LEGATO;
			break;
		case 25:
			/* Legato Mode */
			baudio->mixflags &= ~VCO_GLIDE_MASK;
			baudio->mixflags |= VCO_GLIDE_A << ivalue;
			break;
		case 26:
			/* Filter Pedal */
			if (ivalue == 0)
				baudio->mixflags &= ~FILTER_PEDAL;
			else
				baudio->mixflags |= FILTER_PEDAL;
			break;
		case 27:
			/* Velocitry tracking in emulator or envelope */
			if (ivalue == 0)
				baudio->mixflags |= TOUCH_SENSE;
			else
				baudio->mixflags &= ~TOUCH_SENSE;
			break;
	}

	return(0);
}

int
operateTrilogyODCPreops(audioMain *audiomain, Baudio *baudio,
bristolVoice *voice, register float *startbuf)
{
	/*
	 * Operate LFO for divider circuit
	 */
	audiomain->palette[16]->specs->io[1].buf
		= audiomain->palette[16]->specs->io[2].buf
		= audiomain->palette[16]->specs->io[3].buf
		= audiomain->palette[16]->specs->io[4].buf
		= audiomain->palette[16]->specs->io[5].buf
		= audiomain->palette[16]->specs->io[6].buf
			= NULL;

	audiomain->palette[16]->specs->io[0].buf = TRILOGYLCL->zerobuf;
	/* Request a sine wave */
	audiomain->palette[16]->specs->io[1].buf = TRILOGYLCL->lfobuf;

	(*baudio->sound[5]).operate(
		(audiomain->palette)[16],
		voice,
		(*baudio->sound[5]).param,
		baudio->locals[voice->index][5]);

	return(0);
}

static float
fillLFOgainTable(float *buf, float current, float slope, float target, int i)
{
	for (; i > 0; i-=4)
	{
		if ((current += slope) > target) current = target;
		*buf++ = current;
		if ((current += slope) > target) current = target;
		*buf++ = current;
		if ((current += slope) > target) current = target;
		*buf++ = current;
		if ((current += slope) > target) current = target;
		*buf++ = current;
	}
	return(current);
}

int
operateTrilogyPreops(audioMain *audiomain, Baudio *baudio,
bristolVoice *voice, register float *startbuf)
{
	if ((baudio->mixflags & MULTI_LFO) == 0)
	{
		/*
		 * Operate LFO if we have a single LFO per synth.
		 */
		audiomain->palette[16]->specs->io[1].buf
			= audiomain->palette[16]->specs->io[2].buf
			= audiomain->palette[16]->specs->io[3].buf
			= audiomain->palette[16]->specs->io[4].buf
			= audiomain->palette[16]->specs->io[5].buf
			= audiomain->palette[16]->specs->io[6].buf
				= NULL;

		audiomain->palette[16]->specs->io[0].buf = TRILOGYLCL->zerobuf;

		switch (baudio->mixflags & LFO_WAVE_MASK) {
			default:
			case LFO_TRI:
				audiomain->palette[16]->specs->io[1].buf = TRILOGYLCL->scratch;
				break;
			case LFO_SAW:
				audiomain->palette[16]->specs->io[6].buf = TRILOGYLCL->scratch;
				break;
			case LFO_RAMP:
				audiomain->palette[16]->specs->io[5].buf = TRILOGYLCL->scratch;
				break;
			case LFO_SQUARE:
				audiomain->palette[16]->specs->io[2].buf = TRILOGYLCL->scratch;
				break;
		}

		(*baudio->sound[5]).operate(
			(audiomain->palette)[16],
			voice,
			(*baudio->sound[5]).param,
			baudio->locals[voice->index][5]);

		bufadd(TRILOGYLCL->scratch, 1.0, audiomain->samplecount);

		/*
		 * Evaluate the gain of the LFO and then amplify it. Since this is a
		 * single preop function with a unique LFO then we might try and look
		 * at making the MULTI/MONO work in the true legato option. The other
		 * setting, Uni LFO without legato is dumb and will not be implemented,
		 * it would have refired the LFO env for all voices for each keypress.
		 */
		if ((voice->flags & (BRISTOL_KEYON|BRISTOL_KEYREON))
			&& (baudio->lvoices == 0))
		{
			/* Put in up to 10s of delay */
			TRILOGYLCL->lfoDelays[0] = TRILOGYLCL->lfoDelay;
			TRILOGYLCL->lfoGains[0] = 0;
		}

		bristolbzero(TRILOGYLCL->lfobuf, audiomain->segmentsize);

		if ((TRILOGYLCL->lfoDelays[0] -= audiomain->samplecount) <= 0)
		{
			TRILOGYLCL->lfoGains[0] =
				fillLFOgainTable(TRILOGYLCL->adsrbuf, TRILOGYLCL->lfoGains[0],	
					TRILOGYLCL->lfoSlope, TRILOGYLCL->lfoGain,
					audiomain->samplecount);

			audiomain->palette[2]->specs->io[0].buf = TRILOGYLCL->scratch;
			audiomain->palette[2]->specs->io[1].buf = TRILOGYLCL->adsrbuf;
			audiomain->palette[2]->specs->io[2].buf = TRILOGYLCL->lfobuf;

			(*baudio->sound[4]).operate(
				(audiomain->palette)[2],
				voice,
				(*baudio->sound[4]).param,
				baudio->locals[voice->index][4]);

			TRILOGYLCL->lfoDelays[0] = 0;
		}
	}

	return(0);
}

int
operateOneTrilogyODCVoice(audioMain *audiomain, Baudio *baudio,
bristolVoice *voice, register float *startbuf)
{
	/*
	 * Get a frequency table, generate the oscillator, generate the ADSR and
	 * run them through the AMP. For the organ the ADSR is really just for 
	 * grooming but we might push a bit of keyclick into it from the GUI.
	 *
	 * We need to take care of the envelope key_done stage, it should only be
	 * honoured for the string envelope, not the organ envelope.
	 */
	fillFreqTable(baudio, voice, TRILOGYLCL->freqbuf,
		audiomain->samplecount, 0);

/* Organ code */
	/* Run the organ grooming envelope */
	audiomain->palette[1]->specs->io[0].buf = TRILOGYLCL->adsrbuf;
	(*baudio->sound[1]).operate(
		(audiomain->palette)[1],
		voice,
		(*baudio->sound[1]).param,
		voice->locals[voice->index][1]);

	if (TRILOGYLCL->mixorgan != 0)
	{
		bristolbzero(TRILOGYLCL->outbuf1, audiomain->segmentsize);
		bristolbzero(TRILOGYLCL->outbuf2, audiomain->segmentsize);
		bristolbzero(TRILOGYLCL->oscabuf, audiomain->segmentsize);
		bristolbzero(TRILOGYLCL->oscbbuf, audiomain->segmentsize);

		/*
		 * The oscillator will take some amount of LFO for vibrato but this will
		 * be done by passing the buffer and letting the OSC do the work separately
		 * for each harmonic and passing different harmonics to different output
		 * buffers.
		 */
		audiomain->palette[33]->specs->io[0].buf = TRILOGYLCL->freqbuf;
		audiomain->palette[33]->specs->io[1].buf = TRILOGYLCL->oscabuf;
		audiomain->palette[33]->specs->io[2].buf = TRILOGYLCL->oscbbuf;
		/* PWM buff */
		audiomain->palette[33]->specs->io[3].buf = TRILOGYLCL->lfobuf;
		audiomain->palette[33]->specs->io[4].buf = TRILOGYLCL->zerobuf;

		(*baudio->sound[0]).operate(
			(audiomain->palette)[33],
			voice,
			(*baudio->sound[0]).param,
			voice->locals[voice->index][0]);

		audiomain->palette[2]->specs->io[0].buf = TRILOGYLCL->oscabuf;
		audiomain->palette[2]->specs->io[1].buf = TRILOGYLCL->adsrbuf;
		audiomain->palette[2]->specs->io[2].buf = TRILOGYLCL->outbuf1;
		(*baudio->sound[4]).operate(
			(audiomain->palette)[2],
			voice,
			(*baudio->sound[4]).param,
			baudio->locals[voice->index][4]);

		audiomain->palette[2]->specs->io[0].buf = TRILOGYLCL->oscbbuf;
		audiomain->palette[2]->specs->io[1].buf = TRILOGYLCL->adsrbuf;
		audiomain->palette[2]->specs->io[2].buf = TRILOGYLCL->outbuf2;
		(*baudio->sound[4]).operate(
			(audiomain->palette)[2],
			voice,
			(*baudio->sound[4]).param,
			baudio->locals[voice->index][4]);

		/* Organ Spacialisation */
		bufmerge(TRILOGYLCL->outbuf1,
			TRILOGYLCL->spaceorgan * TRILOGYLCL->mixorgan
				* TRILOGYLCL->mastervolume * (1.0 - TRILOGYLCL->panorgan),
			baudio->leftbuf, 1.0, audiomain->samplecount);
		bufmerge(TRILOGYLCL->outbuf1,
			(1.0 - TRILOGYLCL->spaceorgan) * TRILOGYLCL->mixorgan
				* TRILOGYLCL->mastervolume * TRILOGYLCL->panorgan,
			baudio->rightbuf, 1.0, audiomain->samplecount);

		bufmerge(TRILOGYLCL->outbuf2,
			(1.0 - TRILOGYLCL->spaceorgan) * TRILOGYLCL->mixorgan
				* TRILOGYLCL->mastervolume * (1.0 - TRILOGYLCL->panorgan),
			baudio->leftbuf, 1.0, audiomain->samplecount);
		bufmerge(TRILOGYLCL->outbuf2,
			TRILOGYLCL->spaceorgan * TRILOGYLCL->mixorgan
				* TRILOGYLCL->mastervolume * TRILOGYLCL->panorgan,
			baudio->rightbuf, 1.0, audiomain->samplecount);
//printf("%f %f %f\n", TRILOGYLCL->mixorgan, TRILOGYLCL->panorgan, 
//TRILOGYLCL->spaceorgan);
	}

	/*
	 * The organ has a very fast release as it is only for key grooming.
	 * The string envelope will decide on the flags.
	 */
	voice->flags &= ~BRISTOL_KEYDONE;
/* Organ code done */

/* Strings code */
	/* Run the string grooming envelope */
	audiomain->palette[1]->specs->io[0].buf = TRILOGYLCL->adsrbuf;
	(*baudio->sound[3]).operate(
		(audiomain->palette)[1],
		voice,
		(*baudio->sound[3]).param,
		voice->locals[voice->index][3]);

	if (TRILOGYLCL->mixstring != 0)
	{
		bristolbzero(TRILOGYLCL->outbuf1, audiomain->segmentsize);
		bristolbzero(TRILOGYLCL->outbuf2, audiomain->segmentsize);
		bristolbzero(TRILOGYLCL->oscabuf, audiomain->segmentsize);
		bristolbzero(TRILOGYLCL->oscbbuf, audiomain->segmentsize);

		audiomain->palette[33]->specs->io[0].buf = TRILOGYLCL->freqbuf;
		audiomain->palette[33]->specs->io[1].buf = TRILOGYLCL->oscabuf;
		audiomain->palette[33]->specs->io[2].buf = TRILOGYLCL->oscbbuf;
		audiomain->palette[33]->specs->io[3].buf = TRILOGYLCL->lfobuf;
		audiomain->palette[33]->specs->io[4].buf = TRILOGYLCL->zerobuf;

		(*baudio->sound[2]).operate(
			(audiomain->palette)[33],
			voice,
			(*baudio->sound[2]).param,
			voice->locals[voice->index][2]);

		audiomain->palette[2]->specs->io[0].buf = TRILOGYLCL->oscabuf;
		audiomain->palette[2]->specs->io[1].buf = TRILOGYLCL->adsrbuf;
		audiomain->palette[2]->specs->io[2].buf = TRILOGYLCL->outbuf1;
		(*baudio->sound[4]).operate(
			(audiomain->palette)[2],
			voice,
			(*baudio->sound[4]).param,
			baudio->locals[voice->index][4]);

		audiomain->palette[2]->specs->io[0].buf = TRILOGYLCL->oscbbuf;
		audiomain->palette[2]->specs->io[1].buf = TRILOGYLCL->adsrbuf;
		audiomain->palette[2]->specs->io[2].buf = TRILOGYLCL->outbuf2;
		(*baudio->sound[4]).operate(
			(audiomain->palette)[2],
			voice,
			(*baudio->sound[4]).param,
			baudio->locals[voice->index][4]);

		/* String Spacialisation */
		bufmerge(TRILOGYLCL->outbuf1,
			TRILOGYLCL->spacestring * TRILOGYLCL->mixstring
				* TRILOGYLCL->mastervolume * (1.0 - TRILOGYLCL->panstring),
			baudio->leftbuf, 1.0, audiomain->samplecount);
		bufmerge(TRILOGYLCL->outbuf1,
			(1.0 - TRILOGYLCL->spacestring) * TRILOGYLCL->mixstring
				* TRILOGYLCL->mastervolume * TRILOGYLCL->panstring,
			baudio->rightbuf, 1.0, audiomain->samplecount);

		bufmerge(TRILOGYLCL->outbuf2,
			(1.0 - TRILOGYLCL->spacestring) * TRILOGYLCL->mixstring
				 * TRILOGYLCL->mastervolume * (1.0 - TRILOGYLCL->panstring),
			baudio->leftbuf, 1.0, audiomain->samplecount);
		bufmerge(TRILOGYLCL->outbuf2,
			TRILOGYLCL->spacestring * TRILOGYLCL->mixstring
				 * TRILOGYLCL->mastervolume * TRILOGYLCL->panstring,
			baudio->rightbuf, 1.0, audiomain->samplecount);
	}
/* Strings code done */

	return(0);
}

int
operateTrilogyODCPostops(audioMain *audiomain, Baudio *baudio,
bristolVoice *voice, register float *startbuf)
{
	/* Pan and master volume
	bufmerge(baudio->leftbuf,
		TRILOGYLCL->mastervolume * (1.0 - TRILOGYLCL->panorgan),
		baudio->leftbuf, 0.0, audiomain->samplecount);
	bufmerge(baudio->rightbuf,
		TRILOGYLCL->mastervolume * TRILOGYLCL->panorgan,
		baudio->rightbuf, 0.0, audiomain->samplecount);
	 */

	return(0);
}

int
operateOneTrilogyVoice(audioMain *audiomain, Baudio *baudio,
bristolVoice *voice, register float *startbuf)
{
//	if (TRILOGYLCL->mixsynth == 0)
//		return(0);

	bristolbzero(TRILOGYLCL->oscabuf, audiomain->segmentsize);
	bristolbzero(TRILOGYLCL->oscbbuf, audiomain->segmentsize);
	bristolbzero(TRILOGYLCL->filtbuf, audiomain->segmentsize);
	bristolbzero(TRILOGYLCL->outbuf1, audiomain->segmentsize);

/* LFO */
	if (baudio->mixflags & MULTI_LFO)
	{
		/*
		 * Operate LFO if we have a single LFO per synth.
		 */
		audiomain->palette[16]->specs->io[1].buf
			= audiomain->palette[16]->specs->io[2].buf
			= audiomain->palette[16]->specs->io[3].buf
			= audiomain->palette[16]->specs->io[4].buf
			= audiomain->palette[16]->specs->io[5].buf
			= audiomain->palette[16]->specs->io[6].buf
				= NULL;

		audiomain->palette[16]->specs->io[0].buf = TRILOGYLCL->zerobuf;

		switch (baudio->mixflags & LFO_WAVE_MASK) {
			default:
			case LFO_TRI:
				audiomain->palette[16]->specs->io[1].buf = TRILOGYLCL->scratch;
				break;
			case LFO_SAW:
				audiomain->palette[16]->specs->io[6].buf = TRILOGYLCL->scratch;
				break;
			case LFO_RAMP:
				audiomain->palette[16]->specs->io[5].buf = TRILOGYLCL->scratch;
				break;
			case LFO_SQUARE:
				audiomain->palette[16]->specs->io[2].buf = TRILOGYLCL->scratch;
				break;
		}

		(*baudio->sound[5]).operate(
			(audiomain->palette)[16],
			voice,
			(*baudio->sound[5]).param,
			baudio->locals[voice->index][5]);

		bufadd(TRILOGYLCL->scratch, 1.0, audiomain->samplecount);

		/*
		 * Evaluate the gain of the LFO and then amplify it. Since this is a
		 * single preop function with a unique LFO then we might try and look
		 * at making the MULTI/MONO work in the true legato option. The other
		 * setting, Uni LFO without legato is dumb and will not be implemented,
		 * it would have refired the LFO env for all voices for each keypress.
		 */
		if (voice->flags & (BRISTOL_KEYON|BRISTOL_KEYREON))
		{
			/* Put in up to 10s of delay */
			TRILOGYLCL->lfoDelays[voice->index] = TRILOGYLCL->lfoDelay;
			TRILOGYLCL->lfoGains[voice->index] = 0;
		}

		bristolbzero(TRILOGYLCL->lfobuf, audiomain->segmentsize);

		if ((TRILOGYLCL->lfoDelays[voice->index] -= audiomain->samplecount)
			<= 0)
		{
			TRILOGYLCL->lfoGains[voice->index] =
				fillLFOgainTable(TRILOGYLCL->adsrbuf,
					TRILOGYLCL->lfoGains[voice->index],	
					TRILOGYLCL->lfoSlope, TRILOGYLCL->lfoGain,
					audiomain->samplecount);

			audiomain->palette[2]->specs->io[0].buf = TRILOGYLCL->scratch;
			audiomain->palette[2]->specs->io[1].buf = TRILOGYLCL->adsrbuf;
			audiomain->palette[2]->specs->io[2].buf = TRILOGYLCL->lfobuf;

			(*baudio->sound[4]).operate(
				(audiomain->palette)[2],
				voice,
				(*baudio->sound[4]).param,
				baudio->locals[voice->index][4]);

			TRILOGYLCL->lfoDelays[voice->index] = 0;
		}
	}
/* LFO DONE */

/* Osc-1 */
	if ((TRILOGYLCL->glideRate != 0) && (TRILOGYLCL->glideDepth != 1.0))
	{
		if (voice->flags & (BRISTOL_KEYON|BRISTOL_KEYREON))
		{
			if (((baudio->mixflags & GLIDE_LEGATO) && (baudio->lvoices == 0))
				|| ((baudio->mixflags & GLIDE_LEGATO) == 0))
			{
//printf("glide reload %i %i\n", baudio->lvoices, baudio->cvoices);
				/* Reload the glide counters for this voice */
				switch (baudio->mixflags & VCO_GLIDE_MASK)
				{
					case VCO_GLIDE_A:
						TRILOGYLCL->glideCurrent[voice->index][0]
							= TRILOGYLCL->glideCurrent[voice->index][1]
							= voice->dFreq / TRILOGYLCL->glideDepth;
						break;
					case VCO_GLIDE_B:
						TRILOGYLCL->glideCurrent[voice->index][0]
							= voice->dFreq;
						TRILOGYLCL->glideCurrent[voice->index][1]
							= voice->dFreq / TRILOGYLCL->glideDepth;
						break;
					case VCO_GLIDE_C:
						TRILOGYLCL->glideCurrent[voice->index][0]
							= voice->dFreq;
						TRILOGYLCL->glideCurrent[voice->index][1]
							= voice->dFreq * TRILOGYLCL->glideDepth;
						break;
					case VCO_GLIDE_D:
						TRILOGYLCL->glideCurrent[voice->index][0]
							= TRILOGYLCL->glideCurrent[voice->index][1]
							= voice->dFreq * TRILOGYLCL->glideDepth;
						break;
				}

				TRILOGYLCL->glideScaler =
					powf(M_E, logf(voice->dFreq
					/ TRILOGYLCL->glideCurrent[voice->index][1])
					/ TRILOGYLCL->glideRate);
			} else
				TRILOGYLCL->glideCurrent[voice->index][0]
					= TRILOGYLCL->glideCurrent[voice->index][1]
					= voice->dFreq;
		}

		if ((baudio->mixflags & (VCO_GLIDE_A|VCO_GLIDE_D))
			&& (voice->dFreq != TRILOGYLCL->glideCurrent[voice->index][0]))
		{
			int i = audiomain->samplecount;
			float *buf = TRILOGYLCL->freqbuf;

			/* This could be optimized just a little bit when it works */
			if (voice->dFreq > TRILOGYLCL->glideCurrent[voice->index][0])
			{
				if (((voice->dFreq < TRILOGYLCL->glideCurrent[voice->index][0])
					&& (TRILOGYLCL->glideScaler > 1.0))
				|| ((voice->dFreq > TRILOGYLCL->glideCurrent[voice->index][0])
					&& (TRILOGYLCL->glideScaler < 1.0)))
					TRILOGYLCL->glideScaler = 1 / TRILOGYLCL->glideScaler;

				for (; i > 0; i--)
				{
					if (voice->dFreq
						> TRILOGYLCL->glideCurrent[voice->index][0])
					{
						TRILOGYLCL->glideCurrent[voice->index][0] *=
							TRILOGYLCL->glideScaler;
						*buf++ = TRILOGYLCL->glideCurrent[voice->index][0];
					} else {
						for (; i > 0; i--)
							*buf++ = voice->dFreq;
					 	TRILOGYLCL->glideCurrent[voice->index][0]
							= voice->dFreq;
						break;
					}
				}
			} else {
				if (((voice->dFreq < TRILOGYLCL->glideCurrent[voice->index][0])
					&& (TRILOGYLCL->glideScaler > 1.0))
				|| ((voice->dFreq > TRILOGYLCL->glideCurrent[voice->index][0])
					&& (TRILOGYLCL->glideScaler < 1.0)))
					TRILOGYLCL->glideScaler = 1 / TRILOGYLCL->glideScaler;

				for (; i > 0; i--)
				{
					if (voice->dFreq
						< TRILOGYLCL->glideCurrent[voice->index][0])
					{
						TRILOGYLCL->glideCurrent[voice->index][0] *=
							TRILOGYLCL->glideScaler;
						*buf++ = TRILOGYLCL->glideCurrent[voice->index][0];
					} else {
						for (; i > 0; i--)
							*buf++ = voice->dFreq;
					 	TRILOGYLCL->glideCurrent[voice->index][0]
							= voice->dFreq;
						break;
					}
				}
			}
		} else {
			register int i = audiomain->samplecount;
			register float *buf = TRILOGYLCL->freqbuf, dFreq = voice->dFreq;

			for (; i > 0; i-=8) {
				*buf++ = dFreq;
				*buf++ = dFreq;
				*buf++ = dFreq;
				*buf++ = dFreq;
				*buf++ = dFreq;
				*buf++ = dFreq;
				*buf++ = dFreq;
				*buf++ = dFreq;
			}
		}
	} else {
		register int i = audiomain->samplecount;
		register float *buf = TRILOGYLCL->freqbuf, dFreq = voice->dFreq;

		for (; i > 0; i-=8) {
			*buf++ = dFreq;
			*buf++ = dFreq;
			*buf++ = dFreq;
			*buf++ = dFreq;
			*buf++ = dFreq;
			*buf++ = dFreq;
			*buf++ = dFreq;
			*buf++ = dFreq;
		}
	}

	TRILOGYLCL->controlVCO = baudio->contcontroller[1] > 0.5?
		0:(0.5 - baudio->contcontroller[1]) * 0.5;
	TRILOGYLCL->controlVCF = baudio->contcontroller[1] <= 0.5?
		0:(baudio->contcontroller[1] - 0.5) * 2;

	bufmerge(TRILOGYLCL->lfobuf,
		(TRILOGYLCL->lfoRouteO + TRILOGYLCL->controlVCO) * 2.0,
		TRILOGYLCL->freqbuf, 1.0, audiomain->samplecount);

	audiomain->palette[30]->specs->io[0].buf = TRILOGYLCL->freqbuf;
	audiomain->palette[30]->specs->io[1].buf = TRILOGYLCL->oscabuf;
	audiomain->palette[30]->specs->io[2].buf = TRILOGYLCL->zerobuf; /* PWM */
	audiomain->palette[30]->specs->io[3].buf = TRILOGYLCL->zerobuf; /* Sync */
	audiomain->palette[30]->specs->io[4].buf = TRILOGYLCL->syncbuf; /* S-out */

	(*baudio->sound[0]).operate(
		(audiomain->palette)[30],
		voice,
		(*baudio->sound[0]).param,
		voice->locals[voice->index][0]);
/* Osc-1 DONE */

/* Osc-2 */
	if ((TRILOGYLCL->glideRate != 0) && (TRILOGYLCL->glideDepth != 1.0)
		&& (voice->dFreq != TRILOGYLCL->glideCurrent[voice->index][1]))
	{
		int i = audiomain->samplecount;
		float *buf = TRILOGYLCL->freqbuf;

		/* This could be optimized just a little bit when it works */
		if (voice->dFreq >= TRILOGYLCL->glideCurrent[voice->index][1])
		{
			if (((voice->dFreq < TRILOGYLCL->glideCurrent[voice->index][1])
					&& (TRILOGYLCL->glideScaler > 1.0))
				|| ((voice->dFreq > TRILOGYLCL->glideCurrent[voice->index][1])
					&& (TRILOGYLCL->glideScaler < 1.0)))
				TRILOGYLCL->glideScaler = 1 / TRILOGYLCL->glideScaler;

			for (; i > 0; i--)
			{
				if (voice->dFreq > TRILOGYLCL->glideCurrent[voice->index][1])
				{
					TRILOGYLCL->glideCurrent[voice->index][1] *=
						TRILOGYLCL->glideScaler;
					*buf++ = TRILOGYLCL->glideCurrent[voice->index][1];
				} else {
					for (; i > 0; i--)
						*buf++ = voice->dFreq;
				 	TRILOGYLCL->glideCurrent[voice->index][1] = voice->dFreq;
					break;
				}
			}
		} else {
			if (((voice->dFreq < TRILOGYLCL->glideCurrent[voice->index][1])
					&& (TRILOGYLCL->glideScaler > 1.0))
				|| ((voice->dFreq > TRILOGYLCL->glideCurrent[voice->index][1])
					&& (TRILOGYLCL->glideScaler < 1.0)))
				TRILOGYLCL->glideScaler = 1 / TRILOGYLCL->glideScaler;

			for (; i > 0; i--)
			{
				if (voice->dFreq < TRILOGYLCL->glideCurrent[voice->index][1])
				{
					TRILOGYLCL->glideCurrent[voice->index][1] *=
						TRILOGYLCL->glideScaler;
					*buf++ = TRILOGYLCL->glideCurrent[voice->index][1];
				} else {
					for (; i > 0; i--)
						*buf++ = voice->dFreq;
				 	TRILOGYLCL->glideCurrent[voice->index][1] = voice->dFreq;
					break;
				}
			}
		}
	} else {
		register int i = audiomain->samplecount;
		register float *buf = TRILOGYLCL->freqbuf, dFreq = voice->dFreq;

		for (; i > 0; i-=8) {
			*buf++ = dFreq;
			*buf++ = dFreq;
			*buf++ = dFreq;
			*buf++ = dFreq;
			*buf++ = dFreq;
			*buf++ = dFreq;
			*buf++ = dFreq;
			*buf++ = dFreq;
		}
	}

	if (baudio->mixflags & VCO_TRILL)
		bufmerge(TRILOGYLCL->scratch, 2.0,
			TRILOGYLCL->freqbuf, 2.0, audiomain->samplecount);

	audiomain->palette[30]->specs->io[0].buf = TRILOGYLCL->freqbuf;
	audiomain->palette[30]->specs->io[1].buf = TRILOGYLCL->oscbbuf;
	audiomain->palette[30]->specs->io[2].buf = TRILOGYLCL->zerobuf;
	if (baudio->mixflags & VCO_SYNC)
		audiomain->palette[30]->specs->io[3].buf = TRILOGYLCL->syncbuf;
	else
		audiomain->palette[30]->specs->io[3].buf = TRILOGYLCL->zerobuf;
	audiomain->palette[30]->specs->io[4].buf = NULL;

	(*baudio->sound[1]).operate(
		(audiomain->palette)[30],
		voice,
		(*baudio->sound[1]).param,
		voice->locals[voice->index][1]);
/* Osc-2 DONE */

	/*
	 * We need to do the alternate AB and legato here.
	 */
	if (baudio->mixflags & VCO_ALTERNATE)
	{
		if (baudio->mixflags & VCO_LEGATO)
		{
			/* Select Oscillator only on note on */
			if ((voice->flags & (BRISTOL_KEYON|BRISTOL_KEYREON))
				&& (baudio->lvoices == 0))
			{
//printf("Alternate Legato: %i %i\n", baudio->lvoices, baudio->cvoices);
				if (baudio->mixflags & ALTERNATE_A) {
					baudio->mixflags |= ALTERNATE_B;
					baudio->mixflags &= ~ALTERNATE_A;
				} else {
					baudio->mixflags &= ~ALTERNATE_B;
					baudio->mixflags |= ALTERNATE_A;
				}
			}
			if (baudio->mixflags & ALTERNATE_A)
				bufmerge(TRILOGYLCL->oscabuf, 24.0,
					TRILOGYLCL->oscbbuf, 0.0, audiomain->samplecount);
			else
				bufmerge(TRILOGYLCL->oscabuf, 0.0,
					TRILOGYLCL->oscbbuf, 24.0, audiomain->samplecount);
		} else {
			/* Select Oscillator based on voice index */
			if (voice->index & 0x01)
				bufmerge(TRILOGYLCL->oscabuf, 0.0,
					TRILOGYLCL->oscbbuf, 24.0, audiomain->samplecount);
			else
				bufmerge(TRILOGYLCL->oscabuf, 24.0,
					TRILOGYLCL->oscbbuf, 0.0, audiomain->samplecount);
		}
	} else
		bufmerge(TRILOGYLCL->oscabuf, 16.0,
			TRILOGYLCL->oscbbuf, 16.0, audiomain->samplecount);

/* ADSR */
	audiomain->palette[1]->specs->io[0].buf = TRILOGYLCL->adsrbuf;
	(*baudio->sound[2]).operate(
		(audiomain->palette)[1],
		voice,
		(*baudio->sound[2]).param,
		voice->locals[voice->index][2]);
/* ADSR DONE */

/* Filter */
	if (baudio->mixflags & FILTER_PEDAL)
		bufadd(TRILOGYLCL->scratch,
			baudio->contcontroller[11],
			audiomain->samplecount);

	bufmerge(TRILOGYLCL->lfobuf,
		(TRILOGYLCL->lfoRouteF + TRILOGYLCL->controlVCF) * 4.0,
		TRILOGYLCL->scratch, 0.0, audiomain->samplecount);

	bufmerge(TRILOGYLCL->adsrbuf, TRILOGYLCL->env2filter,
		TRILOGYLCL->scratch, 1.0, audiomain->samplecount);

	audiomain->palette[3]->specs->io[0].buf = TRILOGYLCL->oscbbuf;
	audiomain->palette[3]->specs->io[1].buf = TRILOGYLCL->scratch;
	audiomain->palette[3]->specs->io[2].buf = TRILOGYLCL->filtbuf;
	(*baudio->sound[3]).operate(
		(audiomain->palette)[3],
		voice,
		(*baudio->sound[3]).param,
		baudio->locals[voice->index][3]);
/* Filter DONE */

/* Amp */
	if (baudio->mixflags & TOUCH_SENSE)
		bufmerge(TRILOGYLCL->adsrbuf, voice->velocity,
			TRILOGYLCL->adsrbuf, 0.0, audiomain->samplecount);

	bufmerge(TRILOGYLCL->lfobuf, TRILOGYLCL->lfoRouteA * 6.0,
		TRILOGYLCL->adsrbuf, 1.0, audiomain->samplecount);
	audiomain->palette[2]->specs->io[0].buf = TRILOGYLCL->filtbuf;
	audiomain->palette[2]->specs->io[1].buf = TRILOGYLCL->adsrbuf;
	audiomain->palette[2]->specs->io[2].buf = baudio->leftbuf;
	(*baudio->sound[4]).operate(
		(audiomain->palette)[2],
		voice,
		(*baudio->sound[4]).param,
		baudio->locals[voice->index][4]);
/* Amp DONE */

	return(0);
}

int
operateTrilogyPostops(audioMain *audiomain, Baudio *baudio,
bristolVoice *voice, register float *startbuf)
{
	bufmerge(baudio->leftbuf,
		TRILOGYLCL->mastervolume * TRILOGYLCL->pansynth
			* TRILOGYLCL->mixsynth,
		baudio->rightbuf, 0.0, audiomain->samplecount);
	bufmerge(baudio->leftbuf, 0.0,
		baudio->leftbuf,
		TRILOGYLCL->mastervolume * (1.0 - TRILOGYLCL->pansynth)
			* TRILOGYLCL->mixsynth,
		audiomain->samplecount);

	return(0);
}

static int
destroyOneTrilogyVoice(audioMain *audiomain, Baudio *baudio)
{
	printf("destroying trilogy synth\n");

	bristolfree(TRILOGYLCL->freqbuf);
	bristolfree(TRILOGYLCL->lfobuf);
	bristolfree(TRILOGYLCL->oscabuf);
	bristolfree(TRILOGYLCL->oscbbuf);
	bristolfree(TRILOGYLCL->adsrbuf);
	bristolfree(TRILOGYLCL->zerobuf);
	bristolfree(TRILOGYLCL->outbuf1);
	bristolfree(TRILOGYLCL->outbuf2);
	bristolfree(TRILOGYLCL->filtbuf);
	bristolfree(TRILOGYLCL->scratch);
	bristolfree(TRILOGYLCL->syncbuf);

	return(0);
}

int
bristolTrilogyODCInit(audioMain *audiomain, Baudio *baudio)
{
	printf("initialising trilogy organ divider circuit\n");

	/*
	 * This is going to emulate the organ and string section of the Trilogy and
	 * Stratus synths. This should be straightforward, one LFO which will feed
	 * both parts (single organ circuitry) one oscillator and one ADSR each,
	 * plus AMP
	 */
	baudio->soundCount = 6; /* Number of operators in this voice (Trilogy) */

	/*
	 * Assign an array of sound pointers.
	 */
	baudio->sound = (bristolSound **)
		bristolmalloc0(sizeof(bristolOP *) * baudio->soundCount);
	baudio->effect = (bristolSound **)
		bristolmalloc0(sizeof(bristolOP *) * baudio->soundCount);

	/*
	 * Two VCO
	 * RingMod
	 * Noise
	 * Env
	 * AMP
	 * Filter
	 * Two LFO, may run to three if we put in a global LFO
	 *
	 * Possibly add a reverb?
	 */
	/* VCO - modified bit-1 oscillators */
	initSoundAlgo(33,	0, baudio, audiomain, baudio->sound);
	/* An ADSR */
	initSoundAlgo(	1,	1, baudio, audiomain, baudio->sound);
	/* VCO */
	initSoundAlgo(33,	2, baudio, audiomain, baudio->sound);
	/* An ADSR */
	initSoundAlgo(	1,	3, baudio, audiomain, baudio->sound);
	/* An amplifier */
	initSoundAlgo(	2,	4, baudio, audiomain, baudio->sound);
	/* LFO */
	initSoundAlgo(	16,	5, baudio, audiomain, baudio->sound);

	baudio->param = trilogyGlobalController;
	baudio->destroy = destroyOneTrilogyVoice;
	baudio->operate = operateOneTrilogyODCVoice;
	baudio->preops = operateTrilogyODCPreops;
	baudio->postops = operateTrilogyODCPostops;

	/* We need to flag this for the parallel filters */
	baudio->mixlocals = bristolmalloc0(sizeof(bTrilogy));

	/*
	 * Put effects in here if needed
	initSoundAlgo(22, 0, baudio, audiomain, baudio->effect);
	 */

	TRILOGYLCL->oscabuf = (float *) bristolmalloc0(audiomain->segmentsize);
	TRILOGYLCL->oscbbuf = (float *) bristolmalloc0(audiomain->segmentsize);
	TRILOGYLCL->freqbuf = (float *) bristolmalloc0(audiomain->segmentsize);
	TRILOGYLCL->filtbuf = (float *) bristolmalloc0(audiomain->segmentsize);
	TRILOGYLCL->lfobuf = (float *) bristolmalloc0(audiomain->segmentsize);
	TRILOGYLCL->adsrbuf = (float *) bristolmalloc0(audiomain->segmentsize);
	TRILOGYLCL->outbuf1 = (float *) bristolmalloc0(audiomain->segmentsize);
	TRILOGYLCL->outbuf2 = (float *) bristolmalloc0(audiomain->segmentsize);
	TRILOGYLCL->zerobuf = (float *) bristolmalloc0(audiomain->segmentsize);
	TRILOGYLCL->scratch = (float *) bristolmalloc0(audiomain->segmentsize);
	TRILOGYLCL->syncbuf = (float *) bristolmalloc0(audiomain->segmentsize);

	baudio->mixflags |= BRISTOL_STEREO;

	return(0);
}

int
bristolTrilogyInit(audioMain *audiomain, Baudio *baudio)
{
	printf("initialising trilogy synth\n");

	baudio->soundCount = 6; /* Number of operators in this voice (Trilogy) */

	/*
	 * Assign an array of sound pointers.
	 */
	baudio->sound = (bristolSound **)
		bristolmalloc0(sizeof(bristolOP *) * baudio->soundCount);
	baudio->effect = (bristolSound **)
		bristolmalloc0(sizeof(bristolOP *) * baudio->soundCount);

	/*
	 * Two VCO
	 * RingMod
	 * Noise
	 * Env
	 * AMP
	 * Filter
	 * Two LFO, may run to three if we put in a global LFO
	 *
	 * Possibly add a reverb?
	 */
	/* VCO */
	initSoundAlgo(	30,	0, baudio, audiomain, baudio->sound);
	initSoundAlgo(	30,	1, baudio, audiomain, baudio->sound);
	/* An ADSR */
	initSoundAlgo(	1,	2, baudio, audiomain, baudio->sound);
	/* A filter */
	initSoundAlgo(	3,	3, baudio, audiomain, baudio->sound);
	/* An amplifier */
	initSoundAlgo(	2,	4, baudio, audiomain, baudio->sound);
	/* LFO */
	initSoundAlgo(	16,	5, baudio, audiomain, baudio->sound);

	baudio->param = trilogyGlobalController;
	baudio->destroy = destroyOneTrilogyVoice;
	baudio->operate = operateOneTrilogyVoice;
	baudio->preops = operateTrilogyPreops;
	baudio->postops = operateTrilogyPostops;

	/* We need to flag this for the parallel filters */
	baudio->mixlocals = bristolmalloc0(sizeof(bTrilogy));

	/*
	 * Put in a vibrachorus on our effects list.
	initSoundAlgo(22, 0, baudio, audiomain, baudio->effect);
	 */

	TRILOGYLCL->mastervolume = 12;

	TRILOGYLCL->spaceorgan = 0.5;
	TRILOGYLCL->mixorgan = 1.0;
	TRILOGYLCL->panorgan = 0.5;

	TRILOGYLCL->spacestring = 0.5;
	TRILOGYLCL->mixstring = 1.0;
	TRILOGYLCL->panstring = 0.5;

	TRILOGYLCL->oscabuf = (float *) bristolmalloc0(audiomain->segmentsize);
	TRILOGYLCL->oscbbuf = (float *) bristolmalloc0(audiomain->segmentsize);
	TRILOGYLCL->freqbuf = (float *) bristolmalloc0(audiomain->segmentsize);
	TRILOGYLCL->filtbuf = (float *) bristolmalloc0(audiomain->segmentsize);
	TRILOGYLCL->lfobuf = (float *) bristolmalloc0(audiomain->segmentsize);
	TRILOGYLCL->adsrbuf = (float *) bristolmalloc0(audiomain->segmentsize);
	TRILOGYLCL->outbuf1 = (float *) bristolmalloc0(audiomain->segmentsize);
	TRILOGYLCL->outbuf2 = (float *) bristolmalloc0(audiomain->segmentsize);
	TRILOGYLCL->zerobuf = (float *) bristolmalloc0(audiomain->segmentsize);
	TRILOGYLCL->scratch = (float *) bristolmalloc0(audiomain->segmentsize);
	TRILOGYLCL->syncbuf = (float *) bristolmalloc0(audiomain->segmentsize);

	baudio->mixflags |= BRISTOL_STEREO;

	return(0);
}

