
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

/* Memory Moog algo */

/*
 * TBF:
 *	Memory loading still a little damaged?
 */

/*#define DEBUG */

#include "bristol.h"
#include "bristolmm.h"
#include "bristolmemorymoog.h"

extern int buildCurrentTable(Baudio *, float);

/*
 * Use of these memorymoog global buffers will be an issue with use of multiple
 * audio threads, unless we ensure a single thread deals with any given algo
 * type, since then they are only used sequentially. Otherwise, global buffers
 * could be used here (noise, LFO, but polyphonic parameters would have to be
 * buried in the voice local structure.
 */
static float *freqbuf = (float *) NULL;
static float *filtbuf = (float *) NULL;
static float *noisebuf = (float *) NULL;
static float *adsrbuf = (float *) NULL;
static float *osc1buf = (float *) NULL;
static float *osc2buf = (float *) NULL;
static float *osc3buf = (float *) NULL;
static float *lfobuf = (float *) NULL;
static float *modbuf = (float *) NULL;
static float *scratch = (float *) NULL;

#define PEDAL1 baudio->contcontroller[7] /* GM-2 Volume pedal */
#define PEDAL2 baudio->contcontroller[11] /* GM-2 Expression pedal */

int
memorymoogController(Baudio *baudio, u_char operator, u_char controller, float value)
{
	int ivalue = value * CONTROLLER_RANGE;

#ifdef DEBUG
	printf("bristolMemoryMoogControl(%i, %i, %f)\n", operator, controller, value);
#endif

	/*
	 * These will be for our chorus.
	 */
	if (operator == 100)
	{
		if (controller == 1) {
			baudio->effect[0]->param->param[3].float_val = value;
		} else {
			value *= 0.2;
			baudio->effect[0]->param->param[0].float_val = 
				(0.1 + value * 20) * 1024 / baudio->samplerate;
			baudio->effect[0]->param->param[1].float_val = value;
			baudio->effect[0]->param->param[2].float_val = value;
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
			if (ivalue)
				baudio->mixflags |= BRISTOL_V_UNISON;
			else
				baudio->mixflags &= ~BRISTOL_V_UNISON;
			break;
		case 3:
			/* LFO gain */
			((mmMods *) baudio->mixlocals)->lfogain = value;
			break;
		case 4:
			/* LFO Wave form */
			((mmMods *) baudio->mixlocals)->lfowaveform = ivalue;
			break;
		case 5:
			if (ivalue)
				baudio->mixflags |= MMOOG_OSC_SYNC;
			else
				baudio->mixflags &= ~MMOOG_OSC_SYNC;
			break;
		case 6:
			if (ivalue)
				baudio->mixflags |= MMOOG_LFO_MULTI;
			else
				baudio->mixflags &= ~MMOOG_LFO_MULTI;
			break;
		case 7:
			if (ivalue)
				baudio->mixflags |= MMOOG_OSC3_KBD;
			else
				baudio->mixflags &= ~MMOOG_OSC3_KBD;
			break;
		case 8:
			if (ivalue)
				baudio->mixflags |= MMOOG_LFO_FM_OSC1;
			else
				baudio->mixflags &= ~MMOOG_LFO_FM_OSC1;
			break;
		case 9:
			if (ivalue)
				baudio->mixflags |= MMOOG_LFO_FM_OSC2;
			else
				baudio->mixflags &= ~MMOOG_LFO_FM_OSC2;
			break;
		case 10:
			if (ivalue)
				baudio->mixflags |= MMOOG_LFO_FM_OSC3;
			else
				baudio->mixflags &= ~MMOOG_LFO_FM_OSC3;
			break;
		case 11:
			if (ivalue)
				baudio->mixflags |= MMOOG_LFO_PWM_OSC1;
			else
				baudio->mixflags &= ~MMOOG_LFO_PWM_OSC1;
			break;
		case 12:
			if (ivalue)
				baudio->mixflags |= MMOOG_LFO_PWM_OSC2;
			else
				baudio->mixflags &= ~MMOOG_LFO_PWM_OSC2;
			break;
		case 13:
			if (ivalue)
				baudio->mixflags |= MMOOG_LFO_PWM_OSC3;
			else
				baudio->mixflags &= ~MMOOG_LFO_PWM_OSC3;
			break;
		case 14:
			if (ivalue)
				baudio->mixflags |= MMOOG_LFO_FILT;
			else
				baudio->mixflags &= ~MMOOG_LFO_FILT;
			break;
		case 15:
			((mmMods *) baudio->mixlocals)->pan = value;
			break;
		case 16:
			((mmMods *) baudio->mixlocals)->gain = value;
			break;
		case 17:
			((mmMods *) baudio->mixlocals)->fenvgain = value;
			break;
		case 20:
			((mmMods *) baudio->mixlocals)->pmOsc3 = value;
			break;
		case 21:
			((mmMods *) baudio->mixlocals)->pmEnv = value;
			break;
		case 22:
			if (ivalue)
				baudio->mixflags |= MMOOG_PM_FM_OSC1;
			else
				baudio->mixflags &= ~MMOOG_PM_FM_OSC1;
			break;
		case 23:
			if (ivalue)
				baudio->mixflags |= MMOOG_PM_FM_OSC2;
			else
				baudio->mixflags &= ~MMOOG_PM_FM_OSC2;
			break;
		case 24:
			if (ivalue)
				baudio->mixflags |= MMOOG_PM_PWM_OSC1;
			else
				baudio->mixflags &= ~MMOOG_PM_PWM_OSC1;
			break;
		case 25:
			if (ivalue)
				baudio->mixflags |= MMOOG_PM_PWM_OSC2;
			else
				baudio->mixflags &= ~MMOOG_PM_PWM_OSC2;
			break;
		case 26:
			if (ivalue)
				baudio->mixflags |= MMOOG_PM_FILT;
			else
				baudio->mixflags &= ~MMOOG_PM_FILT;
			break;
		case 27:
			if (ivalue)
				baudio->mixflags |= MMOOG_PM_CONTOUR;
			else
				baudio->mixflags &= ~MMOOG_PM_CONTOUR;
			break;
		case 28:
			if (ivalue)
				baudio->mixflags |= MMOOG_PM_INVERT;
			else
				baudio->mixflags &= ~MMOOG_PM_INVERT;
			break;
		case 30:
			((mmMods *) baudio->mixlocals)->osc1mix = value * 128;
			break;
		case 31:
			((mmMods *) baudio->mixlocals)->osc2mix = value * 128;
			break;
		case 32:
			((mmMods *) baudio->mixlocals)->osc3mix = value * 128;
			break;
		case 33:
			((mmMods *) baudio->mixlocals)->noisemix = value * 128;
			break;
		case 40:
			((mmMods *) baudio->mixlocals)->pedalamt1 = value;
			break;
		case 41:
			if (ivalue)
				baudio->mixflags |= MMOOG_PEDAL_PITCH;
			else
				baudio->mixflags &= ~MMOOG_PEDAL_PITCH;
			break;
		case 42:
			if (ivalue)
				baudio->mixflags |= MMOOG_PEDAL_FILT;
			else
				baudio->mixflags &= ~MMOOG_PEDAL_FILT;
			break;
		case 43:
			if (ivalue)
				baudio->mixflags |= MMOOG_PEDAL_VOL;
			else
				baudio->mixflags &= ~MMOOG_PEDAL_VOL;
			break;
		case 44:
			((mmMods *) baudio->mixlocals)->pedalamt2 = value;
			break;
		case 45:
			if (ivalue)
				baudio->mixflags |= MMOOG_PEDAL_MOD;
			else
				baudio->mixflags &= ~MMOOG_PEDAL_MOD;
			break;
		case 46:
			if (ivalue)
				baudio->mixflags |= MMOOG_PEDAL_OSC2;
			else
				baudio->mixflags &= ~MMOOG_PEDAL_OSC2;
			break;
		case 47:
			baudio->midi_pitch = value * 12;
			doPitchWheel(baudio);
			alterAllNotes(baudio); 
			break;
	}
	return(0);
}

int
memorymoogPreops(audioMain *audiomain, Baudio *baudio,
bristolVoice *voice, register float *startbuf)
{
	if (((mmMods *) baudio->mixlocals)->lfolocals == 0)
	{
		((mmMods *) baudio->mixlocals)->lfolocals =
			baudio->locals[voice->index][8];
	}

/* NOISE */
	bristolbzero(noisebuf, audiomain->segmentsize);
	audiomain->palette[(*baudio->sound[7]).index]->specs->io[0].buf = noisebuf;
	(*baudio->sound[7]).operate(
		(audiomain->palette)[4],
		voice,
		(*baudio->sound[7]).param,
		voice->locals[voice->index][7]);
/* NOISE DONE */

/* LFO */
	if ((baudio->mixflags & MMOOG_LFO_MULTI) == 0)
	{
		/*
		 * The LFO produces all the waveforms, but we only want to select one at
		 * any one time.
		 */
		audiomain->palette[(*baudio->sound[8]).index]->specs->io[0].buf = 0;
		audiomain->palette[(*baudio->sound[8]).index]->specs->io[3].buf = 0;
		audiomain->palette[(*baudio->sound[8]).index]->specs->io[1].buf = 0;
		audiomain->palette[(*baudio->sound[8]).index]->specs->io[2].buf = 0;
		audiomain->palette[(*baudio->sound[8]).index]->specs->io[4].buf = 0;
		audiomain->palette[(*baudio->sound[8]).index]->specs->io[5].buf = 0;
		audiomain->palette[(*baudio->sound[8]).index]->specs->io[6].buf = 0;

		switch (((mmMods *) baudio->mixlocals)->lfowaveform)
		{
			case 0:
				audiomain->palette[(*baudio->sound[8]).index]->specs->io[1].buf
					= lfobuf;
				break;
			case 1:
				audiomain->palette[(*baudio->sound[8]).index]->specs->io[5].buf
					= lfobuf;
				break;
			case 2:
				audiomain->palette[(*baudio->sound[8]).index]->specs->io[6].buf
					= lfobuf;
				break;
			case 3:
				audiomain->palette[(*baudio->sound[8]).index]->specs->io[2].buf
					= lfobuf;
				break;
			case 4:
				audiomain->palette[(*baudio->sound[8]).index]->specs->io[0].buf
					= noisebuf;
				audiomain->palette[(*baudio->sound[8]).index]->specs->io[3].buf
					= lfobuf;
				break;
		}

		/*
		 * Run the LFO
		 */
		(*baudio->sound[8]).operate(
			(audiomain->palette)[16],
			voice,
			(*baudio->sound[8]).param,
			((mmMods *) baudio->mixlocals)->lfolocals);
		/*
		 * If we have S/H we need to lower the gain to be more in line with the
		 * LFO outputs.
		 */
		if (((mmMods *) baudio->mixlocals)->lfowaveform == 4)
			bufmerge(modbuf, 0.0, lfobuf, 0.1, audiomain->samplecount);
	}
/* LFO DONE */
	return(0);
}

int
operateOneMemoryMoog(audioMain *audiomain, Baudio *baudio,
bristolVoice *voice, register float *startbuf)
{
	float modval;
	float modmode = 1.0;
/*printf("operateOneMemoryMoog(%i, %x, %x)\n", voice->index, audiomain, baudio); */

	modval = ((mmMods *) baudio->mixlocals)->lfogain
		* baudio->contcontroller[1];
	if (baudio->mixflags & MMOOG_PEDAL_MOD)
		modval += PEDAL2 * ((mmMods *) baudio->mixlocals)->pedalamt2;

	if (baudio->mixflags & MMOOG_PM_INVERT)
		modmode = -1;

	/*
	 * This should be straightforward, the mods are not excessive.
	 *	Do the oscillators, there are two representing one and its subosc.
	 *	Run filter ADSR and filter,
	 *	Run amp ADSR and AMP.
	 */
	bristolbzero(osc1buf, audiomain->segmentsize);
	bristolbzero(osc2buf, audiomain->segmentsize);
	bristolbzero(osc3buf, audiomain->segmentsize);
	bristolbzero(freqbuf, audiomain->segmentsize);
	bristolbzero(filtbuf, audiomain->segmentsize);

/* LFO */
	if (baudio->mixflags & MMOOG_LFO_MULTI)
	{
		/*
		 * The LFO produces all the waveforms, but we only want to select one at
		 * any one time.
		 */
		audiomain->palette[(*baudio->sound[8]).index]->specs->io[0].buf = 0;
		audiomain->palette[(*baudio->sound[8]).index]->specs->io[3].buf = 0;
		audiomain->palette[(*baudio->sound[8]).index]->specs->io[1].buf = 0;
		audiomain->palette[(*baudio->sound[8]).index]->specs->io[2].buf = 0;
		audiomain->palette[(*baudio->sound[8]).index]->specs->io[4].buf = 0;
		audiomain->palette[(*baudio->sound[8]).index]->specs->io[5].buf = 0;
		audiomain->palette[(*baudio->sound[8]).index]->specs->io[6].buf = 0;

		switch (((mmMods *) baudio->mixlocals)->lfowaveform)
		{
			case 0:
				audiomain->palette[(*baudio->sound[8]).index]->specs->io[1].buf
					= lfobuf;
				break;
			case 1:
				audiomain->palette[(*baudio->sound[8]).index]->specs->io[5].buf
					= lfobuf;
				break;
			case 2:
				audiomain->palette[(*baudio->sound[8]).index]->specs->io[6].buf
					= lfobuf;
				break;
			case 3:
				audiomain->palette[(*baudio->sound[8]).index]->specs->io[2].buf
					= lfobuf;
				break;
			case 4:
				audiomain->palette[(*baudio->sound[8]).index]->specs->io[0].buf
					= noisebuf;
				audiomain->palette[(*baudio->sound[8]).index]->specs->io[3].buf
					= lfobuf;
				break;
		}

		/*
		 * Run the LFO
		 */
		(*baudio->sound[8]).operate(
			(audiomain->palette)[16],
			voice,
			(*baudio->sound[8]).param,
			((mmMods *) baudio->mixlocals)->lfolocals);
		/*
		 * If we have S/H we need to lower the gain to be more in line with the
		 * LFO outputs.
		 */
		if (((mmMods *) baudio->mixlocals)->lfowaveform == 4)
			bufmerge(modbuf, 0.0, lfobuf, 0.1, audiomain->samplecount);
	}
/* LFO DONE */

/* OSC-3 first, may be used as a poly mod */
	audiomain->palette[(*baudio->sound[2]).index]->specs->io[0].buf = freqbuf;
	audiomain->palette[(*baudio->sound[2]).index]->specs->io[1].buf = modbuf;
	audiomain->palette[(*baudio->sound[2]).index]->specs->io[2].buf = osc3buf;

	if (baudio->mixflags & MMOOG_OSC3_KBD)
	{
		bufset(freqbuf, 0.1, audiomain->samplecount);
	/*
		float *bufptr = freqbuf;
		int i;

		for (i = 0; i < audiomain->samplecount; i+=8)
		{
			*bufptr++ = 0.1;
			*bufptr++ = 0.1;
			*bufptr++ = 0.1;
			*bufptr++ = 0.1;
			*bufptr++ = 0.1;
			*bufptr++ = 0.1;
			*bufptr++ = 0.1;
			*bufptr++ = 0.1;
		}
	*/
	} else
		fillFreqTable(baudio, voice, freqbuf, audiomain->samplecount, 1);

	if (baudio->mixflags & MMOOG_PEDAL_PITCH)
		bufadd(freqbuf, ((mmMods *) baudio->mixlocals)->pedalamt1 * PEDAL1 * 12,
			audiomain->samplecount);

	bristolbzero(modbuf, audiomain->segmentsize);
	if (baudio->mixflags & MMOOG_LFO_FM_OSC3)
		bufmerge(lfobuf, modval, freqbuf, 1.0, audiomain->samplecount);
	if (baudio->mixflags & MMOOG_LFO_PWM_OSC3)
		bufmerge(lfobuf, 512.0 * modval, modbuf, 0.0, audiomain->samplecount);

	(*baudio->sound[2]).operate(
		(audiomain->palette)[8],
		voice,
		(*baudio->sound[2]).param,
		voice->locals[voice->index][2]);
/* OSC 3 done */

/* POLY MODS */
	bristolbzero(scratch, audiomain->segmentsize);

	/*
	 * This should perhaps be a subroutine. Need to take Osc-3 and Env into
	 * the modbuf, potentially contouring it
	 *
	 * We take osc3 output with some gain and the env with another gain
	 * and put it in scratch, then scratch is applied to the modbuf for
	 * PWM and Freqbuf for FM.
	 */
/* FILTER ENV - USED BY POLY MODS */
	/*
	 * Run the ADSR for the filter.
	 */
	audiomain->palette[(*baudio->sound[4]).index]->specs->io[0].buf = adsrbuf;
	(*baudio->sound[4]).operate(
		(audiomain->palette)[1],
		voice,
		(*baudio->sound[4]).param,
		voice->locals[voice->index][4]);
/* FILTER ENV DONE */

	if (baudio->mixflags & MMOOG_PM_CONTOUR)
	{
#pragma Need inverse envelope here as well.
		/*
		 * Apply contour to osc3. Reuse the amplifier.
		 */
		audiomain->palette[(*baudio->sound[5]).index]->specs->io[0].buf
			= osc3buf;
		audiomain->palette[(*baudio->sound[5]).index]->specs->io[1].buf
			= adsrbuf;
		audiomain->palette[(*baudio->sound[5]).index]->specs->io[2].buf
			= scratch;

		(*baudio->sound[5]).operate(
			(audiomain->palette)[2],
			voice,
			(*baudio->sound[5]).param,
			voice->locals[voice->index][5]);
		bufmerge(scratch,
			((mmMods *) baudio->mixlocals)->pmOsc3 *
			((mmMods *) baudio->mixlocals)->pmEnv * modmode,
			scratch, 0.0, audiomain->samplecount);
	} else {
		/*
		 * Apply filter env and osc3 to scratch.
		 */
		bufmerge(osc3buf, ((mmMods *) baudio->mixlocals)->pmOsc3 * modmode,
			scratch, 0.0, audiomain->samplecount);
		bufmerge(adsrbuf, ((mmMods *) baudio->mixlocals)->pmEnv * modmode,
			scratch, 1.0, audiomain->samplecount);
	}
/* POLYMODS DONE */

/* OSC-1 */
	audiomain->palette[(*baudio->sound[0]).index]->specs->io[0].buf = freqbuf;
	audiomain->palette[(*baudio->sound[0]).index]->specs->io[1].buf = modbuf;
	audiomain->palette[(*baudio->sound[0]).index]->specs->io[2].buf = osc1buf;

	fillFreqTable(baudio, voice, freqbuf, audiomain->samplecount, 1);

	/*
	 * Put in any desired mods. There are two, LFO/Wheel mod and Poly mods from
	 * osc-3
	 */
	if (baudio->mixflags & MMOOG_LFO_FM_OSC1)
		bufmerge(lfobuf, modval, freqbuf, 1.0, audiomain->samplecount);
	if (baudio->mixflags & MMOOG_LFO_PWM_OSC1)
		bufmerge(lfobuf, 512.0 * modval, modbuf, 0.0, audiomain->samplecount);
	else
		bristolbzero(modbuf, audiomain->segmentsize);

	if (baudio->mixflags & MMOOG_PEDAL_PITCH)
		bufadd(freqbuf, ((mmMods *) baudio->mixlocals)->pedalamt1 * PEDAL1 * 12,
			audiomain->samplecount);
	if (baudio->mixflags & MMOOG_PM_FM_OSC1)
		bufmerge(scratch, 0.5, freqbuf, 1.0, audiomain->samplecount);
	if (baudio->mixflags & MMOOG_PM_PWM_OSC1)
		bufmerge(scratch, 25.0, modbuf, 1.0, audiomain->samplecount);

	(*baudio->sound[0]).operate(
		(audiomain->palette)[8],
		voice,
		(*baudio->sound[0]).param,
		voice->locals[voice->index][0]);
/* OSC 1 done */

/* OSC-2 */
	audiomain->palette[(*baudio->sound[1]).index]->specs->io[0].buf = freqbuf;
	audiomain->palette[(*baudio->sound[1]).index]->specs->io[1].buf = modbuf;
	audiomain->palette[(*baudio->sound[1]).index]->specs->io[2].buf = osc2buf;
	audiomain->palette[(*baudio->sound[1]).index]->specs->io[3].buf = osc1buf;

	fillFreqTable(baudio, voice, freqbuf, audiomain->samplecount, 1);

	/*
	 * Put in any desired mods. There are two, LFO/Wheel mod and Poly mods from
	 * osc-3
	 */
	if (baudio->mixflags & MMOOG_LFO_FM_OSC2)
		bufmerge(lfobuf, modval, freqbuf, 1.0, audiomain->samplecount);
	if (baudio->mixflags & MMOOG_LFO_PWM_OSC2)
		bufmerge(lfobuf, 512.0 * modval, modbuf, 0.0, audiomain->samplecount);
	else
		bristolbzero(modbuf, audiomain->segmentsize);

	if (baudio->mixflags & MMOOG_PEDAL_PITCH)
		bufadd(freqbuf, ((mmMods *) baudio->mixlocals)->pedalamt1 * PEDAL1 * 12,
			audiomain->samplecount);
	if (baudio->mixflags & MMOOG_PEDAL_OSC2)
		bufadd(freqbuf, ((mmMods *) baudio->mixlocals)->pedalamt2 * PEDAL2 * 12,
			audiomain->samplecount);

	if (baudio->mixflags & MMOOG_PM_FM_OSC2)
		bufmerge(scratch, 0.5, freqbuf, 1.0, audiomain->samplecount);
	if (baudio->mixflags & MMOOG_PM_PWM_OSC2)
		bufmerge(scratch, 25.0, modbuf, 1.0, audiomain->samplecount);

	(*baudio->sound[1]).operate(
		(audiomain->palette)[8],
		voice,
		(*baudio->sound[1]).param,
		voice->locals[voice->index][1]);
/* OSC 2 done */

/* MIXER */
	bufmerge(osc1buf, ((mmMods *) baudio->mixlocals)->osc1mix,
		osc3buf, ((mmMods *) baudio->mixlocals)->osc3mix,
			audiomain->samplecount);
	bufmerge(osc2buf, ((mmMods *) baudio->mixlocals)->osc2mix,
		osc3buf, 1.0, audiomain->samplecount);
	bufmerge(noisebuf, ((mmMods *) baudio->mixlocals)->noisemix,
		osc3buf, 1.0, audiomain->samplecount);
/* MIXER DONE */

/* FILTER */
	audiomain->palette[(*baudio->sound[3]).index]->specs->io[0].buf
		= osc3buf;
	audiomain->palette[(*baudio->sound[3]).index]->specs->io[1].buf
		= adsrbuf;
	audiomain->palette[(*baudio->sound[3]).index]->specs->io[2].buf
		= filtbuf;

	if (baudio->mixflags & MMOOG_LFO_FILT)
		bufmerge(lfobuf, modval * 2.0,
			adsrbuf, ((mmMods *) baudio->mixlocals)->fenvgain,
			audiomain->samplecount);
	else
		bufmerge(lfobuf, 0, adsrbuf, ((mmMods *) baudio->mixlocals)->fenvgain,
			audiomain->samplecount);
	if (baudio->mixflags & MMOOG_PM_FILT)
			bufmerge(scratch, 1.0, adsrbuf, 1.0, audiomain->samplecount);

	if (baudio->mixflags & MMOOG_PEDAL_FILT)
	{
		float *bufptr = adsrbuf, value;
		int i;

		value = ((mmMods *) baudio->mixlocals)->pedalamt1 * PEDAL1 * 8;

		for (i = 0; i < audiomain->samplecount; i+=8)
		{
			*bufptr++ += value;
			*bufptr++ += value;
			*bufptr++ += value;
			*bufptr++ += value;
			*bufptr++ += value;
			*bufptr++ += value;
			*bufptr++ += value;
			*bufptr++ += value;
		}
	}

	(*baudio->sound[3]).operate(
		(audiomain->palette)[3],
		voice,
		(*baudio->sound[3]).param,
		voice->locals[voice->index][3]);
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

/* AMP */
	/*
	 * Need something else for stereo - chorus?
	bristolbzero(modbuf, audiomain->segmentsize);
	 */

	audiomain->palette[(*baudio->sound[5]).index]->specs->io[0].buf = filtbuf;
	audiomain->palette[(*baudio->sound[5]).index]->specs->io[1].buf = adsrbuf;
	audiomain->palette[(*baudio->sound[5]).index]->specs->io[2].buf =
		baudio->rightbuf;
	(*baudio->sound[5]).operate(
		(audiomain->palette)[2],
		voice,
		(*baudio->sound[5]).param,
		voice->locals[voice->index][5]);

	/*
	 * Need something else for stereo
	bufmerge(modbuf, ((mmMods *) baudio->mixlocals)->pan,
		baudio->leftbuf, 1.0,
		audiomain->samplecount);
	bufmerge(modbuf, 1.0 - ((mmMods *) baudio->mixlocals)->pan,
		baudio->rightbuf, 1.0,
		audiomain->samplecount);
	((mmMods *) baudio->mixlocals)->pan
		= 1.0 - ((mmMods *) baudio->mixlocals)->pan;
	 */
/* AMP DONE */
	return(0);
}

int
memorymoogPostops(audioMain *audiomain, Baudio *baudio,
bristolVoice *voice, register float *startbuf)
{
	float gain = ((mmMods *) baudio->mixlocals)->gain;

	if (baudio->mixflags & MMOOG_PEDAL_VOL)
		gain += ((mmMods *) baudio->mixlocals)->pedalamt1 * PEDAL1;

	/*
	 * Final stage gain
	 */
	bufmerge(baudio->rightbuf, gain, baudio->leftbuf, 0.0,
		audiomain->samplecount);

	bristolbzero(baudio->rightbuf, audiomain->segmentsize);
	return(0);
}

static int
bristolMemoryMoogDestroy(audioMain *audiomain, Baudio *baudio)
{
printf("removing one memorymoog\n");
	return(0);
	bristolfree(freqbuf);
	bristolfree(filtbuf);
	bristolfree(noisebuf);
	bristolfree(adsrbuf);
	bristolfree(osc1buf);
	bristolfree(osc2buf);
	bristolfree(osc3buf);
	bristolfree(lfobuf);
	bristolfree(modbuf);
	bristolfree(scratch);
	return(0);
}

int
bristolMemoryMoogInit(audioMain *audiomain, Baudio *baudio)
{
printf("initialising one memorymoog\n");

	baudio->soundCount = 9; /* Number of operators in this voice */

	/*
	 * Assign an array of sound pointers.
	 */
	baudio->sound = (bristolSound **)
		bristolmalloc0(sizeof(bristolOP *) * baudio->soundCount);
	baudio->effect = (bristolSound **)
		bristolmalloc0(sizeof(bristolOP *) * baudio->soundCount);

	/* Three oscillators */
	initSoundAlgo(8, 0, baudio, audiomain, baudio->sound);
	initSoundAlgo(8, 1, baudio, audiomain, baudio->sound);
	initSoundAlgo(8, 2, baudio, audiomain, baudio->sound);
	/* A filter */
	initSoundAlgo(3, 3, baudio, audiomain, baudio->sound);
	/* Filter ADSR */
	initSoundAlgo(1, 4, baudio, audiomain, baudio->sound);
	/* An amplifier */
	initSoundAlgo(2, 5, baudio, audiomain, baudio->sound);
	/* AMP ADSR */
	initSoundAlgo(1, 6, baudio, audiomain, baudio->sound);
	/* An noise source */
	initSoundAlgo(4, 7, baudio, audiomain, baudio->sound);
	/* LFO */
	initSoundAlgo(16, 8, baudio, audiomain, baudio->sound);

	baudio->param = memorymoogController;
	baudio->destroy = bristolMemoryMoogDestroy;
	baudio->operate = operateOneMemoryMoog;
	baudio->preops = memorymoogPreops;
	baudio->postops = memorymoogPostops;

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
	if (adsrbuf == NULL)
		adsrbuf = (float *) bristolmalloc0(audiomain->segmentsize);
	if (osc1buf == NULL)
		osc1buf = (float *) bristolmalloc0(audiomain->segmentsize);
	if (osc2buf == NULL)
		osc2buf = (float *) bristolmalloc0(audiomain->segmentsize);
	if (osc3buf == NULL)
		osc3buf = (float *) bristolmalloc0(audiomain->segmentsize);
	if (lfobuf == NULL)
		lfobuf = (float *) bristolmalloc0(audiomain->segmentsize);
	if (modbuf == NULL)
		modbuf = (float *) bristolmalloc0(audiomain->segmentsize);
	if (scratch == NULL)
		scratch = (float *) bristolmalloc0(audiomain->segmentsize);

/*	baudio->mixflags |= BRISTOL_STEREO; // Memory Moog is mono for now. */
	baudio->mixlocals = (float *) bristolmalloc0(sizeof(mmMods));
	return(0);
}

