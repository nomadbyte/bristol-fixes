
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

/* ARP Arp2600 algorithm. */

/*
 * Should consider adding a Hz->V for vocorder functions.
 */

/*#define DEBUG */

#include "bristol.h"
#include "bristolmm.h"
#include "bristolarp2600.h"

/*
 * Use of these arp2600 global buffers will be an issue with use of multiple
 * audio threads, unless we ensure a single thread deals with any given algo
 * type, since then they are only used sequentially.
 */
static float *fmbuf = (float *) NULL;
static float *filtbuf = (float *) NULL;
static float *noisebuf = (float *) NULL;
static float *adsrbuf = (float *) NULL;
static float *arbuf = (float *) NULL;
static float *mixbuf = (float *) NULL;
static float *shmix = (float *) NULL;
static float *zerobuf = (float *) NULL;
static float *lfosh = (float *) NULL;
static float *lfosine = (float *) NULL;
static float *lfosquare = (float *) NULL;
static float *osc1ramp = (float *) NULL;
static float *osc1puls = (float *) NULL;
static float *osc2ramp = (float *) NULL;
static float *osc2puls = (float *) NULL;
static float *ringmod = (float *) NULL;
static float *scratch = (float *) NULL;

/*
 * All buffers are going to be collapsed into a single 'megabuf', this will be
 * used for all local information for every possible one of these voices. As
 * such it might get big, but will allow for arbitrary buffering.
 */
static struct {
	float *buf;
	float *outputs[128][ARP_OUTCNT]; /* offsets into buffer for data */
	float *inputs[128][ARP_INCOUNT]; /* pointer offsets for input source */
	float gains[ARP_INCOUNT]; /* gain level for given input */
	int defaults[ARP_INCOUNT];
	int voiceCount, samplecount;
} abufs;

static int
arp2600ClearPatchtable()
{
	int i, v;

/*	printf("arp2600ClearPatchtable()\n"); */

	/*
	 * We need to put in all the default patches such that all the inputs 
	 * point to the desised output buffers.
	 */
	for (i = 0; i < ARP_INCOUNT; i++)
	{
		for (v = 0; v < abufs.voiceCount; v++)
			abufs.inputs[v][i] = &abufs.buf[
				abufs.defaults[i] + abufs.samplecount * v * ARP_OUTCNT];
	}
	return(0);
}

static int
arp2600AddPatch(int from, int to)
{
	int v;

	//printf("arp2600AddPatch(%i, %i)\n", from, to);

	for (v = 0; v < abufs.voiceCount; v++)
	{
		abufs.inputs[v][to] = &abufs.buf[
			abufs.samplecount * (from + v * ARP_OUTCNT)];
	}
	return(0);
}

static int
arp2600ClearPatch(int from, int to)
{
	int v;

	//printf("arp2600ClearPatch(%i, %i)\n", from, to);

	for (v = 0; v < abufs.voiceCount; v++)
	{
		abufs.inputs[v][to] = &abufs.buf[
			abufs.defaults[to] + abufs.samplecount * v * ARP_OUTCNT];
	}
	return(0);
}

int
arp2600Controller(Baudio *baudio, u_char operator, u_char controller, float value)
{
	int ivalue = value * CONTROLLER_RANGE;

#ifdef DEBUG
	printf("bristolArp2600Control(%i, %i, %f)\n", operator, controller, value);
#endif

	/*
	 * 99 will be the reverb
	 */
	if (operator == 99)
	{
		if (controller == 0)
		{
			baudio->effect[0]->param->param[controller].float_val = value;
			baudio->effect[0]->param->param[controller].int_val = 1;
		}
			/*
			else if (controller == 3)
			 * This is input gain and wet/dry level
			baudio->effect[0]->param->param[4].float_val
				= baudio->effect[0]->param->param[controller].float_val = 1;
			 */
		else
			baudio->effect[0]->param->param[controller].float_val = value;

/*
printf("F %f, C %f, W %f, G %f ? %f\n",
baudio->effect[0]->param->param[1].float_val,
baudio->effect[0]->param->param[2].float_val,
baudio->effect[0]->param->param[3].float_val,
baudio->effect[0]->param->param[4].float_val,
baudio->effect[0]->param->param[5].float_val);
*/

		return(0);
	}

	/*
	 * Operator 100/101/102 will be used for patching. We need a few general
	 * a few commands, clear all patches, clear one patch and add one patch.
	 *
	 * 102 will be to clear all patches to default.
	 * 100 will be to add a patch.
	 * 101 will be to clear a patch.
	 *
	 * We could use just 100 and have it be a toggle, but I don't like toggles
	 * as they are difficult to track between GUI and engine.
	 */
	if (operator == 102)
		/*
		 * Clear to default patch table.
		 */
		return(arp2600ClearPatchtable());

	if (operator == 100)
		return(arp2600AddPatch(controller, value * CONTROLLER_RANGE));

	if (operator == 101)
		return(arp2600ClearPatch(controller, value * CONTROLLER_RANGE));

	if (operator == 103)
	{
		/*
		 * Gain controls for the different inputs.
		 */
		abufs.gains[controller] = value;
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
		case 2: /* pan */
			((arp2600mods *) baudio->mixlocals)->pan = value;
			break;
		case 3: /* Oscillator tracking */
			switch (ivalue)
			{
				case 0:
					((arp2600mods *) baudio->mixlocals)->flags
						|= ARP_OSC1_TRACKING;
					break;
				case 2:
					((arp2600mods *) baudio->mixlocals)->flags
						&= ~ARP_OSC1_TRACKING;
					((arp2600mods *) baudio->mixlocals)->flags
						|= ARP_OSC1_LFO;
					break;
				case 1:
					((arp2600mods *) baudio->mixlocals)->flags
						&= ~ARP_OSC1_TRACKING;
					((arp2600mods *) baudio->mixlocals)->flags
						&= ~ARP_OSC1_LFO;
					break;
			}
			break;
		case 4: /* Oscillator tracking */
			switch (ivalue)
			{
				case 0:
					((arp2600mods *) baudio->mixlocals)->flags
						|= ARP_OSC2_TRACKING;
					break;
				case 2:
					((arp2600mods *) baudio->mixlocals)->flags
						&= ~ARP_OSC2_TRACKING;
					((arp2600mods *) baudio->mixlocals)->flags
						|= ARP_OSC2_LFO;
					break;
				case 1:
					((arp2600mods *) baudio->mixlocals)->flags
						&= ~ARP_OSC2_TRACKING;
					((arp2600mods *) baudio->mixlocals)->flags
						&= ~ARP_OSC2_LFO;
					break;
			}
			break;
		case 5: /* Oscillator tracking */
			switch (ivalue)
			{
				case 0:
					((arp2600mods *) baudio->mixlocals)->flags
						|= ARP_OSC3_TRACKING;
					break;
				case 2:
					((arp2600mods *) baudio->mixlocals)->flags
						&= ~ARP_OSC3_TRACKING;
					((arp2600mods *) baudio->mixlocals)->flags
						|= ARP_OSC3_LFO;
					break;
				case 1:
					((arp2600mods *) baudio->mixlocals)->flags
						&= ~ARP_OSC3_TRACKING;
					((arp2600mods *) baudio->mixlocals)->flags
						&= ~ARP_OSC3_LFO;
					break; } break;
		case 6:
			((arp2600mods *) baudio->mixlocals)->lfogain = value;
			break;
		case 7:
			/*
			 * Else multi lfo
			 */
			if (ivalue)
				((arp2600mods *) baudio->mixlocals)->flags &= ~A_AR_MULTI;
			else
				((arp2600mods *) baudio->mixlocals)->flags |= A_AR_MULTI;
			break;
		case 8:
			((arp2600mods *) baudio->mixlocals)->fxLpan = value;
			break;
		case 9:
			((arp2600mods *) baudio->mixlocals)->fxRpan = value;
			break;
		case 10:
			((arp2600mods *) baudio->mixlocals)->preamp = value * 16.0f;
			break;
		case 11: /* This looks quite high, it has to normalise the signals */
			((arp2600mods *) baudio->mixlocals)->volume = value * 256;
			break;
		case 12:
			((arp2600mods *) baudio->mixlocals)->initialvolume = value * 12;
			break;
	}
	return(0);
}

int
arp2600Preops(audioMain *audiomain, Baudio *baudio,
bristolVoice *voice, register float *startbuf)
{
	arp2600mods *mods = (arp2600mods *) baudio->mixlocals;
	register int samplecount = audiomain->samplecount, i;
	register float gain = mods->preamp;
	float *b = abufs.outputs[0][ARP_O_PREAMP], *s = startbuf;

	/*
	 * There are a couple of operations that should be done here, but the main
	 * one is the pre-amplifier on the startbuf which should only be applied
	 * once. We first copy it to the first voice, and worry about the rest 
	 * when they come around in the poly routine below.
	 */
	memset(b, 0.0f, samplecount * sizeof(float));
	for (i = 0; i < samplecount; i+=8)
	{
		*b++ = *s++ * gain;
		*b++ = *s++ * gain;
		*b++ = *s++ * gain;
		*b++ = *s++ * gain;
		*b++ = *s++ * gain;
		*b++ = *s++ * gain;
		*b++ = *s++ * gain;
		*b++ = *s++ * gain;
	}

	return(0);
}

static void
invert(register float *src, float *dst, register int sc)
{
	register int i;

	for (i = 0; i < sc; i+=8)
	{
		*dst++ = -*src++;
		*dst++ = -*src++;
		*dst++ = -*src++;
		*dst++ = -*src++;
		*dst++ = -*src++;
		*dst++ = -*src++;
		*dst++ = -*src++;
		*dst++ = -*src++;
	}
}

int
operateOneArp2600(audioMain *audiomain, Baudio *baudio,
bristolVoice *voice, register float *startbuf)
{
	arp2600mods *mods = (arp2600mods *) baudio->mixlocals;
	int sc = audiomain->samplecount, kflags = 0, vindex = 0;
	int ss = audiomain->segmentsize, i;
	float *s = abufs.outputs[0][ARP_O_PREAMP], *b, g;

/*printf("operateOneArp2600(%i, %x, %x)\n", vindex, audiomain, baudio); */

	vindex = voice->index;

	b = abufs.outputs[vindex][ARP_O_PREAMP];

	/*
	 * As of 0.50.4 we should be supporting 4 IO for CV or audio from Jack,
	 * these buffers if available should be copied over to maintain consistency.
	 */
	if (audiomain->iocount > 0)
	{
		for (i = 0; i < audiomain->iocount; i++)
		{
			if (audiomain->io_i[i] == NULL)
				continue;

			bufmerge(
				audiomain->io_i[i], 12.0f, //4.0 * audiomain->m_io_igc,
				abufs.outputs[vindex][ARP_O_IN_1 + i], 0.0,
				sc);
		}
	}

	bristolbzero(abufs.inputs[vindex][ARP_I_OUT_1], sc);
	bristolbzero(abufs.inputs[vindex][ARP_I_OUT_2], sc);
	bristolbzero(abufs.inputs[vindex][ARP_I_OUT_3], sc);
	bristolbzero(abufs.inputs[vindex][ARP_I_OUT_4], sc);

/* Preamp patching and envelope follower. */
	/*
	 * Since the input buffer is singular and we may have multiple voice, then
	 * to get the correct routing we need to copy this to each voice buffers.
	 */
	if (vindex != 0)
	{
		for (i = 0; i < sc; i+=8)
		{
			*b++ = *s++;
			*b++ = *s++;
			*b++ = *s++;
			*b++ = *s++;
			*b++ = *s++;
			*b++ = *s++;
			*b++ = *s++;
			*b++ = *s++;
		}
	}

	/* Envelope follower. */
	audiomain->palette[(*baudio->sound[11]).index]->specs->io[0].buf =
		abufs.inputs[vindex][ARP_I_ENVFOLLOW];
	audiomain->palette[(*baudio->sound[11]).index]->specs->io[1].buf =
		abufs.outputs[vindex][ARP_O_ENVFOLLOW];
	(*baudio->sound[11]).operate(
		(audiomain->palette)[23],
		voice,
		(*baudio->sound[11]).param,
		voice->locals[vindex][11]);
/* Preamp done */

/* NOISE */
	/*
	 * This had to be moved to a multi operator to support the correct routing.
	 */
	bristolbzero(abufs.outputs[vindex][ARP_O_NOISE], ss);
	audiomain->palette[(*baudio->sound[6]).index]->specs->io[0].buf =
		abufs.outputs[vindex][ARP_O_NOISE];
	(*baudio->sound[6]).operate(
		(audiomain->palette)[4],
		voice,
		(*baudio->sound[6]).param,
		voice->locals[vindex][6]);
/* NOISE DONE */

/* LFO */
	audiomain->palette[(*baudio->sound[9]).index]->specs->io[0].buf
		= abufs.inputs[vindex][ARP_I_SH_IN];
	audiomain->palette[(*baudio->sound[9]).index]->specs->io[1].buf
		= abufs.outputs[vindex][ARP_O_CLOCK];
	audiomain->palette[(*baudio->sound[9]).index]->specs->io[3].buf
		= abufs.outputs[vindex][ARP_O_SH];

	(*baudio->sound[9]).operate(
		(audiomain->palette)[16],
		voice,
		(*baudio->sound[9]).param,
		voice->locals[vindex][9]);
	/*
	 * Apply a gain to the LFO for mods.
	 */
	bufmerge(abufs.outputs[vindex][ARP_O_KBD_CV], 0.0, abufs.outputs[vindex][ARP_O_CLOCK],
		4.0 * mods->lfogain, sc);
/* */

/* ADSR */

	/*
	 * AR:
	 * We are either going to do KBD gate and trig, or alternative input.
	 * If we have the alternative input then we need to look for a leading 
	 * edge to detect KEY_REON and positive signal to build a gate?
	 *
	if ((mods->arAutoTransit < 0) && (*lfosquare > 0) &&
		((voice->flags & (BRISTOL_KEYOFF|BRISTOL_KEYOFFING)) == 0))
			kflags |= BRISTOL_KEYREON;
	mods->arAutoTransit  = *lfosquare;
	 */

	kflags = voice->flags;

	if (mods->flags & A_AR_MULTI)
	{
		/*
		 * We have a buffer, see if we need to detect edge.
		 */
		if ((*abufs.inputs[vindex][ARP_I_AR_GATE] > 0) &&
			((voice->flags & (BRISTOL_KEYOFF|BRISTOL_KEYOFFING)) == 0))
				voice->flags |= BRISTOL_KEYREON;
		if (*abufs.inputs[vindex][ARP_I_AR_GATE] < 0)
				voice->flags |= BRISTOL_KEYOFF;
	}

	audiomain->palette[(*baudio->sound[7]).index]->specs->io[0].buf
		= abufs.outputs[vindex][ARP_O_AR];
	(*baudio->sound[7]).operate(
		(audiomain->palette)[1],
		voice,
		(*baudio->sound[7]).param,
		voice->locals[vindex][7]);

	voice->flags = kflags;

	/*
	 * Run the ADSR.
	 */
	audiomain->palette[(*baudio->sound[4]).index]->specs->io[0].buf =
		abufs.outputs[vindex][ARP_O_ADSR];
	(*baudio->sound[4]).operate(
		(audiomain->palette)[1],
		voice,
		(*baudio->sound[4]).param,
		voice->locals[vindex][4]);

/*
	if (mods->adsrAuto)
		voice->flags |= kflags;
	else
		voice->flags &= ~kflags;
*/
/* ADSR - OVER */

	/*
	 * We now have our mods, Noise, ADSR, S/H, Sine, Square.
	 *
	 * Run the oscillators.
	 */
/* OSC */
	audiomain->palette[(*baudio->sound[0]).index]->specs->io[0].buf = fmbuf;
	audiomain->palette[(*baudio->sound[0]).index]->specs->io[1].buf = zerobuf;
	audiomain->palette[(*baudio->sound[0]).index]->specs->io[2].buf =
		abufs.outputs[vindex][ARP_O_VCO1_SAW];
	audiomain->palette[(*baudio->sound[0]).index]->specs->io[3].buf = NULL;
	audiomain->palette[(*baudio->sound[0]).index]->specs->io[4].buf =
		abufs.outputs[vindex][ARP_O_VCO1_SQR];
	audiomain->palette[(*baudio->sound[0]).index]->specs->io[5].buf = NULL;
	audiomain->palette[(*baudio->sound[0]).index]->specs->io[6].buf = NULL;
/*printf("%i: osc1 to %x\n", vindex, */
/*abufs.outputs[vindex][ARP_O_VCO1_SQR]); */

	/*
	 * We need to make this connection for CV KBD open again. If nothing is
	 * patched in then we execute this code, or rather, this freq table goes
	 * into a buffer which is the default input. If the control is repatched
	 * then we need to configure a 1/Octave intepretter for the alternative
	 * buffer. This leads to a few issues with the filter which I think takes
	 * a linear 1.0f for frequency control - these differences are historical
	 * and we should see about resolving them for this emulator, later the 
	 * others.
	 */
	fillFreqTable(baudio, voice,
		abufs.outputs[vindex][ARP_O_KBD_CV], sc, 1);

	if (mods->flags & ARP_OSC1_TRACKING)
		bufmerge(abufs.inputs[vindex][ARP_I_VCO1_KBD], 1.0,
			fmbuf, 0.0, sc);
	else {
		register int i;
		register float *bufptr = fmbuf, value = 5.0, mult = 0.25;

		if (mods->flags & ARP_OSC1_LFO)
		{
			value = 0.005;
			mult = 0.10;
		}

		if (abufs.inputs[vindex][ARP_I_VCO1_KBD] == 
			abufs.outputs[vindex][ARP_O_KBD_CV])
			mult *= baudio->contcontroller[1];

		mult += value;
		/*
		 * If we do not have keyboard tracking of Osc, then put in a LFO
		 * frequency. This is about 0.1 Hz, although we should consider
		 * having an alternative LFO, since this method plays each sample
		 * about 400 times.
		 *
		 * Frequency of OSC without keyboard control is under management of
		 * midi continuous controller 1.
		 */
		for (i = 0; i < sc; i+=8)
		{
			*bufptr++ = mult;
			*bufptr++ = mult;
			*bufptr++ = mult;
			*bufptr++ = mult;
			*bufptr++ = mult;
			*bufptr++ = mult;
			*bufptr++ = mult;
			*bufptr++ = mult;
		}
	}

	/*
	 * Put in any desired mods. There are two, MG and Wheel mod. Wheel mod is
	 * actually quite week, takes the LFO to freqbuf, same as MG without the
	 * env. Hm.
	 *
	 * There are 3 FM (KBD CV separate) and 1 PM.
	 */
	if (abufs.gains[ARP_I_VCO1_SH] != 0.0f)
		bufmerge(abufs.inputs[vindex][ARP_I_VCO1_SH],
			abufs.gains[ARP_I_VCO1_SH], fmbuf, 1.0, sc);
	if (abufs.gains[ARP_I_VCO1_ADSR] != 0.0f)
		bufmerge(abufs.inputs[vindex][ARP_I_VCO1_ADSR],
			abufs.gains[ARP_I_VCO1_ADSR], fmbuf, 1.0, sc);
	if (abufs.gains[ARP_I_VCO1_VCO2] != 0.0f)
		bufmerge(abufs.inputs[vindex][ARP_O_VCO2_SIN],
			abufs.gains[ARP_I_VCO1_VCO2], fmbuf, 1.0, sc);

	bristolbzero(abufs.outputs[vindex][ARP_O_VCO1_SAW], ss);
	bristolbzero(abufs.outputs[vindex][ARP_O_VCO1_SQR], ss);

	(*baudio->sound[0]).operate(
		(audiomain->palette)[19],
		voice,
		(*baudio->sound[0]).param,
		voice->locals[vindex][0]);

/* done-1 */

	audiomain->palette[(*baudio->sound[1]).index]->specs->io[0].buf = fmbuf;
	audiomain->palette[(*baudio->sound[1]).index]->specs->io[1].buf = scratch;
	audiomain->palette[(*baudio->sound[1]).index]->specs->io[2].buf =
		abufs.outputs[vindex][ARP_O_VCO2_SAW];
	audiomain->palette[(*baudio->sound[1]).index]->specs->io[3].buf =
		abufs.outputs[vindex][ARP_O_VCO1_SQR]; /* sync source */
	audiomain->palette[(*baudio->sound[1]).index]->specs->io[4].buf =
		abufs.outputs[vindex][ARP_O_VCO2_SQR];
	audiomain->palette[(*baudio->sound[1]).index]->specs->io[5].buf =
		abufs.outputs[vindex][ARP_O_VCO2_SIN];
	audiomain->palette[(*baudio->sound[1]).index]->specs->io[6].buf =
		abufs.outputs[vindex][ARP_O_VCO2_TRI];

	/*
	 * Fill our fmbuf scratch table again.
	 */
	if (mods->flags & ARP_OSC2_TRACKING)
		bufmerge(abufs.inputs[vindex][ARP_I_VCO2_KBD], 1.0,
			fmbuf, 0.0, sc);
	else {
		register int i;
		register float *bufptr = fmbuf, value = 5.0, mult = 0.25;

		if (mods->flags & ARP_OSC2_LFO)
		{
			value = 0.005;
			mult = 0.10;
		}

		if (abufs.inputs[vindex][ARP_I_VCO2_KBD] == 
			abufs.outputs[vindex][ARP_O_KBD_CV])
			mult *= baudio->contcontroller[1];

		mult += value;

		/*
		 * If we do not have keyboard tracking of Osc, then put in a LFO
		 * frequency. This is about 0.1 Hz, although we should consider
		 * having an alternative LFO, since this method plays each sample
		 * about 400 times.
		 *
		 * Frequency of OSC without keyboard control is under management of
		 * midi continuous controller 1.
		 */
		for (i = 0; i < sc; i+=8)
		{
			*bufptr++ = mult;
			*bufptr++ = mult;
			*bufptr++ = mult;
			*bufptr++ = mult;
			*bufptr++ = mult;
			*bufptr++ = mult;
			*bufptr++ = mult;
			*bufptr++ = mult;
		}
	}

	if (abufs.gains[ARP_I_VCO2_SH] != 0.0f)
		bufmerge(abufs.inputs[vindex][ARP_I_VCO2_SH],
			abufs.gains[ARP_I_VCO2_SH], fmbuf, 1.0, sc);

	if (abufs.gains[ARP_I_VCO2_ADSR] != 0.0f)
		bufmerge(abufs.inputs[vindex][ARP_I_VCO2_ADSR],
			abufs.gains[ARP_I_VCO2_ADSR], fmbuf, 1.0, sc);

	if (abufs.gains[ARP_I_VCO2_VCO1] != 0.0f)
		bufmerge(abufs.inputs[vindex][ARP_I_VCO2_VCO1],
			abufs.gains[ARP_I_VCO2_VCO1], fmbuf, 1.0, sc);

	bufmerge(abufs.inputs[vindex][ARP_I_VCO2_NSE],
		abufs.gains[ARP_I_VCO2_NSE] * 48, scratch, 0.0, sc);

	bristolbzero(abufs.outputs[vindex][ARP_O_VCO2_SAW], ss);
	bristolbzero(abufs.outputs[vindex][ARP_O_VCO2_SQR], ss);
	bristolbzero(abufs.outputs[vindex][ARP_O_VCO2_SIN], ss);
	bristolbzero(abufs.outputs[vindex][ARP_O_VCO2_TRI], ss);

	(*baudio->sound[1]).operate(
		(audiomain->palette)[19],
		voice,
		(*baudio->sound[1]).param,
		voice->locals[vindex][1]);

/* done-2 */

	audiomain->palette[(*baudio->sound[1]).index]->specs->io[0].buf = fmbuf;
	audiomain->palette[(*baudio->sound[1]).index]->specs->io[1].buf = scratch;
	audiomain->palette[(*baudio->sound[1]).index]->specs->io[2].buf =
		abufs.outputs[vindex][ARP_O_VCO3_SAW];
	audiomain->palette[(*baudio->sound[1]).index]->specs->io[3].buf = zerobuf;
	audiomain->palette[(*baudio->sound[1]).index]->specs->io[4].buf =
		abufs.outputs[vindex][ARP_O_VCO3_SQR];
	audiomain->palette[(*baudio->sound[1]).index]->specs->io[5].buf =
		abufs.outputs[vindex][ARP_O_VCO3_SIN];
	audiomain->palette[(*baudio->sound[1]).index]->specs->io[6].buf =
		abufs.outputs[vindex][ARP_O_VCO3_TRI];

	/*
	 * Fill our fmbuf scratch table again.
	 */
	if (mods->flags & ARP_OSC3_TRACKING)
		bufmerge(abufs.inputs[vindex][ARP_I_VCO3_KBD], 1.0,
			fmbuf, 0.0, sc);
	else {
		register int i;
		register float *bufptr = fmbuf, value = 5.0f, mult = 0.25f;

		if (mods->flags & ARP_OSC3_LFO)
		{
			value = 0.005;
			mult = 0.10;
		}

		if (abufs.inputs[vindex][ARP_I_VCO3_KBD] == 
			abufs.outputs[vindex][ARP_O_KBD_CV])
			mult *= baudio->contcontroller[1];

		mult += value;

		/*
		 * If we do not have keyboard tracking of Osc, then put in a LFO
		 * frequency. This is about 0.1 Hz, although we should consider
		 * having an alternative LFO, since this method plays each sample
		 * about 400 times.
		 *
		 * Frequency of OSC without keyboard control is under management of
		 * midi continuous controller 1.
		 */
		for (i = 0; i < sc; i+=8)
		{
			*bufptr++ = mult;
			*bufptr++ = mult;
			*bufptr++ = mult;
			*bufptr++ = mult;
			*bufptr++ = mult;
			*bufptr++ = mult;
			*bufptr++ = mult;
			*bufptr++ = mult;
		}
	}

	if (abufs.gains[ARP_I_VCO3_NSE] != 0.0f)
		bufmerge(abufs.inputs[vindex][ARP_I_VCO3_NSE],
			abufs.gains[ARP_I_VCO3_NSE], fmbuf, 1.0, sc);
	if (abufs.gains[ARP_I_VCO3_ADSR] != 0.0f)
		bufmerge(abufs.inputs[vindex][ARP_I_VCO3_ADSR],
			abufs.gains[ARP_I_VCO3_ADSR], fmbuf, 1.0, sc);
	if (abufs.gains[ARP_I_VCO3_SIN] != 0.0f)
		bufmerge(abufs.inputs[vindex][ARP_I_VCO3_SIN],
			abufs.gains[ARP_I_VCO3_SIN], fmbuf, 1.0, sc);

	bufmerge(abufs.inputs[vindex][ARP_I_VCO3_TRI],
		abufs.gains[ARP_I_VCO3_TRI] * 48, scratch, 0.0, sc);

	bristolbzero(abufs.outputs[vindex][ARP_O_VCO3_SAW], ss);
	bristolbzero(abufs.outputs[vindex][ARP_O_VCO3_SQR], ss);
	bristolbzero(abufs.outputs[vindex][ARP_O_VCO3_SIN], ss);
	bristolbzero(abufs.outputs[vindex][ARP_O_VCO3_TRI], ss);

	(*baudio->sound[2]).operate(
		(audiomain->palette)[19],
		voice,
		(*baudio->sound[2]).param,
		voice->locals[vindex][2]);
/* OSC done */

/* Ring mod */
	audiomain->palette[(*baudio->sound[8]).index]->specs->io[0].buf
		= abufs.inputs[vindex][ARP_I_RM1];
	audiomain->palette[(*baudio->sound[8]).index]->specs->io[1].buf
		= abufs.inputs[vindex][ARP_I_RM2];
	audiomain->palette[(*baudio->sound[8]).index]->specs->io[2].buf
		= abufs.outputs[vindex][ARP_O_RINGMOD];
	(*baudio->sound[8]).operate(
		(audiomain->palette)[20],
		voice,
		(*baudio->sound[8]).param,
		voice->locals[vindex][8]);
/* */

/* Voltage processors */
	/*
	 * These should not be put into operators as they are far from generic.
	 * This 'may' be done in a future release depending on the EMS Synthi A
	 * developments.
	 *
	 * These are controllers 44 to 47.
	 */
	bufmerge(abufs.inputs[vindex][ARP_I_MIX_1_1],
		abufs.gains[ARP_I_MIX_1_1] * 2.0f,
		abufs.outputs[vindex][ARP_O_MIX_1], 0.0, sc);
	bufmerge(abufs.inputs[vindex][ARP_I_MIX_1_2],
		abufs.gains[ARP_I_MIX_1_2] * 2.0f,
		abufs.outputs[vindex][ARP_O_MIX_1], 1.0, sc);
	bufmerge(abufs.inputs[vindex][ARP_I_MIX_1_3], 1.0,
		abufs.outputs[vindex][ARP_O_MIX_1], 1.0, sc);
	bufmerge(abufs.inputs[vindex][ARP_I_MIX_1_4], 1.0,
		abufs.outputs[vindex][ARP_O_MIX_1], 1.0, sc);

	/* Invert the signal */
	invert(abufs.outputs[vindex][ARP_O_MIX_1],
		abufs.outputs[vindex][ARP_O_MIX_1], sc);

	bufmerge(abufs.inputs[vindex][ARP_I_MIX_2_1],
		abufs.gains[ARP_I_MIX_2_1] * 2.0f,
		abufs.outputs[vindex][ARP_O_MIX_2], 0.0, sc);
	bufmerge(abufs.inputs[vindex][ARP_I_MIX_2_2], 1.0,
		abufs.outputs[vindex][ARP_O_MIX_2], 1.0, sc);
	/* Invert the signal */
	invert(abufs.outputs[vindex][ARP_O_MIX_2],
		abufs.outputs[vindex][ARP_O_MIX_2], sc);

	/*
	 * Build the second output
	 */
	bufmerge(abufs.outputs[vindex][ARP_O_MIX_2], 1.0,
		abufs.outputs[vindex][ARP_O_MIX_2_2], 0.0, sc);

	/*
	 * The lag processor. Hm, this is actually very similar to a rooney filter
	 * with a stupidly low cutoff. FFS as it needs to be a native rooney and
	 * not a modified one with emphasis.
	 */
	audiomain->palette[(*baudio->sound[12]).index]->specs->io[0].buf
		= abufs.inputs[vindex][ARP_I_LAG];
	audiomain->palette[(*baudio->sound[12]).index]->specs->io[2].buf
		= abufs.outputs[vindex][ARP_O_LAG];
	(*baudio->sound[12]).operate(
		(audiomain->palette)[3],
		voice,
		(*baudio->sound[12]).param,
		voice->locals[vindex][12]);
/* */

/* VP */

/* Electroswitch */
	audiomain->palette[(*baudio->sound[10]).index]->specs->io[0].buf
		= abufs.inputs[vindex][ARP_I_SWITCH_1];
	audiomain->palette[(*baudio->sound[10]).index]->specs->io[1].buf
		= abufs.inputs[vindex][ARP_I_SWITCH_2];
	audiomain->palette[(*baudio->sound[10]).index]->specs->io[2].buf
		= abufs.inputs[vindex][ARP_I_SWITCHCLK];
	audiomain->palette[(*baudio->sound[10]).index]->specs->io[3].buf
		= abufs.outputs[vindex][ARP_O_SWITCH];
	(*baudio->sound[10]).operate(
		(audiomain->palette)[21],
		voice,
		(*baudio->sound[10]).param,
		voice->locals[vindex][10]);
/* */

/* Mixer */
	bufmerge(abufs.inputs[vindex][ARP_I_FILT_RM],
		abufs.gains[ARP_I_FILT_RM] * 96.0, mixbuf, 0.0, sc);
	bufmerge(abufs.inputs[vindex][ARP_I_FILT_1SQR],
		abufs.gains[ARP_I_FILT_1SQR] * 96.0, mixbuf, 1.0, sc);
	bufmerge(abufs.inputs[vindex][ARP_I_FILT_2SQR],
		abufs.gains[ARP_I_FILT_2SQR] * 96.0, mixbuf, 1.0, sc);
	bufmerge(abufs.inputs[vindex][ARP_I_FILT_3TRI],
		abufs.gains[ARP_I_FILT_3TRI] * 96.0, mixbuf, 1.0, sc);
	bufmerge(abufs.inputs[vindex][ARP_I_FILT_NSE],
		abufs.gains[ARP_I_FILT_NSE] * 48.0, mixbuf, 1.0, sc);
/* */

/* Filter */
	audiomain->palette[(*baudio->sound[3]).index]->specs->io[0].buf = mixbuf;
	audiomain->palette[(*baudio->sound[3]).index]->specs->io[1].buf = scratch;
	audiomain->palette[(*baudio->sound[3]).index]->specs->io[2].buf
		= abufs.outputs[vindex][ARP_O_VCF];

	/*
	 * Fill the KBD_CV now with a linear value related to the MIDI key number.
	 */
	b = abufs.outputs[vindex][ARP_O_KBD_CV];
	g = abufs.gains[ARP_I_FILT_KBD] * voice->cfreq * 1024 / baudio->samplerate;
//printf("gain is %f %f %f\n", g, voice->cfreq, abufs.gains[ARP_I_FILT_KBD]);
	for (i = 0; i < sc; i+=8)
	{
		*b++ = g;
		*b++ = g;
		*b++ = g;
		*b++ = g;
		*b++ = g;
		*b++ = g;
		*b++ = g;
		*b++ = g;
	}

	bufmerge(abufs.inputs[vindex][ARP_I_FILT_KBD],
		abufs.gains[ARP_I_FILT_KBD], scratch, 0.0, sc);
	bufmerge(abufs.inputs[vindex][ARP_I_FILT_ADSR],
		abufs.gains[ARP_I_FILT_ADSR] * 0.25, scratch, 1.0, sc);
	bufmerge(abufs.inputs[vindex][ARP_I_FILT_2SIN],
		abufs.gains[ARP_I_FILT_2SIN] * 0.25, scratch, 1.0, sc);

	bristolbzero(abufs.outputs[vindex][ARP_O_VCF], ss);

	(*baudio->sound[3]).operate(
		(audiomain->palette)[B_FILTER2],
		voice,
		(*baudio->sound[3]).param,
		voice->locals[vindex][3]);

	/* Need to normalise the filter output again */
	bufmerge(abufs.outputs[vindex][ARP_O_VCF], 0.016,
		abufs.outputs[vindex][ARP_O_VCF], 0.0, sc);
/*
 * If the filter is taken back through another device, probably unlikely but 
 * very possible, the front end gain from the mixer will result in very strong
 * signal.
 */
/* Filter DONE */

/* AMP */
	audiomain->palette[(*baudio->sound[5]).index]->specs->io[0].buf
		= mixbuf;
	audiomain->palette[(*baudio->sound[5]).index]->specs->io[1].buf
		= scratch;
	audiomain->palette[(*baudio->sound[5]).index]->specs->io[2].buf
		= abufs.outputs[vindex][ARP_O_VCA];

	bufmerge(abufs.inputs[vindex][ARP_I_VCA_VCF],
		abufs.gains[ARP_I_VCA_VCF], mixbuf, 0.0, sc);
	bufmerge(abufs.inputs[vindex][ARP_I_VCA_RM],
		abufs.gains[ARP_I_VCA_RM], mixbuf, 1.0, sc);

	bufmerge(abufs.inputs[vindex][ARP_I_VCA_AR],
		abufs.gains[ARP_I_VCA_AR], scratch, 0.0, sc);
	bufmerge(abufs.inputs[vindex][ARP_I_VCA_ADSR],
		abufs.gains[ARP_I_VCA_ADSR], scratch, 1.0, sc);

	bufadd(scratch, mods->initialvolume, audiomain->samplecount);

	bristolbzero(abufs.outputs[vindex][ARP_O_VCA], ss);

	(*baudio->sound[5]).operate(
		(audiomain->palette)[2],
		voice,
		(*baudio->sound[5]).param,
		voice->locals[vindex][5]);

	/* Need to normalise the amp output again */
	bufmerge(abufs.outputs[vindex][ARP_O_VCA], 0.07,
		abufs.outputs[vindex][ARP_O_VCA], 0.0, sc);
/* AMP OVER */

/* Mixer */
	/*
	 * Still need to get the specifications of the mixer output. It minimally
	 * goes into the panning as seen in the postOps, but there are two other
	 * input/outputs and they are either full mix outputs, or, as done here,
	 * 'postfades' on the respetive inputs.
	 *
	 * Mixing:
	 *	This is the polyphonic algorithm. There are several components that 
	 *	need to be collapsed onto single buffers rather than one per voice:
	 *
	 *	Main out.
	 *	Chorus input.
	 */
	bufmerge(
		abufs.inputs[vindex][ARP_I_MIX_VCF], abufs.gains[ARP_I_MIX_VCF],
		abufs.outputs[vindex][ARP_O_MIX_OUT1], 0.0, sc);
	bufmerge(
		abufs.inputs[vindex][ARP_I_MIX_VCA], abufs.gains[ARP_I_MIX_VCA],
		abufs.outputs[vindex][ARP_O_MIX_OUT2], 0.0, sc);

	/*
	 * Pan should take an alternative input, so we should premix into a shared
	 * buffer and then pan, plus give access to this buffer. These are done
	 * independently so that they can be rerouted.
	 */
	bufmerge(
		abufs.outputs[vindex][ARP_O_MIX_OUT1], 1.0,
		abufs.outputs[vindex][ARP_O_MIXER], 0.0, sc);
	bufmerge(
		abufs.outputs[vindex][ARP_O_MIX_OUT2], 1.0,
		abufs.outputs[vindex][ARP_O_MIXER], 1.0, sc);

	/*
	 * These are the separate L/R inputs. They are collapsed between all voices
	 * and then mixed into the stereo output in PostOps.
	 * At this point there is not remixing possible - collapse the outputs.
	 */
	if (vindex != 0)
	{
		/*
		 * The only way I can see to get this through would be to 'steal' the
		 * next available pan buffer as we only use one anyway.
		 */
		bufmerge(
			abufs.inputs[vindex][ARP_I_LEFT_IN], 128.0,
			abufs.outputs[1][ARP_O_PAN], 1.0, sc);
		bufmerge(
			abufs.inputs[vindex][ARP_I_RIGHT_IN], 128.0,
			abufs.outputs[2][ARP_O_PAN], 1.0, sc);
		bufmerge(
			abufs.inputs[vindex][ARP_I_PAN], 1.0,
			abufs.outputs[0][ARP_O_PAN], 1.0, sc);
		bufmerge(
			abufs.inputs[vindex][ARP_I_MIX_FX], 1.0,
			baudio->leftbuf, 1.0, sc);
	}
/* */

	/*
	 * As of 0.50.4 we should be supporting 4 IO for CV or audio from Jack,
	 * these buffers if available should be copied over to maintain consistency.
	 */
	if (audiomain->iocount > 0)
	{
		register int i;
		register float *s, *d;

		if (audiomain->io_o[0] != NULL)
		{
			//bristolbzero(audiomain->io_o[0], audiomain->segmentsize);
			bufmerge(
				abufs.inputs[vindex][ARP_I_OUT_1], 0.0833f,
				audiomain->io_o[0], 1.0f, //4.0 * audiomain->m_io_igc,
				audiomain->samplecount);
		}

		if ((audiomain->io_o[1] != NULL) && (audiomain->iocount > 1))
		{
			//bristolbzero(audiomain->io_o[1], audiomain->segmentsize);
			bufmerge(
				abufs.inputs[vindex][ARP_I_OUT_2], 0.0833f,
				audiomain->io_o[1], 1.0f, //4.0 * audiomain->m_io_igc,
				audiomain->samplecount);
		}

		if (((d = audiomain->io_o[2]) != NULL) && (audiomain->iocount > 2))
		{
			s = abufs.inputs[vindex][ARP_I_OUT_3];
			//bristolbzero(d, audiomain->segmentsize);

			/* Third IO will be DC */
			for (i = 0; i < sc; i+=8)
			{
				if ((*d += *s * 0.04166667f + 0.5f) < 0) *d = 0.0f;s++;d++;
				if ((*d += *s * 0.04166667f + 0.5f) < 0) *d = 0.0f;s++;d++;
				if ((*d += *s * 0.04166667f + 0.5f) < 0) *d = 0.0f;s++;d++;
				if ((*d += *s * 0.04166667f + 0.5f) < 0) *d = 0.0f;s++;d++;
				if ((*d += *s * 0.04166667f + 0.5f) < 0) *d = 0.0f;s++;d++;
				if ((*d += *s * 0.04166667f + 0.5f) < 0) *d = 0.0f;s++;d++;
				if ((*d += *s * 0.04166667f + 0.5f) < 0) *d = 0.0f;s++;d++;
				if ((*d += *s * 0.04166667f + 0.5f) < 0) *d = 0.0f;s++;d++;
			}
		}

		if (((d = audiomain->io_o[3]) != NULL) && (audiomain->iocount > 3))
		{
			s = abufs.inputs[vindex][ARP_I_OUT_4];
			//bristolbzero(d, audiomain->segmentsize);

			/* Fourth IO will be DC */
			for (i = 0; i < sc; i+=8)
			{
				if ((*d += *s * 0.04166667f + 0.5f) < 0) *d = 0.0f;s++;d++;
				if ((*d += *s * 0.04166667f + 0.5f) < 0) *d = 0.0f;s++;d++;
				if ((*d += *s * 0.04166667f + 0.5f) < 0) *d = 0.0f;s++;d++;
				if ((*d += *s * 0.04166667f + 0.5f) < 0) *d = 0.0f;s++;d++;
				if ((*d += *s * 0.04166667f + 0.5f) < 0) *d = 0.0f;s++;d++;
				if ((*d += *s * 0.04166667f + 0.5f) < 0) *d = 0.0f;s++;d++;
				if ((*d += *s * 0.04166667f + 0.5f) < 0) *d = 0.0f;s++;d++;
				if ((*d += *s * 0.04166667f + 0.5f) < 0) *d = 0.0f;s++;d++;
			}
		}
	}

	return(0);
}

int
operateArp2600Postops(audioMain *audiomain, Baudio *baudio,
bristolVoice *voice, register float *startbuf)
{
	/*
	if (baudio->mixlocals == NULL)
		baudio->mixlocals = (float *) voice->locals[voice->index];
	*/

	/*
	 * Drive data through the effects:
	 * We have the reverb, mono in, stereo out. Chorus is later.
	 */
	audiomain->palette[(*baudio->sound[13]).index]->specs->io[0].buf =
		baudio->leftbuf;
	audiomain->palette[(*baudio->sound[13]).index]->specs->io[1].buf =
		baudio->leftbuf;
	audiomain->palette[(*baudio->sound[13]).index]->specs->io[2].buf =
		baudio->rightbuf;
	(*baudio->sound[13]).operate(
		(audiomain->palette)[12],
		baudio->firstVoice,
		(*baudio->sound[13]).param,
		voice->locals[0][13]);
	/*
	 * At this point we could copy this data back to a pair of buffers (we
	 * have the FX_L/R available) and allow the chorus to be remixed back into
	 * the synth.
	 */
/* Effects done */

/* Output panning. */
	/*
	 * I seem to have two components left to mix in, these are the pan settings
	 * and L/R in
	 */
	bufmerge(abufs.outputs[0][ARP_O_PAN],
		(1.0 - ((arp2600mods *) baudio->mixlocals)->pan) *
			((arp2600mods *) baudio->mixlocals)->volume * 2,
		baudio->leftbuf,
		(1.0 - ((arp2600mods *) baudio->mixlocals)->pan) *
			((arp2600mods *) baudio->mixlocals)->volume * 2,
		audiomain->samplecount);
	bufmerge(abufs.outputs[0][ARP_O_PAN],
		((arp2600mods *) baudio->mixlocals)->pan *
			((arp2600mods *) baudio->mixlocals)->volume * 2,
		baudio->rightbuf,
		((arp2600mods *) baudio->mixlocals)->pan *
			((arp2600mods *) baudio->mixlocals)->volume * 2,
		audiomain->samplecount);
	bufmerge(abufs.outputs[1][ARP_O_PAN], 1.0,
		baudio->leftbuf, 1.0,
		audiomain->samplecount);
	bufmerge(abufs.outputs[2][ARP_O_PAN], 1.0,
		baudio->rightbuf, 1.0,
		audiomain->samplecount);
	bristolbzero(abufs.inputs[0][ARP_I_LEFT_IN], audiomain->segmentsize);
	bristolbzero(abufs.inputs[0][ARP_I_RIGHT_IN], audiomain->segmentsize);
	bristolbzero(abufs.outputs[0][ARP_O_PAN], audiomain->segmentsize);
	bristolbzero(abufs.outputs[1][ARP_O_PAN], audiomain->segmentsize);
	bristolbzero(abufs.outputs[2][ARP_O_PAN], audiomain->segmentsize);
/* */

	return(0);
}

static int bristolArp2600Destroy(audioMain *audiomain, Baudio *baudio)
{
	printf("removing one arp2600\n");

	/*
	 * We need to leave these, we may have multiple invocations running
	 */
	return(0);

	bristolfree(fmbuf);
	bristolfree(filtbuf);
/*	bristolfree(noisebuf); */
	bristolfree(adsrbuf);
	bristolfree(arbuf);
	bristolfree(mixbuf);
	bristolfree(shmix);
	bristolfree(zerobuf);
	bristolfree(lfosine);
	bristolfree(lfosquare);
	bristolfree(lfosh);
	bristolfree(osc1ramp);
	bristolfree(osc1puls);
	bristolfree(osc2ramp);
	bristolfree(osc2puls);
	bristolfree(ringmod);

	bristolfree(abufs.buf);

	return(0);
}

int
bristolArp2600Init(audioMain *audiomain, Baudio *baudio)
{
	printf("initialising arp2600\n");

	baudio->soundCount = 14; /* Number of operators in this voice */
	/*
	 * Assign an array of sound pointers.
	 */
	baudio->sound = (bristolSound **)
		bristolmalloc0(sizeof(bristolOP *) * baudio->soundCount);
	baudio->effect = (bristolSound **)
		bristolmalloc0(sizeof(bristolOP *) * baudio->soundCount);

	/* oscillator 1 */
	initSoundAlgo(	19,	0, baudio, audiomain, baudio->sound);
	/* oscillator 2 */
	initSoundAlgo(	19,	1, baudio, audiomain, baudio->sound);
	/* oscillator 3 */
	initSoundAlgo(	19,	2, baudio, audiomain, baudio->sound);
	/* A filter */
	initSoundAlgo(	B_FILTER2,	3, baudio, audiomain, baudio->sound);
	/* Single ADSR */
	initSoundAlgo(	1,	4, baudio, audiomain, baudio->sound);
	/* An amplifier */
	initSoundAlgo(	2,	5, baudio, audiomain, baudio->sound);
	/* An noise source */
	initSoundAlgo(	4,	6, baudio, audiomain, baudio->sound);
	/* Second ADSR to be AR envelope */
	initSoundAlgo(	1,	7, baudio, audiomain, baudio->sound);
	/* Ring Mod */
	initSoundAlgo(	20,	8, baudio, audiomain, baudio->sound);
	/* LFO */
	initSoundAlgo(	16, 9, baudio, audiomain, baudio->sound);
	/* Electroswitch */
	initSoundAlgo(	21, 10, baudio, audiomain, baudio->sound);
	/* Envelope follower */
	initSoundAlgo(	23, 11, baudio, audiomain, baudio->sound);
	/* Filter, used with a simplified rooney to be a lag filter */
	initSoundAlgo(	3,	12, baudio, audiomain, baudio->sound);
	/* Moved Vibra chorus into algorithm. */
	initSoundAlgo(	12,	13, baudio, audiomain, baudio->sound);

	/*
	 * Reverb - stereo in/out - This really needs to be in the audio engine
	 * since it should roll on passed note-off events to sound correct.
	 */
/*	initSoundAlgo(	12,	0, baudio, audiomain, baudio->effect); */
	initSoundAlgo(	22,	0, baudio, audiomain, baudio->effect);

	/*
	 * We will need to add
	 *
	 *	input gain
	 *	envelope follower
	 *
	 * The engine only supports a single chained effect, so we will probably
	 * use chorus for that and then implement a delay to feed it? Perhaps not,
	 * both effects need to be fully stereo which means we need some further
	 * development effort.
	 */

	baudio->param = arp2600Controller;
	baudio->destroy = bristolArp2600Destroy;
	baudio->operate = operateOneArp2600;
	baudio->preops = arp2600Preops;
	baudio->postops = operateArp2600Postops;

	/*
	if (filtbuf == NULL)
		filtbuf = (float *) bristolmalloc0(audiomain->segmentsize);
	if (adsrbuf == NULL)
		adsrbuf = (float *) bristolmalloc0(audiomain->segmentsize);
	if (arbuf == NULL)
		arbuf = (float *) bristolmalloc0(audiomain->segmentsize);
	if (shmix == NULL)
		shmix = (float *) bristolmalloc0(audiomain->segmentsize);
	if (zerobuf == NULL)
		zerobuf = (float *) bristolmalloc0(audiomain->segmentsize);
	if (lfosh == NULL)
		lfosh = (float *) bristolmalloc0(audiomain->segmentsize);
	if (lfosquare == NULL)
		lfosquare = (float *) bristolmalloc0(audiomain->segmentsize);
	if (lfosine == NULL)
		lfosine = (float *) bristolmalloc0(audiomain->segmentsize);
	if (osc1ramp == NULL)
		osc1ramp = (float *) bristolmalloc0(audiomain->segmentsize);
	if (osc1puls == NULL)
		osc1puls = (float *) bristolmalloc0(audiomain->segmentsize);
	if (osc2ramp == NULL)
		osc2ramp = (float *) bristolmalloc0(audiomain->segmentsize);
	if (osc2puls == NULL)
		osc2puls = (float *) bristolmalloc0(audiomain->segmentsize);
	if (ringmod == NULL)
		ringmod = (float *) bristolmalloc0(audiomain->segmentsize);
	*/
	if (zerobuf == NULL)
		zerobuf = (float *) bristolmalloc0(audiomain->segmentsize);
	/*
	 * Noisebuf will be a single fixed offset into our buffer table? It should
	 * only exist once anyway, not once per voice.
	 */
	if (noisebuf == NULL)
		noisebuf = &abufs.buf[ARP_O_NOISE * audiomain->samplecount];
	if (scratch == NULL)
		scratch = (float *) bristolmalloc0(audiomain->segmentsize);
	if (mixbuf == NULL)
		mixbuf = (float *) bristolmalloc0(audiomain->segmentsize);
	if (fmbuf == NULL)
		fmbuf = (float *) bristolmalloc0(audiomain->segmentsize);

	baudio->mixlocals = (float *) bristolmalloc0(sizeof(arp2600mods));

	if (abufs.buf == NULL)
	{
		int i, v, sc = audiomain->samplecount;

		abufs.voiceCount = audiomain->voiceCount;
		abufs.samplecount = audiomain->samplecount;

		/*
		 * Hm, may be more than we need, but need to reserve it all now. It is
		 * one buffer per output per voice (max numbers of voices).
		 */
		abufs.buf = (float *) bristolmalloc0(
			audiomain->samplecount
			* sizeof(float)
			* audiomain->voiceCount
			* ARP_OUTCNT);
/*printf("malloc %i to ARP buffers at %x\n", */
/*audiomain->segmentsize * audiomain->voiceCount * ARP_OUTCNT, abufs.buf); */

		for (v = 0; v < audiomain->voiceCount; v++)
		{
			for (i = 0; i < ARP_OUTCNT; i++)
			{
				abufs.outputs[v][i] =
					&abufs.buf[audiomain->samplecount * (v * ARP_OUTCNT + i)];
			}
		}

		/*
		 * The inputs have to be manually configured as they are the original
		 * synth default routings.
		 */
		abufs.defaults[0] = sc * ARP_O_PREAMP;
		abufs.defaults[1] = sc * ARP_O_VCO1_SAW;
		abufs.defaults[2] = sc * ARP_O_VCO2_SIN;
		abufs.defaults[3] = sc * ARP_O_KBD_CV;
		abufs.defaults[4] = sc * ARP_O_SH;
		abufs.defaults[5] = sc * ARP_O_ADSR;
		abufs.defaults[6] = sc * ARP_O_VCO2_SIN;
		abufs.defaults[7] = sc * ARP_O_KBD_CV;
		abufs.defaults[8] = sc * ARP_O_SH;
		abufs.defaults[ARP_I_VCO2_ADSR] = sc * ARP_O_ADSR;
		abufs.defaults[ARP_I_VCO2_VCO1] = sc * ARP_O_VCO1_SQR;
		abufs.defaults[ARP_I_VCO2_NSE] = sc * ARP_O_NOISE;
		abufs.defaults[12] = sc * ARP_O_KBD_CV;
		abufs.defaults[13] = sc * ARP_O_NOISE;
		abufs.defaults[14] = sc * ARP_O_ADSR;
		abufs.defaults[15] = sc * ARP_O_VCO2_SIN;
		abufs.defaults[16] = sc * ARP_O_VCO2_TRI;
		abufs.defaults[ARP_I_FILT_RM] = sc * ARP_O_RINGMOD;
		abufs.defaults[ARP_I_FILT_1SQR] = sc * ARP_O_VCO1_SQR;
		abufs.defaults[ARP_I_FILT_2SQR] = sc * ARP_O_VCO2_SQR;
		abufs.defaults[ARP_I_FILT_3TRI] = sc * ARP_O_VCO3_SAW;
		abufs.defaults[ARP_I_FILT_NSE] = sc * ARP_O_NOISE;
		abufs.defaults[ARP_I_FILT_KBD] = sc * ARP_O_KBD_CV;
		abufs.defaults[23] = sc * ARP_O_ADSR;
		abufs.defaults[24] = sc * ARP_O_VCO2_SIN;
		abufs.defaults[25] = sc * ARP_O_VCF; /* Wrong - gate. ZERO */
		abufs.defaults[26] = sc * ARP_O_CLOCK;
		abufs.defaults[27] = sc * ARP_ZERO;
		abufs.defaults[28] = sc * ARP_ZERO;
		abufs.defaults[29] = sc * ARP_O_VCF;
		abufs.defaults[30] = sc * ARP_O_RINGMOD;
		abufs.defaults[31] = sc * ARP_O_AR;
		abufs.defaults[32] = sc * ARP_O_ADSR;
		abufs.defaults[33] = sc * ARP_O_VCF;
		abufs.defaults[34] = sc * ARP_O_VCA;
		abufs.defaults[ARP_I_MIX_FX] = sc * ARP_O_MIXER;
		abufs.defaults[ARP_I_LEFT_IN] = sc * ARP_ZERO;
		abufs.defaults[ARP_I_RIGHT_IN] = sc * ARP_ZERO;
		abufs.defaults[38] = sc * ARP_ZERO;
		abufs.defaults[39] = sc * ARP_ZERO;
		abufs.defaults[40] = sc * ARP_O_NOISE;
		abufs.defaults[41] = sc * ARP_O_CLOCK;
		abufs.defaults[42] = sc * ARP_O_CLOCK;
		abufs.defaults[43] = sc * ARP_O_MIXER;
		abufs.defaults[44] = sc * ARP_ZERO;
		abufs.defaults[45] = sc * ARP_ZERO;
		abufs.defaults[46] = sc * ARP_ZERO;
		abufs.defaults[47] = sc * ARP_ZERO;
		abufs.defaults[48] = sc * ARP_ZERO;
		abufs.defaults[49] = sc * ARP_ZERO;
		abufs.defaults[50] = sc * ARP_ZERO;
		abufs.defaults[51] = sc * ARP_ZERO;
		abufs.defaults[52] = sc * ARP_ZERO;
		abufs.defaults[53] = sc * ARP_ZERO;
		abufs.defaults[54] = sc * ARP_ZERO;

		for (i = 0; i < audiomain->voiceCount; i++) {
			abufs.inputs[i][0] = &abufs.buf[
				abufs.defaults[0] + audiomain->samplecount * i * ARP_OUTCNT];
			abufs.inputs[i][1] = &abufs.buf[
				abufs.defaults[1] + audiomain->samplecount * i * ARP_OUTCNT];
			abufs.inputs[i][2] = &abufs.buf[
				abufs.defaults[2] + audiomain->samplecount * i * ARP_OUTCNT];
			abufs.inputs[i][3] = &abufs.buf[
				abufs.defaults[3] + audiomain->samplecount * i * ARP_OUTCNT];
			abufs.inputs[i][4] = &abufs.buf[
				abufs.defaults[4] + audiomain->samplecount * i * ARP_OUTCNT];
			abufs.inputs[i][5] = &abufs.buf[
				abufs.defaults[5] + audiomain->samplecount * i * ARP_OUTCNT];
			abufs.inputs[i][6] = &abufs.buf[
				abufs.defaults[6] + audiomain->samplecount * i * ARP_OUTCNT];
			abufs.inputs[i][7] = &abufs.buf[
				abufs.defaults[7] + audiomain->samplecount * i * ARP_OUTCNT];
			abufs.inputs[i][8] = &abufs.buf[
				abufs.defaults[8] + audiomain->samplecount * i * ARP_OUTCNT];
			abufs.inputs[i][9] = &abufs.buf[
				abufs.defaults[9] + audiomain->samplecount * i * ARP_OUTCNT];
			abufs.inputs[i][10] = &abufs.buf[
				abufs.defaults[10] + audiomain->samplecount * i * ARP_OUTCNT];
			abufs.inputs[i][11] = &abufs.buf[
				abufs.defaults[11] + audiomain->samplecount * i * ARP_OUTCNT];
			abufs.inputs[i][12] = &abufs.buf[
				abufs.defaults[12] + audiomain->samplecount * i * ARP_OUTCNT];
			abufs.inputs[i][13] = &abufs.buf[
				abufs.defaults[13] + audiomain->samplecount * i * ARP_OUTCNT];
			abufs.inputs[i][14] = &abufs.buf[
				abufs.defaults[14] + audiomain->samplecount * i * ARP_OUTCNT];
			abufs.inputs[i][15] = &abufs.buf[
				abufs.defaults[15] + audiomain->samplecount * i * ARP_OUTCNT];
			abufs.inputs[i][16] = &abufs.buf[
				abufs.defaults[16] + audiomain->samplecount * i * ARP_OUTCNT];
			abufs.inputs[i][17] = &abufs.buf[
				abufs.defaults[17] + audiomain->samplecount * i * ARP_OUTCNT];
			abufs.inputs[i][18] = &abufs.buf[
				abufs.defaults[18] + audiomain->samplecount * i * ARP_OUTCNT];
			abufs.inputs[i][19] = &abufs.buf[
				abufs.defaults[19] + audiomain->samplecount * i * ARP_OUTCNT];
			abufs.inputs[i][20] = &abufs.buf[
				abufs.defaults[20] + audiomain->samplecount * i * ARP_OUTCNT];
			abufs.inputs[i][21] = &abufs.buf[
				abufs.defaults[21] + audiomain->samplecount * i * ARP_OUTCNT];
			abufs.inputs[i][22] = &abufs.buf[
				abufs.defaults[22] + audiomain->samplecount * i * ARP_OUTCNT];
			abufs.inputs[i][23] = &abufs.buf[
				abufs.defaults[23] + audiomain->samplecount * i * ARP_OUTCNT];
			abufs.inputs[i][24] = &abufs.buf[
				abufs.defaults[24] + audiomain->samplecount * i * ARP_OUTCNT];
			abufs.inputs[i][25] = &abufs.buf[
				abufs.defaults[25] + audiomain->samplecount * i * ARP_OUTCNT];
			abufs.inputs[i][26] = &abufs.buf[
				abufs.defaults[26] + audiomain->samplecount * i * ARP_OUTCNT];
			abufs.inputs[i][27] = &abufs.buf[
				abufs.defaults[27] + audiomain->samplecount * i * ARP_OUTCNT];
			abufs.inputs[i][28] = &abufs.buf[
				abufs.defaults[28] + audiomain->samplecount * i * ARP_OUTCNT];
			abufs.inputs[i][29] = &abufs.buf[
				abufs.defaults[29] + audiomain->samplecount * i * ARP_OUTCNT];
			abufs.inputs[i][30] = &abufs.buf[
				abufs.defaults[30] + audiomain->samplecount * i * ARP_OUTCNT];
			abufs.inputs[i][31] = &abufs.buf[
				abufs.defaults[31] + audiomain->samplecount * i * ARP_OUTCNT];
			abufs.inputs[i][32] = &abufs.buf[
				abufs.defaults[32] + audiomain->samplecount * i * ARP_OUTCNT];
			abufs.inputs[i][33] = &abufs.buf[
				abufs.defaults[33] + audiomain->samplecount * i * ARP_OUTCNT];
			abufs.inputs[i][34] = &abufs.buf[
				abufs.defaults[34] + audiomain->samplecount * i * ARP_OUTCNT];
			abufs.inputs[i][35] = &abufs.buf[
				abufs.defaults[35] + audiomain->samplecount * i * ARP_OUTCNT];
			abufs.inputs[i][36] = &abufs.buf[
				abufs.defaults[36] + audiomain->samplecount * i * ARP_OUTCNT];
			abufs.inputs[i][37] = &abufs.buf[
				abufs.defaults[37] + audiomain->samplecount * i * ARP_OUTCNT];
			abufs.inputs[i][38] = &abufs.buf[
				abufs.defaults[38] + audiomain->samplecount * i * ARP_OUTCNT];
			abufs.inputs[i][39] = &abufs.buf[
				abufs.defaults[39] + audiomain->samplecount * i * ARP_OUTCNT];
			abufs.inputs[i][39] = &abufs.buf[
				abufs.defaults[39] + audiomain->samplecount * i * ARP_OUTCNT];
			abufs.inputs[i][40] = &abufs.buf[
				abufs.defaults[40] + audiomain->samplecount * i * ARP_OUTCNT];
			abufs.inputs[i][41] = &abufs.buf[
				abufs.defaults[41] + audiomain->samplecount * i * ARP_OUTCNT];
			abufs.inputs[i][42] = &abufs.buf[
				abufs.defaults[42] + audiomain->samplecount * i * ARP_OUTCNT];
			abufs.inputs[i][43] = &abufs.buf[
				abufs.defaults[43] + audiomain->samplecount * i * ARP_OUTCNT];
			abufs.inputs[i][44] = &abufs.buf[
				abufs.defaults[44] + audiomain->samplecount * i * ARP_OUTCNT];
			abufs.inputs[i][45] = &abufs.buf[
				abufs.defaults[45] + audiomain->samplecount * i * ARP_OUTCNT];
			abufs.inputs[i][46] = &abufs.buf[
				abufs.defaults[46] + audiomain->samplecount * i * ARP_OUTCNT];
			abufs.inputs[i][47] = &abufs.buf[
				abufs.defaults[47] + audiomain->samplecount * i * ARP_OUTCNT];
			abufs.inputs[i][48] = &abufs.buf[
				abufs.defaults[48] + audiomain->samplecount * i * ARP_OUTCNT];
			abufs.inputs[i][40] = &abufs.buf[
				abufs.defaults[49] + audiomain->samplecount * i * ARP_OUTCNT];
			abufs.inputs[i][50] = &abufs.buf[
				abufs.defaults[50] + audiomain->samplecount * i * ARP_OUTCNT];
			/* These are the CV I/O */
			abufs.inputs[i][51] = &abufs.buf[
				abufs.defaults[51] + audiomain->samplecount * 0 * ARP_OUTCNT];
			abufs.inputs[i][52] = &abufs.buf[
				abufs.defaults[52] + audiomain->samplecount * 0 * ARP_OUTCNT];
			abufs.inputs[i][53] = &abufs.buf[
				abufs.defaults[53] + audiomain->samplecount * 0 * ARP_OUTCNT];
			abufs.inputs[i][54] = &abufs.buf[
				abufs.defaults[54] + audiomain->samplecount * 0 * ARP_OUTCNT];
		}

		for (v = 0; v < ARP_OUTCNT; v++)
			((arp2600mods *) baudio->mixlocals)->mod[v].o1gain = 1.0;
	}

	((arp2600mods *) baudio->mixlocals)->mod[ARP_O_VCF].o1gain = 0.125;

	baudio->mixflags |= BRISTOL_STEREO;

	return(0);
}

