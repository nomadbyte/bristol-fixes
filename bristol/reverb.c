
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
 * For the first releases of the 2600 this was a dual tap feedback loop with 
 * crossover. Cheezy, but so was the original 2600 spring reverb. Second 
 * extension was to insert a two pairs of HPF and LPF, one for feedback and
 * another for crossover through the two delay lines. Improved considerably.
 *
 * Next set of extensions introduced 4 feedback delay lines of 'prime' length
 * into a 5th filtered delay line to produce the wet output, plus feeding the
 * wet output back into the 4 lines.
 */

/*#define DEBUG */

#include <math.h>

#include "bristol.h"
#include "reverb.h"

#define DELAY		0
#define FEEDBACK	1
#define CROSSOVER	2
#define WET			3
#define RIGHT		4
#define LEFT		5

#define OPNAME "Reverb"
#define OPDESCRIPTION "Stereo input/output reverb"
#define PCOUNT 6
#define IOCOUNT 3

#define VREVERB_IN 0
#define VREVERB_OUT 1
#define VREVERB_OUT2 2

/*
 * Reset any local memory information.
 */
static int destroy(bristolOP *operator)
{
#ifdef BRISTOL_DBG
	printf("destroy(%x)\n", operator);
#endif

	bristolfree(operator->specs);

	cleanup(operator);
	return(0);
}

/*
 * This is called by the frontend when a parameter is changed.
static int param(param, value)
 */
static int param(bristolOP *operator, bristolOPParams *param,
	unsigned char index, float value)
{
#ifdef DEBUG
	printf("checkParams(%f)\n", value);
#endif

	param->param[index].float_val = value;

	return(0);
}

/*
 * Reset any local memory information.
 */
static int reset(bristolOP *operator, bristolOPParams *param)
{
#ifdef BRISTOL_DBG
	printf("reset(%x)\n", operator);
#endif

	if (param->param[0].mem)
		bristolfree(param->param[0].mem);

	param->param[0].mem = bristolmalloc0(sizeof(float) * DMAX * DLINES);

	param->param[0].float_val = 0.3;
	param->param[0].int_val = 1;
	param->param[1].float_val = 0.6;
	param->param[2].float_val = 0.5;
	param->param[3].float_val = 0.5;
	param->param[4].float_val = 1.0;
	param->param[5].float_val = 1.0;
	return(0);
}

static int operate(bristolOP *operator,
	bristolVoice *voice,
	bristolOPParams *param,
	void *lcl)
{
	bristolVREVERB *specs;
	register bristolVREVERBlocal *local = (bristolVREVERBlocal *) lcl;
	register float *source, *dest, *dest2, *dest2_1, *dest_1, *history;
	register int count, i, j;
	register float wetdry, crossover, feedback, lgain, rgain, wet, linedata;

	specs = (bristolVREVERB *) operator->specs;

	if (param->param[DELAY].int_val)
	{
		/*
		 * Need to force some runtime initialisation.
		 */
		param->param[DELAY].int_val = 0;

		local->start[0] = local->lptr[0] = 0;
		local->end[0] = local->start[0]
			+ primes[0] * (int) ((float) 11 * param->param[DELAY].float_val);
		local->start[1] = local->lptr[1] = 1 * DMAX;
		local->end[1] = local->start[1]
			+ primes[1] * (int) ((float) 13 * param->param[DELAY].float_val);
		local->start[2] = local->lptr[2] = 2 * DMAX;
		local->end[2] = local->start[2]
			+ primes[2] * (int) ((float) 7 * param->param[DELAY].float_val);
		local->start[3] = local->lptr[3] = 3 * DMAX;
		local->end[3] = local->start[3]
			+ primes[3] * (int) ((float) 19 * param->param[DELAY].float_val);
		local->start[4] = local->lptr[4] = 4 * DMAX;
		local->end[4] = local->start[4]
			+ primes[4] * (int) ((float) 23 * param->param[DELAY].float_val);
		local->start[5] = local->lptr[5] = 5 * DMAX;
		local->end[5] = local->start[5]
			+ primes[5] * (int) ((float) 29 * param->param[DELAY].float_val);
/*
printf("resync lengths: %f (%f, %f, %f): %i %i %i %i %i\n",
param->param[0].float_val,
param->param[1].float_val,
param->param[2].float_val,
param->param[3].float_val,
local->end[0],
local->end[1],
local->end[2],
local->end[3],
local->end[4]);
*/
	}

	count = specs->spec.io[VREVERB_IN].samplecount;
	source = specs->spec.io[VREVERB_IN].buf;
	dest_1 = dest = specs->spec.io[VREVERB_OUT].buf;
	dest2_1 = dest2 = specs->spec.io[VREVERB_OUT2].buf;

	feedback = param->param[FEEDBACK].float_val * 0.5;
	crossover = param->param[CROSSOVER].float_val * 0.5;
	wetdry = param->param[WET].float_val;
	lgain = param->param[LEFT].float_val;
	rgain = param->param[RIGHT].float_val;

	history = param->param[0].mem;

#ifdef DEBUG
	printf("reverb()\n");
#endif

	for (i = 0; i < count; i++) {
		linedata = 0;
		/*
		 * We should first take a sample out of the last delay line, it will
		 * be fed back into the primary lines.
		 */
		wet = (history[local->lptr[4]] + history[local->lptr[5]]) * 0.5;

		for (j = 0; j < 4; j++)
		{
			/*
			 * Then, one for one, run the 4 primary delay lines feeding back
			 * into themselves
			 */
			linedata += history[local->lptr[j]];
			history[local->lptr[j]] =
				(*source * wetdry) + (wet + history[local->lptr[j]]) * feedback;
			if (++(local->lptr[j]) >= local->end[j])
				local->lptr[j] = local->start[j];
		}

		/* 
		 * And finally feed the output from the delay lines into the last line
		 * and mix the wet/dry signal.
		 */
		*dest2++ = *dest2_1++ * 0.5
			+ (history[local->lptr[5]] - linedata * 0.25) * lgain;
		*dest++ = *dest_1++* 0.5
			+ (history[local->lptr[4]] - linedata * 0.25) * rgain;

		history[local->lptr[5]] = linedata * 0.25 - wet * crossover;
		if (++(local->lptr[5]) >= local->end[5])
			local->lptr[5] = local->start[5];

		/* 
		 * And finally feed the output from the delay lines into the last line
		 * and mix the wet/dry signal.
		 */
		history[local->lptr[4]] = linedata * 0.25 - wet * crossover;
		if (++(local->lptr[4]) >= local->end[4])
			local->lptr[4] = local->start[4];

		source++;
	}
	return(0);
}

/*
 * Setup any variables in our OP structure, in our IO structures, and malloc
 * any memory we need.
 */
bristolOP *
reverbinit(bristolOP **operator, int index, int samplerate, int samplecount)
{
	bristolVREVERB *specs;

	*operator = bristolOPinit(operator, index, samplecount);

#ifdef BRISTOL_DBG
	printf("reverbinit(%x(%x), %i, %i, %i)\n",
		operator, *operator, index, samplerate, samplecount);
#endif

	/*
	 * Then the local parameters specific to this operator. These will be
	 * the same for each operator, but must be inited in the local code.
	 */
	(*operator)->operate = operate;
	(*operator)->destroy = destroy;
	(*operator)->reset = reset;
	(*operator)->param = param;
	(*operator)->flags |= BRISTOL_FX_STEREO;

	specs = (bristolVREVERB *) bristolmalloc0(sizeof(bristolVREVERB));
	(*operator)->specs = (bristolOPSpec *) specs;
	(*operator)->size = sizeof(bristolVREVERB);

	/*
	 * These are specific to this operator, and will need to be altered for
	 * each operator.
	 */
	specs->spec.opname = OPNAME;
	specs->spec.description = OPDESCRIPTION;
	specs->spec.pcount = PCOUNT;
	specs->spec.iocount = IOCOUNT;
	specs->spec.localsize = sizeof(bristolVREVERBlocal);

	/*
	 * Now fill in the specs for this operator.
	 */
	specs->spec.param[0].pname = "delay";
	specs->spec.param[0].description= "reverb delay line depth";
	specs->spec.param[0].type = BRISTOL_FLOAT;
	specs->spec.param[0].low = 0;
	specs->spec.param[0].high = 1;
	specs->spec.param[0].flags = BRISTOL_ROTARY|BRISTOL_SLIDER;

	specs->spec.param[1].pname = "feedback";
	specs->spec.param[1].description = "depth of mono feedback";
	specs->spec.param[1].type = BRISTOL_FLOAT;
	specs->spec.param[1].low = 0;
	specs->spec.param[1].high = 1;
	specs->spec.param[1].flags = BRISTOL_ROTARY|BRISTOL_SLIDER;

	specs->spec.param[2].pname = "crossfade";
	specs->spec.param[2].description = "depth of stereo feedback signal";
	specs->spec.param[2].type = BRISTOL_FLOAT;
	specs->spec.param[2].low = 0;
	specs->spec.param[2].high = 1;
	specs->spec.param[2].flags = BRISTOL_ROTARY|BRISTOL_SLIDER;

	specs->spec.param[3].pname = "depth";
	specs->spec.param[3].description = "wet/dry mix";
	specs->spec.param[3].type = BRISTOL_FLOAT;
	specs->spec.param[3].low = 0;
	specs->spec.param[3].high = 1;
	specs->spec.param[3].flags = BRISTOL_ROTARY|BRISTOL_SLIDER;

	specs->spec.param[4].pname = "left";
	specs->spec.param[4].description = "mix";
	specs->spec.param[4].type = BRISTOL_FLOAT;
	specs->spec.param[4].low = 0;
	specs->spec.param[4].high = 1;
	specs->spec.param[4].flags = BRISTOL_ROTARY|BRISTOL_SLIDER;

	specs->spec.param[5].pname = "right";
	specs->spec.param[5].description = "mix";
	specs->spec.param[5].type = BRISTOL_FLOAT;
	specs->spec.param[5].low = 0;
	specs->spec.param[5].high = 1;
	specs->spec.param[5].flags = BRISTOL_ROTARY|BRISTOL_SLIDER;

	/*
	 * Now fill in the IO specs.
	 */
	specs->spec.io[0].ioname = "left input buffer";
	specs->spec.io[0].description = "input signal";
	specs->spec.io[0].samplerate = samplerate;
	specs->spec.io[0].samplecount = samplecount;
	specs->spec.io[0].flags = BRISTOL_AC|BRISTOL_INPUT;

	specs->spec.io[1].ioname = "right input buffer";
	specs->spec.io[1].description = "input signal";
	specs->spec.io[1].samplerate = samplerate;
	specs->spec.io[1].samplecount = samplecount;
	specs->spec.io[1].flags = BRISTOL_AC|BRISTOL_INPUT;

	specs->spec.io[2].ioname = "left output";
	specs->spec.io[2].description = "output signal";
	specs->spec.io[2].samplerate = samplerate;
	specs->spec.io[2].samplecount = samplecount;
	specs->spec.io[2].flags = BRISTOL_AC|BRISTOL_OUTPUT;

	specs->spec.io[3].ioname = "right output";
	specs->spec.io[3].description = "output signal";
	specs->spec.io[3].samplerate = samplerate;
	specs->spec.io[3].samplecount = samplecount;
	specs->spec.io[3].flags = BRISTOL_AC|BRISTOL_OUTPUT;

	return(*operator);
}

