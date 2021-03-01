
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
#include "rotary.h"

static float *tmpbuf1 = (float *) NULL;
static float *tmpbuf2 = (float *) NULL;
static float *tmpbuf3 = (float *) NULL;
static float *tmpbuf4 = (float *) NULL;

#define HAMMOND_VIBRA 0x01
#define HAMMOND_SERMON 0x02

extern int bristolGlobalController(struct BAudio *, u_char, u_char, float);
extern int initthesermon(int, int, int);

static int sineform = 0, samplecount, samplerate;
static int operateHammondPostops();

int
hammondGlobalController(Baudio *baudio, u_char controller,
u_char operator, float value)
{
	/*
	 * Hammond global controller code.
	 */
/*	printf("hammondGlobalController(%x, %i, %i, %f)\n", */
/*		baudio, controller, operator, value); */

	/*
	 * Find out if this is our Leslie controller.
	 */
	if (controller == 100)
	{
		if (baudio->effect[1] == 0)
			return(0);

		switch (operator) {
			case 0:
				/*
				 * Need to put some logic in here for start/stop. It is easy
				 * enough to migrate the speed to some ridiculously slow 
				 * amount such that the leslie does not appear to rotate, but
				 * then we also need to get it kickstarted afterwards.
				 */
				baudio->effect[1]->param->param[operator].int_val
					= 515 - value * 512;
				/*
				if (value == 0.0) {
					leslieStopped = 1;
					baudio->effect[1]->param->param[operator].int_val = 4096;
				} else {
					if (leslieStopped == 1) {
						leslieStopped = 0;
						baudio->effect[1]->param->param[operator].int_val = 227;
					}
				}
				*/
/*printf("speed is %i\n", baudio->effect[1]->param->param[operator].int_val); */
				break;
			case 1:
				baudio->effect[1]->param->param[operator].float_val
					= value / 10 + 0.01;
				baudio->effect[1]->param->param[operator].int_val
					= value * 20;
/*printf("inertia is %i (%f)\n", */
/*baudio->effect[1]->param->param[operator].int_val, */
/*baudio->effect[1]->param->param[operator].float_val); */
				break;
			case 2:
				baudio->effect[1]->param->param[operator].int_val
					= value * 20 + 1;
/*printf("delay is %i\n", baudio->effect[1]->param->param[operator].int_val); */
				break;
			case 3:
				baudio->effect[1]->param->param[operator].float_val = value;
/*printf("depth is %f\n", baudio->effect[1]->param->param[operator].float_val); */
				break;
			case 4:
				baudio->effect[1]->param->param[operator].float_val
					= value * 0.5;
				break;
			case 5:
			/*
				value = 0.5 - value;
				baudio->effect[1]->param->param[operator].float_val
					= 1.0 + value * 10;
				if (value < 0)
					baudio->effect[1]->param->param[operator].float_val
						= 1 / (1.0 - (value * 10));
			*/
				baudio->effect[1]->param->param[operator].float_val
					= 1.0 - (value * 0.8);
				break;
			case 6: /* Crossover frequency */
				 if ((baudio->effect[1]->param->param[operator].float_val =
				 	value * 0.1) == 0)
					baudio->effect[1]->param->param[operator].float_val
						= (float) 1.0 / CONTROLLER_RANGE;
/*printf("crossover now %f\n", */
/*baudio->effect[1]->param->param[operator].float_val); */
				break;
			case 7: /* flags 0=off, 1=normal, 2=SYNC, 3=HornOnly. */
				if (((int) (value * CONTROLLER_RANGE)) == 4)
					/*
					 * Cheat here. Reuse the int val for depth, which is float.
					 */
					baudio->effect[1]->param->param[3].int_val = 0;
				else if (((int) (value * CONTROLLER_RANGE)) == 5)
					/*
					 * Cheat here. Reuse the int val for depth, which is float.
					 */
					baudio->effect[1]->param->param[3].int_val = 1;
				else
					baudio->effect[1]->param->param[operator].int_val =
				 		value * CONTROLLER_RANGE;
/*printf("flags are now %i, %i\n", */
/*baudio->effect[1]->param->param[operator].int_val, */
/*baudio->effect[1]->param->param[3].int_val); */
				break;
		}
	}

	/*
	 * Reverb
	 */
	if (controller == 101)
	{
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
					baudio->mixflags &= ~HAMMOND_VIBRA;
				else
					baudio->mixflags |= HAMMOND_VIBRA;
				break;
			case 1:
				if ((sineform = value * CONTROLLER_RANGE) < 0)
					sineform = 0;
				else if (sineform > 3)
					sineform = 3;
				break;
			case 2:
				/*
				 * samplecount is a bit of a hack
				 */
				if (value == 0)
					initthesermon(samplecount, samplerate, 0);
				else
					initthesermon(samplecount, samplerate, 1);
				break;
			case 3:
				if (value == 0)
					baudio->mixflags &= ~HAMMOND_SERMON;
				else
					baudio->mixflags |= HAMMOND_SERMON;
				break;
		}
	}
	return(0);
}

extern void thesermon(int, int);
extern void therequiem(float *, float *, int);
/*
 * Dosermon is a cheat, sort of. If we are running multiple synths, all using
 * the sermon, then we only want to run it once per buffer cycle. The second
 * call will find this zeroed. Apart from that, at 500mips per go, thesermon()
 * is rather heavyweight.
 */
int dosermon = 1;

int
operateHammondPreops(audioMain *audiomain, Baudio *baudio,
bristolVoice *voice, register float *startbuf)
{
/*	samplecount = audiomain->samplecount; */
/*	samplerate = audiomain->samplerate; */

	if ((baudio->mixflags & HAMMOND_SERMON) && dosermon)
		thesermon(audiomain->samplecount, sineform);

	dosermon = 0;

	return(0);
}

int
operateHammondB3Postops(audioMain *audiomain, Baudio *baudio,
bristolVoice *voice, register float *startbuf)
{
	/*
	 * Going to have to cheat here. We want to run the postops for the upper
	 * manual only. This is the next synth on the list, since it is created
	 * before the lower manual.
	 */
	if (dosermon)
		return(0);

	/*
	 * Hm. If the upper manual has not been played yet, then the lower manual
	 * is going to have to remain silent for the time being. There is a simple
	 * hack in the gui - send a note on/note off command for the upper manual,
	 * but for now....
	 */
	if (baudio->next->firstVoice == 0)
		return(0);

	operateHammondPostops(audiomain, baudio->next, baudio->next->firstVoice,
		startbuf);

	return(0);
}

/* Should bury this somewhere..... */
float pbHLast;
float pbLLast;

static int
operateHammondPostops(audioMain *audiomain, Baudio *baudio,
bristolVoice *voice, register float *startbuf)
{
	int i, flags, samplecount = audiomain->samplecount;
	float *hpf;

/*	therequiem(audiomain->samplecount, &baudio->ctab[0], sineform); */
	therequiem(baudio->leftbuf, baudio->rightbuf, samplecount);

	if (dosermon)
		return(0);

	dosermon = 1;

#ifdef DEBUG
	printf("operateHammondPostops(%x, %x, %x) %i\n",
		baudio, voice, startbuf, baudio->cvoices);
#endif

	if (baudio->mixlocals == NULL)
		/* ((void *) baudio->mixlocals) = voice->locals[voice->index]; */
		baudio->mixlocals = (float *) voice->locals[voice->index];

	/*
	 * Run the ADSR for the percussives. We have to play about a little, since
	 * this envelope should be keyed on when a single voice activates, and then
	 * remain on until the last key is released, ie, there should be no 
	 * retriggers for each key.
	if ((*baudio->sound[4]).param->param[2].float_val != 0)
		return;
	 */

	flags = voice->flags;

	if (flags & (BRISTOL_KEYON|BRISTOL_KEYREON))
	{
		if (baudio->lvoices != 0)
			voice->flags &= ~(BRISTOL_KEYON|BRISTOL_KEYREON);
	} else if (flags & BRISTOL_KEYOFF) {
		if (baudio->lvoices != 1)
			voice->flags &= ~BRISTOL_KEYOFF;
	}

	audiomain->palette[(*baudio->sound[4]).index]->specs->io[0].buf = tmpbuf1;
	(*baudio->sound[4]).operate(
		(audiomain->palette)[1],
		voice,
		(*baudio->sound[4]).param,
		((void **) baudio->mixlocals)[4]);
	voice->flags = flags;

	/*
	 * See if we need to merge the lower manual
	 */
	if (tmpbuf4)
	{
		bufmerge(tmpbuf4, 1.0, baudio->leftbuf, 1.0, samplecount);
		bristolbzero(tmpbuf4, audiomain->segmentsize);
	}

	if (baudio->mixflags & HAMMOND_VIBRA)
	{
		audiomain->palette[(*baudio->sound[6]).index]->specs->io[0].buf =
			baudio->leftbuf;
		audiomain->palette[(*baudio->sound[6]).index]->specs->io[1].buf =
			baudio->leftbuf;
		(*baudio->sound[6]).operate(
			(audiomain->palette)[28],
			voice,
			(*baudio->sound[6]).param,
			((void **) baudio->mixlocals)[6]);
	}

	/*
	 * Run the percussive oscillators into the amplifier, summing the output
	 * in with our other non-percussive oscillator.
	 *
	 * Certain models (A100) had a HPF running over the percussive bus.
	 * Bypassing the vibra already improved the quality of the "ping", and
	 * the highpass filter?
	 */
	hpf = baudio->rightbuf;

	for (i = 0; i < samplecount; i++)
	{
		pbHLast += (*hpf - pbHLast) * 0.2;
		*hpf -= pbHLast;
		*hpf++ *= 0.8;
	}

	audiomain->palette[(*baudio->sound[5]).index]->specs->io[0].buf =
		baudio->rightbuf;
	audiomain->palette[(*baudio->sound[5]).index]->specs->io[1].buf =
		tmpbuf1;
	audiomain->palette[(*baudio->sound[5]).index]->specs->io[2].buf =
		baudio->leftbuf;

	(*baudio->sound[5]).operate(
		(audiomain->palette)[2],
		voice,
		(*baudio->sound[5]).param,
		voice->locals[voice->index][5]);

	bufmerge(baudio->leftbuf, 0.0, baudio->leftbuf, 4.0, samplecount);

	bristolbzero(baudio->rightbuf, audiomain->segmentsize);

	return(0);
}

/*
 * Operate one hammond voice will function as the upper manual for a B3. If
 * requested there may also be a second manual active, it will have limited 
 * functionality regarding percussives, etc. We should rewrite this, the voices
 * should not actually produce any sound: they should build a gaintable for the
 * harmonics that need to be tapped off the gearbox with offsets for new notes.
 * Then we call therequiem() that will generate any wheels that have non-zero
 * gain and mix them all down.
 */
int
operateOneHammondVoice(audioMain *audiomain, Baudio *baudio,
bristolVoice *voice, register float *startbuf)
{
	int samplecount = audiomain->samplecount;
	float *bufptr = tmpbuf1;

#ifdef DEBUG
	printf("operateOneHammondVoice(%x, %x, %x)\n",
		baudio, voice, startbuf);
#endif

	bristolbzero(tmpbuf2, audiomain->segmentsize);
	bristolbzero(startbuf, audiomain->segmentsize);

	bufptr = tmpbuf1;

	/*
	 * Fill the wavetable with the correct note value
	 */
	fillFreqTable(baudio, voice, tmpbuf1, samplecount, 0);

	/*
	 * Run the first oscillators
	 */
	audiomain->palette[(*baudio->sound[0]).index]->specs->io[0].buf = tmpbuf1;
	audiomain->palette[(*baudio->sound[0]).index]->specs->io[1].buf = startbuf;
	audiomain->palette[(*baudio->sound[0]).index]->specs->io[2].buf = tmpbuf2;

	(*baudio->sound[0]).operate(
		(audiomain->palette)[B_HAMMOND],
		voice,
		(*baudio->sound[0]).param,
		voice->locals[voice->index][0]);

	bristolbzero(tmpbuf1, audiomain->segmentsize);

	/*
	 * Run the ADSR for the final stage amplifier, reusing the startbuf.
	 */
	audiomain->palette[(*baudio->sound[1]).index]->specs->io[0].buf = tmpbuf1;
	(*baudio->sound[1]).operate(
		(audiomain->palette)[B_ENV],
		voice,
		(*baudio->sound[1]).param,
		voice->locals[voice->index][1]);

	/*
	 * Run the mixed oscillators into the amplifier. Input and output buffers
	 * are the same.
	 */
	audiomain->palette[(*baudio->sound[2]).index]->specs->io[0].buf = startbuf;
	audiomain->palette[(*baudio->sound[2]).index]->specs->io[1].buf = tmpbuf1;
	audiomain->palette[(*baudio->sound[2]).index]->specs->io[2].buf
		= baudio->leftbuf;

	(*baudio->sound[2]).operate(
		(audiomain->palette)[B_DCA],
		voice,
		(*baudio->sound[2]).param,
		voice->locals[voice->index][2]);

	/*
	 * Run the perc oscillators into the amplifier. Input and output buffers
	 * are the same.
	 */
	audiomain->palette[(*baudio->sound[2]).index]->specs->io[0].buf = tmpbuf2;

	if ((*baudio->sound[4]).param->param[2].float_val == 0)
		audiomain->palette[(*baudio->sound[2]).index]->specs->io[2].buf
			= baudio->rightbuf;
	else
		audiomain->palette[(*baudio->sound[2]).index]->specs->io[2].buf
			= baudio->leftbuf;

	(*baudio->sound[2]).operate(
		(audiomain->palette)[B_DCA],
		voice,
		(*baudio->sound[2]).param,
		voice->locals[voice->index][2]);

	return(0);
}

/*
 * Operate one hammond voice will function as the lower manual for a B3.
 * This also manages the bass pedalboard.
 */
int
operateOneHammondB3Voice(audioMain *audiomain, Baudio *baudio,
bristolVoice *voice, register float *startbuf)
{
#ifdef DEBUG
	printf("operateOneHammondB3Voice(%x, %x, %x)\n",
		baudio, voice, startbuf);
#endif

	bristolbzero(tmpbuf2, audiomain->segmentsize);
	bristolbzero(tmpbuf3, audiomain->segmentsize);
/*	bristolbzero(startbuf, audiomain->segmentsize); */

	/* PUT IN THE KEY LIMIT CHECKING, ADJUST THE KEY VALUE FOR BASS */

	/*
	 * Fill the wavetable with the correct note value - we may not need to do
	 * this - its not for the preacher algorithm.
	 */

	if (voice->key.key < 36) {
		voice->key.key += 36;

		fillFreqTable(baudio, voice, tmpbuf1, audiomain->samplecount, 0);
		/*
		 * Run the first oscillators for the bass pedals.
		 */
		audiomain->palette[(*baudio->sound[3]).index]->specs->io[0].buf
			= tmpbuf1;
		audiomain->palette[(*baudio->sound[3]).index]->specs->io[1].buf
			= tmpbuf2;
		audiomain->palette[(*baudio->sound[3]).index]->specs->io[2].buf
			= tmpbuf3;

		(*baudio->sound[3]).operate(
			(audiomain->palette)[B_HAMMOND],
			voice,
			(*baudio->sound[3]).param,
			voice->locals[voice->index][3]);
		voice->key.key -= 36;
	} else {
		fillFreqTable(baudio, voice, tmpbuf1, audiomain->samplecount, 0);
		/*
		 * Run the first oscillators
		 */
		audiomain->palette[(*baudio->sound[0]).index]->specs->io[0].buf
			= tmpbuf1;
		audiomain->palette[(*baudio->sound[0]).index]->specs->io[1].buf
			= tmpbuf2;
		audiomain->palette[(*baudio->sound[0]).index]->specs->io[2].buf
			= tmpbuf3;

		(*baudio->sound[0]).operate(
			(audiomain->palette)[B_HAMMOND],
			voice,
			(*baudio->sound[0]).param,
			voice->locals[voice->index][0]);
	}

	bristolbzero(tmpbuf1, audiomain->segmentsize);

	/*
	 * Run the ADSR for the final stage amplifier, reusing the startbuf.
	 */
	audiomain->palette[(*baudio->sound[1]).index]->specs->io[0].buf = tmpbuf1;
	(*baudio->sound[1]).operate(
		(audiomain->palette)[B_ENV],
		voice,
		(*baudio->sound[1]).param,
		voice->locals[voice->index][1]);

	/*
	 * Run the mixed oscillators into the amplifier. Input and output buffers
	 * are the same.
	 */
	audiomain->palette[(*baudio->sound[2]).index]->specs->io[0].buf = tmpbuf2;
	audiomain->palette[(*baudio->sound[2]).index]->specs->io[1].buf = tmpbuf1;
	audiomain->palette[(*baudio->sound[2]).index]->specs->io[2].buf = tmpbuf4;

	(*baudio->sound[2]).operate(
		(audiomain->palette)[B_DCA],
		voice,
		(*baudio->sound[2]).param,
		voice->locals[voice->index][2]);

	return(0);
}

int
destroyOneHammondVoice(audioMain *audiomain, Baudio *baudio)
{
printf("removing hammond sound\n");
//	bristolfree(tmpbuf1);
//	bristolfree(tmpbuf2);

	baudio->mixlocals = NULL;
	return(0);
}

int
destroyOneHammondB3Voice(audioMain *audiomain, Baudio *baudio)
{
printf("removing B3 sound\n");
//	bristolfree(tmpbuf3);
//	bristolfree(tmpbuf4);

//	tmpbuf3 = NULL;
//	tmpbuf4 = NULL;

	baudio->mixlocals = NULL;
	return(0);
}

int
bristolHammondInit(audioMain *audiomain, Baudio *baudio)
{
printf("initialising one hammond sound\n");

	baudio->soundCount = 7; /* Number of operators in this hammond voice */

	/*
	 * Assign an array of sound pointers.
	 */
	baudio->sound = (bristolSound **)
		bristolmalloc0(sizeof(bristolOP *) * baudio->soundCount);
	baudio->effect = (bristolSound **)
		bristolmalloc0(sizeof(bristolOP *) * baudio->soundCount);

	if ((audiomain->debuglevel & BRISTOL_DEBUG_MASK) > BRISTOL_DEBUG3)
		printf("malloced sound: %p\n", baudio->sound);

	/*
	 * Hammond oscillator
	 */
	initSoundAlgo(5, 0, baudio, audiomain, baudio->sound);
	/*
	 * Envelope for note_on conditioning.
	 */
	initSoundAlgo(1, 1, baudio, audiomain, baudio->sound);
	/*
	 * this envelope needs an amp.
	 */
	initSoundAlgo(2, 2, baudio, audiomain, baudio->sound);
	/*
	 * Envelope for key click, amp not required.
	 */
	initSoundAlgo(1, 3, baudio, audiomain, baudio->sound);
	/*
	 * Envelope for percussive keys. This is a preop.
	 */
	initSoundAlgo(1, 4, baudio, audiomain, baudio->sound);
	/*
	 * this envelope needs an amp.
	 */
	initSoundAlgo(2, 5, baudio, audiomain, baudio->sound);
	/*
	 * Add in a hammond vibrachorus
	 */
	initSoundAlgo(28, 6, baudio, audiomain, baudio->sound);
	/*
	 * Hammond oscillator, lower manual
	initSoundAlgo(5, 7, baudio, audiomain, baudio->sound);
	 */

	baudio->param = hammondGlobalController;
	baudio->destroy = destroyOneHammondVoice;
	baudio->operate = operateOneHammondVoice;
	baudio->preops = operateHammondPreops;
	baudio->postops = operateHammondPostops;

	/*
	 * Put in a reverb and leslie rotary on our effects list. First need to 
	 * make the engine aware of linked output effects lists.....
	 */
	initSoundAlgo(22, 0, baudio, audiomain, baudio->effect);
	initSoundAlgo(7, 1, baudio, audiomain, baudio->effect);

	samplecount = audiomain->samplecount;
	samplerate = audiomain->samplerate;
	initthesermon(audiomain->samplecount, audiomain->samplerate, 0);

	/*
	 * Get some workspace
	 */
	if (tmpbuf1 == (float *) NULL)
	{
		tmpbuf1 = (float *) bristolmalloc0(audiomain->segmentsize);
		tmpbuf2 = (float *) bristolmalloc0(audiomain->segmentsize);
	}

	return(0);
}

int
bristolHammondB3Init(audioMain *audiomain, Baudio *baudio)
{
printf("initialising one hammond second manual\n");

	baudio->soundCount = 4; /* Number of operators in this hammond voice */

	/*
	 * Assign an array of sound pointers.
	 */
	baudio->sound = (bristolSound **)
		bristolmalloc0(sizeof(bristolOP *) * baudio->soundCount);
	baudio->effect = (bristolSound **)
		bristolmalloc0(sizeof(bristolOP *) * baudio->soundCount);

	/*
	 * Hammond oscillator
	 */
	initSoundAlgo(B_HAMMOND, 0, baudio, audiomain, baudio->sound);
	/*
	 * Envelope for note_on conditioning.
	 */
	initSoundAlgo(B_ENV, 1, baudio, audiomain, baudio->sound);
	/*
	 * this envelope needs an amp.
	 */
	initSoundAlgo(B_DCA, 2, baudio, audiomain, baudio->sound);
	/*
	 * Hammond oscillator
	 */
	initSoundAlgo(B_HAMMOND, 3, baudio, audiomain, baudio->sound);

	baudio->param = hammondGlobalController;
	baudio->destroy = destroyOneHammondB3Voice;
	baudio->operate = operateOneHammondB3Voice;
	baudio->preops = operateHammondPreops;
	baudio->postops = operateHammondB3Postops;

	/*
	 * Get some workspace
	 */
	if (tmpbuf3 == (float *) NULL)
	{
		tmpbuf3 = (float *) bristolmalloc0(audiomain->segmentsize);
		tmpbuf4 = (float *) bristolmalloc0(audiomain->segmentsize);
	}

	return(0);
}

