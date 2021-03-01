
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
#include "vibrachorus.h"

#define SPEED		0
#define DEPTH		1
#define SCAN		2
#define GAIN		3

#define OPNAME "Dimension"
#define OPDESCRIPTION "Vibrato Chorus"
#define PCOUNT 3
#define IOCOUNT 3

#define VCHORUS_IN_IND 0
#define VCHORUS_LOUT_IND 1
#define VCHORUS_ROUT_IND 2

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
	bristolVCHORUS *specs;

	specs = (bristolVCHORUS *) operator->specs;

#ifdef DEBUG
	printf("checkParams(%f)\n", value);
#endif

	switch (index) {
		case 0:
			/*
			 * We want to go from 0.1Hz up to perhaps 20Hz. That means we need
			 * to step through the 1024 sinewave table at that rate.
			 *
			 * specs->samplerate
			 */
			param->param[index].float_val = 
				(0.1 + value * 20) * 1024 / specs->samplerate;
			return(0);
		case 1:
			if ((param->param[index].int_val = value * 256) <= 0)
				param->param[index].int_val = 1;
			break;
		case 2:
			if (value == 0)
				param->param[index].int_val = 0;
			else
				param->param[index].int_val = 1;
			break;
	}
	param->param[index].float_val = value;
	return(0);
}

static float sinewave[1024];

/*
 * Reset any local memory information.
 */
static int reset(bristolOP *operator, bristolOPParams *param)
{
	int i;

#ifdef BRISTOL_DBG
	printf("reset(%x)\n", operator);
#endif

	if (param->param[0].mem)
		bristolfree(param->param[0].mem);

	param->param[0].mem = bristolmalloc0(sizeof(float) * HISTSIZE);

	param->param[0].int_val = 10;
	param->param[1].int_val = 10;
	param->param[2].int_val = 0;

	// Get a normalised sinewave.
	for (i = 0; i < 1024; i++)
		sinewave[i] = sinf(M_PI * ((float) i) * 2.0 / 1024.0f) + 1.0;

	return(0);
}

static float table[TABSIZE];

/*
 * Stereo chorus. Create a vibrato effect at given speed from mono input signal
 * and mix it in varying amount back into the left and right channels. The
 * left and right should be given a copy of the input data as pass through.
 *
 * We need a parameter for depth that controls how far into the history we are
 * going to scan, the rate at which we scan, the depth of the returned signal
 * that defines the amount of 'flanging', and perhaps a second rate parameter
 * that alters the stereo panning of the signals. Having this separate from
 * the scan speed should improve the helicopter effect. All parameters will 
 * have to be float to ensure we can have subsample scan rates, previous 
 * versions have been a bit limited with single sample steps. We should also 
 * make this a resampling chorus.
 *
 * Extra parameters could be used to adjust the gain of both the original and
 * the vibrato signal, and we could consider different scaning waveforms. The
 * only one at the moment is a triangular which is close enough to sine, but a
 * front or back loaded tri wave would introduce some interesting leading and
 * trailing pumping effects, something that could be done by a factor that 
 * justs affects the ramp rate, applied +ve when ramping up and -ve when down.
 */
static int operate(bristolOP *operator,
	bristolVoice *voice,
	bristolOPParams *param,
	void *lcl)
{
	bristolVCHORUS *specs;
	bristolVCHORUSlocal *local = (bristolVCHORUSlocal *) lcl;
	register float *source, *ldest, *rdest;
	register int count;
	register float *history, depth, speed, scan, fdir;
	register float histout, value, scanp, gain, cg, dir, scanr;
	register int histin, i;

	specs = (bristolVCHORUS *) operator->specs;

	count = specs->spec.io[VCHORUS_LOUT_IND].samplecount;
	source = specs->spec.io[VCHORUS_IN_IND].buf;
	ldest = specs->spec.io[VCHORUS_LOUT_IND].buf;
	rdest = specs->spec.io[VCHORUS_ROUT_IND].buf;

	/*
	 * operational parameters. Speed should be a function of depth, ie, if 
	 * depth changes, speed should be adjusted to maintain scan rate.
	 *
	 * Let speed be 10 seconds require to reach the given depth. That can be 
	 * calculated as 441000 samples, so we divide speed by depth * 44100;
	 */
	depth = param->param[DEPTH].float_val * 1024;
	speed = param->param[SPEED].float_val;

	/*
	 * Let scan be ten seconds to reach gain
	 */
	gain = param->param[GAIN].float_val * 1.5;
	scan = param->param[SCAN].float_val * 0.0005 * gain;

	history = param->param[0].mem;

#ifdef DEBUG
	printf("dimensionD()\n");
#endif

	histin = local->Histin;
	histout = local->Histout;
	scanr = local->scanr;
	scanp = local->scanp;
	cg = local->cg;
	dir = local->dir;
	fdir = local->fdir;

	for (i = 0; i < count; i++) {
		/*
		 * Save our current signal into the history buffer.
		 */
		history[histin] = *source;

		/*
		 * Take our sample. Resample the two nearest for the position of our
		 * current scan through the history.
		 *
		 * Resampling is to take the current sample, plus the difference of the
		 * distance between this and the next sample.
		 value = history[histout]
		 	+ (history[histout + 1] - history[histout])
				* histout - (int) histout)
		 */
		if ((histout + 1) >= HISTSIZE)
		{
			value = history[(int) histout] +
				(history[0] - history[(int) histout])
					* (histout - ((float) ((int) histout)));
		} else {
			value = history[(int) histout] +
				(history[(int) histout + 1] - history[(int) histout])
					* (histout - ((float) ((int) histout)));
		}

		/*
		 * This is wrong. We take our sample and add to one channel and then
		 * subtract from the other. Correct operation is to take the sample
		 * in value, and then also alter the add/subtract value. We actually
		 * need three parameters, the speed at which we scan through the data
		 * history, the depth to which we scan, and the gain to apply the
		 * phasing effect. FIXED.
		 */
		if (dir == 0)
		{
			if ((cg += scan) > gain)
				dir = 1;
		} else {
			if ((cg -= scan) < 0.0)
				dir = 0;
		}
	
		if (scan == 0)
		{
			*rdest++ = (*source * (1.5 - gain)) + value * gain;
			*ldest++ = (*source * (1.5 - gain)) + value * gain;
		} else {
			*rdest++ = (*source * (1.5 - gain)) + value * (gain - cg);
			*ldest++ = (*source * (1.5 - gain)) + value * cg;
		}

		history[histin] += value * gain * 0.5;
		source++;

		if (++histin >= HISTSIZE) histin = 0;

		if ((histout = ((float) histin) - scanp) < 0)
			while (histout < 0)
				histout += HISTSIZE;

		while (histout >= HISTSIZE)
			histout -= HISTSIZE;

		/*
		 * Adjust the scan rate through memory. Initially out is zero, and we
		 * tend it towards depth by adding the speed increments. When we reach
		 * depth we change the direction.
		if (fdir == 0)
		{
			if ((scanp += speed) > depth)
				fdir = 1;
		} else {
			if ((scanp -= speed) < speed)
				fdir = 0;
		}
		 */
		if ((scanr += speed) >= 1024)
			scanr -= 1024;
		scanp = sinewave[(int) scanr] * depth;
	}

	local->Histin = histin;
	local->Histout = histout;
	local->scanr = scanr;
	local->scanp = scanp;
	local->cg = cg;
	local->dir = dir;
	local->fdir = fdir;
	return(0);
}

static void
buildSineTable(float *table)
{
	int i;

	for (i = 260; i < (TABSIZE + 260); i++)
		table[i - 260] = (float) sin(2 * M_PI * ((double) i) / TABSIZE);
}

/*
 * Setup any variables in our OP structure, in our IO structures, and malloc
 * any memory we need.
 */
bristolOP *
chorusinit(bristolOP **operator, int index, int samplerate, int samplecount)
{
	bristolVCHORUS *specs;

	*operator = bristolOPinit(operator, index, samplecount);

#ifdef BRISTOL_DBG
	printf("chorusinit(%x(%x), %i, %i, %i)\n",
		operator, *operator, index, samplerate, samplecount);
#endif

	/*
	 * Then the local parameters specific to this operator. These will be
	 * the same for each operator, but must be inited in the local code.
	 */
	(*operator)->operate = operate;
	(*operator)->destroy = destroy;
	(*operator)->reset = reset;
	(*operator)->param= param;

	specs = (bristolVCHORUS *) bristolmalloc0(sizeof(bristolVCHORUS));
	specs->samplerate = samplerate;
	(*operator)->specs = (bristolOPSpec *) specs;
	(*operator)->size = sizeof(bristolVCHORUS);

	/*
	 * These are specific to this operator, and will need to be altered for
	 * each operator.
	 */
	specs->spec.opname = OPNAME;
	specs->spec.description = OPDESCRIPTION;
	specs->spec.pcount = PCOUNT;
	specs->spec.iocount = IOCOUNT;
	specs->spec.localsize = sizeof(bristolVCHORUSlocal);

	/*
	 * Now fill in the specs for this operator.
	 */
	specs->spec.param[0].pname = "speed";
	specs->spec.param[0].description= "rotation speed";
	specs->spec.param[0].type = BRISTOL_FLOAT;
	specs->spec.param[0].low = 0;
	specs->spec.param[0].high = 1;
	specs->spec.param[0].flags = BRISTOL_ROTARY|BRISTOL_SLIDER;

	specs->spec.param[1].pname = "depth";
	specs->spec.param[1].description = "depth of rotation";
	specs->spec.param[1].type = BRISTOL_FLOAT;
	specs->spec.param[1].low = 0;
	specs->spec.param[1].high = 1;
	specs->spec.param[1].flags = BRISTOL_ROTARY|BRISTOL_SLIDER;

	specs->spec.param[2].pname = "crunch";
	specs->spec.param[2].description = "gain of return signal";
	specs->spec.param[2].type = BRISTOL_FLOAT;
	specs->spec.param[2].low = 0;
	specs->spec.param[2].high = 1;
	specs->spec.param[2].flags = BRISTOL_ROTARY|BRISTOL_SLIDER;

	specs->spec.param[3].pname = "scan";
	specs->spec.param[3].description = "stereo pan rate";
	specs->spec.param[3].type = BRISTOL_FLOAT;
	specs->spec.param[3].low = 0;
	specs->spec.param[3].high = 1;
	specs->spec.param[3].flags = BRISTOL_ROTARY|BRISTOL_SLIDER;


	/*
	 * Now fill in the IO specs.
	 */
	specs->spec.io[0].ioname = "input";
	specs->spec.io[0].description = "Input signal";
	specs->spec.io[0].samplerate = samplerate;
	specs->spec.io[0].samplecount = samplecount;
	specs->spec.io[0].flags = BRISTOL_AC|BRISTOL_INPUT;

	specs->spec.io[1].ioname = "left output";
	specs->spec.io[1].description = "output signal";
	specs->spec.io[1].samplerate = samplerate;
	specs->spec.io[1].samplecount = samplecount;
	specs->spec.io[1].flags = BRISTOL_AC|BRISTOL_OUTPUT;

	specs->spec.io[2].ioname = "right output";
	specs->spec.io[2].description = "output signal";
	specs->spec.io[2].samplerate = samplerate;
	specs->spec.io[2].samplecount = samplecount;
	specs->spec.io[2].flags = BRISTOL_AC|BRISTOL_OUTPUT;

	buildSineTable(table);

	return(*operator);
}

