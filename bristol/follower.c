
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

/*
 * This envelope follower will track the input signal to extract an amplitude
 * envelope from it. It will track the rectified signal.
 */

#include <math.h>

#include "bristol.h"
#include "follower.h"

#define OPNAME "Envelope Follower"
#define OPDESCRIPTION "Mono envelope follower"
#define PCOUNT 1
#define IOCOUNT 2

#define GAIN 0

#define FOLLOWER_LIN_IND 0
#define FOLLOWER_LOUT_IND 1

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

	if (index == 0)
	{
		int offset = (value * (CONTROLLER_RANGE - 1)) + 1;
		/*
		 * This will be used to control the tracking rate. Attack should be
		 * quite a lot faster than decay.
		 */
		param->param[1].float_val = gainTable[offset].rate;
		param->param[2].float_val = 1/(gainTable[offset].rate * 4);
	}
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

	param->param[0].float_val = 0;
	return(0);
}

static int operate(bristolOP *operator,
	bristolVoice *voice,
	bristolOPParams *param,
	void *lcl)
{
	bristolFOLLOWER *specs;
	register float *source, *dest, envelope, signal, attack, decay;
	register int count, i;

	specs = (bristolFOLLOWER *) operator->specs;

	count = specs->spec.io[FOLLOWER_LOUT_IND].samplecount;
	source = specs->spec.io[FOLLOWER_LIN_IND].buf;
	dest = specs->spec.io[FOLLOWER_LOUT_IND].buf;

	envelope = ((bristolFOLLOWERlocal *) lcl)->envelope;

	attack = param->param[1].float_val;
	decay = param->param[2].float_val;

#ifdef DEBUG
	printf("follower()\n");
#endif

	/*
	 * This should be something like a VU meter, and a root mean square with a
	 * gentle average function might fit the bill but could be quite intensive.
	 * Alternatively a rectifier and smoother would also work, but the single
	 * available parameter would really have to be the smoothing rate rather
	 * than overall gain - not an issue as the typical source would be the input
	 * signal that will already have gone through a preamp.
	 */
	for (i = 0; i < count; i++) {
		if ((signal = *source++) < 0)
			signal = -signal;

		if (signal > envelope)
			envelope *= attack;
		else if (signal < envelope)
			envelope *= decay;

		*dest++ = envelope;
	}

	((bristolFOLLOWERlocal *) lcl)->envelope = envelope;
	return(0);
}

/*
 * Setup any variables in our OP structure, in our IO structures, and malloc
 * any memory we need.
 */
bristolOP *
followerinit(bristolOP **operator, int index, int samplerate, int samplecount)
{
	bristolFOLLOWER *specs;

	*operator = bristolOPinit(operator, index, samplecount);

#ifdef BRISTOL_DBG
	printf("followerinit(%x(%x), %i, %i, %i)\n",
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

	specs = (bristolFOLLOWER *) bristolmalloc0(sizeof(bristolFOLLOWER));
	(*operator)->specs = (bristolOPSpec *) specs;
	(*operator)->size = sizeof(bristolFOLLOWER);

	/*
	 * These are specific to this operator, and will need to be altered for
	 * each operator.
	 */
	specs->spec.opname = OPNAME;
	specs->spec.description = OPDESCRIPTION;
	specs->spec.pcount = PCOUNT;
	specs->spec.iocount = IOCOUNT;
	specs->spec.localsize = sizeof(bristolFOLLOWERlocal);

	/*
	 * Now fill in the specs for this operator.
	 */
	specs->spec.param[0].pname = "rate";
	specs->spec.param[0].description= "follower envelope attack and decay rate";
	specs->spec.param[0].type = BRISTOL_FLOAT;
	specs->spec.param[0].low = 0;
	specs->spec.param[0].high = 1;
	specs->spec.param[0].flags = BRISTOL_ROTARY|BRISTOL_SLIDER;

	/*
	 * Now fill in the IO specs.
	 */
	specs->spec.io[0].ioname = "input buffer";
	specs->spec.io[0].description = "input signal";
	specs->spec.io[0].samplerate = samplerate;
	specs->spec.io[0].samplecount = samplecount;
	specs->spec.io[0].flags = BRISTOL_AC|BRISTOL_INPUT;

	specs->spec.io[1].ioname = "output buffer";
	specs->spec.io[1].description = "output signal";
	specs->spec.io[1].samplerate = samplerate;
	specs->spec.io[1].samplecount = samplecount;
	specs->spec.io[1].flags = BRISTOL_AC|BRISTOL_INPUT;

	return(*operator);
}

