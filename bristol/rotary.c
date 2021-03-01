
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

b3 intertia control alterations do not reset as expected
30.3, 64bit, -no-jack-midi option

observed behaviour:
Altering the intertia control from minimum to maximum gives expected
result, but when the control is returned to minimum, the interia is
apparently not altered

steps to reproduce:
1. configure inertia to minimum value, leslie on, slow speed, mode
separate.
2. save profile, exit
3. start new bristol session, load saved profile
4. play chord of your choice
4. switch leslie to fast, observe spin-up is within 4 or 5 seconds
5. alter inertia to maximum, observe spin-up is very slow
6. reset inertia to minimum

notice that the spin-up is very slow. It does not return the the speed in
step 4.

bristol needs to be restarted to return to normal.

As an aside, even at minimum inertia, I can't get the leslie to spin up
quickly enough when compared to real B3 performances I've heard. Is there
some other parameter that I need to alter?

 */

/*#define DEBUG */

#include <math.h>

#include "bristol.h"
#include "rotary.h"

/*
 * Used by the user interface in the effects rack.
#define EFFECT_NAME "leslie"
#define SHORT_NAME "Leslie"
 */

#define SPEED		0
#define INERTIA		1
#define DELAY		2
#define REVERB		3
#define FEEDBACK	4
#define OVERDRIVE	5
#define CROSSOVER	6
#define FLAGS		7 /* Sync on rotations. No rotation bass. On/Off. */

#define OPNAME "Leslie"
#define OPDESCRIPTION "Rotary speaker simulator"
#define PCOUNT 8
#define IOCOUNT 3

#define LESLIE_IN_IND 0
#define LESLIE_OUT_IND 1
#define LESLIE_OUT2_IND 2

static void newValvify(float *, int, float);
#define ROOT2 1.4142135623730950488
static float pidsr;
static int stopped = 0;

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
 *
 * Too many of these parameters are integers, and this is obscuring the min
 * and max rotary speeds, especially the max speed. They should be made into
 * floats, additionally changing the speed to have 3 values:
 *	High, Low, Stopped
 */
static int param(bristolOP *operator, bristolOPParams *param,
	unsigned char index, float value)
{
#ifdef DEBUG
	printf("checkParams(%i, %i)\n", param, value);
#endif
	switch (index) {
		case SPEED:
			param->param[index].float_val = value * value * value;
			param->param[index].int_val = 532 - value * 512;
/* printf("speed is %i\n", param->param[index].int_val); */
			break;
		case INERTIA:
			param->param[index].float_val = value / 2;
			break;
		case DELAY:
			param->param[index].float_val = value;
			param->param[index].int_val
				= value * (LESLIE_TBL_SZE - 24) + 1;
/*			param->param[index].int_val * LESLIE_TBL_SZE; */
/*printf("delay is %i\n", param->param[index].int_val); */
			break;
		case REVERB:
		case FEEDBACK:
			param->param[index].float_val = value * 0.9;
			break;
		case OVERDRIVE:
			/*
			value = 0.5 - value;
			param->param[index].float_val = 1.0 + value * 10;
			if (value < 0)
				param->param[index].float_val
					= -1 / (1.0 - (value * 10));
			*/
			param->param[index].float_val = 1.0 - (value * 0.5);
			break;
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
	if (param->param[0].mem)
		bristolfree(param->param[0].mem);
	if (param->param[1].mem)
		bristolfree(param->param[1].mem);

	param->param[0].mem = bristolmalloc0(sizeof(float) * HISTSIZE);
	param->param[1].mem = bristolmalloc0(sizeof(float) * HISTSIZE);

	param->param[1].int_val = 5;
	param->param[3].float_val = 0.7;
	param->param[6].float_val = 0.01;
	param->param[7].int_val = 1;
	return(0);
}

static float table[TABSIZE];

static void
rotate(bristolLESLIElocal *local, float *history, float *ib, float *obl,
float *obr, int count, float scan, float feedback, float delay, float reverb)
{
	float volleft, volright, freq1, histout, value, depth, scanp, scanr, rev;
	int histin, i, revout, revout1, revout2, revout3;

	volleft = local->Vol1;
	volright = local->Vol2;
	freq1 = local->Freq1;
	histin = local->Histin;
	histout = local->Histout;
	scanp = local->scanp;
	scanr = local->scanr;
	revout = local->Revout;

	/*
	 * Reverb should be a function of the rotational speed? We have two totally
	 * distinct delay lines, one for the treble rotor, and one for the base
	 * rotor, and should use them. Range is about 300.
	 */
	if ((revout = histin - TAP1) < 0) revout = histin - TAP1 + HISTSIZE;
	if ((revout1 = histin - TAP2) < 0) revout1 = histin - TAP2 + HISTSIZE;
	if ((revout2 = histin - TAP3) < 0) revout2 = histin - TAP3 + HISTSIZE;
	if ((revout3 = histin - TAP4) < 0) revout3 = histin - TAP4 + HISTSIZE;

	for (i = 0; i < count; i++) {
		history[histin] = *ib;

		/*
		 * Some doppler
		 */
		if ((histout + 1) >= HISTSIZE)
		{
			value = history[0] * (histout - ((float) ((int) histout)))
				+ history[(int) histout]
					* (1.0 - (histout - ((float) ((int) histout))));
		} else {
			value = history[(int) histout + 1]
					* (histout - ((float) ((int) histout)))
				+ history[(int) histout]
					* (1.0 - (histout - ((float) ((int) histout))));
		}

		/*
		 * Some tremelo, and for now put doppler in both sides.
		 */
		rev = history[revout1] * 0.8
			+ history[revout2] * 0.36
			+ history[revout3] * 0.62;

		obr[i] += *ib * volright + value * reverb
			+ (history[revout] + rev) * volleft * feedback;
		obl[i] += *ib * volleft + value * reverb
			+ (history[revout] + rev) * volright * feedback;

		history[revout] += (value + rev) * feedback;
		history[histin] += rev * feedback * 0.5;

		++ib;

#ifdef DEBUG2
		printf("in %i out %f step %f depth %f\n", histin, histout, freq1,
			histin - histout < 0? histin + HISTSIZE - histout:histin - histout);
#endif

		if (++histin >= HISTSIZE) histin = 0;
		if ((histout += freq1) >= HISTSIZE) histout -= HISTSIZE;
		/*
		 * Revout should be made into a 4 tap delay line, potentially with dual
		 * taps (ie, 8 tap), with feedback returned to first tap.
		 */
		if (++revout >= HISTSIZE) revout = 0;
		if (++revout1 >= HISTSIZE) revout1 = 0;
		if (++revout2 >= HISTSIZE) revout2 = 0;
		if (++revout3 >= HISTSIZE) revout3 = 0;

		/*
		 * The output buffers get a filtered volume fraction, plus a phase 
		 * component.
		 */
		if (--scanr <= 0)
		{
			scanr = scan;

			volleft = (1.0 - table[(int) scanp]) * 0.5;
			volright = (1.0 - table[(int) ((scanp - 80) < 0?
				scanp - 80 + TABSIZE:scanp - 80)]) * 0.5;
			/*
			 * Freq1 will initially define a length of the delay chain. Then
			 * we interpolate the changes we need to make to histout to move
			 * to this depth.
			 */
			freq1 = delay - table[(int) ((scanp + 50) >= TABSIZE?
				scanp + 50 - TABSIZE:scanp + 50)] * delay;

#ifdef DEBUG2
			printf("scan %3.0f: ddepth %f, tbl %f", scanp, freq1,
				table[(int) ((scanp + 50) >= TABSIZE?
					scanp + 50 - TABSIZE
					:scanp + 50)]);
#endif
			/*
			 * Get the current depth
			 */
			depth = histin + scan - histout < 0?
				((float) histin) + scan - histout + HISTSIZE
				:((float) histin) + scan - histout;

			freq1 = (depth - freq1) / scan;
#ifdef DEBUG2
			printf(" adepth %f, step %f\n", depth, freq1);
#endif
			if ((scan > 400) && (scanp == (TABSIZE>>1) + local->toff))
				--scanp;
			if (++scanp >= TABSIZE)
				scanp = 0;
		}
	}

	local->Vol1 = volleft;
	local->Vol2 = volright;
	local->Freq1 = freq1;
	local->Histin = histin;
	local->Histout = histout;
	local->Revout = revout;
	local->scanr = scanr;
	local->scanp = scanp;
}

extern void butter_filter(float *, float *, float *, float, int);

/*
 * Leslie will operate in several separate modes, potentially not all 
 * exclusive:
 *
 * Crossover bin to horn on/off (probably bad idea - we have to emulate speaker
 * type 12 inch and bullet horn).
 * No bin rotation.
 * Sync/noSync horn to bin.
 *
 * Also, valvify should be progressively high end damped as it cuts in more.
 */
static int operate(bristolOP *operator,
	bristolVoice *voice,
	bristolOPParams *param,
	void *lcl)
{
	bristolLESLIE *specs;
	bristolLESLIElocal *local = (bristolLESLIElocal *) lcl, *local2;
	register float *source;
	float *dest1, *dest2, *tbuf = local->tbuf;
	int nSpeed = local->nSpeed, Speed, Delay, count, inertia = local->inertia;
	register int i;
	float Feedback, Reverb;

	((bristolLESLIElocal *) lcl)->toff = -25;
	local2 = (bristolLESLIElocal *) (lcl + sizeof(bristolLESLIElocal));
	((bristolLESLIElocal *) local2)->toff = 10;

	specs = (bristolLESLIE *) operator->specs;

	count = specs->spec.io[LESLIE_OUT_IND].samplecount;
	source = specs->spec.io[LESLIE_IN_IND].buf;
	dest1 = specs->spec.io[LESLIE_OUT_IND].buf;
	dest2 = specs->spec.io[LESLIE_OUT2_IND].buf;

	if (tbuf == 0)
	{
/*printf("mallocing tbuf %i\n", sizeof(float) * count); */
		local2->tbuf = (float *) bristolmalloc0(sizeof(float) * count);
		tbuf = (float *) bristolmalloc0(sizeof(float) * count);
		local->tbuf = tbuf;
	}

	/*
	 * operational parameters.
	 */
	if (param->param[7].int_val == 0) {
		/* This is stopped. Put in a very slow rotation, it will spin down and */
		/* eventually stall facing us (more or less facing). */
		Speed = 16383;
		stopped = 1;
	} else {
		if (stopped == 1) {
			nSpeed = 168;
			stopped = 0;
		}
		Speed = param->param[SPEED].int_val +
			(127 - (voice->baudio->contcontroller[1] * 127));
		if (param->param[SPEED].int_val == 532)
			Speed = (532 - (voice->baudio->contcontroller[1] * 512));
		else
			Speed = param->param[SPEED].int_val
				+ (127 - (voice->baudio->contcontroller[1] * 127));
	}
	Feedback = param->param[FEEDBACK].float_val;
	Reverb = param->param[REVERB].float_val;
	Delay = param->param[DELAY].int_val;
	/*
	 * Reverb was moved out of the rotary for being weak - it was only intended
	 * for doppler effects so now going to fix the values.
	 */
/*	Reverb = 0.7; */
/*	Feedback = 0.4; */
/*printf("Rev %f, Delay %i\n", Reverb, Delay); */

	if (nSpeed != Speed)
	{
		if (nSpeed == 0) {
			local->tinc = 0;
			nSpeed = 138;
		}
/*
printf("ns: %i s %i i %i b %i t %f\n",
nSpeed, Speed, inertia, param->param[3].int_val, local->tinc);
*/
		/* 
		 * The inertia algorithm needs to change.
		 */
		if (nSpeed < Speed) {
			if ((param->param[3].int_val) ||
				(++local->inertia >= local->tinc))
			{
				local->inertia = 0;
				if ((local->tinc -= param->param[INERTIA].float_val) < 0)
					local->tinc = 0;

				if ((nSpeed += 1) >= Speed)
				{
					nSpeed = Speed;
					local->tinc = 0;
				}
			}
		} else {
			if (++local->inertia >= local->tinc)
			{
				local->inertia = 0;
				local->tinc += param->param[INERTIA].float_val;

				if ((nSpeed -= 1) <= Speed)
					nSpeed = Speed;
			}
		}
	}

#ifdef DEBUG
	printf("leslie(%x, %x, %x, %i)\n", source, dest1, dest2, count);
#endif

	/*
	 * Put in some distortion
	 */
	newValvify(source, count, param->param[OVERDRIVE].float_val);

/*printf("leslie %i\n", param->param[7].int_val); */
/*
	if (param->param[7].int_val == 0)
	{
		for (i = 0; i < count; i++)
			*dest2++ = *source++;
		return;
	}
*/

	/* 
	 * crossover should be made configurable?
	 */
	if (local->crossover != param->param[6].float_val)
	{
		float c;

		local->crossover = param->param[6].float_val;

		if (local->crossover == 0)
		{
			local->crossover = (float) 1.0 / CONTROLLER_RANGE;
			param->param[6].float_val = local->crossover;
		}

		pidsr = M_PI / sqrt(M_E);

		/*
		 * Configure a lowpass butterworth
		 */
		c = 1.0 / (float) tan((double) (pidsr * local->crossover));
		local->a[1] = 1.0 / (1.0 + ROOT2 * c + c * c);
		local->a[2] = 2 * local->a[1];
		local->a[3] = local->a[1];
		local->a[4] = 2.0 * (1.0 - c*c) * local->a[1];
		local->a[5] = (1.0 - ROOT2 * c + c * c) * local->a[1];
		local->a[6] = 0;
		local->a[7] = 0;

		/*
		 * And a highpass butterworth.
		 */
		c = (float) tan((double) (pidsr * local->crossover));
		local2->a[1] = 1.0 / (1.0 + ROOT2 * c + c * c);
		local2->a[2] = -(local2->a[1] + local2->a[1]);
		local2->a[3] = local2->a[1];
		local2->a[4] = 2.0 * (c * c - local2->a[1]);
		local2->a[5] = (1.0 - ROOT2 * c + c * c) * local2->a[1];
	}

	bristolbzero(local->tbuf, count * sizeof(float));
	bristolbzero(local2->tbuf, count * sizeof(float));

	/*
	 * The butterworth filters do not like having zero inputs, they head into
	 * max CPU land.
	 */
	tbuf = source;
	for (i = 0; i < count; i++)
		*tbuf++ += 1;

	/*
	 * Put together a low pass and high pass for the crossover.
	 */
	butter_filter(source, local->tbuf, &local->a[0], 2.0, count);
	butter_filter(source, local2->tbuf, &local2->a[0], 2.0, count);

	/*
	 * Dest1 is probably also the source pointer, so clean it up.
	 */
	bristolbzero(dest1, count * sizeof(float));

	/*
	 * Rotate the horn.
	 */
	rotate(lcl + sizeof(bristolLESLIElocal), param->param[1].mem,
		local2->tbuf, dest1, dest2, count, nSpeed,
		Feedback, Delay, Reverb);

	if (param->param[7].int_val == 3)
	{
		/*
		 * No rotation on the bass drum.
		 */
		tbuf = local->tbuf;
		for (i = 0; i < count; i++)
		{
			*dest1++ += *tbuf;
			*dest2++ += *tbuf++;
		}
	} else {
		if (param->param[7].int_val != 2)
			inertia = nSpeed * 1.13;
		else
			inertia = nSpeed;

		rotate(local, param->param[0].mem,
			local->tbuf, dest1, dest2, count, inertia,
			Feedback, Delay, Reverb);
	}

	local->nSpeed = nSpeed;
	return(0);
}

static void
iSnewValvify(register float *busData, register int count, register float valve)
{
	register float div, *busData2 = busData;

	if ((valve == 0) || (valve == 1))
		return;

	div = 1/valve;

#ifdef DEBUG
	printf("newValvify()\n");
#endif

	for (; count > 0; count--)
	{
		/*
		 * The upper output is flattened, and the lower output is extended to
		 * imitate logarithmic valve distortion. Gain levels are corrected to 
		 * prevent clipping where possible.
		 *
		 * The positive compression would be better emulated with a tanh()
		 * function.
		 */
		if (*busData > 0)
			*busData++ = *busData2++ * valve;
		else
			*busData++ = *busData2++ * div;
	}
}

static float lpf = 0; // need to bury in a local structure

static void
newValvify(register float *busData, register int count, register float valve)
{
	register int div, i;
	register float s;

#ifdef DEBUG
	printf("newValvify(%f)\n", valve);
#endif

	if ((div = (int) (valve * 3.0f)) >= 2)
		return;

	for (i = count; i > 0; i--, busData++)
	{
		 switch (div) {
		 	case 3:
		 	case 2:
				return;
		 	default:
				s = *busData * -0.00390625f + 8.0f;
				*busData = (s * s * s * 0.00390625f - 8.0f) * 256.0f;
				break;
		 	case 1:
				s = *busData * 0.00390625f + 8.0f;
				*busData = (s * s * 0.0625f - 8.0f) * 256.0f;
				break;
		}
		*busData -= (lpf += (*busData - lpf) * 0.0001);
	}
}

static void
buildSineTable(float *table)
{
	int i;

	for (i = 130; i < (TABSIZE + 130); i++)
		table[i - 130] = (float) sin(2 * M_PI * ((double) i) / TABSIZE);
}

/*
 * Setup any variables in our OP structure, in our IO structures, and malloc
 * any memory we need.
 */
bristolOP *
leslieinit(bristolOP **operator, int index, int samplerate, int samplecount)
{
	bristolLESLIE *specs;

	*operator = bristolOPinit(operator, index, samplecount);

#ifdef BRISTOL_DBG
	printf("leslieinit(%x(%x), %i, %i, %i)\n",
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

	specs = (bristolLESLIE *) bristolmalloc0(sizeof(bristolLESLIE));
	(*operator)->specs = (bristolOPSpec *) specs;
	(*operator)->size = sizeof(bristolLESLIE);

	/*
	 * These are specific to this operator, and will need to be altered for
	 * each operator.
	 */
	specs->spec.opname = OPNAME;
	specs->spec.description = OPDESCRIPTION;
	specs->spec.pcount = PCOUNT;
	specs->spec.iocount = IOCOUNT;
	specs->spec.localsize = sizeof(bristolLESLIElocal) * 2;

	/*
	 * Now fill in the specs for this operator.
	 */
	specs->spec.param[0].pname = "speed";
	specs->spec.param[0].description= "rotation speed";
	specs->spec.param[0].type = BRISTOL_FLOAT;
	specs->spec.param[0].low = 0;
	specs->spec.param[0].high = 1;
	specs->spec.param[0].flags = BRISTOL_ROTARY|BRISTOL_SLIDER;

	specs->spec.param[1].pname = "inertia";
	specs->spec.param[1].description = "inertia of rotation changes";
	specs->spec.param[1].type = BRISTOL_FLOAT;
	specs->spec.param[1].low = 0;
	specs->spec.param[1].high = 1;
	specs->spec.param[1].flags = BRISTOL_ROTARY|BRISTOL_SLIDER;

	specs->spec.param[2].pname = "delay";
	specs->spec.param[2].description = "delayed signal";
	specs->spec.param[2].type = BRISTOL_FLOAT;
	specs->spec.param[2].low = 0;
	specs->spec.param[2].high = 1;
	specs->spec.param[2].flags = BRISTOL_ROTARY|BRISTOL_SLIDER;

	specs->spec.param[3].pname = "reverb";
	specs->spec.param[3].description = "reverberation signal";
	specs->spec.param[3].type = BRISTOL_FLOAT;
	specs->spec.param[3].low = 0;
	specs->spec.param[3].high = 1;
	specs->spec.param[3].flags = BRISTOL_ROTARY|BRISTOL_SLIDER|BRISTOL_HIDE;

	specs->spec.param[4].pname = "tuning";
	specs->spec.param[4].description = "speaker reverberation";
	specs->spec.param[4].type = BRISTOL_FLOAT;
	specs->spec.param[4].low = 0;
	specs->spec.param[4].high = 1;
	specs->spec.param[4].flags = BRISTOL_ROTARY|BRISTOL_SLIDER|BRISTOL_HIDE;

	specs->spec.param[5].pname = "overdrive";
	specs->spec.param[5].description = "poweramp overdrive";
	specs->spec.param[5].type = BRISTOL_FLOAT;
	specs->spec.param[5].low = 0;
	specs->spec.param[5].high = 1;
	specs->spec.param[5].flags = BRISTOL_ROTARY|BRISTOL_SLIDER|BRISTOL_HIDE;

	specs->spec.param[6].pname = "crossover";
	specs->spec.param[6].description = "crossover frequency";
	specs->spec.param[6].type = BRISTOL_FLOAT;
	specs->spec.param[6].low = 0;
	specs->spec.param[6].high = 1;
	specs->spec.param[6].flags = BRISTOL_ROTARY|BRISTOL_SLIDER|BRISTOL_HIDE;

	specs->spec.param[7].pname = "operation";
	specs->spec.param[7].description = "off HornBass HornBassSync Horn";
	specs->spec.param[7].type = BRISTOL_ENUM;
	specs->spec.param[7].low = 0;
	specs->spec.param[7].high = 3;
	specs->spec.param[7].flags = BRISTOL_BUTTON;

	/*
	 * Now fill in the IO specs.
	 */
	specs->spec.io[0].ioname = "input";
	specs->spec.io[0].description = "rotary speak input signal";
	specs->spec.io[0].samplerate = samplerate;
	specs->spec.io[0].samplecount = samplecount;
	specs->spec.io[0].flags = BRISTOL_AC|BRISTOL_INPUT;

	specs->spec.io[1].ioname = "left output";
	specs->spec.io[1].description = "rotary speaker left channel output";
	specs->spec.io[1].samplerate = samplerate;
	specs->spec.io[1].samplecount = samplecount;
	specs->spec.io[1].flags = BRISTOL_AC|BRISTOL_OUTPUT;

	specs->spec.io[2].ioname = "right output";
	specs->spec.io[2].description = "rotary speaker right channel output";
	specs->spec.io[2].samplerate = samplerate;
	specs->spec.io[2].samplecount = samplecount;
	specs->spec.io[2].flags = BRISTOL_AC|BRISTOL_OUTPUT;

	buildSineTable(table);

	return(*operator);
}

