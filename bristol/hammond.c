
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
 *	hammondinit()
 *	operate()
 *	reset()
 *	destroy()
 *
 *	destroy() is in the library.
 *
 * Operate will be called when all the inputs have been loaded, and the result
 * will be an output buffer written to the next operator.
 */

#include <stdlib.h>
#include <math.h>

#include "click.h"

int clickset[128];

#include "bristol.h"
#include "hammond.h"

float note_diff;
int samplecount;

static void fillWave(float *, int, int);
static void buildHammondSound(bristolOP *, unsigned char);
static void fillHammondWave(bristolOP *);

/*
 * Use of these mean that we can only product a single hammond oscillator.
 * Even though it is reasonably flexible this does lead to problems, and when
 * we move to a dual manual Hammond it will need to be resolved.
 *
 * The same is true of using single global result waves, wave[6] and wave[7].
 * These need to be moved to private address space, ie, we need more 
 * instantiation.
static int sineform[16] = {1, 1, 1, 1, 1, 1, 1, 1, 1};
static int wavelevel[16];
static int waveindex[16] = {264, 23, 268, 491, 523, 708, 354, 112, 661};
static int percussion[16] = {0, 0, 0, 0, 0, 0, 0, 0, 0};
 */
static int *sineform;
static int *wavelevel;
static int *waveindex;
static int *percussion;

float *wave1;
float *wave2;

/*
 * This can be a single list, it is used to generate the different pipes.
static int sweeps[16] = {1, 2, 3, 4, 6, 8, 10, 12, 16, 0,0,0,0,0,0,0};
static float sweeps[16] = {1, 1.5, 2, 4, 6, 8, 10, 12, 16, 0,0,0,0,0,0,0};
 */
static float sweeps[16] = {2, 6, 4, 8, 12, 16, 20, 24, 32, 0,0,0,0,0,0,0};

/*
 * The name of this operator, IO count, and IO names.
 */
#define OPNAME "Hammond"
#define OPDESCRIPTION "Digitally Controlled Oscillator"
#define PCOUNT 8
#define IOCOUNT 3

#define HAMMOND_IN_IND 0
#define HAMMOND_OUT_IND 1
#define HAMMOND_OUT2_IND 2

/*
 * Reset any local memory information.
 */
static int destroy(bristolOP *operator)
{
#ifdef BRISTOL_DBG
	printf("hammonddestroy(%x)\n", operator);
#endif

	/*
	 * Unmalloc anything we added to this structure
	 */
	bristolfree(((bristolHAMMOND *) operator)->wave[0]);
	bristolfree(((bristolHAMMOND *) operator)->wave[1]);
	bristolfree(((bristolHAMMOND *) operator)->wave[2]);
	bristolfree(((bristolHAMMOND *) operator)->wave[3]);
	bristolfree(((bristolHAMMOND *) operator)->wave[4]);
	bristolfree(((bristolHAMMOND *) operator)->wave[5]);
	bristolfree(((bristolHAMMOND *) operator)->wave[6]);
	bristolfree(((bristolHAMMOND *) operator)->wave[7]);

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
	printf("hammondreset(%x, %x)\n", operator, param);
#endif

	if (param->param[0].mem) bristolfree(param->param[0].mem);
	if (param->param[1].mem) bristolfree(param->param[1].mem);
	if (param->param[2].mem) bristolfree(param->param[2].mem);
	if (param->param[3].mem) bristolfree(param->param[3].mem);
	if (param->param[4].mem) bristolfree(param->param[4].mem);
	if (param->param[5].mem) bristolfree(param->param[5].mem);

	/*
	 * For the hammond we want to put in a few local memory variables for this
	 * operator specifically.
	 *
	 * These are sineform, waveindex, wavelevel, and percussions, plus two
	 * unique wavetables.
	 */
	param->param[0].mem = bristolmalloc0(sizeof(int) * 16);
	param->param[1].mem = bristolmalloc0(sizeof(float) * HAMMOND_WAVE_SZE);
	param->param[2].mem = bristolmalloc0(sizeof(int) * 16);
	param->param[3].mem = bristolmalloc0(sizeof(int) * 16);
	param->param[4].mem = bristolmalloc0(sizeof(float) * HAMMOND_WAVE_SZE);
	param->param[5].mem = bristolmalloc0(sizeof(int) * 16);
	param->param[7].int_val = 1;
	param->param[7].int_val = 0;
	param->param[4].float_val = 0.166880;
	return(0);
}

/*
 * Alter an internal parameter of an operator.
 */
static int param(bristolOP *operator, bristolOPParams *param,
	unsigned char index, float value)
{
	int parm = value * CONTROLLER_RANGE, controller;

	sineform = (int *) param->param[0].mem;
	wavelevel = (int *) param->param[2].mem;
	percussion = (int *) param->param[3].mem;
	waveindex = (int *) param->param[5].mem;

	wave1 = (float *) param->param[1].mem;
	wave2 = (float *) param->param[4].mem;

#ifdef BRISTOL_DBG
	printf("hammondparam(%x, %x, %i, %x)\n", operator, param, index,
		((int) value * CONTROLLER_RANGE));
#endif

	switch (index) {
		case 0:
			if ((controller = parm >> 3) > 16)
				return(0);
			/*
			 * Mask out the level of this wave.
			((int *) *param->param[0].mem)[controller] = parm & 0x07;
			 */
			sineform[controller] = parm & 0x07;

			buildHammondSound(operator, -1);
			break;
		case 1: /* Tune in "small" amounts - not used - using for damp */
			/*
			 * Want to use this for damping.
			 */
			param->param[index].float_val = value;
			break;
		case 2: /* harmonics - redo waveform? */
			/*
			 * We have a param which is XCCCCLLL, for controller (up to 9 waves)
			 * and LLL is level.
			 */
			buildHammondSound(operator, (int) (value * CONTROLLER_RANGE));

			break;
		case 3: /* Percussion on a given wave */
			if ((controller = parm >> 3) > 16)
				return(0);
			/*
			 * Mask out the level of this wave.
			 */
			percussion[controller] = parm & 0x07;
			buildHammondSound(operator, -1);
			break;
		case 4: /* gain */
			param->param[index].float_val = value;
			break;
		case 5: /* rand */
			if ((controller = parm >> 10) > 16)
				return(0);

			if ((waveindex[controller] = parm & 0x3ff) == HAMMOND_WAVE_SZE)
				waveindex[controller] -= 1;
			buildHammondSound(operator, -1);
			break;
		case 6: /* click */
			param->param[index].float_val = value * value * 0.25;
			break;
		case 7: /* Algo */
			param->param[index].int_val = value * CONTROLLER_RANGE;

			if (param->param[index].int_val)
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
 *	gain.
 *
 * With optimisations this is reduced to a nominal amount. Put most parameters
 * in registers, and then stretched the for loop to multiples of 16 samples.
 */
extern int preach(float *, float *, int, int *, int *, int, float, float, float, int, float);

static int operate(bristolOP *operator,
	bristolVoice *voice,
	bristolOPParams *param,
	void *lcl)
{
	register bristolHAMMONDlocal *local = lcl;
	register int obp, count;
	register float *ib, *ob, *pb, *wt, *pt, wtp, gain, clicklev, damp;
	bristolHAMMOND *specs;

	specs = (bristolHAMMOND *) operator->specs;

#ifdef BRISTOL_DBG
	printf("hammond(%x, %x, %x)\n", operator, param, local);
#endif

	count = specs->spec.io[HAMMOND_OUT_IND].samplecount;
	ib = specs->spec.io[HAMMOND_IN_IND].buf;
	ob = specs->spec.io[HAMMOND_OUT_IND].buf;
	pb = specs->spec.io[HAMMOND_OUT2_IND].buf;
	damp = param->param[1].float_val;
	wt = (float *) param->param[1].mem;
	pt = (float *) param->param[4].mem;

	/*
	 * Control the gain under the foot pedal.
	 */
	gain = param->param[4].float_val * voice->baudio->contcontroller[4];

	wtp = local->wtp;

	/*
	 * Go jumping through the wavetable, with each jump defined by the value
	 * given on our input line.
	 *
	 * Make sure we fill one output buffer.
	 *
	 * The '-36' may look odd, but the tonewheel generator does not use MIDI
	 * note indeces and this puts the key.key back into the gearing numbers.
	 */
	if (param->param[7].int_val) {
		preach(ob, pb, voice->key.key - 36, param->param[2].mem,
			param->param[3].mem, count, gain, damp, param->param[6].float_val,
			voice->flags, voice->velocity);
	} else {

		/*
		 * Click may be in multiple stages - each click array is 2k samples,
		 * so if our sample block size is less than this we need to hit
		 * multiple consequetive blocks.
		 *
		 * We should consider randomising the use of several different key
		 * clicks - this is however only done in the preacher.
		 */
		if (voice->flags & (BRISTOL_KEYON|BRISTOL_KEYREON))
		{
			wtp = 0.0;
			clickset[voice->key.key] = 0;
		}

		if (clickset[voice->key.key] < (CLICK_SIZE - count))
		{
			register int i;
			register short *thisclick;

			thisclick = &click[clickset[voice->key.key]];

			if ((clicklev = param->param[6].float_val * gain) != 0)
			{
				for (i = 0; i < count; i++)
					ob[i] += ((float) thisclick[i]) * clicklev;
			}
			clickset[voice->key.key] += count;
		}

		for (obp = 0; obp < count;)
		{
			/*
			 * Take a sample from the wavetable into the output buffer. This 
			 * should also be scaled by gain parameter.
			ob[obp] += wt[(int) wtp] * gain;
			pb[obp] += pt[(int) wtp] * gain;
			 */
			if (wtp == HAMMOND_WAVE_SZE)
			{
				ob[obp] += (wt[0] * (wtp - ((float) ((int) wtp)))
					+ wt[(int) wtp] * (1.0 - (wtp - ((float) ((int) wtp)))))
					* gain;
				pb[obp] += (pt[0] * (wtp - ((float) ((int) wtp)))
					+ pt[(int) wtp] * (1.0 - (wtp - ((float) ((int) wtp)))))
					* gain;
			} else  {
				ob[obp] += (wt[(int) wtp + 1] * (wtp - ((float) ((int) wtp)))
					+ wt[(int) wtp] * (1.0 - (wtp - ((float) ((int) wtp)))))
					* gain;
				pb[obp] += (pt[(int) wtp + 1] * (wtp - ((float) ((int) wtp)))
					+ pt[(int) wtp] * (1.0 - (wtp - ((float) ((int) wtp)))))
					* gain;
			}
			/*
			 * Move the wavetable pointer forward by amount indicated in input 
			 * buffer for this sample.
			 */
			if ((wtp += ib[obp++] * 0.25) >= HAMMOND_WAVE_SZE)
				wtp -= HAMMOND_WAVE_SZE;
			if (wtp < 0) wtp += HAMMOND_WAVE_SZE;
		}
	}

	local->wtp = wtp;
	return(0);
}

/*
 * Setup any variables in our OP structure, in our IO structures, and malloc
 * any memory we need.
 */
bristolOP *
hammondinit(bristolOP **operator, int index, int samplerate, int samplecount)
{
	bristolHAMMOND *specs;

#ifdef BRISTOL_DBG
	printf("hammondinit(%x(%x), %i, %i, %i)\n",
		operator, *operator, index, samplerate, samplecount);
#endif

	*operator = bristolOPinit(operator, index, samplecount);
	note_diff = pow(2, ((float) 1)/12);;

	/*
	 * Then the local parameters specific to this operator. These will be
	 * the same for each operator, but must be inited in the local code.
	 */
	(*operator)->operate = operate;
	(*operator)->destroy = destroy;
	(*operator)->reset = reset;
	(*operator)->param = param;

	specs = (bristolHAMMOND *) bristolmalloc0(sizeof(bristolHAMMOND));
	(*operator)->specs = (bristolOPSpec *) specs;
	(*operator)->size = sizeof(bristolHAMMOND);

	/*
	 * These are specific to this operator, and will need to be altered for
	 * each operator.
	 */
	specs->spec.opname = OPNAME;
	specs->spec.description = OPDESCRIPTION;
	specs->spec.pcount = PCOUNT;
	specs->spec.iocount = IOCOUNT;
	specs->spec.localsize = sizeof(bristolHAMMONDlocal);

	/*
	 * We are going to assign multiple waves to this oscillator.
	 * sine, ramp, square, triangle?
	 */
	specs->wave[0] = (float *) bristolmalloc(HAMMOND_WAVE_SZE * sizeof(float));
	specs->wave[1] = (float *) bristolmalloc(HAMMOND_WAVE_SZE * sizeof(float));
	specs->wave[2] = (float *) bristolmalloc(HAMMOND_WAVE_SZE * sizeof(float));
	specs->wave[3] = (float *) bristolmalloc(HAMMOND_WAVE_SZE * sizeof(float));
	specs->wave[4] = (float *) bristolmalloc(HAMMOND_WAVE_SZE * sizeof(float));
	specs->wave[5] = (float *) bristolmalloc(HAMMOND_WAVE_SZE * sizeof(float));
	specs->wave[6] = (float *) bristolmalloc(HAMMOND_WAVE_SZE * sizeof(float));
	specs->wave[7] = (float *) bristolmalloc(HAMMOND_WAVE_SZE * sizeof(float));

	/*
	 * FillWave is something that should be called as a parameter change, but
	 * for testing will load it here.
	 */
	fillWave(specs->wave[0], HAMMOND_WAVE_SZE, 0);
	fillWave(specs->wave[1], HAMMOND_WAVE_SZE, 1);
	fillWave(specs->wave[2], HAMMOND_WAVE_SZE, 2);
	fillWave(specs->wave[3], HAMMOND_WAVE_SZE, 3);
	fillWave(specs->wave[4], HAMMOND_WAVE_SZE, 4);
	fillWave(specs->wave[5], HAMMOND_WAVE_SZE, 5);
	fillWave(specs->wave[6], HAMMOND_WAVE_SZE, 6);
	fillWave(specs->wave[7], HAMMOND_WAVE_SZE, 7);

	/*
	 * Now fill in the hammond specs for this operator. These are specific to an
	 * oscillator.
	 */
	specs->spec.param[0].pname = "Sineform";
	specs->spec.param[0].description = "Pulse code of sinewave";
	specs->spec.param[0].type = BRISTOL_INT;
	specs->spec.param[0].low = 0;
	specs->spec.param[0].high = 1;
	specs->spec.param[0].flags = BRISTOL_ROTARY|BRISTOL_SLIDER;

	specs->spec.param[1].pname = "damp";
	specs->spec.param[1].description = "tonewheel damping";
	specs->spec.param[1].type = BRISTOL_FLOAT;
	specs->spec.param[1].low = 0;
	specs->spec.param[1].high = 1;
	specs->spec.param[1].flags = BRISTOL_ROTARY|BRISTOL_SLIDER;

	specs->spec.param[2].pname = "oscillator";
	specs->spec.param[2].description = "Harmonic content definition";
	specs->spec.param[2].type = BRISTOL_INT;
	specs->spec.param[2].low = 0;
	specs->spec.param[2].high = 127;
	specs->spec.param[2].flags = BRISTOL_ROTARY|BRISTOL_SLIDER;

	specs->spec.param[3].pname = "percussion";
	specs->spec.param[3].description = "apply percussion to wave";
	specs->spec.param[3].type = BRISTOL_FLOAT;
	specs->spec.param[3].low = 0;
	specs->spec.param[3].high = 2;
	specs->spec.param[3].flags = BRISTOL_ROTARY|BRISTOL_SLIDER;

	specs->spec.param[4].pname = "gain";
	specs->spec.param[4].description = "output gain on signal";
	specs->spec.param[4].type = BRISTOL_FLOAT;
	specs->spec.param[4].low = 0;
	specs->spec.param[4].high = 2;
	specs->spec.param[4].flags = BRISTOL_ROTARY|BRISTOL_SLIDER;

	specs->spec.param[5].pname = "rand";
	specs->spec.param[5].description = "phase randomisation";
	specs->spec.param[5].type = BRISTOL_FLOAT;
	specs->spec.param[5].low = 0;
	specs->spec.param[5].high = 1;
	specs->spec.param[5].flags = BRISTOL_ROTARY|BRISTOL_SLIDER|BRISTOL_HIDE;

	specs->spec.param[6].pname = "click";
	specs->spec.param[6].description = "Key click";
	specs->spec.param[6].type = BRISTOL_FLOAT;
	specs->spec.param[6].low = 0;
	specs->spec.param[6].high = 1;
	specs->spec.param[6].flags = BRISTOL_ROTARY|BRISTOL_SLIDER|BRISTOL_HIDE;

	specs->spec.param[7].pname = "algo";
	specs->spec.param[7].description = "Sine generation algorithm";
	specs->spec.param[7].type = BRISTOL_INT;
	specs->spec.param[7].low = 0;
	specs->spec.param[7].high = 1;
	specs->spec.param[7].flags = BRISTOL_BUTTON;

	/*
	 * Now fill in the hammond IO specs.
	 */
	specs->spec.io[0].ioname = "input";
	specs->spec.io[0].description = "input rate modulation signal";
	specs->spec.io[0].samplerate = samplerate;
	specs->spec.io[0].samplecount = samplecount;
	specs->spec.io[0].flags = BRISTOL_DC|BRISTOL_INPUT;

	specs->spec.io[1].ioname = "output";
	specs->spec.io[1].description = "oscillator output signal";
	specs->spec.io[1].samplerate = samplerate;
	specs->spec.io[1].samplecount = samplecount;
	specs->spec.io[1].flags = BRISTOL_AC|BRISTOL_OUTPUT;

	specs->spec.io[2].ioname = "percussive";
	specs->spec.io[2].description = "percussive signal output";
	specs->spec.io[2].samplerate = samplerate;
	specs->spec.io[2].samplecount = samplecount;
	specs->spec.io[2].flags = BRISTOL_AC|BRISTOL_OUTPUT;

	return(*operator);
}

static void
filltriwave(register float *mem, register int count,
register double reach)
{
	register int i, recalc1 = 1, recalc2 = 1;
	register double j = 0, Count = (double) count, inc = reach;

	for (i = 0;i < count; i++)
	{
		mem[i] = sin(((double) (2 * M_PI * j)) / Count) * HAMMOND_WAVE_GAIN;

		if ((j > (count * 3 / 4)) && recalc2)
		{
			recalc2 = 0;
			inc = ((float) (count / 4)) / (((float) count) - i);
		} else if ((j > (count / 4)) && recalc1) {
			recalc1 = 0;
			/* 
			 * J has the sine on a peak, we need to calculate the number
			 * of steps required to get the sine back to zero crossing by
			 * the time i = count / 2, ie, count /4 steps in total.
			 */
			inc = ((float) (count / 4)) / (((float) (count / 2)) - i);
		}

		j += inc;
	}

/*
	printf("\n%f, %f\n", j, inc);
	for (i = 0; i < count; i+=8)
	{
		printf("\n%i: ", i + k);
		for (k = 0; k < 8; k++)
			printf("%03.1f, ", mem[i + k]);
	}
	printf("\n");
*/
}

static void
fillWave(register float *mem, register int count, int type)
{
	register double Count;

#ifdef BRISTOL_DBG
	printf("fillWave(%x, %i, %i)\n", mem, count, type);
#endif

	Count = (double) count;

	switch (type) {
		case 0:
			/*
			 * This will be a sine wave. We have count samples, and
			 * 2PI radians in a full sine wave. Thus we take
			 * 		(2PI * i / count) * 2048.
			 */
			filltriwave(mem, count, 1.0);
		case 1:
		default:
			filltriwave(mem, count, 1.05);
			return;
		case 2:
			filltriwave(mem, count, 1.10);
			return;
		case 3:
			filltriwave(mem, count, 1.30);
			return;
		case 4:
			filltriwave(mem, count, 1.60);
			return;
		case 5:
			filltriwave(mem, count, 2.0);
			return;
		case 6:
			filltriwave(mem, count, 3.0);
			return;
		case 7:
			filltriwave(mem, count, 5.0);
			return;
	}
}

static void
buildHammondSound(bristolOP *operator, unsigned char parm)
{
	int controller, value;

#ifdef BRISTOL_DBG
	printf("buildHammondSound(%x, %i, %i, %i)\n", operator, parm, parm >> 3, parm & 0x07);
#endif

	if (parm != 0xff)
	{
		if ((controller = parm / 9) > 8)
			return;

		value = parm - controller * 9;

		/*
		 * Mask out the level of this wave. Note that if we are running the
		 * preacher we could exit here, however, can we know that?
		 */
		wavelevel[controller] = value;
	}

	fillHammondWave(operator);
}

static void
fillHammondWave(bristolOP *operator)
{
	register float gain, *source, *dest, oscindex, sweep;
	register int i, j;
	bristolHAMMOND *specs;

	specs = (bristolHAMMOND *) operator->specs;

#ifdef BRISTOL_DBG
	printf("fillHammondWave(%x)\n", operator);
#endif

	/*
	 * Dest is actually the local wave for this operator.
	 */
	bristolbzero(wave1, HAMMOND_WAVE_SZE * sizeof(float));
	bristolbzero(wave2, HAMMOND_WAVE_SZE * sizeof(float));

	/*
	 * We now have a volume array - 16 levels, of which we should only use 9.
	 * Each represents a base requency in terms of pipe length:
	 *	16, 8, 5 1/3, 4, 2 2/3, 2, 1 3/5, 1 1/3, 1
	 * These are frequencies:

		f
		2f
		3f
		4f
		6f
		8f
		10f
		12f
		16f

printf("%4i %4i %4i %4i %4i %4i %4i %4i %4i\n",
wavelevel[0], wavelevel[1], wavelevel[2], wavelevel[3], wavelevel[4],
wavelevel[5], wavelevel[6], wavelevel[7], wavelevel[8]);
printf("%4i %4i %4i %4i %4i %4i %4i %4i %4i\n",
sineform[0], sineform[1], sineform[2], sineform[3], sineform[4],
sineform[5], sineform[6], sineform[7], sineform[8]);
printf("%4i %4i %4i %4i %4i %4i %4i %4i %4i\n",
waveindex[0], waveindex[1], waveindex[2], waveindex[3], waveindex[4],
waveindex[5], waveindex[6], waveindex[7], waveindex[8]);
printf("%4i %4i %4i %4i %4i %4i %4i %4i %4i\n",
percussion[0], percussion[1], percussion[2], percussion[3], percussion[4],
percussion[5], percussion[6], percussion[7], percussion[8]);
	 */
	/*
	 * This could be optimised for a single sweep through each sample, but
	 * would require a lot of parameters.
	 */
	for (i = 0; i < 9; i++)
	{
		source = specs->wave[sineform[i]];

		if (percussion[i] == 1)
			/*
		if ((i > 1) && (i < 6))
			 * Percussives.
			 */
			dest = wave2;
		else
			dest = wave1;

		/*
		 * This ensures we do not have direct phase relationships.
		while ((oscindex = rand() & 0x03ff) == HAMMOND_WAVE_SZE);
		 */
		oscindex = waveindex[i];

		gain = ((float) wavelevel[i]) / 8.0;

		if (gain == 0)
			continue;

		sweep = sweeps[i];

		for (j = 0; j < HAMMOND_WAVE_SZE; j++)
		{
			*dest++ += source[(int) oscindex] * gain;
			if ((oscindex += sweep) >= HAMMOND_WAVE_SZE)
				oscindex -= HAMMOND_WAVE_SZE;
		}
	}
}

