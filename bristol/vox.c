
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
 *	voxinit()
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

#include "bristol.h"
#include "bristolblo.h"
#include "vox.h"

float note_diff;
int samplecount;

static void fillWave(float *, int, int);
static void buildVoxSound(bristolOP *, bristolOPParams *, unsigned char);
static void fillVoxWave(bristolOP *, int);
static void fillVoxM2Wave(bristolOP *, bristolOPParams *);

/*
 * Use of these mean that we can only product a single vox oscillator.
 * Even though it is reasonably flexible this does lead to problems, and if
 * we move to a dual manual Vox it will need to be resolved.
 *
 * The same is true of using single global result waves, wave[6] and wave[7].
 * These need to be moved to private address space, ie, we need more 
 * instantiation.
 */
static int *wavelevel;

float *wave1;
float *wave2;

/*
 * This can be a single list, it is used to generate the different pipes.
 *
 * We have 16, 8, 4, 2, 1
{16', 8', 4', 5-1/3', 2-2/3', 2', 1-1/3', 1-1/5', 1', 0,0,0,0,0,0,0};
{ 2,  4,  8,      6,     12, 16,     20,     24, 32, 0,0,0,0,0,0,0};
 */
static float sweeps[16] = {2, 4, 8, 6, 12, 16, 20, 24, 32, 0,0,0,0,0,0,0};

/*
 * The name of this operator, IO count, and IO names.
 */
#define OPNAME "Vox"
#define OPDESCRIPTION "Digitally Controlled Oscillator"
#define PCOUNT 3
#define IOCOUNT 2

#define VOX_IN_IND 0
#define VOX_OUT_IND 1
#define VOX_PERC_IND 2

/*
 * Reset any local memory information.
 */
static int destroy(bristolOP *operator)
{
#ifdef BRISTOL_DBG
	printf("voxdestroy(%x)\n", operator);
#endif

	/*
	 * Unmalloc anything we added to this structure
	 */
	bristolfree(((bristolVOX *) operator)->wave[0]);
	bristolfree(((bristolVOX *) operator)->wave[1]);
	bristolfree(((bristolVOX *) operator)->wave[2]);
	bristolfree(((bristolVOX *) operator)->wave[3]);
	bristolfree(((bristolVOX *) operator)->wave[4]);
	bristolfree(((bristolVOX *) operator)->wave[5]);
	bristolfree(((bristolVOX *) operator)->wave[6]);
	bristolfree(((bristolVOX *) operator)->wave[7]);

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
	printf("voxreset(%x, %x)\n", operator, param);
#endif

	/*
	 * For the vox we want to put in a few local memory variables for this
	 * operator specifically.
	 *
	 * These are sineform, waveindex, wavelevel, and percussions, plus two
	 * unique wavetables.
	 */
	param->param[0].mem = bristolmalloc0(sizeof(int) * 16);
	param->param[1].mem = bristolmalloc0(sizeof(float) * VOX_WAVE_SZE);
	param->param[2].mem = bristolmalloc0(sizeof(int) * 16);
	param->param[3].mem = bristolmalloc0(sizeof(int) * 16);
	param->param[4].mem = bristolmalloc0(sizeof(float) * VOX_WAVE_SZE);
	param->param[5].mem = bristolmalloc0(sizeof(int) * 16);
	param->param[6].mem = bristolmalloc0(sizeof(float) * VOX_WAVE_SZE);
	param->param[7].int_val = 1;
	param->param[7].int_val = 0;
	param->param[4].float_val = 0.166880;
	param->param[1].float_val = 1;
	param->param[5].int_val = 0;
	return(0);
}

/*
 * Alter an internal parameter of an operator.
 */
static int param(bristolOP *operator, bristolOPParams *param,
	unsigned char index, float value)
{
	wavelevel = (int *) param->param[2].mem;

	wave1 = (float *) param->param[1].mem;
	wave2 = (float *) param->param[6].mem;

#ifdef BRISTOL_DBG
	printf("	voxparam(%x, %x, %i, %x)\n", operator, param, index,
		((int) (value * CONTROLLER_RANGE)));
#endif

	switch (index) {
		case 0:
			param->param[index].float_val = value;
			buildVoxSound(operator, param, (int) (value * CONTROLLER_RANGE));
			break;
		case 1: /* gain */
			param->param[index].float_val = value;
			break;
		case 2: /* First percussive */
		case 3: /* Second percussive */
			if (value == 0.0)
				param->param[index].int_val = 0;
			else
				param->param[index].int_val = 1;
			break;
		case 4: /* Mark-1 or Mark-II oscillator */
			if (value == 0.0)
				param->param[index].int_val = 0;
			else
				param->param[index].int_val = 1;
			break;
		case 5:
			/* Wavefore - 0=infinite, X=deformed */
			param->param[index].int_val =  (int) (value * CONTROLLER_RANGE);;
			buildVoxSound(operator, param,
				(int) (param->param[0].float_val * CONTROLLER_RANGE));
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
static int operate(bristolOP *operator,
	bristolVoice *voice,
	bristolOPParams *param,
	void *lcl)
{
	register bristolVOXlocal *local = lcl;
	register int obp = 0, count;
	register float *ib, *ob, *pb, *pt, *wt, wtp, gain;
	bristolVOX *specs;

	specs = (bristolVOX *) operator->specs;

#ifdef BRISTOL_DBG
	printf("vox(%x, %x, %x)\n", operator, param, local);
#endif

	count = specs->spec.io[VOX_OUT_IND].samplecount;
	ib = specs->spec.io[VOX_IN_IND].buf;
	ob = specs->spec.io[VOX_OUT_IND].buf;
	pb = specs->spec.io[VOX_PERC_IND].buf;
	wt = (float *) param->param[1].mem;
	pt = (float *) param->param[6].mem;
	gain = param->param[1].float_val;
	wtp = local->wtp;

	if (voice->flags & BRISTOL_KEYON)
	{
		wtp = 0;
		if ((~voice->flags & BRISTOL_KEYREON) &&
			(voice->offset > 0) && (voice->offset < count))
		{
			if (voice->baudio->midiflags & BRISTOL_MIDI_DEBUG1)
				printf("envelope trigger offset %i frames\n", voice->offset);

			//count += voice->offset;
			obp += voice->offset;
		}
	}

	for (; obp < count;)
	{
		/*
		 * Take a sample from the wavetable into the output buffer. This 
		 * should also be scaled by gain parameter.
		 */
		if (wtp >= VOX_WAVE_SZE_M) {
			ob[obp] += (wt[0] * (wtp - ((float) ((int) wtp)))
				+ wt[(int) wtp] * (1.0 - (wtp - ((float) ((int) wtp)))))
				* gain;
			pb[obp] += (pt[0] * (wtp - ((float) ((int) wtp)))
				+ pt[(int) wtp] * (1.0 - (wtp - ((float) ((int) wtp)))))
				* gain;
		} else {
			ob[obp] += (wt[(int) wtp + 1] * (wtp - ((float) ((int) wtp)))
				+ wt[(int) wtp] * (1.0 - (wtp - ((float) ((int) wtp)))))
				* gain;
			pb[obp] += (pt[(int) wtp + 1] * (wtp - ((float) ((int) wtp)))
				+ pt[(int) wtp] * (1.0 - (wtp - ((float) ((int) wtp)))))
				* gain;
		}

		if ((wtp += ib[obp++] * 0.25) >= (float) VOX_WAVE_SZE)
			wtp -= VOX_WAVE_SZE;
	}

	local->wtp = wtp;
	return(0);
}

/*
 * Setup any variables in our OP structure, in our IO structures, and malloc
 * any memory we need.
 */
bristolOP *
voxdcoinit(bristolOP **operator, int index, int samplerate, int samplecount)
{
	bristolVOX *specs;

#ifdef BRISTOL_DBG
	printf("voxinit(%x(%x), %i, %i, %i)\n",
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

	specs = (bristolVOX *) bristolmalloc0(sizeof(bristolVOX));
	(*operator)->specs = (bristolOPSpec *) specs;
	(*operator)->size = sizeof(bristolVOX);

	/*
	 * These are specific to this operator, and will need to be altered for
	 * each operator.
	 */
	specs->spec.opname = OPNAME;
	specs->spec.description = OPDESCRIPTION;
	specs->spec.pcount = PCOUNT;
	specs->spec.iocount = IOCOUNT;
	specs->spec.localsize = sizeof(bristolVOXlocal);

	/*
	 * We are going to assign multiple waves to this oscillator.
	 * sine, ramp, square, triangle?
	 */
	specs->wave[0] = (float *) bristolmalloc(VOX_WAVE_SZE * sizeof(float));
	specs->wave[1] = (float *) bristolmalloc(VOX_WAVE_SZE * sizeof(float));
	specs->wave[2] = (float *) bristolmalloc(VOX_WAVE_SZE * sizeof(float));
	specs->wave[3] = (float *) bristolmalloc(VOX_WAVE_SZE * sizeof(float));
	specs->wave[4] = (float *) bristolmalloc(VOX_WAVE_SZE * sizeof(float));
	specs->wave[5] = (float *) bristolmalloc(VOX_WAVE_SZE * sizeof(float));
	specs->wave[6] = (float *) bristolmalloc(VOX_WAVE_SZE * sizeof(float));
	specs->wave[7] = (float *) bristolmalloc(VOX_WAVE_SZE * sizeof(float));

	/*
	 * FillWave is something that should be called as a parameter change, but
	 * for testing will load it here.
	 */
	fillWave(specs->wave[0], VOX_WAVE_SZE, 0);
	fillWave(specs->wave[1], VOX_WAVE_SZE, 1);
	fillWave(specs->wave[2], VOX_WAVE_SZE, 2);
	fillWave(specs->wave[3], VOX_WAVE_SZE, 3);
	fillWave(specs->wave[4], VOX_WAVE_SZE, 4);
	fillWave(specs->wave[5], VOX_WAVE_SZE, 5);
	fillWave(specs->wave[6], VOX_WAVE_SZE, 6);
	fillWave(specs->wave[7], VOX_WAVE_SZE, 7);

	/*
	 * Now fill in the vox specs for this operator. These are specific to an
	 * oscillator.
	 */
	specs->spec.param[0].pname = "Sineform";
	specs->spec.param[0].description = "Pulse code of sinewave";
	specs->spec.param[0].type = BRISTOL_INT;
	specs->spec.param[0].low = 0;
	specs->spec.param[0].high = 1;
	specs->spec.param[0].flags = BRISTOL_ROTARY|BRISTOL_SLIDER;

	specs->spec.param[1].pname = "Gain";
	specs->spec.param[1].description = "Pitch correction factor";
	specs->spec.param[1].type = BRISTOL_FLOAT;
	specs->spec.param[1].low = 0;
	specs->spec.param[1].high = 1;
	specs->spec.param[1].flags = BRISTOL_ROTARY|BRISTOL_SLIDER;

	/*
	 * Now fill in the vox IO specs.
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

	specs->spec.io[2].ioname = "percussive output";
	specs->spec.io[2].description = "output of signals for percussive process";
	specs->spec.io[2].samplerate = samplerate;
	specs->spec.io[2].samplecount = samplecount;
	specs->spec.io[2].flags = BRISTOL_AC|BRISTOL_OUTPUT;

	return(*operator);
}

/*
static void filltriwave(register float *mem, register int count,
register double reach)
{
	register int i;

	if (bloflags & BRISTOL_BLO)
	{
		for (i = 0;i < count; i++)
			mem[i] = blotriangle[i];
		return;
	}
}
*/

/*
 * Hm, just in case you could not tell, this is a phase distortion oscillator.
 * It is somewhat warmer than the original vox.
 */
static void fillpdwave(register float *mem, register int count,
register double reach)
{
	register int i, recalc1 = 1, recalc2 = 1;
	register double j = 0, Count = (double) count, inc = reach;

	for (i = 0;i < count; i++)
	{
		mem[i] = sin(((double) (2 * M_PI * j)) / Count) * VOX_WAVE_GAIN;

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
			filltriwave(mem, count, 1.0);
			 */
			fillpdwave(mem, count, 1.0);
		case 1:
		default:
			fillpdwave(mem, count, 1.05);
			return;
		case 2:
			fillpdwave(mem, count, 1.10);
			return;
		case 3:
			fillpdwave(mem, count, 1.30);
			return;
		case 4:
			fillpdwave(mem, count, 1.60);
			return;
		case 5:
			fillpdwave(mem, count, 2.0);
			return;
		case 6:
			fillpdwave(mem, count, 3.0);
			return;
		case 7:
			fillpdwave(mem, count, 10.0);
			return;
	}
}

static void
buildVoxSound(bristolOP *operator, bristolOPParams *params, unsigned char parm)
{
	int controller;

#ifdef BRISTOL_DBG
	printf("buildVoxSound(%x, %i, %i, %i)\n", operator, parm, parm >> 3, parm & 0x07);
#endif

	if (parm != 0xff)
	{
		if ((controller = parm >> 4) > 16)
			return;

		/*
		 * Mask out the level of this wave.
		 */
		wavelevel[controller] = parm & 0x0f;
	}

	if (params->param[4].int_val == 0)
		fillVoxWave(operator, params->param[5].int_val);
	else
		fillVoxM2Wave(operator, params);
}

static void
fillVoxWave(bristolOP *operator, int form)
{
	register float gain, *source, *dest, sweep, oscindex;
	register int i, j;
	bristolVOX *specs;

	specs = (bristolVOX *) operator->specs;

#ifdef BRISTOL_DBG
	printf("fillVoxWave(%x, %i)\n", operator, form);
#endif

	/*
	 * Dest is actually the local wave for this operator.
	 */
	bristolbzero(wave1, VOX_WAVE_SZE * sizeof(float));

	/*
	 * We now have a volume array:

		f 16'
		2f 8'
		3f 4'
		2-2/3 2, 1 1/3 1
		flute level (sine)
		reed level (ramp)
	 */

/*	printf("%4i %4i %4i %4i %4i %4i %4i %4i %4i\n", */
/*	wavelevel[0], wavelevel[1], wavelevel[2], wavelevel[3], wavelevel[4], */
/*	wavelevel[5], wavelevel[6], wavelevel[7], wavelevel[8]); */

	/*
	 * Do this for the flute, or sine waves.
	 */
	for (i = 0; i < 3; i++)
	{
		oscindex = 0;

		/*
		 * Take the sinewave and the destination wave.
		 */
		if (form == 0) {
			source = specs->wave[0];
			gain = ((float) (wavelevel[i] * wavelevel[4])) / 64.0;
		} else {
			source = blotriangle;
			gain = ((float) (wavelevel[i] * wavelevel[4])) / 3.0;
		}

		dest = wave1;

		if (gain == 0)
			continue;

		sweep = sweeps[i] * 2;

		for (j = 0; j < VOX_WAVE_SZE; j++)
		{
			*dest++ += source[(int) oscindex] * gain;
			if ((oscindex += sweep) >= VOX_WAVE_SZE)
				oscindex -= VOX_WAVE_SZE;
		}
	}

	/*
	 * Do this for the reed, or ramp waves.
	 */
	for (i = 0; i < 3; i++)
	{
		oscindex = 0;
		/*
		 * Take the sinewave and the destination wave.
		 */
		source = specs->wave[7];

		dest = wave1;

		gain = ((float) (wavelevel[i] * wavelevel[5])) / 64.0;

		if (gain == 0)
			continue;

		sweep = sweeps[i] * 4;

		for (j = 0; j < VOX_WAVE_SZE; j++)
		{
			*dest++ += source[(int) oscindex] * gain;
			if ((oscindex += sweep) >= VOX_WAVE_SZE)
				oscindex -= VOX_WAVE_SZE;
		}
	}

	/*
	 * Do Draw-IV for the flute, or sine waves.
	 */
	for (i = 3; i < 9; i++)
	{
		if ((i == 6) || (i == 7))
			continue;

		oscindex = 0;
		/*
		 * Take the sinewave and the destination wave.
		 */
		if (form == 0) {
			source = specs->wave[0];
			gain = ((float) (wavelevel[3] * wavelevel[4])) / 64.0;
		} else {
			source = blotriangle;
			gain = ((float) (wavelevel[3] * wavelevel[4])) / 3.0;
		}

		dest = wave1;

		if (gain == 0)
			continue;

		sweep = sweeps[i];

		for (j = 0; j < VOX_WAVE_SZE; j++)
		{
			*dest++ += source[(int) oscindex] * gain;
			if ((oscindex += sweep) >= VOX_WAVE_SZE)
				oscindex -= VOX_WAVE_SZE;
		}
	}

	/*
	 * Do Draw-IV for the reed, or ramp waves.
	 */
	for (i = 3; i < 9; i++)
	{
		if ((i == 6) || (i == 7))
			continue;

		oscindex = 0;
		/*
		 * Take the sinewave and the destination wave.
		 */
		source = specs->wave[7];

		dest = wave1;

		gain = ((float) (wavelevel[3] * wavelevel[5])) / 64.0;

		if (gain == 0)
			continue;

		sweep = sweeps[i] * 2;

		for (j = 0; j < VOX_WAVE_SZE; j++)
		{
			*dest++ += source[(int) oscindex] * gain;
			if ((oscindex += sweep) >= VOX_WAVE_SZE)
				oscindex -= VOX_WAVE_SZE;
		}
	}
}

/*
 * This was changed for release 0.10.9 so that it could support two wave table
 * sources, one for the drawbars and another for the percussives.
 */
static void
fillVoxM2Wave(bristolOP *operator, bristolOPParams *params)
{
	register float gain, *source, *dest, sweep, oscindex;
	register int i, j;
	bristolVOX *specs;

	specs = (bristolVOX *) operator->specs;

#ifdef BRISTOL_DBG
	printf("fillVoxM2Wave(%x)\n", operator);
#endif

	/*
	 * Dest is actually the local wave for this operator.
	 */
	bristolbzero(wave1, VOX_WAVE_SZE * sizeof(float));
	bristolbzero(wave2, VOX_WAVE_SZE * sizeof(float));

	/*
	 * We now have a volume array:

		f 16'
		2f 8'
		3f 4'
		2-2/3, 2, 1-1/3, 1
		flute level (sine)
		reed level (ramp)
		5-1/3, 1-3/5
		2-2/3, 2, 1


	printf("%4i %4i %4i %4i %4i %4i %4i %4i %4i\n",
	wavelevel[0], wavelevel[1], wavelevel[2], wavelevel[3], wavelevel[4],
	wavelevel[5], wavelevel[6], wavelevel[7], wavelevel[8]);
	 */

	/*
	 * Do 16/8/4 for the flute, or sine waves.
	 */
	for (i = 0; i < 3; i++)
	{
		oscindex = 0;

		/*
		 * Take the sinewave and the destination wave.
		 */
		source = specs->wave[0];

		dest = wave1;
		if ((i == 2) && (params->param[2].int_val != 0))
			dest = wave2;

		gain = ((float) (wavelevel[i] * wavelevel[4])) / 64.0;

		if (gain == 0)
			continue;

		sweep = sweeps[i] * 2;

		for (j = 0; j < VOX_WAVE_SZE; j++)
		{
			*dest++ += source[(int) oscindex] * gain;
			if ((oscindex += sweep) >= VOX_WAVE_SZE)
				oscindex -= VOX_WAVE_SZE;
		}
	}

	/*
	 * Do 16/8/4 for the reed, or ramp waves.
	 */
	for (i = 0; i < 3; i++)
	{
		oscindex = 0;
		/*
		 * Take the sinewave and the destination wave.
		 */
		source = specs->wave[7];

		dest = wave1;
		if ((i == 2) && (params->param[2].int_val != 0))
			dest = wave2;

		gain = ((float) (wavelevel[i] * wavelevel[5])) / 64.0;

		if (gain == 0)
			continue;

		sweep = sweeps[i] * 4;

		for (j = 0; j < VOX_WAVE_SZE; j++)
		{
			*dest++ += source[(int) oscindex] * gain;
			if ((oscindex += sweep) >= VOX_WAVE_SZE)
				oscindex -= VOX_WAVE_SZE;
		}
	}

	/*
	 * Do Draw-II for the flute, or sine waves. This is 5-1/3 and 1-3/5
	 * which are indeces 3 and 7. There is an optional percussive.
	 */
	gain = ((float) (wavelevel[6] * wavelevel[4])) / 64.0;
	if (gain != 0)
	{
		oscindex = 0;
		/*
		 * Take the sinewave and the destination wave.
		 */
		source = specs->wave[0];
		dest = wave1;
		if (params->param[3].int_val != 0)
			dest = wave2;

		sweep = sweeps[3];

		for (j = 0; j < VOX_WAVE_SZE; j++)
		{
			*dest++ += source[(int) oscindex] * gain;
			if ((oscindex += sweep) >= VOX_WAVE_SZE)
				oscindex -= VOX_WAVE_SZE;
		}

		oscindex = 0;
		dest = wave1;
		if (params->param[3].int_val != 0)
			dest = wave2;

		sweep = sweeps[7];

		for (j = 0; j < VOX_WAVE_SZE; j++)
		{
			*dest++ += source[(int) oscindex] * gain;
			if ((oscindex += sweep) >= VOX_WAVE_SZE)
				oscindex -= VOX_WAVE_SZE;
		}
	}
	gain = ((float) (wavelevel[6] * wavelevel[5])) / 64.0;
	if (gain != 0)
	{
		oscindex = 0;
		/*
		 * Take the rempwave and the destination wave.
		 */
		source = specs->wave[7];
		dest = wave1;
		if (params->param[3].int_val != 0)
			dest = wave2;

		sweep = sweeps[3] * 2;

		for (j = 0; j < VOX_WAVE_SZE; j++)
		{
			*dest++ += source[(int) oscindex] * gain;
			if ((oscindex += sweep) >= VOX_WAVE_SZE)
				oscindex -= VOX_WAVE_SZE;
		}

		oscindex = 0;
		dest = wave1;
		if (params->param[3].int_val != 0)
			dest = wave2;

		sweep = sweeps[7] * 2;

		for (j = 0; j < VOX_WAVE_SZE; j++)
		{
			*dest++ += source[(int) oscindex] * gain;
			if ((oscindex += sweep) >= VOX_WAVE_SZE)
				oscindex -= VOX_WAVE_SZE;
		}
	}




	/*
	 * Do Draw-III for the reed, or ramp waves. This is 2-2/3, 2 and 1 which 
	 * are indices 4, 5 and 8.
	 */
	gain = ((float) (wavelevel[7] * wavelevel[4])) / 64.0;
	if (gain != 0)
	{
		oscindex = 0;
		/*
		 * Take the sinewave and the destination wave.
		 */
		source = specs->wave[0];
		dest = wave1;
		sweep = sweeps[4];
		for (j = 0; j < VOX_WAVE_SZE; j++)
		{
			*dest++ += source[(int) oscindex] * gain;
			if ((oscindex += sweep) >= VOX_WAVE_SZE)
				oscindex -= VOX_WAVE_SZE;
		}

		oscindex = 0;
		dest = wave1;
		sweep = sweeps[5];
		for (j = 0; j < VOX_WAVE_SZE; j++)
		{
			*dest++ += source[(int) oscindex] * gain;
			if ((oscindex += sweep) >= VOX_WAVE_SZE)
				oscindex -= VOX_WAVE_SZE;
		}

		oscindex = 0;
		dest = wave1;
		sweep = sweeps[8];
		for (j = 0; j < VOX_WAVE_SZE; j++)
		{
			*dest++ += source[(int) oscindex] * gain;
			if ((oscindex += sweep) >= VOX_WAVE_SZE)
				oscindex -= VOX_WAVE_SZE;
		}
	}

	gain = ((float) (wavelevel[7] * wavelevel[5])) / 64.0;
	if (gain != 0)
	{
		oscindex = 0;
		/*
		 * Take the rampwave and the destination wave.
		 */
		source = specs->wave[7];

		dest = wave1;
		sweep = sweeps[4];
		for (j = 0; j < VOX_WAVE_SZE; j++)
		{
			*dest++ += source[(int) oscindex] * gain;
			if ((oscindex += sweep) >= VOX_WAVE_SZE)
				oscindex -= VOX_WAVE_SZE;
		}

		oscindex = 0;
		dest = wave1;
		sweep = sweeps[5];
		for (j = 0; j < VOX_WAVE_SZE; j++)
		{
			*dest++ += source[(int) oscindex] * gain;
			if ((oscindex += sweep) >= VOX_WAVE_SZE)
				oscindex -= VOX_WAVE_SZE;
		}

		oscindex = 0;
		dest = wave1;
		sweep = sweeps[8];
		for (j = 0; j < VOX_WAVE_SZE; j++)
		{
			*dest++ += source[(int) oscindex] * gain;
			if ((oscindex += sweep) >= VOX_WAVE_SZE)
				oscindex -= VOX_WAVE_SZE;
		}
	}
}

