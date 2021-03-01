
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

/* EMS Synthi AKS algorithm. */
/*
 * Revermix to reverb (Needs another reverb).
 * Need another filter
 * Ramp signal is noisy
 * Input streams broken
 *
 * VU Meter does not operate, but that is outside the scope of this emulator.
 *
 * Frequency range too small and biased to low end.
 * Frequency rangee of Osc-3 should be lower.
 */

/*#define DEBUG */

#include "bristol.h"
#include "bristolmm.h"
#include "bristolaks.h"

/*
 * Use of these aks global buffers will be an issue with use of multiple
 * audio threads, unless we ensure a single thread deals with any given algo
 * type, since then they are only used sequentially.
 */
static float *freqbuf = (float *) NULL;
static float *zerobuf = (float *) NULL;
static float *tmpbuf = (float *) NULL;
static float *patchbuf1 = (float *) NULL;
static float *patchbuf2 = (float *) NULL;

/*
 * All buffers are going to be collapsed into a single 'megabuf', this will be
 * used for all local information for every possible one of these voices. As
 * such it might get big, but will allow for arbitrary buffering.
 */
static struct {
	float *buf;
	float *outputs[128][AKS_OUTCOUNT]; /* offsets into buffer for data */
/*	float *inputs[128][AKS_INCOUNT]; pointer offsets for input source */
/*	int defaults[AKS_INCOUNT]; */
	float matrix[AKS_INCOUNT][AKS_OUTCOUNT]; /* This are gains by bus. */
	int voiceCount, samplecount;
} abufs;

int
aksController(Baudio *baudio, u_char operator, u_char controller, float value)
{
#ifdef DEBUG
	printf("bristolAksControl(%i, %i, %f)\n", operator, controller, value);
#endif

	/*
	 * Operator 100/101 will be used for patching. We need a few general
	 * a few commands, clear all patches, clear one patch and add one patch.
	 *
	 * 100 will be to add a patch.
	 * 101 will be to clear all patches to default.
	 */
	if (operator == 99)
	{
		int in, out;

		for (in = 0; in < AKS_INCOUNT; in++)
			for (out = 0; out < AKS_OUTCOUNT; out++)
				abufs.matrix[in][out] = 0.0;

		return(0);
	}

	/* Hm, fixed as 16 which works for the AKS */
	if ((operator >= 100) && (operator <= 115))
	{
		int in = operator - 100, out = controller;

/*		if (value != 0.0) */
/*			printf("engine %i %i %f\n", in, out, value); */

		if ((out < 0) || (out > 15))
			return(0);

		/* controller needs to map two values, in/out */
		if (value == 1.0)
			abufs.matrix[in][out] = -1.0f;
		else if (value == 0.0f)
			abufs.matrix[in][out] = 0.0;
		else
			abufs.matrix[in][out] = value;

		return(0);
	}

	if (operator != 126)
		return(0);

	switch (controller) {
		case 0:
			/*
			 * Synth does not have glide.....
			 */
			baudio->glide = value * value * baudio->glidemax;
			break;
		case 1:
			/*
			 * We will need to set the general tuning, must be requested by 
			 * the GUI.
			 */
			baudio->gtune = 1.0
				+ (baudio->note_diff - 1)
				* (value * 2 - 1);

			buildCurrentTable(baudio, baudio->gtune);
			alterAllNotes(baudio);
			break;
		case 2:
			((aksmods *) baudio->mixlocals)->lingain = value * 8;
			break;
		case 3:
			((aksmods *) baudio->mixlocals)->ringain = value * 8;
			break;
		case 4:
			((aksmods *) baudio->mixlocals)->loutfilt = value;
			break;
		case 5:
			((aksmods *) baudio->mixlocals)->routfilt = value;
			break;
		case 6:
			((aksmods *) baudio->mixlocals)->hrange = value * 8;
			break;
		case 7:
			((aksmods *) baudio->mixlocals)->vrange = value * 8;
			break;
		case 8:
			((aksmods *) baudio->mixlocals)->chan1gain = value;
			break;
		case 9:
			((aksmods *) baudio->mixlocals)->chan1pan = value;
			break;
		case 10:
			((aksmods *) baudio->mixlocals)->chan2pan = value;
			break;
		case 11:
			((aksmods *) baudio->mixlocals)->chan2gain = value;
			break;
		case 12: /* Joystick horizontal From GUI */
		case 13: /* Joystick vertical From GUI */
			baudio->contcontroller[controller] = value;
			break;
		case 14: /* Mon source left switch (0 leftpan/mute/monitor 2) */
			((aksmods *) baudio->mixlocals)->chan1route
				= value * CONTROLLER_RANGE;
			break;
		case 15: /* Mon source right switch (0 rightpan/mute/triggermute 2) */
			((aksmods *) baudio->mixlocals)->chan2route
				= value * CONTROLLER_RANGE;
			break;
		case 16:
			((aksmods *) baudio->mixlocals)->trapezoid = value * 4;
			break;
		case 17:
			((aksmods *) baudio->mixlocals)->signal = value;
			break;
	}
	return(0);
}

static float
tof(float *buf, int size, float lim, float start)
{
	int i;
	float *buf2 = buf;

	for (i = 0; i < size; i++)
		*buf++ = (start += ((*buf2++ - start) * lim));

	return(start);
}

/*
 * Scan the matrix for this input by all outputs.
 */
static int
fillmodbuf(float *buf, int input, int voice, int count)
{
	register int i, j = 0;

	bristolbzero(buf, count * sizeof(float));

	for (i = 0; i < AKS_OUTCOUNT; i++)
	{
		if (abufs.matrix[input][i] != 0.0f)
		{
			bufmerge(abufs.outputs[voice][i], abufs.matrix[input][i],
				buf, 1.0, count);
			j++;
		}
	}
	return(j);
}

int
operateOneAks(audioMain *audiomain, Baudio *baudio,
bristolVoice *voice, register float *startbuf)
{
	aksmods *mods = (aksmods *) baudio->mixlocals;
	int sc = audiomain->samplecount;
	int ss = audiomain->segmentsize, i;
	float *buf = abufs.outputs[voice->index][AKS_O_H];
	float value = mods->hrange * baudio->contcontroller[12];

/* Input */
/* Input done */

/* Stick */
	/*
	 * These will take a couple of continuous controller ID and scale their
	 * value into the respective buffers. Seeing as there are no generic joy
	 * stick controls in MIDI then we are going to use the effect control
	 * parameters 12 and 13.
	 */
	for (i = 0; i < sc; i+=8)
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

	buf = abufs.outputs[voice->index][AKS_O_V];
	value = mods->vrange * baudio->contcontroller[13];

	for (i = 0; i < sc; i+=8)
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
/* Stick done */

/*printf("operateOneAks(%i, %x, %x)\n", voice->index, audiomain, baudio); */

	fillFreqTable(baudio, voice, freqbuf, sc, 1);

/* Osc 1 */
	/* Put in the patch settings */
	fillmodbuf(patchbuf1, AKS_I_OSC1, voice->index, sc);

	bufmerge(freqbuf, 1.0, patchbuf1, 1.0, sc);

	/*
	 * Mod buf [1] is not used by the AKS oscillator at the moment.
	 */
	audiomain->palette[(*baudio->sound[0]).index]->specs->io[0].buf
		= patchbuf1;
	audiomain->palette[(*baudio->sound[0]).index]->specs->io[1].buf
		= zerobuf;
	audiomain->palette[(*baudio->sound[0]).index]->specs->io[2].buf
		= abufs.outputs[voice->index][AKS_O_OSC1];
	audiomain->palette[(*baudio->sound[0]).index]->specs->io[3].buf
		= tmpbuf;

	(*baudio->sound[0]).operate(
		(audiomain->palette)[24],
		voice,
		(*baudio->sound[0]).param,
		voice->locals[voice->index][0]);
	/* Fold both buffers down to a single one */
	bufmerge(tmpbuf, 1.0, abufs.outputs[voice->index][AKS_O_OSC1], 1.0, sc);
/* Osc 1 done */

/* Osc 2 */
	/* Put in the patch settings */
	fillmodbuf(patchbuf1, AKS_I_OSC2, voice->index, sc);

	bufmerge(freqbuf, 1.0, patchbuf1, 1.0, sc);

	audiomain->palette[(*baudio->sound[1]).index]->specs->io[0].buf
		= patchbuf1;
	audiomain->palette[(*baudio->sound[1]).index]->specs->io[1].buf
		= zerobuf;
	audiomain->palette[(*baudio->sound[1]).index]->specs->io[2].buf
		= abufs.outputs[voice->index][AKS_O_OSC2];
	audiomain->palette[(*baudio->sound[1]).index]->specs->io[3].buf
		= tmpbuf;

	(*baudio->sound[1]).operate(
		(audiomain->palette)[24],
		voice,
		(*baudio->sound[1]).param,
		voice->locals[voice->index][1]);
	/* Fold both buffers down to a single one */
	bufmerge(tmpbuf, 1.0, abufs.outputs[voice->index][AKS_O_OSC2], 1.0, sc);
/* Osc 2 done */

/* Osc 3 */
	/* Put in the patch settings */
	fillmodbuf(patchbuf1, AKS_I_OSC3, voice->index, sc);

	bufmerge(freqbuf, 1.0, patchbuf1, 1.0, sc);

	audiomain->palette[(*baudio->sound[2]).index]->specs->io[0].buf
		= patchbuf1;
	audiomain->palette[(*baudio->sound[2]).index]->specs->io[1].buf
		= zerobuf;
	audiomain->palette[(*baudio->sound[2]).index]->specs->io[2].buf
		= abufs.outputs[voice->index][AKS_O_OSC3_SQR];
	audiomain->palette[(*baudio->sound[2]).index]->specs->io[3].buf
			= abufs.outputs[voice->index][AKS_O_OSC3_TRI];

	(*baudio->sound[2]).operate(
		(audiomain->palette)[24],
		voice,
		(*baudio->sound[2]).param,
		voice->locals[voice->index][2]);
/* Osc 3 done */

/* Env */
	/*
	 * Env needs to have its decay buffer filled, then it is run to get the
	 * env. After that it needs to be applied to its input buffer through to
	 * its output buffer and finally scaled to its own trapezoid output.
	 */
	fillmodbuf(patchbuf1, AKS_I_DECAY, voice->index, sc);

	audiomain->palette[(*baudio->sound[4]).index]->specs->io[0].buf =
		abufs.outputs[voice->index][AKS_O_ADSR];
	audiomain->palette[(*baudio->sound[4]).index]->specs->io[1].buf =
		patchbuf1;
	audiomain->palette[(*baudio->sound[4]).index]->specs->io[2].buf =
		abufs.outputs[voice->index][AKS_O_TRIGGER];
	(*baudio->sound[4]).operate(
		(audiomain->palette)[25],
		voice,
		(*baudio->sound[4]).param,
		voice->locals[voice->index][4]);

	/*
	 * We now have an env, put this through the amp.
	 */
	fillmodbuf(patchbuf1, AKS_I_ENVELOPE, voice->index, sc);
	audiomain->palette[(*baudio->sound[5]).index]->specs->io[0].buf
		= patchbuf1;
	audiomain->palette[(*baudio->sound[5]).index]->specs->io[1].buf
		= abufs.outputs[voice->index][AKS_O_ADSR];
	audiomain->palette[(*baudio->sound[5]).index]->specs->io[2].buf
		= abufs.outputs[voice->index][AKS_O_VCA];

	bristolbzero(abufs.outputs[voice->index][AKS_O_VCA], ss);

	(*baudio->sound[5]).operate(
		(audiomain->palette)[2],
		voice,
		(*baudio->sound[5]).param,
		voice->locals[voice->index][5]);

	bufmerge(abufs.outputs[voice->index][AKS_O_VCA], 0.0,
		abufs.outputs[voice->index][AKS_O_VCA], mods->signal, sc);
	bufmerge(abufs.outputs[voice->index][AKS_O_ADSR], mods->trapezoid,
		abufs.outputs[voice->index][AKS_O_ADSR], 0.0, sc);
/* Env done */

/* Noise */
	/*
	 * This had to be moved to a multi operator to support the correct routing.
	 */
	bristolbzero(abufs.outputs[voice->index][AKS_O_NOISE], ss);
	audiomain->palette[(*baudio->sound[6]).index]->specs->io[0].buf =
		abufs.outputs[voice->index][AKS_O_NOISE];
	(*baudio->sound[6]).operate(
		(audiomain->palette)[4],
		voice,
		(*baudio->sound[6]).param,
		voice->locals[voice->index][6]);
	bufmerge(abufs.outputs[voice->index][AKS_O_NOISE], 0.0,
		abufs.outputs[voice->index][AKS_O_NOISE], 0.05, sc);
/* Noise done */

/* Ring Mod */
	fillmodbuf(patchbuf1, AKS_I_RM1, voice->index, sc);
	fillmodbuf(patchbuf2, AKS_I_RM2, voice->index, sc);
	audiomain->palette[(*baudio->sound[7]).index]->specs->io[0].buf
		= patchbuf1;
	audiomain->palette[(*baudio->sound[7]).index]->specs->io[1].buf
		= patchbuf2;
	audiomain->palette[(*baudio->sound[7]).index]->specs->io[2].buf
		= abufs.outputs[voice->index][AKS_O_RM];
	(*baudio->sound[7]).operate(
		(audiomain->palette)[20],
		voice,
		(*baudio->sound[7]).param,
		voice->locals[voice->index][7]);
/* Ring Mod done */

/* Filter */
	fillmodbuf(patchbuf1, AKS_I_FILTER, voice->index, sc);
	/* Filter frequency */
	fillmodbuf(patchbuf2, AKS_I_VCF, voice->index, sc);

	audiomain->palette[(*baudio->sound[3]).index]->specs->io[0].buf =
		patchbuf1;
	audiomain->palette[(*baudio->sound[3]).index]->specs->io[1].buf =
		patchbuf2;
	audiomain->palette[(*baudio->sound[3]).index]->specs->io[2].buf
		= abufs.outputs[voice->index][AKS_O_VCF];

	bristolbzero(abufs.outputs[voice->index][AKS_O_VCF], ss);

	(*baudio->sound[3]).operate(
		(audiomain->palette)[27],
		voice,
		(*baudio->sound[3]).param,
		voice->locals[voice->index][3]);
/* Filter done */

/* Reverb */
	fillmodbuf(patchbuf1, AKS_I_REVERB, voice->index, sc);
	fillmodbuf(patchbuf2, AKS_I_REVERBMIX, voice->index, sc);
	/*
	 * We have the reverb, mono in, stereo out, but cannot use the second
	 * channel.
	 *
	 * Reverb needs some rewrite to cater for the reverb mix buffer.....
	 * Perhaps another one, the original used a 26ms and 30ms spring delay 
	 * line with feedback, we can take these values and extend them a little,
	 * work in the feedback based on the reverb mix (that can also control the
	 * length). FFS.
	 */
	audiomain->palette[(*baudio->sound[8]).index]->specs->io[0].buf =
		patchbuf1;
	audiomain->palette[(*baudio->sound[8]).index]->specs->io[1].buf =
		patchbuf2;
	audiomain->palette[(*baudio->sound[8]).index]->specs->io[2].buf =
		abufs.outputs[voice->index][AKS_O_REVERB];

	bristolbzero(abufs.outputs[voice->index][AKS_O_REVERB], ss);

	(*baudio->sound[8]).operate(
		(audiomain->palette)[26],
		baudio->firstVoice,
		(*baudio->sound[8]).param,
		voice->locals[0][8]);
/* Reverb done */

/* Output stage */
	/*
	 * Fill the monitor buffer
	 */
	fillmodbuf(abufs.outputs[voice->index][AKS_O_METER],
		AKS_I_METER, voice->index, sc);
	/*
	 * There are pins to go to the output mix, which are fed back into the
	 * mix. Then they are filtered (HM, panned and actualy go to the outputs.
	 *
	 * The signal from the patch panel is amplified with the mod buffer then
	 * filtered, then sent back to the panel and to speakers.
	 * It is then panned to the outputs.
	 *
	 * Since we do not have speakers then the amplified, filtered signal
	 * will go back to the panel, and the panned values will go to the audio
	 * buffers. The data that goes to the baudio buffers will depend on the
	 * channel switches next to the speakers (they will allow override of the
	 * data).
	 */

	fillmodbuf(patchbuf1, AKS_I_CH1, voice->index, sc);
	fillmodbuf(patchbuf2, AKS_I_CH1_LVL, voice->index, sc);

	buf = patchbuf2;
	value = mods->chan1gain * 8;
	for (i = 0; i < sc; i+=8)
	{
		*buf++ += value;
		*buf++ += value;
		*buf++ += value;
		*buf++ += value;
		*buf++ += value;
		*buf++ += value;
		*buf++ += value;
		*buf++ += value;
	}

	audiomain->palette[(*baudio->sound[5]).index]->specs->io[0].buf
		= patchbuf1;
	audiomain->palette[(*baudio->sound[5]).index]->specs->io[1].buf
		= patchbuf2;
	audiomain->palette[(*baudio->sound[5]).index]->specs->io[2].buf
		= abufs.outputs[voice->index][AKS_O_CH1];

	bristolbzero(abufs.outputs[voice->index][AKS_O_CH1], ss);

	(*baudio->sound[5]).operate(
		(audiomain->palette)[2],
		voice,
		(*baudio->sound[5]).param,
		voice->locals[voice->index][5]);

	mods->fs1 = tof(abufs.outputs[voice->index][AKS_O_CH1], sc,
		mods->loutfilt, mods->fs1);

	fillmodbuf(patchbuf1, AKS_I_CH2, voice->index, sc);
	fillmodbuf(patchbuf2, AKS_I_CH2_LVL, voice->index, sc);

	buf = patchbuf2;
	value = mods->chan2gain * 8;
	for (i = 0; i < sc; i+=8)
	{
		*buf++ += value;
		*buf++ += value;
		*buf++ += value;
		*buf++ += value;
		*buf++ += value;
		*buf++ += value;
		*buf++ += value;
		*buf++ += value;
	}

	audiomain->palette[(*baudio->sound[5]).index]->specs->io[0].buf
		= patchbuf1;
	audiomain->palette[(*baudio->sound[5]).index]->specs->io[1].buf
		= patchbuf2;
	audiomain->palette[(*baudio->sound[5]).index]->specs->io[2].buf
		= abufs.outputs[voice->index][AKS_O_CH2];

	bristolbzero(abufs.outputs[voice->index][AKS_O_CH2], ss);

	(*baudio->sound[5]).operate(
		(audiomain->palette)[2],
		voice,
		(*baudio->sound[5]).param,
		voice->locals[voice->index][5]);

	mods->fs2 = tof(abufs.outputs[voice->index][AKS_O_CH2], sc,
		mods->routfilt, mods->fs2);

	/* 
	 * finally we have the panning stage. Some of this stuff can be muted and
	 * can also take different input that needs to be taken care of here.
printf("%f %f, %f %f, %f %f\n",
mods->chan1pan, mods->chan1gain,
mods->chan2pan, mods->chan2gain,
mods->loutfilt, mods->routfilt);
	 */
	if (mods->chan1route == 0)
	{
		bufmerge(abufs.outputs[voice->index][AKS_O_CH1],
			(1.0 - mods->chan1pan) * 16,
			baudio->leftbuf, 1.0, sc);
		bufmerge(abufs.outputs[voice->index][AKS_O_CH1],
			mods->chan1pan * 16,
			baudio->rightbuf, 1.0, sc);
	} else if (mods->chan1route == 2) {
		fillmodbuf(patchbuf1, AKS_I_METER, voice->index, sc);
		bufmerge(patchbuf1, 16, baudio->leftbuf, 1.0, sc);
	}

	/*
	 * If channel routing is zero then just pan the channel. If it has the
	 * value of 2 then send the buffer to the envelope trigger.
	 */
	if (mods->chan2route == 0)
	{
		bufmerge(abufs.outputs[voice->index][AKS_O_CH2],
			(1.0 - mods->chan2pan) * 16,
			baudio->leftbuf, 1.0, sc);
		bufmerge(abufs.outputs[voice->index][AKS_O_CH2],
			mods->chan2pan * 16,
			baudio->rightbuf, 1.0, sc);
	} else if (mods->chan2route == 2) {
		bufmerge(abufs.outputs[voice->index][AKS_O_CH2], 1.0,
			abufs.outputs[voice->index][AKS_O_TRIGGER], 0.0, sc);
	}
/* Output stage done */
	return(0);
}

int
operateAksPostops(audioMain *audiomain, Baudio *baudio,
bristolVoice *voice, register float *startbuf)
{
	if (baudio->mixlocals == NULL)
		/* ((void *) baudio->mixlocals) = voice->locals[voice->index]; */
		baudio->mixlocals = (float *) voice->locals[voice->index];
	return(0);
}

static int
bristolAksDestroy(audioMain *audiomain, Baudio *baudio)
{
	printf("removing one aks\n");
	baudio->mixlocals = NULL;
	return(0);

	bristolfree(freqbuf); /* replace with 'buf = NULL'; */
	bristolfree(zerobuf); /* replace with 'buf = NULL'; */
	bristolfree(patchbuf1);
	bristolfree(patchbuf2);
	bristolfree(tmpbuf);

	bristolfree(abufs.buf);

	return(0);
}

int
bristolAksInit(audioMain *audiomain, Baudio *baudio)
{
	aksmods *mods;

	printf("initialising one aks\n");

	baudio->soundCount = 9; /* Number of operators in this voice */
	/*
	 * Assign an array of sound pointers.
	 */
	baudio->sound = (bristolSound **)
		bristolmalloc0(sizeof(bristolOP *) * baudio->soundCount);
	baudio->effect = (bristolSound **)
		bristolmalloc0(sizeof(bristolOP *) * baudio->soundCount);

	/*
	 * The synth has 3 oscillators, a filter, envelope/amp, ring mod, a reverb
	 * noise generator, plus inputs and an output filter stage.
	 */
	/* oscillator 1 */
	initSoundAlgo(	24,	0, baudio, audiomain, baudio->sound);
	initSoundAlgo(	24,	1, baudio, audiomain, baudio->sound);
	initSoundAlgo(	24,	2, baudio, audiomain, baudio->sound);
	/* A filter */
	initSoundAlgo(	27,	3, baudio, audiomain, baudio->sound);
	/* Single ADSR */
	initSoundAlgo(	25,	4, baudio, audiomain, baudio->sound);
	/* An amplifier */
	initSoundAlgo(	2,	5, baudio, audiomain, baudio->sound);
	/* An noise source */
	initSoundAlgo(	4,	6, baudio, audiomain, baudio->sound);
	/* Ring Mod */
	initSoundAlgo(	20,	7, baudio, audiomain, baudio->sound);
	/* Reverb */
	initSoundAlgo(	26,	8, baudio, audiomain, baudio->sound);

	baudio->param = aksController;
	baudio->destroy = bristolAksDestroy;
	baudio->operate = operateOneAks;
	baudio->postops = operateAksPostops;

	/*
	 */
	if (patchbuf1 == NULL)
		patchbuf1 = (float *) bristolmalloc0(audiomain->segmentsize);
	if (patchbuf2 == NULL)
		patchbuf2 = (float *) bristolmalloc0(audiomain->segmentsize);
	if (freqbuf == NULL)
		freqbuf = (float *) bristolmalloc0(audiomain->segmentsize);
	if (zerobuf == NULL)
		zerobuf = (float *) bristolmalloc0(audiomain->segmentsize);
	if (tmpbuf == NULL)
		tmpbuf = (float *) bristolmalloc0(audiomain->segmentsize);

	if (abufs.buf == NULL)
	{
		int i, v;

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
			* AKS_OUTCOUNT);
/*printf("malloc %i to AKS buffers at %x\n", */
/*audiomain->segmentsize * audiomain->voiceCount * AKS_OUTCOUNT, abufs.buf); */

		for (v = 0; v < audiomain->voiceCount; v++)
		{
			for (i = 0; i < AKS_OUTCOUNT; i++)
			{
				abufs.outputs[v][i] =
					&abufs.buf[audiomain->samplecount * (v * AKS_OUTCOUNT + i)];
			}
		}
	}

	baudio->mixlocals = (float *) bristolmalloc0(sizeof(aksmods));
	mods = (aksmods *) baudio->mixlocals;

	baudio->mixflags |= BRISTOL_STEREO;

	return(0);
}

