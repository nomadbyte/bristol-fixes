
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
/*#define BRISTOL_DBG */
/*
 * Need to have basic template for an operator. Will consist of
 *
 *	lfoinit()
 *	operate()
 *	reset()
 *	destroy()
 *
 *	destroy() is in the library.
 *
 * Operate will be called when all the inputs have been loaded, and the result
 * will be an output buffer written to the next operator.
 */

#include <math.h>

#include "bristol.h"
#include "lfo.h"

float note_diff;

/*
 * The name of this operator, IO count, and IO names.
 */
#define OPNAME "LFO"
#define OPDESCRIPTION "Digitally Controlled LFO"
#define PCOUNT 6
#define IOCOUNT 7

#define LFO_IN_IND 0
#define LFO_TRI_IND 1
#define LFO_SQUARE_IND 2
#define LFO_SH_IND 3
#define LFO_SINE_IND 4
#define LFO_RAMP_IND 5
#define LFO_DRAMP_IND 6

#define LFO_WAVE_COUNT 6

static void fillWave();
static float *bitbucket = NULL;

/*
 * Reset any local memory information.
 */
static int destroy(bristolOP *operator)
{
#ifdef BRISTOL_DBG
	printf("destroy(%x)\n", operator);
#endif

	/*
	 * Unmalloc anything we added to this structure
	 */
	bristolfree(((bristolLFO *) operator)->wave[0]);
	bristolfree(((bristolLFO *) operator)->wave[1]);
	bristolfree(((bristolLFO *) operator)->wave[2]);
	bristolfree(((bristolLFO *) operator)->wave[3]);
	bristolfree(((bristolLFO *) operator)->wave[4]);
	bristolfree(((bristolLFO *) operator)->wave[5]);
	bristolfree(((bristolLFO *) operator)->wave[6]);
	bristolfree(((bristolLFO *) operator)->wave[7]);

	bristolfree(operator->specs);

	/*
	 * Free any local memory. We should also free ourselves, since we did the
	 * initial allocation.
	 */
	cleanup(operator);
	return(0);
}

/*
 * Reset any local memory information.
 */
static int reset(bristolOP *operator, bristolOPParams *param)
{
#ifdef BRISTOL_DBG
	printf("lforeset(%x)\n", operator);
#endif

	param->param[0].float_val = 3;
	param->param[1].int_val = 0;
	param->param[2].float_val = 0.0;
	param->param[3].float_val = 0.0;

	param->param[4].float_val = 0.1;
	param->param[5].float_val = 20;

	return(0);
}

/*
 * Alter an internal parameter of an operator.
 */
static int param(bristolOP *operator, bristolOPParams *param,
	unsigned char index, float value)
{
#ifdef BRISTOL_DBG
	printf("lfoparam(%x, %x, %i, %f)\n", operator, param, index, value);
#endif

	/* printf("sr %i\n", operator->specs->io[0].samplerate); */

	switch (index) {
		case 0:
			/*
			 * Tune in "small" amounts. This give a range of about 0.01Hz to
			 * somewhere near 50Hhz. The Crumar bit-1 ran them up to 250Hz for
			 * some FM type effects although am not certain I want to do that.
			 *
			 * Revised to be closer to 100Hz.
			param->param[index].float_val = 0.058
				+ gainTable[(int) (value * C_RANGE_MIN_1)].gain * 18;
			 *
			 * Min value = min freq = 0.01 Hz:
			 * 	Samples per wave = sr / f.
			 * 	Samples per wtbl = 1024 / samples per wave
			 *
			 * 	Step = SZE * f / sr;
			 *
			 * 	Min = 1024 * 0.1 / 44100;
			 * 	Max = 1024 * 100 / 44100;
			 */
			param->param[index].float_val =
				 (param->param[4].float_val
				 	+ value * value * value * param->param[5].float_val)
				 	* LFO_WAVE_SZE / operator->specs->io[0].samplerate;
			break;
		case 1:
			if (value == 0)
				param->param[index].int_val = 0;
			else
				param->param[index].int_val = 1;
			break;
		case 2: /* Velocity sensitive frequencies. */
			param->param[index].float_val = value * 15;
			break;
		case 3: /* Wheel sensitive frequencies. */
			param->param[index].float_val = value * 15;
			break;
		case 4: /* lower limit, cycle from 0.01Hz up to 0.5Hz */
			param->param[index].float_val = 0.01 + 0.5 * value;
			break;
		case 5: /* upper limit, cycle from 5Hz up to 200Hz */
			param->param[index].float_val = 5 + 195 * value;
			break;
	}

	return(0);
}

/*
 * As of the first write, 9/11/01, this takes nearly 20mips to operate a single
 * oscillator. Will work on optimisation of the code, using non-referenced 
 * variables in register workspace.
 *
 *	output buffer pointer.
 *	output buffer index.
 *	input buffer index.
 *	wavetable.
 *	wavetable index.
 *	count.
 *
 * With optimisations this is reduced to a nominal amount. Put most parameters
 * in registers, and then stretched the for loop to multiples of 16 samples.
 *
 * I would prefer to have this turned into a linear input, rated at 1float per
 * octave. This would require some considerable alterations to the oscillator
 * sample step code, though. The basic operation is reasonably simple - we take
 * a (midikey/12) and it gives us a step rate. We need to work in portamento and
 * some more subtle oscillator driver changes.
 */
static int operate(bristolOP *operator,
	bristolVoice *voice,
	bristolOPParams *param,
	void *lcl)
{
	bristolLFOlocal *local = lcl;
	register int obp = 0, count;
	register float *ib, *ob, *sb, *shb, *wt, *wt2, wtp, freq, sandh;
	register float *sine, *wt3, *ramp, *dramp, *wt4, *wt5;
	bristolLFO *specs;

	specs = (bristolLFO *) operator->specs;

#ifdef BRISTOL_DBG
	printf("lfo(%x, %x, %x)\n", operator, param, local);
#endif

	count = specs->spec.io[LFO_TRI_IND].samplecount;

	ib = specs->spec.io[LFO_IN_IND].buf;
	if ((ob = specs->spec.io[LFO_TRI_IND].buf) == NULL)
		ob = bitbucket;
	if ((sb = specs->spec.io[LFO_SQUARE_IND].buf) == NULL)
		sb = bitbucket;
	if ((shb = specs->spec.io[LFO_SH_IND].buf) == NULL)
		shb = bitbucket;
	if ((sine = specs->spec.io[LFO_SINE_IND].buf) == NULL)
		sine = bitbucket;
	if ((ramp = specs->spec.io[LFO_RAMP_IND].buf) == NULL)
		ramp = bitbucket;
	if ((dramp = specs->spec.io[LFO_DRAMP_IND].buf) == NULL)
		dramp = bitbucket;

	wt = specs->wave[4]; /* triwave */
	wt2 = specs->wave[1]; /* square wave */
	wt3 = specs->wave[0]; /* sine wave */
	wt4 = specs->wave[3]; /* ramp wave */
	wt5 = specs->wave[7]; /* down ramp wave */
	wtp = local->wtp;
	sandh = local->sandh;

	freq = param->param[0].float_val
		+ voice->velocity * param->param[2].float_val
		+ voice->baudio->contcontroller[1] * param->param[3].float_val;

	/*
	 * There is some awkward logic here. The sync flags was typically only
	 * available on mono synths but we are trying to be poly. This implies that
	 * LFO sync to each reon (restruck key) will give choppy results if there
	 * is only one LFO per emulator. If there is one per voice then we perhaps
	 * do want this enabled.
	 */
	if (((voice->flags & BRISTOL_KEYON) || (voice->flags & BRISTOL_KEYREON))
		&& (param->param[1].int_val != 0))
	{
		wtp = 0;

		/* if ((~voice->flags & BRISTOL_KEYREON) && */
		if ((voice->offset > 0) && (voice->offset < count))
		{
			if (voice->baudio->midiflags & BRISTOL_MIDI_DEBUG1)
				printf("lfo trigger offset %i frames\n", voice->offset);

			obp += voice->offset;
			memset(ob, 0.0f, voice->offset * sizeof(float));
			memset(sb, 0.0f, voice->offset * sizeof(float));
			memset(sine, 0.0f, voice->offset * sizeof(float));
			memset(ramp, 0.0f, voice->offset * sizeof(float));
			memset(dramp, 0.0f, voice->offset * sizeof(float));
			memset(shb, 0.0f, voice->offset * sizeof(float));
		}
	}

	/*
	 * Go jumping through the wavetable, with each jump defined by the value
	 * given on our input line, making sure we fill one output buffer.
	 */
	for (; obp < count; obp++)
	{
		/*
		 * Take a sample from the wavetable into the output buffer.
		 */
		ob[obp] = wt[(int) wtp];
		sb[obp] = wt2[(int) wtp];
		sine[obp] = wt3[(int) wtp];
		ramp[obp] = wt4[(int) wtp];
		dramp[obp] = wt5[(int) wtp];
		shb[obp] = sandh;

		/*
		 * Move the wavetable pointer forward by amount indicated in input 
		 * buffer for this sample.
		 */
		if ((wtp += freq) >= LFO_WAVE_SZE)
		{
			wtp -= LFO_WAVE_SZE;
			if (ib)
				sandh = ib[obp];
		}

		/*
		 * If we have gone negative, round back up. Allows us to run the 
		 * oscillator backwards.
		 */
		if (wtp < 0)
			wtp += LFO_WAVE_SZE;
	}

	local->wtp = wtp;
	local->sandh = sandh;
	return(0);
}

/*
 * Setup any variables in our OP structure, in our IO structures, and malloc
 * any memory we need.
 */
bristolOP *
lfoinit(bristolOP **operator, int index, int samplerate, int samplecount)
{
	bristolLFO *specs;

	*operator = bristolOPinit(operator, index, samplecount);

#ifdef BRISTOL_DBG
	printf("lfoinit(%x(%x), %i, %i, %i)\n",
		operator, *operator, index, samplerate, samplecount);
#endif

	note_diff = pow(2, ((double) 1)/12);

	if (bitbucket == NULL)
		bitbucket = bristolmalloc(sizeof(float) * samplecount);
		

	/*
	 * Then the local parameters specific to this operator. These will be
	 * the same for each operator, but must be inited in the local code.
	 */
	(*operator)->operate = operate;
	(*operator)->destroy = destroy;
	(*operator)->reset = reset;
	(*operator)->param= param;

	specs = (bristolLFO *) bristolmalloc0(sizeof(bristolLFO));
	(*operator)->specs = (bristolOPSpec *) specs;
	(*operator)->size = sizeof(bristolLFO);
	/*
	 * These are specific to this operator, and will need to be altered for
	 * each operator.
	 */
	specs->spec.opname = OPNAME;
	specs->spec.description = OPDESCRIPTION;
	specs->spec.pcount = PCOUNT;
	specs->spec.iocount = IOCOUNT;
	specs->spec.localsize = sizeof(bristolLFOlocal);

	/*
	 * We are going to assign multiple waves to this oscillator.
	 * sine, ramp, square, triangle?
	 */
	specs->wave[0] = (float *) bristolmalloc(LFO_WAVE_SZE * sizeof(float));
	specs->wave[1] = (float *) bristolmalloc(LFO_WAVE_SZE * sizeof(float));
	specs->wave[2] = (float *) bristolmalloc(LFO_WAVE_SZE * sizeof(float));
	specs->wave[3] = (float *) bristolmalloc(LFO_WAVE_SZE * sizeof(float));
	specs->wave[4] = (float *) bristolmalloc(LFO_WAVE_SZE * sizeof(float));
	specs->wave[5] = (float *) bristolmalloc(LFO_WAVE_SZE * sizeof(float));
	specs->wave[6] = (float *) bristolmalloc(LFO_WAVE_SZE * sizeof(float));
	specs->wave[7] = (float *) bristolmalloc(LFO_WAVE_SZE * sizeof(float));

	/*
	 * FillWave is something that should be called as a parameter change, but
	 * for testing will load it here.
	 */
	fillWave(specs->wave[0], LFO_WAVE_SZE, 0);
	fillWave(specs->wave[1], LFO_WAVE_SZE, 1);
	fillWave(specs->wave[2], LFO_WAVE_SZE, 2);
	fillWave(specs->wave[3], LFO_WAVE_SZE, 3);
	fillWave(specs->wave[4], LFO_WAVE_SZE, 4);
	fillWave(specs->wave[5], LFO_WAVE_SZE, 5);
	fillWave(specs->wave[6], LFO_WAVE_SZE, 6);
	fillWave(specs->wave[7], LFO_WAVE_SZE, 7);

	/*
	 * Now fill in the lfo specs for this operator. These are specific to an
	 * oscillator.
	 */
	specs->spec.param[0].pname = "Tune";
	specs->spec.param[0].description = "fine tuning of frequency";
	specs->spec.param[0].type = BRISTOL_FLOAT;
	specs->spec.param[0].low = 0;
	specs->spec.param[0].high = 1;
	specs->spec.param[0].flags = BRISTOL_ROTARY|BRISTOL_SLIDER;

	specs->spec.param[1].pname = "sync";
	specs->spec.param[1].description = "synchronised start";
	specs->spec.param[1].type = BRISTOL_INT;
	specs->spec.param[1].low = 0;
	specs->spec.param[1].high = 1;
	specs->spec.param[1].flags = BRISTOL_ROTARY|BRISTOL_SLIDER|BRISTOL_HIDE;

	specs->spec.param[2].pname = "velocity";
	specs->spec.param[2].description = "frequency is velocity sensitive";
	specs->spec.param[2].type = BRISTOL_FLOAT;
	specs->spec.param[2].low = 0;
	specs->spec.param[2].high = 1;
	specs->spec.param[2].flags = BRISTOL_ROTARY|BRISTOL_SLIDER|BRISTOL_HIDE;

	specs->spec.param[3].pname = "wheel";
	specs->spec.param[3].description = "frequency is mod wheel sensitive";
	specs->spec.param[3].type = BRISTOL_FLOAT;
	specs->spec.param[3].low = 0;
	specs->spec.param[3].high = 1;
	specs->spec.param[3].flags = BRISTOL_ROTARY|BRISTOL_SLIDER|BRISTOL_HIDE;

	specs->spec.param[4].pname = "Lower limit";
	specs->spec.param[4].description = "lower frequency range, def 0.1Hz";
	specs->spec.param[4].type = BRISTOL_FLOAT;
	specs->spec.param[4].low = 0;
	specs->spec.param[4].high = 1;
	specs->spec.param[4].flags = BRISTOL_ROTARY|BRISTOL_SLIDER|BRISTOL_HIDE;

	specs->spec.param[5].pname = "Upper limit";
	specs->spec.param[5].description = "upper frequency range, def 10Hz";
	specs->spec.param[5].type = BRISTOL_FLOAT;
	specs->spec.param[5].low = 0;
	specs->spec.param[5].high = 1;
	specs->spec.param[5].flags = BRISTOL_ROTARY|BRISTOL_SLIDER|BRISTOL_HIDE;

	/*
	 * Now fill in the lfo IO specs.
	 */
	specs->spec.io[0].ioname = "noise input";
	specs->spec.io[0].description = "input signal and hold";
	specs->spec.io[0].samplerate = samplerate;
	specs->spec.io[0].samplecount = samplecount;
	specs->spec.io[0].flags = BRISTOL_DC|BRISTOL_INPUT;

	specs->spec.io[1].ioname = "triangular output";
	specs->spec.io[1].description = "oscillator output signal";
	specs->spec.io[1].samplerate = samplerate;
	specs->spec.io[1].samplecount = samplecount;
	specs->spec.io[1].flags = BRISTOL_AC|BRISTOL_OUTPUT;

	specs->spec.io[2].ioname = "square output";
	specs->spec.io[2].description = "oscillator output signal";
	specs->spec.io[2].samplerate = samplerate;
	specs->spec.io[2].samplecount = samplecount;
	specs->spec.io[2].flags = BRISTOL_AC|BRISTOL_OUTPUT;

	specs->spec.io[3].ioname = "sample and hold output";
	specs->spec.io[3].description = "oscillator output signal";
	specs->spec.io[3].samplerate = samplerate;
	specs->spec.io[3].samplecount = samplecount;
	specs->spec.io[3].flags = BRISTOL_AC|BRISTOL_OUTPUT;

	specs->spec.io[4].ioname = "sine output";
	specs->spec.io[4].description = "oscillator sine output signal";
	specs->spec.io[4].samplerate = samplerate;
	specs->spec.io[4].samplecount = samplecount;
	specs->spec.io[4].flags = BRISTOL_AC|BRISTOL_OUTPUT;

	specs->spec.io[5].ioname = "sawtooth output";
	specs->spec.io[5].description = "oscillator sine output signal";
	specs->spec.io[5].samplerate = samplerate;
	specs->spec.io[5].samplecount = samplecount;
	specs->spec.io[5].flags = BRISTOL_AC|BRISTOL_OUTPUT;

	specs->spec.io[6].ioname = "ramp output";
	specs->spec.io[6].description = "oscillator sine output signal";
	specs->spec.io[6].samplerate = samplerate;
	specs->spec.io[6].samplecount = samplecount;
	specs->spec.io[6].flags = BRISTOL_AC|BRISTOL_OUTPUT;

	return(*operator);
}

#define BRISTOL_LFO 1

/*
 * Waves have a range of 24, which is basically two octaves. For larger 
 * differences will have to apply apms.
 */
static void
fillWave(float *mem, int count, int type)
{
	int i;

#ifdef BRISTOL_DBG
	printf("fillWave(%x, %i, %i)\n", mem, count, type);
#endif

	switch (type) {
		case 0:
			/*
			 * This will be a sine wave. We have count samples, and
			 * 2PI radians in a full sine wave. Thus we take
			 * 		(2PI * i / count) * 2048.
			 */
			for (i = 0;i < count; i++)
				mem[i] = sin(2 * M_PI * ((double) i) / count) * BRISTOL_LFO;
			return;
		case 1:
		default:
			/* 
			 * This is a square wave.
			 */
			for (i = 0;i < count / 2; i++)
				mem[i] = BRISTOL_LFO * 2.0 / 3.0;
			for (;i < count; i++)
				mem[i] = -BRISTOL_LFO * 2.0 / 3.0;
			return;
		case 2:
			/* 
			 * This is a pulse wave (we do not have PWM yet).
			 */
			for (i = 0;i < count / 3; i++)
				mem[i] = BRISTOL_LFO * 2.0 / 3.0;
			for (;i < count; i++)
				mem[i] = -BRISTOL_LFO * 2.0 / 3.0;
			return;
		case 3:
			/* 
			 * This is a ramp wave. We scale the index from -.5 to .5, and
			 * multiply by the range. We go from rear to front to table to make
			 * the ramp wave have a positive leading edge.
			 */
			for (i = count - 1;i >= 0; i--)
				mem[i] = (((float) i / count) - 0.5) * BRISTOL_LFO * 2.0;
			return;
		case 4:
			/*
			 * Triangular wave. From MIN point, ramp up at twice the rate of
			 * the ramp wave, then ramp down at same rate.
			 */
			for (i = 0;i < count / 2; i++)
				mem[i] = -BRISTOL_LFO
					+ ((float) i * 2 / (count / 2)) * BRISTOL_LFO; 
			for (;i < count; i++)
				mem[i] = BRISTOL_LFO -
					(((float) (i - count / 2) * 2) / (count / 2)) * BRISTOL_LFO;
			return;
		case 5:
			/*
			 * Would like to put in a jagged edged ramp wave. Should make some
			 * interesting strings sounds.
			 */
			{
				float accum = -BRISTOL_LFO;

				for (i = 0; i < count / 2; i++)
				{
					mem[i] = accum +
						(0.5 - (((float) i) / count)) * BRISTOL_LFO * 4;
					if (i == count / 8)
						accum = -BRISTOL_LFO * 3 / 4;
					if (i == count / 4)
						accum = -BRISTOL_LFO / 2;
					if (i == count * 3 / 8)
						accum = -BRISTOL_LFO / 4;
				}
				for (; i < count; i++)
					mem[i] = -mem[count - i];
			}
			return;
		case 6:
			/*
			 * Tangiential wave. We limit some of the values, since they do get
			 * excessive. This is only half a tan as well, to maintain the
			 * base frequency.
			 */
			for (i = 0;i < count; i++)
			{
				if ((mem[i] =
					tan(M_PI * ((double) i) / count) * BRISTOL_LFO / 16)
					> BRISTOL_LFO * 8)
					mem[i] = BRISTOL_LFO * 6;
				if (mem[i] < -(BRISTOL_LFO * 6))
					mem[i] = -BRISTOL_LFO * 6;
			}
			return;
		case 7:
			/* 
			 * This is a downramp wave. We scale the index from .5 to -.5, and
			 * multiply by the range. We go from rear to front to table to make
			for (i = count - 1;i >= 0; i--)
				mem[i] = (((float) i / count) - 0.5) * BRISTOL_LFO * 2.0;
			 */
			for (i = 0;i < count; i++)
				mem[i] = (0.5 - ((float) i / count)) * BRISTOL_LFO * 2.0;
			return;
	}
}

