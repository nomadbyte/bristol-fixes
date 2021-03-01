
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
#include "hammondchorus.h"

#define DELAY		0
#define FILTER		1
#define DEPTH		2
#define GAIN		3
#define VC			4

#define OPNAME "HammondChorus"
#define OPDESCRIPTION "Hammond Chorus"
#define PCOUNT 6
#define IOCOUNT 2

#define HCHORUS_IN_IND 0
#define HCHORUS_OUT_IND 1

int scanrate = 172;

static void fillGainTable(float *, int);
static void fillDrainTable(float *, int);
float *upgain = NULL;
float *downgain = NULL;

/*
 * Three different taps rates for V1/V2/V3, fast and shallow to slow and deep,
 * then we mix them for chorus. They are also staggered to reduce matches and
 * there is a null (non moving) tap for some of the depths.
int atap[32]= {0,0,1,1,2,2,3,3,4,4,5,5,6,6,7,7,8,8,7,7,6,6,5,5,4,4,3,3,2,2,1,1};
int btap[32]= {0,1,1,2,2,3,3,4,4,5,5,6,6,7,7,8,8,7,7,6,6,5,5,4,4,3,3,2,2,1,1,0};
 */
int atap[32]= {0,1,2,3,4,5,6,7,8,7,6,5,4,3,2,1,0,1,2,3,4,5,6,7,8,7,6,5,4,3,2,1};
int btap[32]= {1,2,3,4,5,6,7,8,7,6,5,4,3,2,1,0,1,2,3,4,5,6,7,8,7,6,5,4,3,2,1,0};

int ctap[32]= {3,3,2,2,1,1,0,0,1,1,2,2,3,3,4,4,3,3,2,2,1,1,0,0,1,1,2,2,3,3,4,4};
int dtap[32]= {3,2,2,1,1,0,0,1,1,2,2,3,3,4,4,3,3,2,2,1,1,0,0,1,1,2,2,3,3,4,4,3};

int etap[32]= {4,4,5,5,6,6,7,7,8,8,7,7,6,6,5,5,4,4,5,5,6,6,7,7,8,8,7,7,6,6,5,5};
int ftap[32]= {4,5,5,6,6,7,7,8,8,7,7,6,6,5,5,4,4,5,5,6,6,7,7,8,8,7,7,6,6,5,5,4};

int ztap[32]= {7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7};

//int *tap1, *tap2, *tap3, *tap4, *tap5, *tap6;
int *tap1 = NULL, *tap2 = NULL, *tap3 = NULL;
int *tap4 = NULL, *tap5 = NULL, *tap6 = NULL;

//float tapgain[TAPS] = {1.5, 2, 2.5, 3.0, 3.5, 4, 4.5, 5, 5.5};
/* As the filtering goes up the signal gain goes down, so a small correction */
//float tapgain[TAPS] = {1.0, 1.03, 1.06, 1.09, 1.12, 1.15, 1.18, 1.21, 1.24};
float tapgain[TAPS] = {1.0, 1.01, 1.02, 1.03, 1.04, 1.05, 1.06, 1.07, 1.08};

/* float tapfilt[TAPS] = {0.95, 0.9, 0.85, 0.8, 0.75, 0.7, 0.65, 0.6, 0.55}; */
/* These get reset anyway, the are a cummulative multiplication */
float tapfilt[TAPS] = {0.99, 0.98, 0.97, 0.96, 0.95, 0.94, 0.93, 0.92, 0.91};

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

static int reDelay = 0;

/*
 * This is called by the frontend when a parameter is changed.
static int param(param, value)
 */
static int param(bristolOP *operator, bristolOPParams *param,
	unsigned char index, float value)
{
	int i;
#ifdef DEBUG
	printf("hvcParams(%i, %f)\n", index, value);
#endif

	/*
	 * We need few parameters. Speed will be a value in sample for which we
	 * linger on any given tap, and Phase will control the lag of the LC phase
	 * shifting filter. May add a parameter for the duration of any crossover
	 * from one tap to the next.
	 */
	switch (index) {
		case 0:
			/*
			 * See if this really is a new value - we have to dump our history
			 * memory when it changes so if it can be avoided then better
			 */
			value = value * 6 + 1;

			/*
			 * Slight delay line is used to emphasise the RC delay line.
			if ((param->param[index].int_val = value * HISTSIZE) <= 0)
				param->param[index].int_val = 1;
			 *
			 * Since large values sound pretty gruesome we are going to firstly
			 * limit it to some small range, 16 samples at the moment. We also
			 * need to consider using some resampling as that would improve the
			 * sonic range.
printf("DL: %i/%0.4f\n",
param->param[index].int_val, param->param[index].float_val);
			 */
			reDelay = 1;
			param->param[index].float_val = value;
			switch (param->param[DEPTH].int_val) {
				case 1:
					if ((param->param[0].int_val
						= (param->param[0].float_val * 0.5)) < 2)
						param->param[0].int_val = 2;
					break;
				case 2:
					if ((param->param[0].int_val
						= 1 + (param->param[0].float_val)) < 3)
						param->param[0].int_val = 3;
					break;
				case 3:
					if ((param->param[0].int_val
						= 1 + (param->param[0].float_val * 2)) < 4)
						param->param[0].int_val = 4;
					break;
			}
			break;
		case 1:
			/*
			 * LC lag
			 */
			param->param[index].float_val = value;
			tapfilt[0] = value;
			for (i = 1; i < TAPS; i++)
				tapfilt[i] = tapfilt[i-1] * value;
			/*
			tapgain[0] = 1.0 / value;
			for (i = 1; i < TAPS; i++)
				tapgain[i] = tapgain[i-1] / value;
			*/
			break;
		case DEPTH:
			/*
			 * VibraChorus.
			 */
			param->param[index].int_val = value * CONTROLLER_RANGE;

			switch (param->param[index].int_val) {
				case 1:
					param->param[index].float_val = 0.10;
					tap1 = atap;
					tap2 = btap;
					tap3 = ztap;
					tap4 = ztap;
					tap5 = ztap;
					tap6 = ztap;
					reDelay = 1;
					if ((param->param[0].int_val
						= (param->param[0].float_val * 0.5)) < 2)
						param->param[0].int_val = 2;
					break;
				case 2:
					param->param[index].float_val = 0.95;
					tap1 = etap;
					tap2 = ftap;
					tap3 = ztap;
					tap4 = ztap;
					tap5 = atap;
					tap6 = btap;
					reDelay = 1;
					if ((param->param[0].int_val
						= 1 + (param->param[0].float_val)) < 3)
						param->param[0].int_val = 3;
					break;
				case 3:
					param->param[index].float_val = 0.90;
					tap1 = etap;
					tap2 = ftap;
					tap3 = ctap;
					tap4 = dtap;
					tap5 = atap;
					tap6 = btap;
					reDelay = 1;
					if ((param->param[0].int_val
						= 1 + (param->param[0].float_val * 2)) < 3)
						param->param[0].int_val = 3;
					break;
			}
			return(0);
			switch (param->param[index].int_val) {
				case 1:
					param->param[index].float_val = 0.99;
					tap1 = ctap;
					tap2 = dtap;
					tap3 = atap;
					tap4 = btap;
					tap5 = ztap;
					tap6 = ztap;
					break;
				case 2:
					param->param[index].float_val = 0.95;
					tap1 = ctap;
					tap2 = dtap;
					tap3 = atap;
					tap4 = btap;
					tap5 = ztap;
					tap6 = ztap;
					break;
				case 3:
					param->param[index].float_val = 0.90;
					tap1 = etap;
					tap2 = ftap;
					tap3 = ctap;
					tap4 = dtap;
					tap5 = atap;
					tap6 = btap;
					break;
			}
			break;
		case 3:
			/*
			 * 3 is gain.
			 */
			param->param[index].float_val = value;
			break;
		case 4:
			/*
			 * VC
			 */
			param->param[index].int_val = value * CONTROLLER_RANGE;
			break;
		case 5:
			scanrate = 65 + (1.0 - value) * 256;
			if (upgain == NULL)
				break;

			fillGainTable(upgain, scanrate);
			fillDrainTable(downgain, scanrate);

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

	param->param[0].mem = bristolmalloc0(sizeof(float) * MEMSIZE);

	param->param[0].int_val = 1024;
	param->param[1].float_val = 0.1;
	param->param[2].int_val = 1;
	param->param[3].float_val = 0.8;
	return(0);
}

/*
 * This was derived from the bristol vibra chorus. The original was just a 
 * variable length resampled delay line. It worked, but it did not have the
 * real depth of the hammond chorus.
 *
 * This implementation will be closer to the original, implementing 8 cascaded
 * LC filters which introduce a phase and frequency change, and a tap that 
 * links to each for a period of time. The result will pass through another
 * final filter to remove pops as the 'rotor' crosses between taps. May consider
 * crossing them over smoothly depending on the results.
 *
 * Worked quite well except that the filtering effects are deeper than the 
 * phasing effects due to the filter characteristics - there is a lot more 
 * filtering than phase shifting.
 *
 * To improve it we are going to use 8 cascaded delay lines with a fixed delay
 * and change the filtering out of each tap to emulate the Hammond cascade.
 */
static int operate(bristolOP *operator,
	bristolVoice *voice,
	bristolOPParams *param,
	void *lcl)
{
	bristolHCHORUS *specs;
	bristolHCHORUSlocal *local = (bristolHCHORUSlocal *) lcl;
	register float *source, *dest;
	register int count, i, j;
	register int rate, chorus, tap, tcount;
	register float g1, g2, g3, g4, g5, g6, gain, *history, ph, rs, depth;
	register bristolHCTap *phase;

	specs = (bristolHCHORUS *) operator->specs;

	count = specs->spec.io[HCHORUS_OUT_IND].samplecount;
	source = specs->spec.io[HCHORUS_IN_IND].buf;
	dest = specs->spec.io[HCHORUS_OUT_IND].buf;

	/*
	 * operational parameters.
	 *
	 * Rate just defines the delay of the phases.
	 */
	rate = param->param[DELAY].int_val;
	rs = param->param[DELAY].float_val;
	chorus = param->param[VC].int_val;
	gain = param->param[GAIN].float_val;
	depth = param->param[DEPTH].float_val;

	history = param->param[0].mem;

	if (reDelay)
	{
		/*
		 * Rate has changed so also need to rework the delay line taps.
		 */
		reDelay = 0;
		for (i = 0; i < TAPS; i++)
		{
			local->mcur[i] = local->mstart[i] = (i + 1) * rate;
			local->mend[i] = local->mstart[i] + rate;
		}
		local->mcur[TAPS] = 0;
		ph = *source;
	} else
		ph = local->ph;

#ifdef DEBUG
	printf("hammondchorus(%i, %1.2f, %i)\n", rate, chorus);
#endif

	tap = local->ctap;
	phase = &local->phase[0];
	tcount = local->tcount;
	g1 = local->g1;
	g2 = local->g2;
	g3 = local->g3;
	g4 = local->g4;
	g5 = local->g5;
	g6 = local->g6;

	if (tap1 == NULL)
		return(0);

//printf("hammondchorus(%i, %i): %i, %i\n",
//rate, chorus, tcount, tap);

	/*
	 * We now need to take the input signal and separate it out into several
	 * stages. The original had 8 stages of delay, and that is emulated here,
	 * however it also contained a capacitive receiver that scanned each tap,
	 * and that introduced some smoothing between the phases. That is more
	 * work to emulate so we will use an extra tap and more phases such that
	 * some phases will be crossover between taps.
	 */
	for (i = 0; i < count; i++)
	{
		for (j = 0; j <= TAPS; j++)
		{
			if (--local->mcur[j] < 0)
				local->mcur[j] = MEMSIZE - 1;

			phase[j].out +=
				(history[local->mcur[j]] - phase[j].out) * tapfilt[j] * depth;
		}
		/*
		 * This is resampling but it does not currently work
		history[local->mcur[TAPS]] = *source * rs + ph * (1.0 - rs);
		history[local->mcur[TAPS]] = rs * (*source - ph) + ph;
		 */
		history[local->mcur[TAPS]] = *source;

		/*
		 * See if we need to move the tap forward. We seem to have 32 taps but
		 * that is not the case. There are 8 taps and 32 phases, the extra
		 * phases being used to crossover the signal to emulate the capacitive
		 * armature.
		 */
		if (++tcount >= scanrate) {
			tcount = 0;

			/*
			 * If we have been on this tap for a sufficient number of samples
			 * then move on.
			 */
			if (++tap >= 32)
				tap = 0;

			if (chorus) {
				g1 = tapgain[tap1[tap]] * (1.0 - gain);
				g2 = tapgain[tap2[tap]] * (1.0 - gain);
				g3 = tapgain[tap3[tap]] * (1.0 - gain);
				g4 = tapgain[tap4[tap]] * (1.0 - gain);
				g5 = tapgain[tap5[tap]] * (1.0 - gain);
				g6 = tapgain[tap6[tap]] * (1.0 - gain);
			} else {
				g1 = tapgain[tap1[tap]];
				g2 = tapgain[tap2[tap]];
			}
		}

		if (chorus) {
			*dest++ = *source * gain
				+ (phase[tap1[tap]].out * downgain[tcount] * g1
				+ phase[tap2[tap]].out * upgain[tcount] * g2
				+ phase[tap3[tap]].out * downgain[tcount] * g3
				+ phase[tap4[tap]].out * upgain[tcount] * g4
				+ phase[tap5[tap]].out * downgain[tcount] * g5
				+ phase[tap6[tap]].out * upgain[tcount] * g6) * 0.5f;
		} else {
			*dest++ =
				(phase[tap1[tap]].out * downgain[tcount] * g1
				+ phase[tap2[tap]].out * upgain[tcount] * g2) * 0.7f;
		}

//		*dest++ = *source;

		ph = *(source++);
	}

	local->ctap = tap;
	local->tcount = tcount;
	local->g1 = g1;
	local->g2 = g2;
	local->g3 = g3;
	local->g4 = g4;
	local->g5 = g5;
	local->g6 = g6;
	local->ph = ph;

	return(0);
}

/*
 * This was derived from the bristol vibra chorus. The original was just a 
 * variable length resampled delay line. It worked, but it did not have the
 * real depth of the hammond chorus.
 *
 * This implementation will be closer to the original, implementing 8 cascaded
 * LC filters which introduce a phase and frequency change, and a tap that 
 * links to each for a period of time. The result will pass through another
 * final filter to remove pops as the 'rotor' crosses between taps. May consider
 * crossing them over smoothly depending on the results.
 *
 * Works quite well except that the filtering effects are deeper than the 
 * phasing effects due to the filter characteristics - there is a lot more 
 * filtering than phase shifting.
 */
static void
fillGainTable(float table[], int count)
{
	int i;
	float CROSS = 0.5f * M_PI / ((float) count);

	memset(table, 1.0f, 2048 * sizeof(float));
	for (i = 0; i < count; i++)
		table[i] = sinf(i * CROSS);
}
static void
fillDrainTable(float table[], int count)
{
	int i;
	float CROSS = 0.5f * M_PI / ((float) count);

	memset(table, 0.0f, 2048 * sizeof(float));
	for (i = 0; i < count; i++)
		table[i] = cosf(i * CROSS);
}

/*
 * Setup any variables in our OP structure, in our IO structures, and malloc
 * any memory we need.
 */
bristolOP *
hammondchorusinit(bristolOP **operator, int index, int samplerate, int samplecount)
{
	bristolHCHORUS *specs;

	*operator = bristolOPinit(operator, index, samplecount);

#ifdef BRISTOL_DBG
	printf("hammondchorusinit(%x(%x), %i, %i, %i)\n",
		operator, *operator, index, samplerate, samplecount);
#endif

	/*
	 * We only support a single scanning rate, 416 rpm which I think is the
	 * original rate give or take a few rpm. This translates to a scan rate
	 * through our 8 taps and back as follows. The use of 32 steps may look a
	 * bit odd, but we actually cross each tap a couple of times fading through
	 * to the next tap so the 8 forward stages and same 8 stages in reverse
	 * with two scans of each for the crossover gives us 32 stages.
	 */
	scanrate = samplerate * 60 / 416 / 32;

	/*
	 * Then the local parameters specific to this operator. These will be
	 * the same for each operator, but must be inited in the local code.
	 */
	(*operator)->operate = operate;
	(*operator)->destroy = destroy;
	(*operator)->reset = reset;
	(*operator)->param= param;

	specs = (bristolHCHORUS *) bristolmalloc0(sizeof(bristolHCHORUS));
	(*operator)->specs = (bristolOPSpec *) specs;
	(*operator)->size = sizeof(bristolHCHORUS);

	upgain = (float *) bristolmalloc0(sizeof(float) * 2048);
	downgain = (float *) bristolmalloc0(sizeof(float) * 2048);

	fillGainTable(upgain, scanrate);
	fillDrainTable(downgain, scanrate);

	/*
	 * These are specific to this operator, and will need to be altered for
	 * each operator.
	 */
	specs->spec.opname = OPNAME;
	specs->spec.description = OPDESCRIPTION;
	specs->spec.pcount = PCOUNT;
	specs->spec.iocount = IOCOUNT;
	specs->spec.localsize = sizeof(bristolHCHORUSlocal);

	/*
	 * Now fill in the specs for this operator.
	 */
	specs->spec.param[0].pname = "Delay";
	specs->spec.param[0].description= "Tap delay";
	specs->spec.param[0].type = BRISTOL_FLOAT;
	specs->spec.param[0].low = 0;
	specs->spec.param[0].high = 1;
	specs->spec.param[0].flags = BRISTOL_ROTARY|BRISTOL_SLIDER;

	specs->spec.param[1].pname = "Damping";
	specs->spec.param[1].description = "Tap signal damping (rc filter)";
	specs->spec.param[1].type = BRISTOL_FLOAT;
	specs->spec.param[1].low = 0;
	specs->spec.param[1].high = 1;
	specs->spec.param[1].flags = BRISTOL_ROTARY|BRISTOL_SLIDER;

	specs->spec.param[2].pname = "Vibra";
	specs->spec.param[2].description = "Depth of modulation";
	specs->spec.param[2].type = BRISTOL_INT;
	specs->spec.param[2].low = 0;
	specs->spec.param[2].high = 1;
	specs->spec.param[2].flags = BRISTOL_BUTTON;

	specs->spec.param[3].pname = "Gain";
	specs->spec.param[3].description = "Wet/Dry signal mix";
	specs->spec.param[3].type = BRISTOL_INT;
	specs->spec.param[3].low = 0;
	specs->spec.param[3].high = 1;
	specs->spec.param[3].flags = BRISTOL_BUTTON;

	specs->spec.param[4].pname = "VC";
	specs->spec.param[4].description = "Vibra/Chorus";
	specs->spec.param[4].type = BRISTOL_INT;
	specs->spec.param[4].low = 0;
	specs->spec.param[4].high = 1;
	specs->spec.param[4].flags = BRISTOL_BUTTON;

	specs->spec.param[5].pname = "Rate";
	specs->spec.param[5].description = "Rotor speed";
	specs->spec.param[5].type = BRISTOL_FLOAT;
	specs->spec.param[5].low = 0;
	specs->spec.param[5].high = 1;
	specs->spec.param[5].flags = BRISTOL_ROTARY|BRISTOL_SLIDER;

	/*
	 * Now fill in the IO specs.
	 */
	specs->spec.io[0].ioname = "input";
	specs->spec.io[0].description = "Input signal";
	specs->spec.io[0].samplerate = samplerate;
	specs->spec.io[0].samplecount = samplecount;
	specs->spec.io[0].flags = BRISTOL_AC|BRISTOL_INPUT;

	specs->spec.io[1].ioname = "output";
	specs->spec.io[1].description = "output signal";
	specs->spec.io[1].samplerate = samplerate;
	specs->spec.io[1].samplecount = samplecount;
	specs->spec.io[1].flags = BRISTOL_AC|BRISTOL_OUTPUT;

	return(*operator);
}

