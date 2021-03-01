
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

#include <math.h>

#include "bristol.h"
#include "aksreverb.h"

#define DELAY		0
#define FEEDBACK	1
#define CROSSOVER	2
#define WETDRY		3
#define LINE1		4

#define OPNAME "AKS Reverb"
#define OPDESCRIPTION "Stereo input/output reverb"
#define PCOUNT 6
#define IOCOUNT 3

#define AKSREVERB_IN_IND 0
#define AKSREVERB_MOD_IND 1
#define AKSREVERB_OUT_IND 2

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
	if (param->param[1].mem)
		bristolfree(param->param[1].mem);

	param->param[0].mem = bristolmalloc0(sizeof(float) * HISTSIZE);
	param->param[1].mem = bristolmalloc0(sizeof(float) * HISTSIZE);

	param->param[0].float_val = 0.2;
	param->param[1].float_val = 0.3;
	param->param[2].float_val = 0.3;
	param->param[3].float_val = 0.3;
	param->param[4].float_val = 1.0;
	param->param[5].float_val = 1.0;

	return(0);
}

/*
 * This is going to be a mono reverb with two delay lines. The delay lines
 * will be sized to approximate the 25 and 28 ms delay lines from the original
 * although we should allow it to go outside of that range. Should consider
 * implementing a high pass filter to try and cut down on booming from the low
 * end.
 */
static int operate(bristolOP *operator,
	bristolVoice *voice,
	bristolOPParams *param,
	void *lcl)
{
	bristolAKSREVERB *specs;
	bristolAKSREVERBlocal *local = (bristolAKSREVERBlocal *) lcl;
	register float *source, *dest, *lhistory, *rhistory;
	register int count, i, histin1, histin2, delay1, delay2;
	register float crossover, feedback, line1, *mod, fb, co;

	specs = (bristolAKSREVERB *) operator->specs;

	count = specs->spec.io[AKSREVERB_OUT_IND].samplecount;
	source = specs->spec.io[AKSREVERB_IN_IND].buf;
	dest = specs->spec.io[AKSREVERB_OUT_IND].buf;
	mod = specs->spec.io[AKSREVERB_MOD_IND].buf;

	/*
	 * operational parameters. Speed should be a function of depth, ie, if 
	 * depth changes, speed should be adjusted to maintain scan rate.
	 *
	 * Let speed be 10 seconds require to reach the given depth. That can be 
	 * calculated as 441000 samples, so we divide speed by depth * 44100;
	depth = param->param[DEPTH].float_val * 1024;
	speed = param->param[SPEED].float_val * 0.0002 * depth;
	 */
	/*
	 * Let scan be ten seconds to reach gain
	gain = param->param[GAIN].float_val * 1.5;
	scan = param->param[SCAN].float_val * 0.0005 * gain;
	 */
	delay1 = param->param[DELAY].float_val * HISTSIZE;
	delay2 = delay1 * 25 / 28;
	fb = 0.5 - param->param[FEEDBACK].float_val;
	co = 0.5 - param->param[CROSSOVER].float_val;
	line1 = param->param[LINE1].float_val;

	lhistory = param->param[0].mem;
	rhistory = param->param[1].mem;

#ifdef DEBUG
	printf("reverb()\n");
#endif

	histin1 = local->histin1;
	histin2 = local->histin2;

	for (i = 0; i < count; i++) {
		feedback = fb * *mod;
		crossover = co * *mod++;
		/*
		 * Save our current signal into the history buffers. This should 
		 * include a component of feedback and crossover for echo/reverb effect.
		 * It also needs to go through a HPF.
		 */
		lhistory[histin1] =
			lhistory[histin1] * feedback
			+ rhistory[histin2] * crossover
			+ *source;
		rhistory[histin2] =
			rhistory[histin2] * feedback
			+ lhistory[histin1] * crossover
			+ *source++;

		/*
		 * for testing just make the output be the input with delay.
		 */
		if (++histin1 >= delay1)
			histin1 = 0;
		if (++histin2 >= delay2)
			histin2 = 0;

		*dest++ = (lhistory[histin1] + rhistory[histin2]) * line1;
	}

	local->histin1 = histin1;
	local->histin2 = histin2;

	return(0);
}

/*
 * Setup any variables in our OP structure, in our IO structures, and malloc
 * any memory we need.
 */
bristolOP *
aksreverbinit(bristolOP **operator, int index, int samplerate, int samplecount)
{
	bristolAKSREVERB *specs;

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

	specs = (bristolAKSREVERB *) bristolmalloc0(sizeof(bristolAKSREVERB));
	(*operator)->specs = (bristolOPSpec *) specs;
	(*operator)->size = sizeof(bristolAKSREVERB);

	/*
	 * These are specific to this operator, and will need to be altered for
	 * each operator.
	 */
	specs->spec.opname = OPNAME;
	specs->spec.description = OPDESCRIPTION;
	specs->spec.pcount = PCOUNT;
	specs->spec.iocount = IOCOUNT;
	specs->spec.localsize = sizeof(bristolAKSREVERBlocal);

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

	specs->spec.param[4].pname = "line1 gain";
	specs->spec.param[4].description = "mix";
	specs->spec.param[4].type = BRISTOL_FLOAT;
	specs->spec.param[4].low = 0;
	specs->spec.param[4].high = 1;
	specs->spec.param[4].flags = BRISTOL_ROTARY|BRISTOL_SLIDER;

	/*
	 * Now fill in the IO specs.
	 */
	specs->spec.io[0].ioname = "input buffer";
	specs->spec.io[0].description = "input signal";
	specs->spec.io[0].samplerate = samplerate;
	specs->spec.io[0].samplecount = samplecount;
	specs->spec.io[0].flags = BRISTOL_AC|BRISTOL_INPUT;

	specs->spec.io[1].ioname = "output buffer";
	specs->spec.io[1].description = "input signal";
	specs->spec.io[1].samplerate = samplerate;
	specs->spec.io[1].samplecount = samplecount;
	specs->spec.io[1].flags = BRISTOL_AC|BRISTOL_INPUT;

	specs->spec.io[2].ioname = "mod buffer";
	specs->spec.io[2].description = "input signal";
	specs->spec.io[2].samplerate = samplerate;
	specs->spec.io[2].samplecount = samplecount;
	specs->spec.io[2].flags = BRISTOL_AC|BRISTOL_INPUT;

	return(*operator);
}

