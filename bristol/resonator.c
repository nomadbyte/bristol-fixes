
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
 *	resinit()
 *	operate()
 *	reset()
 *	destroy()
 *
 *	destroy() is in the library.
 *
 * Operate will be called when all the inputs have been loaded, and the result
 * will be an output buffer written to the next operator.
 */

#include "bristol.h"
#include "resonator.h"

/*
 * The name of this operator, IO count, and IO names.
 */
#define OPNAME "RES"
#define OPDESCRIPTION "Digital Filter One"
#define PCOUNT 4
#define IOCOUNT 3

#define RES_IN_IND 0
#define RES_MOD_IND 1
#define RES_OUT_IND 2

/*
 * Reset any local memory information.
 */
static int destroy(bristolOP *operator)
{
#ifdef BRISTOL_DBG
	printf("reset(%x)\n", operator);
#endif

	/*
	 * Unmalloc anything we added to this structure
	 */
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
static int reset(bristolOP *operator, bristolRES *local)
{
#ifdef BRISTOL_DBG
	printf("reset(%x)\n", operator);
#endif
	return(0);
}

/*
 * Alter an internal parameter of an operator.
 */
static int param(bristolOP *operator, bristolOPParams *param,
	unsigned char index, float value)
{
#ifdef BRISTOL_DBG
	printf("param(%x, %f)\n", operator,	value);
	printf("resparam(%x, %x, %i, %f)\n", operator, param, index, value);
#endif

	switch (index) {
		case 1:
			param->param[index].float_val =
				gainTable[(int) (value * (CONTROLLER_RANGE - 1))].gain;
			break;
		case 0:
		case 2:
			/*
			 * Convert these into logarithm values from 1 to 0.
			 */
			param->param[index].float_val = value;
			if (value == 0.0)
				param->param[index].float_val = 0;
			break;
		case 3:
			if (value > 0)
				param->param[index].int_val = 1;
			else
				param->param[index].int_val = 0;
			break;
	}

	return(0);
}

/*
 * filter - takes input signal and filters it according to the mod level.
 */
static int operate(register bristolOP *operator, bristolVoice *voice,
	bristolOPParams *param,
	void *lcl)
{
	bristolRESlocal *local = lcl;
	register int count;
	register float *ib, *ob, *mb;
	bristolRES *specs;

	register float velocity, output, vdelta, cutoff, gain, mod, ccut, cvdelta;

	/*
	 * Every operator accesses these variables, the count, and a pointer to
	 * each buffer. We should consider passing them as readymade parameters?
	 *
	 * The Filter now takes normalised inputs, in ranges of 12PO.
	 */
	specs = (bristolRES *) operator->specs;
	count = specs->spec.io[RES_OUT_IND].samplecount;
	ib = specs->spec.io[RES_IN_IND].buf;
	mb = specs->spec.io[RES_MOD_IND].buf;
	ob = specs->spec.io[RES_OUT_IND].buf;

	if (param->param[3].int_val) {
		cutoff = param->param[0].float_val * voice->dFreq / 4;
		vdelta = param->param[1].float_val * voice->dFreq / 4;
	} else {
		cutoff = param->param[0].float_val;
		vdelta = param->param[1].float_val;
	}
	mod = param->param[2].float_val;

	/*
	 * Increase the gain at the lower frequencies, since we are removing large
	 * amounts of the signal content.
	 */
	gain = (1.5 - param->param[0].float_val) * 0.5;

	/*
	 * This is the code from one of the SLab floating point filter routines.
	 * It has been altered here to have a single lowpass filter, and will be
	 * a starting point for the Bristol filters. There will end up being a
	 * number of filter algorithms.
	 */

	if (voice->flags & BRISTOL_KEYON)
	{
		/*
		 * Do any relevant note_on initialisation.
		 */
		output = 0;
		velocity = 0;
	} else {
		output = local->output;
		velocity = local->velocity;
	}

#ifdef BRISTOL_DBG
	printf("resonator(%x, %x, %x)\n", operator, param, local);
#endif
/*printf("%f %f %f\n", cutoff, vdelta, velocity); */

	for (; count > 0; count-=1)
	{
		cvdelta = cutoff * vdelta;
		if ((ccut = (cutoff * (1 - mod)) + *mb * mod) < 0.001)
			ccut = 0.001;
		/*
		 * Needs to build in a mass, and a velocity. Mass is implemented by 
		 * vdelta, which limits the amount by which the velocity can change.
		 */
		velocity += ((*ib++ - output) * cvdelta);
		if (velocity > ccut)
			velocity = ccut;
		if (velocity < -ccut)
			velocity = -ccut;

		*ob++ = (output += velocity) * gain;
	}
/*printf("%f, %f\n", ccut, cvdelta); */
	/*
	 * Put back the local variables
	 */
	local->output = output;
	local->velocity = velocity;
	return(0);
}

/*
 * Setup any variables in our OP structure, in our IO structures, and malloc
 * any memory we need.
 */
bristolOP *
resinit(bristolOP **operator, int index, int samplerate, int samplecount)
{
	bristolRES *specs;

#ifdef BRISTOL_DBG
	printf("resinit(%x(%x), %i, %i, %i)\n",
		operator, *operator, index, samplerate, samplecount);
#endif

	*operator = bristolOPinit(operator, index, samplecount);

	/*
	 * Then the local parameters specific to this operator. These will be
	 * the same for each operator, but must be inited in the local code.
	 */
	(*operator)->operate = operate;
	(*operator)->destroy = destroy;
	(*operator)->reset = reset;
	(*operator)->param = param;

	specs = (bristolRES *) bristolmalloc0(sizeof(bristolRES));
	(*operator)->specs = (bristolOPSpec *) specs;
	(*operator)->size = sizeof(bristolRES);

	/*
	 * These are specific to this operator, and will need to be altered for
	 * each operator.
	 */
	specs->spec.opname = OPNAME;
	specs->spec.description = OPDESCRIPTION;
	specs->spec.pcount = PCOUNT;
	specs->spec.iocount = IOCOUNT;
	specs->spec.localsize = sizeof(bristolRESlocal);

	/*
	 * Now fill in the control specs for this filter.
	 */
	specs->spec.param[0].pname = "cutoff";
	specs->spec.param[0].description = "Filter cutoff frequency";
	specs->spec.param[0].type = BRISTOL_FLOAT;
	specs->spec.param[0].low = 0;
	specs->spec.param[0].high = 1;
	specs->spec.param[0].flags = BRISTOL_ROTARY|BRISTOL_SLIDER;

	specs->spec.param[1].pname = "resonance";
	specs->spec.param[1].description = "Filter emphasis";
	specs->spec.param[1].type = BRISTOL_FLOAT;
	specs->spec.param[1].low = 0;
	specs->spec.param[1].high = 1;
	specs->spec.param[1].flags = BRISTOL_ROTARY|BRISTOL_SLIDER;

	specs->spec.param[2].pname = "modulation";
	specs->spec.param[2].description = "Depth of modulation control";
	specs->spec.param[2].type = BRISTOL_FLOAT;
	specs->spec.param[2].low = 0;
	specs->spec.param[2].high = 1;
	specs->spec.param[2].flags = BRISTOL_ROTARY|BRISTOL_SLIDER;

	specs->spec.param[3].pname = "keyboard tracking";
	specs->spec.param[3].description = "Cutoff tracks keyboard";
	specs->spec.param[3].type = BRISTOL_TOGGLE;
	specs->spec.param[3].low = 0;
	specs->spec.param[3].high = 1;
	specs->spec.param[3].flags = BRISTOL_BUTTON;

	/*
	 * Now fill in the dco IO specs.
	 */
	specs->spec.io[0].ioname = "input";
	specs->spec.io[0].description = "Filter Input signal";
	specs->spec.io[0].samplerate = samplerate;
	specs->spec.io[0].samplecount = samplecount;
	specs->spec.io[0].flags = BRISTOL_AC|BRISTOL_INPUT;

	specs->spec.io[1].ioname = "mod";
	specs->spec.io[1].description = "Filter Control Signal";
	specs->spec.io[1].samplerate = samplerate;
	specs->spec.io[1].samplecount = samplecount;
	specs->spec.io[1].flags = BRISTOL_DC|BRISTOL_INPUT|BRISTOL_HIDE;

	specs->spec.io[2].ioname = "output";
	specs->spec.io[2].description = "Filter Output Signal";
	specs->spec.io[2].samplerate = samplerate;
	specs->spec.io[2].samplecount = samplecount;
	specs->spec.io[2].flags = BRISTOL_AC|BRISTOL_OUTPUT;

	return(*operator);
}

