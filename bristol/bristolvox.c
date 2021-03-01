
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

static float *tmpbuf1 = (float *) NULL;
static float *vbuf = (float *) NULL;
static float *outbuf = (float *) NULL;
static float *percbuf = (float *) NULL;
static float *percenv = (float *) NULL;
static float *bassbuf = (float *) NULL;
static float *bassenv = (float *) NULL;

#define VOX_VIBRA		0x01
#define VOX_VIBRA_DONE	0x02
#define VOX_HIGH_NOTE	0x04
#define VOX_LOW_NOTE	0x08

extern int bristolGlobalController(struct BAudio *, u_char, u_char, float);

int
voxGlobalController(Baudio *baudio, u_char controller,
u_char operator, float value)
{
	/*
	 * Vox global controller code.
	 */
/* printf("voxGlobalController(%x, %i, %i, %f)\n", */
/* baudio, controller, operator, value); */

	/*
	 * Reverb
	 */
	if (controller == 99)
	{
		if (baudio->effect[0] == NULL)
			return(0);

		if (operator == 0)
		{
			baudio->effect[0]->param->param[operator].float_val = value;
			baudio->effect[0]->param->param[operator].int_val = 1;
		} else
			baudio->effect[0]->param->param[operator].float_val = value;
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

	if (controller == 126)
	{
		switch (operator) {
			case 0:
				if (value == 0)
					baudio->mixflags &= ~VOX_VIBRA;
				else
					baudio->mixflags |= VOX_VIBRA;
				break;
		}
	}
	return(0);
}

int vibraDone = 0;

/*
 * This will only do the envelopes for the Dual manual. It will cover the
 * bass sustain envelope and the percussive envelope, both of which are global
 * in the sense that they are not per note but for the whole manual.
 *
 * The Vox does still use its own preops, but only to clear the vibra flag.
 */
int
operateVoxM2Preops(audioMain *audiomain, Baudio *baudio,
bristolVoice *voice, register float *startbuf)
{
	int flags;

	vibraDone = 0;

	if (baudio->mixlocals == NULL)
		/* ((void *) baudio->mixlocals) = voice->locals[voice->index]; */
		baudio->mixlocals = (float *) voice->locals[voice->index];

	/*
	 * Check our voice flags, we don't want rekeys, etc, for the pair of
	 * global envelopes and the preops takes an arbitrary voice.
	 *
	 * At the moment bass sustain is not quite implemented, we need to look
	 * closer at note off events.
	 */
	flags = voice->flags;

	if (voice->flags & (BRISTOL_KEYON|BRISTOL_KEYREON))
	{
		if (baudio->lvoices != 0)
			voice->flags &= ~(BRISTOL_KEYON|BRISTOL_KEYREON);
	} else if (voice->flags & BRISTOL_KEYOFF) {
		if (baudio->lvoices != 1)
			voice->flags &= ~BRISTOL_KEYOFF;
	}

	/*
	 * Then run the envelope for the percussive.
	 */
	audiomain->palette[(*baudio->sound[2]).index]->specs->io[0].buf = percenv;
	(*baudio->sound[2]).operate(
		(audiomain->palette)[1],
		voice,
		(*baudio->sound[2]).param,
		((void **) baudio->mixlocals)[2]);

	voice->flags = flags;

	return(0);
}

int
operateVoxPreops(audioMain *audiomain, Baudio *baudio,
bristolVoice *voice, register float *startbuf)
{
	if (baudio->mixlocals == NULL)
		/* ((void *) baudio->mixlocals) = voice->locals[voice->index]; */
		baudio->mixlocals = (float *) voice->locals[voice->index];

	vibraDone = 0;

	bristolbzero(bassbuf, audiomain->segmentsize);

	return(0);
}

/*
 * Operate one vox voice. This is the single manual operation and also the 
 * lower manual of the M-II.
 */
int
operateOneVoxVoice(audioMain *audiomain, Baudio *baudio,
bristolVoice *voice, register float *startbuf)
{
	int samplecount = audiomain->samplecount;
	float *bufptr = tmpbuf1;

#ifdef DEBUG
	printf("operateOneVoxVoice(%x, %x, %x)\n",
		baudio, voice, startbuf);
#endif

	bristolbzero(startbuf, audiomain->segmentsize);
	bristolbzero(vbuf, audiomain->segmentsize);

	bufptr = tmpbuf1;

	/*
	 * Fill the wavetable with the correct note value
	 */
	fillFreqTable(baudio, voice, tmpbuf1, samplecount, 0);

//	if (voice->locals[voice->index][0] == NULL);
//		return(0);;

	if (voice->key.key >= 36)
	{
		/*
		 * Run the oscillator
		 */
		audiomain->palette[(*baudio->sound[0]).index]->specs->io[0].buf
			 = tmpbuf1;
		audiomain->palette[(*baudio->sound[0]).index]->specs->io[1].buf
			 = vbuf;
		audiomain->palette[(*baudio->sound[0]).index]->specs->io[2].buf
			 = percbuf;

		(*baudio->sound[0]).operate(
			(audiomain->palette)[17],
			voice,
			(*baudio->sound[0]).param,
			voice->locals[voice->index][0]);

		if (voice->flags & BRISTOL_KEYOFF)
		{
			int i, lsam = 0;
			float lval = 65536;

			/*
			 * We should go for a zero crossing however we might not have one.
			 * look for the lowest value sample and zero from there?
			 */
			for (i = 0; i < samplecount; i++)
			{
				if (vbuf[i] == 0)
				{
					lsam = i;
					break;
				}

				if ((vbuf[i] > 0) && (vbuf[i] < lval))
				{
					lval = vbuf[i];
					lsam = i;
				}
				if ((vbuf[i] < 0) && (vbuf[i] > -lval))
				{
					lval = -vbuf[i];
					lsam = i;
				}
			}

			for (i = 0; i < lsam; i++)
				outbuf[i] += vbuf[i];

			voice->flags |= BRISTOL_KEYDONE;
		} else
			bufmerge(vbuf, 1.0, outbuf, 1.0, audiomain->samplecount);
	} else {
		/*
		 * Ryn yhe bass oscillator. We should consider changing this to just
		 * run in the postops, picking up the first available voice below midi
		 * note 48, that way it would be monophonic and we could apply the
		 * bass envelope quite easily.
		 */
		audiomain->palette[(*baudio->sound[2]).index]->specs->io[0].buf
			= tmpbuf1;
		audiomain->palette[(*baudio->sound[2]).index]->specs->io[1].buf
			= bassbuf;
		audiomain->palette[(*baudio->sound[2]).index]->specs->io[2].buf
			= percbuf;

		(*baudio->sound[2]).operate(
			(audiomain->palette)[17],
			voice,
			(*baudio->sound[2]).param,
			voice->locals[voice->index][2]);

		/*
		 * Then run the envelopes.
		 */
		audiomain->palette[(*baudio->sound[3]).index]->specs->io[0].buf
			= bassenv;
		(*baudio->sound[3]).operate(
			(audiomain->palette)[1],
			voice,
			(*baudio->sound[3]).param,
			voice->locals[voice->index][3]);

		/*
		 * Pass the Bass through the amp using the env into the outbuf.
		 */
		audiomain->palette[(*baudio->sound[4]).index]->specs->io[0].buf
			= bassbuf;
		audiomain->palette[(*baudio->sound[4]).index]->specs->io[1].buf
			= bassenv;
		audiomain->palette[(*baudio->sound[4]).index]->specs->io[2].buf
			= vbuf;

		(*baudio->sound[4]).operate(
			(audiomain->palette)[2],
			voice,
			(*baudio->sound[4]).param,
			voice->locals[voice->index][4]);

		if (voice->flags & BRISTOL_KEYOFF)
		{
			int i, lsam = 0;
			float lval = 65536;

			/*
			 * We should go for a zero crossing however we might not have one.
			 * look for the lowest value sample and zero from there?
			 */
			for (i = 0; i < samplecount; i++)
			{
				if (vbuf[i] == 0)
				{
					lsam = i;
					break;
				}

				if ((vbuf[i] > 0) && (vbuf[i] < lval))
				{
					lval = vbuf[i];
					lsam = i;
				}
				if ((vbuf[i] < 0) && (vbuf[i] > -lval))
				{
					lval = -vbuf[i];
					lsam = i;
				}
			}

			for (i = 0; i < lsam; i++)
				outbuf[i] += vbuf[i];

			voice->flags |= BRISTOL_KEYDONE;
		} else
			bufmerge(vbuf, 1.0, outbuf, 1.0, audiomain->samplecount);
	}
	return(0);
}

/*
 * Operate one vox upper manual voice. This will cover the percussives and the 
 * bass oscillator.
 */
int
operateOneVoxM2Voice(audioMain *audiomain, Baudio *baudio,
bristolVoice *voice, register float *startbuf)
{
	int samplecount = audiomain->samplecount;
	float *bufptr = tmpbuf1;

#ifdef DEBUG
	printf("operateOneVoxVoice(%x, %x, %x)\n",
		baudio, voice, startbuf);
#endif

	bristolbzero(startbuf, audiomain->segmentsize);
	bristolbzero(percbuf, audiomain->segmentsize);

	bufptr = tmpbuf1;

	/*
	 * Fill the wavetable with the correct note value
	 */
	fillFreqTable(baudio, voice, tmpbuf1, samplecount, 0);

	/*
	 * Run the main oscillator
	 */
	audiomain->palette[(*baudio->sound[0]).index]->specs->io[0].buf = tmpbuf1;
	audiomain->palette[(*baudio->sound[0]).index]->specs->io[1].buf = outbuf;
	audiomain->palette[(*baudio->sound[0]).index]->specs->io[2].buf = percbuf;

	(*baudio->sound[0]).operate(
		(audiomain->palette)[17],
		voice,
		(*baudio->sound[0]).param,
		voice->locals[voice->index][0]);

	/*
	 * Pass the percussives through the amp using the env into the outbuf.
	 */
	audiomain->palette[(*baudio->sound[3]).index]->specs->io[0].buf = percbuf;
	audiomain->palette[(*baudio->sound[3]).index]->specs->io[1].buf = percenv;
	audiomain->palette[(*baudio->sound[3]).index]->specs->io[2].buf = outbuf;

	(*baudio->sound[3]).operate(
		(audiomain->palette)[2],
		voice,
		(*baudio->sound[3]).param,
		voice->locals[voice->index][3]);

	if (voice->flags & BRISTOL_KEYOFF)
		voice->flags |= BRISTOL_KEYDONE;

	return(0);
}

int
operateVoxPostops(audioMain *audiomain, Baudio *baudio,
bristolVoice *voice, register float *startbuf)
{
#ifdef DEBUG
	printf("operateVoxPostops(%x, %x, %x) %i\n",
		baudio, voice, startbuf, baudio->cvoices);
#endif

	if (baudio->mixlocals == NULL)
		/* ((void *) baudio->mixlocals) = voice->locals[voice->index]; */
		baudio->mixlocals = (float *) voice->locals[voice->index];

	if (vibraDone)
		return(0);

	/* Volume level EQ */
	bufmerge(outbuf, 0.0, outbuf, 4.0, audiomain->samplecount);

	if (baudio->mixflags & VOX_VIBRA)
	{
		audiomain->palette[(*baudio->sound[1]).index]->specs->io[0].buf =
			outbuf;
		audiomain->palette[(*baudio->sound[1]).index]->specs->io[1].buf =
			baudio->leftbuf;
		(*baudio->sound[1]).operate(
			(audiomain->palette)[13],
			voice,
			(*baudio->sound[1]).param,
			((void **) baudio->mixlocals)[1]);
	} else
		bufmerge(outbuf, 1.0, baudio->leftbuf, 1.0, audiomain->samplecount);

	vibraDone = 1;
	bristolbzero(outbuf, audiomain->segmentsize);

	return(0);
}

int
operateVoxM2Postops(audioMain *audiomain, Baudio *baudio,
bristolVoice *voice, register float *startbuf)
{
#ifdef DEBUG
	printf("operateVoxPostops(%x, %x, %x) %i\n",
		baudio, voice, startbuf, baudio->cvoices);
#endif

	if (baudio->mixlocals == NULL)
		/* ((void *) baudio->mixlocals) = voice->locals[voice->index]; */
		baudio->mixlocals = (float *) voice->locals[voice->index];

	if (vibraDone)
		return(0);

	/* Volume level EQ */
	bufmerge(outbuf, 0.0, outbuf, 4.0, audiomain->samplecount);

	if (baudio->mixflags & VOX_VIBRA)
	{
		audiomain->palette[(*baudio->sound[1]).index]->specs->io[0].buf =
			outbuf;
		audiomain->palette[(*baudio->sound[1]).index]->specs->io[1].buf =
			baudio->leftbuf;
		(*baudio->sound[1]).operate(
			(audiomain->palette)[13],
			voice,
			(*baudio->sound[1]).param,
			((void **) baudio->mixlocals)[1]);
	} else
		bufmerge(outbuf, 1.0, baudio->leftbuf, 1.0, audiomain->samplecount);

	vibraDone = 1;
	bristolbzero(outbuf, audiomain->segmentsize);

	return(0);
}

int
destroyOneVoxVoice(audioMain *audiomain, Baudio *baudio)
{
	printf("removing one vox\n");

	/*
	 * We need to leave these, we may have multiple invocations running
	 */
	baudio->mixlocals = NULL;
	return(0);

	if (tmpbuf1 != NULL)
		bristolfree(tmpbuf1);
	tmpbuf1 = NULL;
	if (vbuf != NULL)
		bristolfree(vbuf);
	vbuf = NULL;
	if (outbuf != NULL)
		bristolfree(outbuf);
	outbuf = NULL;
	if (percenv != NULL)
		bristolfree(percenv);
	percenv = NULL;
	if (bassenv != NULL)
		bristolfree(bassenv);
	bassenv = NULL;
	if (percbuf != NULL)
		bristolfree(percbuf);
	percbuf = NULL;
	if (bassbuf != NULL)
		bristolfree(bassbuf);
	bassbuf = NULL;

	return(0);
}

/*
 * This is the single manual Init routine, there is a separate one for the
 * second manual of the M-II. The code is still kept in this file though.
 */
int
bristolVoxInit(audioMain *audiomain, Baudio *baudio)
{
	printf("initialising vox sound\n");

	baudio->soundCount = 5; /* Number of operators in this vox voice */

	/*
	 * Assign an array of sound pointers.
	 */
	baudio->sound = (bristolSound **)
		bristolmalloc0(sizeof(bristolOP *) * baudio->soundCount);
	baudio->effect = (bristolSound **)
		bristolmalloc0(sizeof(bristolOP *) * baudio->soundCount);

	/*
	 * Vox oscillator
	 */
	initSoundAlgo(17, 0, baudio, audiomain, baudio->sound);
	/*
	 * Add in a vibrachorus - it will be configured to provide only vibra
	 */
	initSoundAlgo(13, 1, baudio, audiomain, baudio->sound);
	/*
	 * Second Bass oscillator
	 */
	initSoundAlgo(17, 2, baudio, audiomain, baudio->sound);
	/*
	 * Bass env
	 */
	initSoundAlgo(1,  3, baudio, audiomain, baudio->sound);
	/* 
	 * And amp
	 */
	initSoundAlgo(2,  4, baudio, audiomain, baudio->sound);

	baudio->param = voxGlobalController;
	baudio->destroy = destroyOneVoxVoice;
	baudio->operate = operateOneVoxVoice;
	baudio->preops = operateVoxPreops;
	baudio->postops = operateVoxPostops;

	/*
	 * Get some workspace
	 */
	if (tmpbuf1 == (float *) NULL)
		tmpbuf1 = (float *) bristolmalloc0(audiomain->segmentsize);
	if (vbuf == (float *) NULL)
		vbuf = (float *) bristolmalloc0(audiomain->segmentsize);
	if (outbuf == (float *) NULL)
		outbuf = (float *) bristolmalloc0(audiomain->segmentsize);
	if (bassenv == (float *) NULL)
		bassenv = (float *) bristolmalloc0(audiomain->segmentsize);
	if (percenv == (float *) NULL)
		percenv = (float *) bristolmalloc0(audiomain->segmentsize);
	if (bassbuf == (float *) NULL)
		bassbuf = (float *) bristolmalloc0(audiomain->segmentsize);
	if (percbuf == (float *) NULL)
		percbuf = (float *) bristolmalloc0(audiomain->segmentsize);

	return(0);
}

/*
 * This is the second manual Init routine, there is a separate one for the
 * single manual of the M-I.
 */
int
bristolVoxM2Init(audioMain *audiomain, Baudio *baudio)
{
printf("initialising vox upper manual\n");

	baudio->soundCount = 4; /* Number of operators in this vox voice */

	/*
	 * Assign an array of sound pointers.
	 */
	baudio->sound = (bristolSound **)
		bristolmalloc0(sizeof(bristolOP *) * baudio->soundCount);
	baudio->effect = (bristolSound **)
		bristolmalloc0(sizeof(bristolOP *) * baudio->soundCount);

	/* 
	 * For the dual manual we also need 2 envelopes, one for percussion, one
	 * for bass sustain. We should consider an extra oscillator for the bass
	 * only used for low keynumbers.
	 */

	baudio->param = voxGlobalController;
	baudio->destroy = destroyOneVoxVoice;
	baudio->operate = operateOneVoxM2Voice;
	baudio->preops = operateVoxM2Preops;
	baudio->postops = operateVoxM2Postops;

	/* 
	 * For the dual manual we also need 2 envelopes, one for percussion, one
	 * for bass sustain. We should consider an extra oscillator for the bass
	 * only used for low keynumbers.
	 */
	initSoundAlgo(17, 0, baudio, audiomain, baudio->sound);
	/* Add in a vibrachorus - it will be configured to provide only vibra */
	initSoundAlgo(13, 1, baudio, audiomain, baudio->sound);
	/* Percussive env */
	initSoundAlgo(1,  2, baudio, audiomain, baudio->sound);
	/* Amp */
	initSoundAlgo(2,  3, baudio, audiomain, baudio->sound);

	/* (Don't) Put in a reverb
	initSoundAlgo(	22, 0, baudio, audiomain, baudio->effect);
	*/

	/*
	 * Get some workspace
	 */
	if (tmpbuf1 == (float *) NULL)
		tmpbuf1 = (float *) bristolmalloc0(audiomain->segmentsize);
	if (outbuf == (float *) NULL)
		outbuf = (float *) bristolmalloc0(audiomain->segmentsize);
	if (bassenv == (float *) NULL)
		bassenv = (float *) bristolmalloc0(audiomain->segmentsize);
	if (percenv == (float *) NULL)
		percenv = (float *) bristolmalloc0(audiomain->segmentsize);
	if (bassbuf == (float *) NULL)
		bassbuf = (float *) bristolmalloc0(audiomain->segmentsize);
	if (percbuf == (float *) NULL)
		percbuf = (float *) bristolmalloc0(audiomain->segmentsize);

	return(0);
}

