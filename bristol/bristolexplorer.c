
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
#include "bristolexplorer.h"

static float *freqbuf = (float *) NULL;
static float *osc3buf = (float *) NULL;
static float *osc1buf = (float *) NULL;
static float *noisebuf = (float *) NULL;
static float *filtbuf = (float *) NULL;
static float *filtbuf2 = (float *) NULL;
static float *lfotri = (float *) NULL;
static float *lforamp = (float *) NULL;
static float *lfosh = (float *) NULL;
static float *adsrbuf = (float *) NULL;
static float *mbuf1 = (float *) NULL;
static float *mbuf2 = (float *) NULL;
static float *onbuf = (float *) NULL;
static float *zerobuf = (float *) NULL;
static float *syncbuf = (float *) NULL;

extern int buildCurrentTable(Baudio *, float);

extern int s440holder;

static int
expGlobalController(Baudio *baudio, u_char controller,
u_char operator, float value)
{
#ifdef DEBUG
	printf("expGlobalControl(%i, %i, %f)\n", controller, operator, value);
#endif

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

	if (controller == 11)
		baudio->glide = value * value * baudio->glidemax;

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
			/*
			 * Master enable.
			 */
			case 1:
				if (ivalue == 0)
					baudio->mixflags &= ~MASTER_ONOFF;
				else
					baudio->mixflags |= MASTER_ONOFF;
				break;
			/*
			 * Five mixer options.
			 */
			case 2:
				if (ivalue == 0)
					baudio->mixflags &= ~MIX_OSC1;
				else
					baudio->mixflags |= MIX_OSC1;
				break;
			case 3:
				if (ivalue == 0)
					baudio->mixflags &= ~MIX_OSC2;
				else
					baudio->mixflags |= MIX_OSC2;
				break;
			case 4:
				if (ivalue == 0)
					baudio->mixflags &= ~MIX_OSC3;
				else
					baudio->mixflags |= MIX_OSC3;
				break;
			case 5:
				if (ivalue == 0)
					baudio->mixflags &= ~MIX_EXT;
				else
					baudio->mixflags |= MIX_EXT;
				break;
			case 6:
				if (ivalue == 0)
					baudio->mixflags &= ~MIX_NOISE;
				else
					baudio->mixflags |= MIX_NOISE;
				break;
			case 7:
				if (ivalue == 0)
					baudio->mixflags &= ~MULTI_LFO;
				else
					baudio->mixflags |= MULTI_LFO;
				break;
			case 8:
				if (ivalue == 0)
					baudio->mixflags &= ~OSC3_LFO;
				else
					baudio->mixflags |= OSC3_LFO;
				break;
			case 9:
				if (ivalue == 0)
					baudio->mixflags &= ~SYNC_1_2;
				else
					baudio->mixflags |= SYNC_1_2;
				break;
			case 10:
				if (ivalue == 0)
					baudio->mixflags &= ~FM_3_1;
				else
					baudio->mixflags |= FM_3_1;
				break;
			case 11:
				((bExp *) baudio->mixlocals)->fenvgain = (value - 0.5) * 2.0f;
				break;
			case 12:
				/* Filter mode DLPF/HPF_LPF */
				if (ivalue == 0)
					baudio->mixflags &= ~FILTER_MODE;
				else
					baudio->mixflags |= FILTER_MODE;
				break;
			case 13:
				((bExp *) baudio->mixlocals)->noisegain = value;
				break;
		}
	}

	/*
	 * Memory management controls for bristolSound memories.
	 */
	if (controller == 13)
	{
	}

	if (controller == 14)
		baudio->extgain = value;

	/*
	 * Mod bus 1
	 */
	if (controller == 15)
	{
		int ivalue = value * CONTROLLER_RANGE;
		int flag;
		modbus *bus = &((bExp *) baudio->mixlocals)->bus[0];

		/*
		 * These are flags that control the sequencing of the audioEngine.
		 */
		switch (operator)
		{
			case 0:
				/*
				 * Six Source options
				 */
				flag = MOD_TRI << ivalue;
				bus->flags &= ~MOD_MASK;
				bus->flags |= flag;
				break;
			case 1:
				flag = SHAPE_OFF << ivalue;
				bus->flags &= ~SHAPE_MASK;
				bus->flags |= flag;
				break;
			case 2:
				if (ivalue == 0)
				{
					bus->flags &= ~DEST_MASK;
					bus->flags |= OSC_1_MOD|OSC_2_MOD|OSC_3_MOD;
					/*if ((bus->flags & MOD_OSC3) == 0) */
					/*	bus->flags |= OSC_3_MOD; */
				} else {
					flag = OSC_1_MOD << ivalue;
					bus->flags &= ~DEST_MASK;
					bus->flags |= flag;
					/*if ((bus->flags & OSC_3_MOD) */
					/*	&& (bus->flags & MOD_OSC3)) */
					/*	bus->flags &= ~OSC_3_MOD; */
				}
				break;
			case 3:
				bus->gain = value;
				break;
		}
	}

	/*
	 * Mod bus 2
	 */
	if (controller == 16)
	{
		int ivalue = value * CONTROLLER_RANGE;
		int flag;
		modbus *bus = &((bExp *) baudio->mixlocals)->bus[1];

		/*
		 * These are flags that control the sequencing of the audioEngine.
		 */
		switch (operator)
		{
			case 0:
				/*
				 * Six Source options
				 */
				flag = MOD_TRI << ivalue;
				bus->flags &= ~MOD_MASK;
				bus->flags |= flag;
				break;
			case 1:
				flag = SHAPE_OFF << ivalue;
				bus->flags &= ~SHAPE_MASK;
				bus->flags |= flag;
				break;
			case 2:
				if (ivalue == 0)
				{
					bus->flags &= ~DEST_MASK;
					bus->flags |= OSC_1_MOD|OSC_2_MOD|OSC_3_MOD;
					/*if ((bus->flags & MOD_OSC3) == 0) */
					/*	bus->flags |= OSC_3_MOD; */
				} else {
					flag = OSC_1_MOD << ivalue;
					bus->flags &= ~DEST_MASK;
					bus->flags |= flag;
					/*if ((bus->flags & OSC_3_MOD) */
					/*	&& (bus->flags & MOD_OSC3)) */
					/*	bus->flags &= ~OSC_3_MOD; */
				}
				break;
			case 3:
				bus->gain = value;
				break;
		}
	}
	return(0);
}

int
operateExpPreops(audioMain *audiomain, Baudio *baudio,
bristolVoice *voice, register float *startbuf)
{
	bristolbzero(noisebuf, audiomain->segmentsize);

	/*
	 * Noise - we do this early for the same reason as oscillator 3
	 */
	audiomain->palette[(*baudio->sound[7]).index]->specs->io[0].buf = noisebuf;
	(*baudio->sound[7]).operate(
		(audiomain->palette)[4],
		voice,
		(*baudio->sound[7]).param,
		voice->locals[voice->index][7]);

	if ((baudio->mixflags & MULTI_LFO) == 0)
	{
		/*
		 * Operate LFO if we have a single LFO per synth.
		 */
		audiomain->palette[(*baudio->sound[8]).index]->specs->io[0].buf
			= noisebuf;
		audiomain->palette[(*baudio->sound[8]).index]->specs->io[1].buf
			= lfotri;
		audiomain->palette[(*baudio->sound[8]).index]->specs->io[6].buf
			= lforamp;
		audiomain->palette[(*baudio->sound[8]).index]->specs->io[3].buf
			= lfosh;
		audiomain->palette[(*baudio->sound[8]).index]->specs->io[4].buf
			= 0;
		(*baudio->sound[8]).operate(
			(audiomain->palette)[16],
			voice,
			(*baudio->sound[8]).param,
			baudio->locals[voice->index][8]);
	}

	return(0);
}

static int
explorerMods(register modbus *bus, register float *ext, register float *dest,
register int count, float c1)
{
	float *source, *mod = NULL, gain = bus->gain * c1;

	switch (bus->flags & SHAPE_MASK) {
		case SHAPE_OFF:
		default:
			if (bus->flags & SHAPE_OFF)
			{
				for (;count > 0; count-=8)
				{
					*dest++ = 0;
					*dest++ = 0;
					*dest++ = 0;
					*dest++ = 0;
					*dest++ = 0;
					*dest++ = 0;
					*dest++ = 0;
					*dest++ = 0;
				}
				return(0);
			}
			break;
		case SHAPE_KEY:
			mod = onbuf;
			gain = gain * 0.1f;
			break;
		case SHAPE_FILTENV:
			mod = adsrbuf;
			gain = gain * 0.1f;
			break;
		case SHAPE_ON:
			mod = onbuf;
			gain = gain * 0.1f;
			break;
	}

	/*
	 * Take the bus flags and gain, and apply them to the buffer. Whatever is
	 * there gets overwritten.
	 */
	switch (bus->flags & MOD_MASK) {
		case 1:
		default:
			source = lfotri;
			break;
		case 2:
			source = lforamp;
			break;
		case 4:
			source = lfosh;
			gain = gain / 64.0f;
			break;
		case 8:
			source = osc3buf;
			gain = gain / 12;
			break;
		case 0x10:
			source = ext;
			break;
	}

	for (;count > 0; count-=8)
	{
		*dest++ = *source++ * *mod++ * gain;
		*dest++ = *source++ * *mod++ * gain;
		*dest++ = *source++ * *mod++ * gain;
		*dest++ = *source++ * *mod++ * gain;
		*dest++ = *source++ * *mod++ * gain;
		*dest++ = *source++ * *mod++ * gain;
		*dest++ = *source++ * *mod++ * gain;
		*dest++ = *source++ * *mod++ * gain;
	}
	return(0);
}

int
operateOneExpVoice(audioMain *audiomain, Baudio *baudio,
bristolVoice *voice, register float *startbuf)
{
	register int samplecount = audiomain->samplecount, i;
	register float extsig = 0, *extmult = startbuf, *bufptr;
	register bExp *bexplorer = (bExp *) baudio->mixlocals;

#ifdef DEBUG
	audiomain->debuglevel = -1;
#endif

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
	 */
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

	bristolbzero(freqbuf, audiomain->segmentsize);
	bristolbzero(osc1buf, audiomain->segmentsize);
	bristolbzero(osc3buf, audiomain->segmentsize);
	bristolbzero(filtbuf, audiomain->segmentsize);
	bristolbzero(filtbuf2, audiomain->segmentsize);
	bristolbzero(mbuf1, audiomain->segmentsize);
	bristolbzero(mbuf2, audiomain->segmentsize);

	if (baudio->mixflags & MULTI_LFO)
	{
		/*
		 * Operate LFO if we have one per voice.
		 */
		audiomain->palette[(*baudio->sound[8]).index]->specs->io[0].buf
			= noisebuf;
		audiomain->palette[(*baudio->sound[8]).index]->specs->io[1].buf
			= lfotri;
		audiomain->palette[(*baudio->sound[8]).index]->specs->io[6].buf
			= lforamp;
		audiomain->palette[(*baudio->sound[8]).index]->specs->io[3].buf
			= lfosh;
		(*baudio->sound[8]).operate(
			(audiomain->palette)[16],
			voice,
			(*baudio->sound[8]).param,
			baudio->locals[voice->index][8]);
	}

	/*
	 * Run the ADSR for the filter. We need it now for possible mod bus shaping.
	 */
	audiomain->palette[(*baudio->sound[3]).index]->specs->io[0].buf = adsrbuf;
	(*baudio->sound[3]).operate(
		(audiomain->palette)[1],
		voice,
		(*baudio->sound[3]).param,
		voice->locals[voice->index][3]);

	/*
	 * See if we can do the mod busses now. This is possible only if osc-3 is
	 * NOT a source for the bus.
	 */
	if ((bexplorer->bus[0].flags & MOD_OSC3) == 0)
		explorerMods(&bexplorer->bus[0], startbuf, mbuf1,
			audiomain->samplecount, baudio->contcontroller[1]);

	if ((bexplorer->bus[1].flags & MOD_OSC3) == 0)
		explorerMods(&bexplorer->bus[1], startbuf, mbuf2,
			audiomain->samplecount, 1.0);

	/*
	 * Third oscillator. We do this first, since it may be used to modulate
	 * the first two oscillators. May have some mods on osc 3, but will not 
	 * have any sync.
	 */
	audiomain->palette[(*baudio->sound[2]).index]->specs->io[0].buf = freqbuf;
	audiomain->palette[(*baudio->sound[2]).index]->specs->io[1].buf = osc3buf;
	if (bexplorer->bus[1].flags & WAVE_1_MOD)
		audiomain->palette[(*baudio->sound[2]).index]->specs->io[2].buf
			= mbuf2;
	else if (bexplorer->bus[0].flags & WAVE_1_MOD)
		audiomain->palette[(*baudio->sound[2]).index]->specs->io[2].buf
			= mbuf1;
	else
		audiomain->palette[(*baudio->sound[2]).index]->specs->io[2].buf
			= zerobuf;
	audiomain->palette[(*baudio->sound[2]).index]->specs->io[3].buf = zerobuf;

	/*
	 * Fill the wavetable with the correct note value, which may be no kbd
	 * tracking enabled.
	 */
	if ((baudio->mixflags & OSC3_TRACKING) == 0)
	{
		register int i;
		register float div = 1;

		bufptr = freqbuf;

		if (baudio->mixflags & OSC3_LFO)
			div = 4;

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
			*bufptr++ = 0.001 + baudio->contcontroller[1] / div;
			*bufptr++ = 0.001 + baudio->contcontroller[1] / div;
			*bufptr++ = 0.001 + baudio->contcontroller[1] / div;
			*bufptr++ = 0.001 + baudio->contcontroller[1] / div;
			*bufptr++ = 0.001 + baudio->contcontroller[1] / div;
			*bufptr++ = 0.001 + baudio->contcontroller[1] / div;
			*bufptr++ = 0.001 + baudio->contcontroller[1] / div;
			*bufptr++ = 0.001 + baudio->contcontroller[1] / div;
		}
	} else {
		/*
		 * Need to find a graceful manner to support LO frequency even when
		 * tracking keyboard.
		if (baudio->mixflags & OSC3_LFO)
			?;
		 */
		fillFreqTable(baudio, voice, freqbuf, samplecount, 1);
	}

	if (bexplorer->bus[1].flags & OSC_3_MOD)
		bufmerge(mbuf2,
			12.0 + (voice->press + voice->chanpressure),
			freqbuf, 1.0, samplecount);
	if (bexplorer->bus[0].flags & OSC_3_MOD)
		bufmerge(mbuf1,
			12.0 + (voice->press + voice->chanpressure),
			freqbuf, 1.0, samplecount);

	/*
	 * And finally operate oscillator 3.
	 */
	(*baudio->sound[2]).operate(
		(audiomain->palette)[15],
		voice,
		(*baudio->sound[2]).param,
		baudio->locals[voice->index][2]);

	/*
	 * If we are mixing the osc3 source, put it into the startbuf (along with
	 * any existing ext source signal).
	 */
	if (baudio->mixflags & MIX_OSC3)
		bufmerge(osc3buf, 1.0, startbuf, 1.0, samplecount);

	if (bexplorer->bus[0].flags & MOD_OSC3)
		explorerMods(&bexplorer->bus[0], startbuf, mbuf1,
			audiomain->samplecount, baudio->contcontroller[1]);
	if (bexplorer->bus[1].flags & MOD_OSC3)
		explorerMods(&bexplorer->bus[1], startbuf, mbuf2,
			audiomain->samplecount, 1.0);

	if ((bexplorer->bus[0].flags & DEST_MASK)
		== (bexplorer->bus[1].flags & DEST_MASK))
		bufmerge(mbuf1, 1.0, mbuf2, 1.0, samplecount);

	/*
	 * If we are mixing the noise source, put it into the startbuf (along with
	 * any existing ext source signal).
	 */
	if (baudio->mixflags & MIX_NOISE)
		bufmerge(noisebuf, bexplorer->noisegain, startbuf, 1.0, samplecount);

	/*
	 * First oscillator
	 */
	audiomain->palette[(*baudio->sound[0]).index]->specs->io[0].buf
		= freqbuf;
	audiomain->palette[(*baudio->sound[0]).index]->specs->io[1].buf
		= osc1buf;
	if (bexplorer->bus[1].flags & WAVE_1_MOD)
		audiomain->palette[(*baudio->sound[0]).index]->specs->io[2].buf
			= mbuf2;
	else if (bexplorer->bus[0].flags & WAVE_1_MOD)
		audiomain->palette[(*baudio->sound[0]).index]->specs->io[2].buf
			= mbuf1;
	else
		audiomain->palette[(*baudio->sound[0]).index]->specs->io[2].buf
			= zerobuf;
	audiomain->palette[(*baudio->sound[0]).index]->specs->io[3].buf = zerobuf;
	audiomain->palette[(*baudio->sound[0]).index]->specs->io[4].buf = syncbuf;
	/*
	 * Fill tmpbuf1 with our frequency information - should already have it
	 * done, including mods?
	 */
	fillFreqTable(baudio, voice, freqbuf, samplecount, 1);
	/*
	 * Put in any mods we want.
	 */
	if (bexplorer->bus[1].flags & OSC_1_MOD)
		bufmerge(mbuf2,
			12.0 + (voice->press + voice->chanpressure),
			freqbuf, 1.0, samplecount);
	if (bexplorer->bus[0].flags & OSC_1_MOD)
		bufmerge(mbuf1,
			12.0 + (voice->press + voice->chanpressure),
			freqbuf, 1.0, samplecount);
	if (baudio->mixflags & FM_3_1)
		bufmerge(osc3buf,
			1.0 + (voice->press + voice->chanpressure),
			freqbuf, 1.0, samplecount);

	/*
	 * Operate this oscillator
	 */
	(*baudio->sound[0]).operate(
		(audiomain->palette)[15],
		voice,
		(*baudio->sound[0]).param,
		voice->locals[voice->index][0]);

	if (baudio->mixflags & MIX_OSC1)
	{
		/*
		 * merge the results.
		 */
		bufmerge(osc1buf, 1.0, startbuf, 1.0, samplecount);
	}

	/*
	 * We need to run through every bristolSound on the baudio sound chain.
	 * We need to pass the correct set of parameters to each operator, and
	 * ensure they get the correct local variable set.
	 *
	 * First insert our buffer pointers, we do this by inserting the buffers
	 * into the existing opspec structures.
	 */
	if (baudio->mixflags & MIX_OSC2)
	{
		audiomain->palette[(*baudio->sound[1]).index]->specs->io[0].buf
			= freqbuf;
		audiomain->palette[(*baudio->sound[1]).index]->specs->io[1].buf
			= startbuf;
		if (bexplorer->bus[1].flags & WAVE_1_MOD)
			audiomain->palette[(*baudio->sound[1]).index]->specs->io[2].buf
				= mbuf2;
		else if (bexplorer->bus[0].flags & WAVE_1_MOD)
			audiomain->palette[(*baudio->sound[1]).index]->specs->io[2].buf
				= mbuf1;
		else
			audiomain->palette[(*baudio->sound[1]).index]->specs->io[2].buf
				= zerobuf;
		if (baudio->mixflags & SYNC_1_2)
			audiomain->palette[(*baudio->sound[1]).index]->specs->io[3].buf
				= syncbuf;
		else
			audiomain->palette[(*baudio->sound[1]).index]->specs->io[3].buf
				= 0;
		audiomain->palette[(*baudio->sound[1]).index]->specs->io[4].buf = 0;

		/*
		 * Fill tmpbuf1 with our frequency information
		 */
		fillFreqTable(baudio, voice, freqbuf, samplecount, 1);
		/*
		 * If we have any mods on the oscillators, we need to put them in here.
		 * This should be under the control of polypressure and/or
		 * channelpressure?
		 */
		if (bexplorer->bus[1].flags & OSC_2_MOD)
			bufmerge(mbuf2,
				12.0 + (voice->press + voice->chanpressure),
				freqbuf, 1.0, samplecount);
		if (bexplorer->bus[0].flags & OSC_2_MOD)
			bufmerge(mbuf1,
				12.0 + (voice->press + voice->chanpressure),
				freqbuf, 1.0, samplecount);

		(*baudio->sound[1]).operate(
			(audiomain->palette)[15],
			voice,
			(*baudio->sound[1]).param,
			voice->locals[voice->index][1]);
	}

/* FIRST FILTER */
	/*
	 * Depending on the filter gain value, we need to adjust the contents of
	 * the adsrbuf.
	 */
	bufmerge(onbuf, 0.0, adsrbuf, bexplorer->fenvgain, samplecount);

	/* Filter input gain EQ */
	bufmerge(onbuf, 0.0, startbuf, 96.0, samplecount);

	if (bexplorer->bus[1].flags & FILTER_MOD)
		bufmerge(mbuf2,
			4.0 + (voice->press + voice->chanpressure),
			adsrbuf, 1.0f, samplecount);
	if (bexplorer->bus[0].flags & FILTER_MOD)
		bufmerge(mbuf1,
			4.0 + (voice->press + voice->chanpressure),
			adsrbuf, 1.0f, samplecount);

	/*
	 * Run the mixed oscillators into the filter. Input and output buffers
	 * are the same.
	 */
	audiomain->palette[(*baudio->sound[4]).index]->specs->io[0].buf = startbuf;
	audiomain->palette[(*baudio->sound[4]).index]->specs->io[1].buf = adsrbuf;
	audiomain->palette[(*baudio->sound[4]).index]->specs->io[2].buf = filtbuf;

	(*baudio->sound[4]).operate(
		(audiomain->palette)[3],
		voice,
		(*baudio->sound[4]).param,
		voice->locals[voice->index][4]);

/* SECOND FILTER */
	/*
	 * We have mods on the frequency, and we have a parameter on 'space' which
	 * is the frequency difference between the two oscillators. This can be
	 * under mod control, so we have to emulate that here.
	 */
	if (bexplorer->bus[1].flags & FSPACE_MOD)
		bufmerge(mbuf2, 4.0, adsrbuf, 1.0, samplecount);
	if (bexplorer->bus[0].flags & FSPACE_MOD)
		bufmerge(mbuf1, 4.0, adsrbuf, 1.0, samplecount);

	if (baudio->mixflags & FILTER_MODE) {
		audiomain->palette[(*baudio->sound[9]).index]->specs->io[0].buf
			= filtbuf;
		audiomain->palette[(*baudio->sound[9]).index]->specs->io[1].buf
			= adsrbuf;
		audiomain->palette[(*baudio->sound[9]).index]->specs->io[2].buf
			= filtbuf;
	} else {
		audiomain->palette[(*baudio->sound[9]).index]->specs->io[0].buf
			= startbuf;
		audiomain->palette[(*baudio->sound[9]).index]->specs->io[1].buf
			= adsrbuf;
		audiomain->palette[(*baudio->sound[9]).index]->specs->io[2].buf
			= filtbuf2;
	}

	(*baudio->sound[9]).operate(
		(audiomain->palette)[3],
		voice,
		(*baudio->sound[9]).param,
		voice->locals[voice->index][9]);
/* SECOND FILTER done */

/* OUTPUT STAGE. Need to alter this to allow for the dual filtering options */
	/*
	 * Run the ADSR for the final stage amplifier.
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

	/*
	 * See if we had HFP/LFP. If it is HPF/LPF we are mono, pass the same signal
	 * to each channel. Otherwise pass the second filter buffer to the rightbuf.
	 */
	if ((baudio->mixflags & FILTER_MODE) == 0)
		audiomain->palette[(*baudio->sound[6]).index]->specs->io[0].buf
			= filtbuf2;
	audiomain->palette[(*baudio->sound[6]).index]->specs->io[2].buf
		= baudio->rightbuf;

	(*baudio->sound[6]).operate(
		(audiomain->palette)[2],
		voice,
		(*baudio->sound[6]).param,
		voice->locals[voice->index][6]);

	return(0);
}

int
explorerPostops(audioMain *am, Baudio *ba, bristolVoice *v, register float *s)
{
	/* Output conditioning, prevents filter self oscillation from overdrive */
	bufmerge(ba->leftbuf, 0.0, ba->leftbuf, 0.2, am->samplecount);
	bufmerge(ba->rightbuf, 0.0, ba->rightbuf, 0.2, am->samplecount);

	bristolbzero(s, am->segmentsize);

	return(0);
}

static int
destroyOneExpVoice(audioMain *audiomain, Baudio *baudio)
{

	/*
	 * We need to leave these, we may have multiple invocations running
	 */
	return(0);

	bristolfree(freqbuf);
	bristolfree(osc1buf);
	bristolfree(osc3buf);
	bristolfree(noisebuf);
	bristolfree(filtbuf);
	bristolfree(filtbuf2);
	bristolfree(lfotri);
	bristolfree(lforamp);
	bristolfree(lfosh);
	bristolfree(adsrbuf);
	bristolfree(mbuf1);
	bristolfree(mbuf2);
	bristolfree(zerobuf);
	bristolfree(syncbuf);
	bristolfree(onbuf);

	return(0);
}

int
bristolExpInit(audioMain *audiomain, Baudio *baudio)
{
printf("initialising one bristolexplorer\n");
	baudio->soundCount = 10; /* Number of operators in this voice (Exp) */

	/*
	 * Assign an array of sound pointers.
	 */
	baudio->sound = (bristolSound **)
		bristolmalloc0(sizeof(bristolOP *) * baudio->soundCount);
	baudio->effect = (bristolSound **)
		bristolmalloc0(sizeof(bristolOP *) * baudio->soundCount);

	initSoundAlgo(15, 0, baudio, audiomain, baudio->sound);
	initSoundAlgo(15, 1, baudio, audiomain, baudio->sound);
	initSoundAlgo(15, 2, baudio, audiomain, baudio->sound);
	/* An ADSR */
	initSoundAlgo(1, 3, baudio, audiomain, baudio->sound);
	/* An explorer filter */
	initSoundAlgo(3, 4, baudio, audiomain, baudio->sound);
	/* Another ADSR */
	initSoundAlgo(1, 5, baudio, audiomain, baudio->sound);
	/* An amplifier */
	initSoundAlgo(2, 6, baudio, audiomain, baudio->sound);
	/* An noise source */
	initSoundAlgo(4, 7, baudio, audiomain, baudio->sound);
	/* LFO */
	initSoundAlgo(16, 8, baudio, audiomain, baudio->sound);
	/* Put in another filter for the overlay design. */
	initSoundAlgo(3, 9, baudio, audiomain, baudio->sound);

	baudio->param = expGlobalController;
	baudio->destroy = destroyOneExpVoice;
	baudio->operate = operateOneExpVoice;
	baudio->preops = operateExpPreops;
	baudio->postops = explorerPostops;

	if (freqbuf == (float *) NULL)
		freqbuf = (float *) bristolmalloc0(audiomain->segmentsize);
	if (osc3buf == (float *) NULL)
		osc3buf = (float *) bristolmalloc0(audiomain->segmentsize);
	if (noisebuf == (float *) NULL)
		noisebuf = (float *) bristolmalloc0(audiomain->segmentsize);
	if (filtbuf == (float *) NULL)
		filtbuf = (float *) bristolmalloc0(audiomain->segmentsize);
	if (filtbuf2 == (float *) NULL)
		filtbuf2 = (float *) bristolmalloc0(audiomain->segmentsize);
	if (lfotri == (float *) NULL)
		lfotri = (float *) bristolmalloc0(audiomain->segmentsize);
	if (lforamp == (float *) NULL)
		lforamp = (float *) bristolmalloc0(audiomain->segmentsize);
	if (lfosh == (float *) NULL)
		lfosh = (float *) bristolmalloc0(audiomain->segmentsize);
	if (adsrbuf == (float *) NULL)
		adsrbuf = (float *) bristolmalloc0(audiomain->segmentsize);
	if (mbuf1 == (float *) NULL)
		mbuf1 = (float *) bristolmalloc0(audiomain->segmentsize);
	if (mbuf2 == (float *) NULL)
		mbuf2 = (float *) bristolmalloc0(audiomain->segmentsize);
	if (osc1buf == (float *) NULL)
		osc1buf = (float *) bristolmalloc0(audiomain->segmentsize);
	if (zerobuf == (float *) NULL)
		zerobuf = (float *) bristolmalloc0(audiomain->segmentsize);
	if (syncbuf == (float *) NULL)
		syncbuf = (float *) bristolmalloc0(audiomain->segmentsize);
	if (onbuf == (float *) NULL)
	{
		int i;

		onbuf = (float *) bristolmalloc0(audiomain->segmentsize);

		for (i = 0; i < audiomain->samplecount; i++)
			onbuf[i] = BRISTOL_VPO;
	}

	/* We need to flag this for the parallel filters */
	baudio->mixflags |= BRISTOL_STEREO;
	baudio->mixlocals = bristolmalloc0(sizeof(bExp));

	return(0);
}

