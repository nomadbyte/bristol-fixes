
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
 *	filterinit()
 *	operate()
 *	reset()
 *	destroy()
 *
 *	destroy() is in the library.
 *
 * Operate will be called when all the inputs have been loaded, and the result
 * will be an output buffer written to the next operator.
 *
 * This file includes the Colin Fletcher fkc factorisation optimisation, his
 * v2 factorisation and some other optimisation removing x, x1, x1, tanh1,
 * tanh2, kf and exp_out variables.
 */
/*
 * This implements three filter algorithms used by the different bristol
 * emulations. These are a butterworth used by the leslie, a chamberlain 
 * used generally, and a rooney used for some of the filter layering and
 * eventually the mixing.
 */
#include <math.h>

#include "bristol.h"
#include "bristolblo.h"
#include "filter.h"

/*
 * The name of this operator, IO count, and IO names.
 */
#define OPNAME "DCF"
#define OPDESCRIPTION "Digital Filter One"
#define PCOUNT 7
#define IOCOUNT 3

#define FILTER_IN_IND 0
#define FILTER_MOD_IND 1
#define FILTER_OUT_IND 2

static float srate;

/*
 * When this uses cfreq we get very good keyboard tracking as cfreq is a true
 * frequency mapping. This filter is supposed to be used with the cFreq tables
 * which do not have that same mapping - they are buffer strides rather than
 * frequency tables and this needs to be corrected here as the use of cfreq
 * causes cutoff jumps when glide is applied.
 *
 * cFreq = 1024 * cfreq / srate
 * cfreq = cFreq * srate / 1024
 * cfreq = cFreq * srate / 1024
 */
//#define getcoff(c, k) ((c*c)*(1.0f-k)*20000 + k*4*(voice->cFreq * srate / 1024)) / srate
//#define getcoff(c, k) ((c*c)*(1.0f-k)*20000.0f + voice->cfreq*k*srate*0.00390625f) / srate
//#define getcoff(c, k) ((c*c)*(1.0f-k)*20000.0f + k*4*voice->cfreq) / srate
#define getcoff(c, k) ((c*c)*(1.0f-k)*20000.0f + k*k*20000.0) / srate

//#define TANHF(x) btanhf(voice->baudio->mixflags, x)
#define TANHF(x) btanhf(mode, x)
//#define TANHF(v) (v + 1e-10f) / sqrtf(1 + v * v)
//#define TANHF(x) mode == 0? tanhf(x):0
#define TANHFEED(x) btanhfeed(mode, x)
//#define TANHFEED tanhf
//#define TANHFEED(v) (v + 1e-10f) / sqrtf(1 + v * v)
#define _f_lim (20000.0 / srate)

#define V2 40000.0
#define OV2 0.000025
#define F_RESAMPLE 88000

static float
btanhf(int mode, float v)
{
	/*
	 * This should be 4 modes as we have two bit flags. Mode 1 are the real
	 * lightweight chamberlain. That is more work on flag checking.
	 */
	switch (mode) {
		case 0: return(v);
		default:
		case 2: return((v + 1e-10f) / sqrtf(1 + v * v));
		case 3: return(tanhf(v));
	}
}

static float
btanhfeed(int mode, float v)
{
	/*
	 * This should be 4 modes as we have two bit flags. Mode 1 are the real
	 * lightweight chamberlain. That is more work on flag checking.
	 */
	switch (mode) {
		default:
		case 0:
		case 2: return((v + 1e-10f) / sqrtf(1 + v * v));
		case 3: return(tanhf(v));
	}
}

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
	bristolfree(operator->specs);

	/*
	 * Free any local memory. We should also free ourselves, since we did the
	 * initial allocation.
	 */
	cleanup(operator);
	return(0);
}

#define ROOT2 1.4142135623730950488
double pidsr;

/*
 * Reset any local memory information.
 */
static int reset(bristolOP *operator, bristolOPParams *param)
{
	float *a, c, d;

#ifdef BRISTOL_DBG
	printf("reset(%x)\n", operator);
#endif

	param->param[0].float_val = 0.5;
	param->param[1].float_val = 0.5;
	param->param[2].float_val = 1.0;
	param->param[3].int_val = 0;
	param->param[4].int_val = 0;
	param->param[5].float_val = 1.0;

	a = (float *) bristolmalloc(8 * sizeof(float));
	param->param[0].mem = a;

	pidsr = M_PI / sqrt(M_E);

	/*
	 * Configure a lowpass butterworth
	 */
	c = 1.0 / (float) tan((double) pidsr * 0.5);

	a[1] = 1.0 / (1.0 + ROOT2 * c + c * c);
	a[2] = 2 * a[1];
	a[3] = a[1];
	a[4] = 2.0 * (1.0 - c*c) * a[1];
	a[5] = (1.0 - ROOT2 * c + c * c) * a[1];

	a[6] = 0;
	a[7] = 0;

	/*
	 * Then configure a bandbass butterworth.
	 */
	a = (float *) bristolmalloc(8 * sizeof(float));
	param->param[1].mem = a;

	c = 1.0 / tan((double)(pidsr * 0.5));
	d = 2.0 * cos(2.0 * (double)(pidsr * 0.5));

	a[1] = 1.0 / (1.0 + c);
	a[2] = 0.0;
	a[3] = -a[1];
	a[4] = - c * d * a[1];
	a[5] = (c - 1.0) * a[1];

	a[6] = 0;
	a[7] = 0;

	return(0);
}

/*
 * Alter an internal parameter of an operator.
 */
static int
param(bristolOP *operator, bristolOPParams *param,
	unsigned char index, float value)
{
	int intvalue = value * CONTROLLER_RANGE;

#ifdef BRISTOL_DBG
	printf("param(%x, %f)\n", index, value);
#endif

	if (blo.flags & BRISTOL_LWF)
		param->param[4].int_val = 0;

	switch (index) {
		case 0:
		{
			if (param->param[4].int_val == 3) {
				param->param[index].float_val =
					gainTable[(int) (value * (CONTROLLER_RANGE - 1))].gain;
			} else if (param->param[4].int_val == 4) {
				param->param[index].float_val = value;
			} else if (param->param[4].int_val == 1) {
				/*param->param[index].float_val = */
				/*	gainTable[(int) (value * (CONTROLLER_RANGE - 1))].gain; */
				if ((param->param[index].float_val = value) < (float) 0.000122)
					param->param[index].float_val = (float) 0.000122;
			} else {
					param->param[index].float_val = value / 3;
			}
/*printf("Configuring filter %f\n", param->param[index].float_val); */
			param->param[index].int_val = intvalue;
			break;
		}
		case 1:
			if (param->param[4].int_val == 1)
				param->param[index].float_val =
					gainTable[CONTROLLER_RANGE - 1
						- (int) (value * (CONTROLLER_RANGE - 1))].gain;
			else
				param->param[index].float_val = value;
			param->param[index].int_val = intvalue;
			break;
		case 2:
		case 5: /* Gain */
			param->param[index].float_val = value;
			break;
		case 3:
			if (value > 0)
				param->param[index].int_val = 1;
			else
				param->param[index].int_val = 0;
			param->param[index].float_val = value;
			break;
		case 4:
			/* See if we are trying to avoid Huovilainen filters */
			if ((intvalue == 4) && (blo.flags & BRISTOL_LWF))
				intvalue = 0;
			if (intvalue > 1) {
				param->param[index].int_val = intvalue;
			} else if (value > 0) {
				float floatvalue = ((float) param->param[0].int_val) /
					(CONTROLLER_RANGE - 1);

				param->param[index].int_val = 1;
				/*
				printf("Selected rooney filter\n");
				 * We also need to rework a couple of the parameters when
				 * changing the filter selection
				 */
				if ((param->param[0].float_val = floatvalue)
					< (float) 0.000122)
					param->param[0].float_val = (float) 0.000122;

				floatvalue = ((float) param->param[1].int_val) /
					(CONTROLLER_RANGE - 1);

				param->param[1].float_val =
					gainTable[CONTROLLER_RANGE - 1
						- (int) (floatvalue * (CONTROLLER_RANGE - 1))].gain;
			} else {
				float floatvalue = ((float) param->param[0].int_val) /
					(CONTROLLER_RANGE - 1);

				printf("Selected chamberlain filter\n");
				param->param[index].int_val = 0;
				param->param[0].float_val = value / 3;

				floatvalue = ((float) param->param[1].int_val) /
					(CONTROLLER_RANGE - 1);
				param->param[1].float_val = value;
			}
			break;
		case 6: /* LP/BP/HP */
			param->param[index].int_val = intvalue;
			break;
	}

	return(0);
}

/*
 * Nice filter, but not with dynamic parameters for cutoff or mods. Use by the
 * leslie for the crossover between horns and voice coil.
 */
void butter_filter(register float *in, register float *out, register float *a,
register float gain, register int count) /* Filter loop */
{
    register float t, y;

    do {
      t = *in++ - a[4] * a[6] - a[5] * a[7];
      y = t * a[1] + a[2] * a[6] + a[3] * a[7];
      a[7] = a[6];
      a[6] = t;
      *out++ += y * gain;
    } while (--count);
}

/*
 * This looks odd being global however it is for denormal reduction, we inject
 * a stupidly small amount of noise into the houvilainen filter to give is some
 * constant signal. The noise algorithm will work over multiple voices with no
 * detrimental effect.
 */
static float scale = 0.00000000001f;
static int dngx1 = 0x67452301;
static int dngx2 = 0xefcdab89;

/*
 * filter - takes input signal and filters it according to the mod level.
 */
static int operate(register bristolOP *operator, bristolVoice *voice,
	bristolOPParams *param,
	void *lcl)
{
	bristolFILTERlocal *local = lcl;
	register int count, mode;
	register float *ib, *ob, *mb;
	bristolFILTER *specs;
	register float BLim, Res, Mod, gain;
	register float Bout = local->Bout;
	register float oSource = local->oSource;

	register float velocity, output, ccut, cvdelta, cutoff;

	mode = bfiltertype(voice->baudio->mixflags);

	/*
	 * Every operator accesses these variables, the count, and a pointer to
	 * each buffer. We should consider passing them as readymade parameters?
	 *
	 * The Filter now takes normalised inputs, in ranges of 12PO.
	 */
	specs = (bristolFILTER *) operator->specs;
	count = specs->spec.io[FILTER_OUT_IND].samplecount;
	ib = specs->spec.io[FILTER_IN_IND].buf;
	mb = specs->spec.io[FILTER_MOD_IND].buf;
	ob = specs->spec.io[FILTER_OUT_IND].buf;

#ifdef BRISTOL_DBG
	printf("filter(%x, %x, %x)\n", operator, param, local);
#endif

/*param->param[4].int_val = 5; */
/*if (mb == 0) */
/*return;

	if (blo.flags & BRISTOL_LWF)
		voice->baudio->mixflags |= BRISTOL_LW_FILTER;
 */

	/* See if we  are limited to lightweight filters */
	if (param->param[4].int_val != 3)
	{
		if ((blo.flags & BRISTOL_LWF) || (mode == 1))
			param->param[4].int_val = 0;
		else
			param->param[4].int_val = 4;
	}

	if (param->param[4].int_val == 1) {
		/*
		 * This is the code from one of the SLab floating point filter routines.
		 * It has been altered here to have a single lowpass filter, and will be
		 * a starting point for the Bristol filters. There will end up being a
		 * number of filter algorithms.
		 */

		if (param->param[3].int_val) {
			if ((BLim = param->param[0].float_val + voice->dFreq /
				(128 * param->param[3].float_val + 1)) > 1.0)
				BLim = 1.0;
			cutoff = param->param[0].float_val * voice->dFreq /
				(8 * param->param[3].float_val + 1);
			if ((Res = param->param[1].float_val * voice->dFreq / 
				(8 * param->param[3].float_val + 1)) == 0)
				Res = 0.000030;
			if (Res > 1.)
				Res = 1.0;
		} else {
			cutoff = BLim = param->param[0].float_val;
			Res = param->param[1].float_val;
		}
		if (voice->flags & BRISTOL_KEYON)
		{
			/*
			 * Do any relevant note_on initialisation.
			 */
			output = 0;
			velocity = 0;
			oSource = 0.0;
			Bout = 0.0;
		} else {
			output = local->output;
			velocity = local->velocity;
		}

		Mod = param->param[2].float_val;
		/*
		 * subtract a delta from original signal?
		 */
		for (; count > 0; count-=1)
		{
			/*
			 * Evaluate a resonant filter
			 */
			cvdelta = cutoff * Res;
			if ((ccut = (cutoff * (1 - Mod)) + *mb * Mod) < 0.001)
				ccut = 0.001;
			velocity += ((*ib - output) * cvdelta);
			if (velocity > ccut)
				velocity = ccut;
			if (velocity < -ccut)
				velocity = -ccut;
			output += velocity;

			/*
			 * Find out our current BLim. We have a specified limit, the
			 * cuttoff. We have our mod signal, and an amount of that mod
			 * signal. The ADSR produces a value between 0.0 and 1.0, so we
			 * could add this to BLim?
			 *
			 * Take the bass component using a rooney filter.
			 */
			if ((gain = (BLim * (1 - Mod) + (*mb++ * Mod))) > 1.0)
				gain = 1.0;
			else if (gain < 0.0)
				gain = 0.0;
			/*
			 * Cross them over. Rooneys are a lot cleaner.
			 */
			*ob++ = ((Bout += (*ib++ - Bout) * gain) * Res +
				output * (1 - Res) * 4) * (1.25 - BLim) * 4;
		}
		/*
		 * Put back the local variables
		 */
		local->Bout = Bout;
		local->oSource = oSource;
		local->output = output;
		local->velocity = velocity;
	} else if (param->param[4].int_val == 2) {
		/* Moog filter */
		register float fc, res, f, fb,
			in1 = local->delay1,
			in2 = local->delay2, 
			in3 = local->delay3,
			in4 = local->delay4,
			out1 = local->out1,
			out2 = local->out2, 
			out3 = local->out3,
			out4 = local->out4;

		Mod = param->param[2].float_val;

		res = param->param[1].float_val * 3.92f;
		fc = param->param[0].float_val;

		if (voice->flags & BRISTOL_KEYON)
			in1 = in2 = in3 = in4 = out1 = out2 = out3 = out4 = 0;

		for (; count > 0; count-=1)
		{
			if ((f = fc * 1.16 + *mb++ * Mod) > 1.0)
				f = 1.0;
			if (f < 0.0f)
				f = 0.000001;
			fb = res * (1.0 - 0.15 * f * f);

			*ib /= 12;

			*ib -= out4 * fb;
			*ib *= 0.35013 * (f*f)*(f*f);
			out1 = *ib + 0.3 * in1 + (1 - f) * out1; /* Pole 1 */
			in1  = *ib++;
			out2 = out1 + 0.3 * in2 + (1 - f) * out2;  /* Pole 2 */
			in2  = out1;
			out3 = out2 + 0.3 * in3 + (1 - f) * out3;  /* Pole 3 */
			in3  = out2;
			out4 = out3 + 0.3 * in4 + (1 - f) * out4;  /* Pole 4 */
			in4  = out3;

			*ob++ = out4 * 12;
		}

		local->delay1 = in1;
		local->delay2 = in2;
		local->delay3 = in3;
		local->delay4 = in4;
		local->out1 = out1;
		local->out2 = out2;
		local->out3 = out3;
		local->out4 = out4;
	} else if (param->param[4].int_val == 3) {
		/*
		 * Simplified rooney filter.
		 */
		BLim = param->param[0].float_val;
/*		BLim = gainTable[(int) (BLim * 3 * (CONTROLLER_RANGE - 1))].gain; */

		if (voice->flags & BRISTOL_KEYON)
			/*
			 * Do any relevant note_on initialisation.
			 */
			Bout = 0.0;

		/*
		 * subtract a delta from original signal?
		 */
		for (; count > 0; count-=4)
		{
			*ob++ = (Bout += ((*ib++ - Bout) * BLim));
			*ob++ = (Bout += ((*ib++ - Bout) * BLim));
			*ob++ = (Bout += ((*ib++ - Bout) * BLim));
			*ob++ = (Bout += ((*ib++ - Bout) * BLim));
		}

		/*
		 * Put back the local variables
		 */
		local->Bout = Bout;
	} else if ((param->param[4].int_val == 4) && (srate >= F_RESAMPLE)) {
		/*
		 * This is an implementation of Antti Huovilainen's non-linear Moog
		 * emulation, tweaked just slightly to align with the sometimes rather
		 * dubious bristol signal levels (its not that they are bad, just that
		 * historically they have used 1.0 per octave rather than +/-1.0).
		 *
		 * This one does not oversample - if the cutoff does not approach
		 * nyquist then we do not have to correct for its inaccuracies.
		 */
		float az1 = local->az1;
		float az2 = local->az2;
		float az3 = local->az3;
		float az4 = local->az4;
		float az5 = local->az5;
		float ay1 = local->ay1;
		float ay2 = local->ay2;
		float ay3 = local->ay3;
		float ay4 = local->ay4;
		float amf = local->amf;

//		float ov2 = 0.000025;// twice the 'thermal voltage of a transistor'

		float kfc;
		float kfcr;
		float kacr;
		float k2vg;
		float coff;

		float resonance = param->param[1].float_val;
		Mod = param->param[2].float_val * param->param[2].float_val * 0.03;

		/*
		 * Cutoff is a power curve for better control at lower frequencies and
		 * the key is used for tracking purposes, it should be possible to make
		 * it reasonably linear at somewhere under unity
		 *
		 * Cutoff goes from 0 to 1.0 = nyquist. Key tracking should be reviewed,
		 * the value should be quite linear for the filter, 0..Nyquist
		 *
		 * If we take midi key 0 = 8Hz and 127 = 12658.22 Hz then using
		 * our samplerate we should be able to fix some tuning:
		 *
		 * (voice->key.key * 12650.22 + 8) / (127 * srate)
		 *
		 * We want to position param[3] such that it tunes at 0.5 and can then
		 * be notched in the GUI, hence srate/4 rathern than /2.
		 *
		 * Needed some changes to key tracking, it was a bit bipolar.
		coff = (param->param[0].float_val * param->param[0].float_val)
			* (1.0f - param->param[3].float_val)
			* 20000 / srate
				+
			param->param[3].float_val * 4 * voice->cfreq / srate;
		coff = getcoff(param->param[0].float_val, param->param[3].float_val);
		 */
		if (param->param[3].float_val == 0)
			coff = param->param[0].float_val * param->param[0].float_val
				* 20000 / srate;
		else
			coff = param->param[3].float_val * param->param[0].float_val
				* 4 * voice->cfreq / srate;

		 /*
		if (param->param[3].int_val == 0)
			coff = param->param[0].float_val * param->param[0].float_val
				* 20000 / srate;
		else // Fraction of the current frequency by tracking by cutoff.
			coff = param->param[3].float_val * param->param[0].float_val * 4 *
				voice->cfreq / srate;
		 */

//			* voice->cfreq * param->param[3].float_val / srate;
//printf("%f %f %f\n", voice->cfreq, coff, param->param[3].float_val);

		float lim = _f_lim;

		for (; count > 0; count--)
		{
			/*
			 * The 0.83 is arbitrary, it is used to prevent the filter being
			 * overdriven however I think 0.5 would be a better value seeing
			 * as the filter is 2x oversampling?
			 *
			 * We should really interpret coff (the configured frequency) as
			 * a function up to about 20KHz whatever the resampling rate.
			 */
			if ((kfc = coff + *mb++ * Mod) > lim)
				kfc = lim;
			else if (kfc < 1e-10f)
				kfc = 1e-10f;

			// frequency & amplitude correction
			kfcr = kfc * ( kfc * (1.8730 * kfc + 0.4955) - 0.6490) + 0.9988;
			kacr = kfc * (-3.9364 * kfc + 1.8409) + 0.9968;

			k2vg = (1 - expf(-2.0 * M_PI * kfcr * kfc));

			// cascade of 4 1st order sections
			dngx1 ^= dngx2;
			ay1  = az1 + k2vg * (TANHFEED((*ib + dngx2 * scale) * OV2
				- 4*resonance*amf*kacr) - TANHF(az1));
			dngx2 += dngx1;
			az1  = ay1;

			ay2  = az2 + k2vg *(TANHF(ay1) -TANHF(az2));
			az2  = ay2;

			ay3  = az3 + k2vg * (TANHF(ay2) - TANHF(az3));
			az3  = ay3;

			ay4  = az4 + k2vg * (TANHF(ay3) - TANHF(az4));
			az4  = ay4;

			amf  = ay4;

			*ob++ += amf * V2;
			ib++;
		}

		local->az1 = az1;
		local->az2 = az2;
		local->az3 = az3;
		local->az4 = az4;
		local->az5 = az5;
		local->ay1 = ay1;
		local->ay2 = ay2;
		local->ay3 = ay3;
		local->ay4 = ay4;
		local->amf = amf;
	} else if ((param->param[4].int_val == 4) && (srate < F_RESAMPLE)) {
		/*
		 * This is an implementation of Antti Huovilainen's non-linear Moog
		 * emulation, tweaked just slightly to align with the sometimes rather
		 * dubious bristol signal levels (its not that they are bad, just that
		 * historically they have used 1.0 per octave rather than +/-1.0).
		 */
		float az1 = local->az1;
		float az2 = local->az2;
		float az3 = local->az3;
		float az4 = local->az4;
		float az5 = local->az5;
		float ay1 = local->ay1;
		float ay2 = local->ay2;
		float ay3 = local->ay3;
		float ay4 = local->ay4;
		float amf = local->amf;

//		float ov2 = 0.000025;// twice the 'thermal voltage of a transistor'

		float kfc;
		float kfcr;
		float kacr;
		float k2vg;
		float coff;

		float resonance = param->param[1].float_val;
		Mod = param->param[2].float_val * param->param[2].float_val * 0.02;

		/*
		 * Cutoff is a power curve for better control at lower frequencies and
		 * the key is used for tracking purposes, it should be possible to make
		 * it reasonably linear at somewhere under unity
		 *
		 * Cutoff goes from 0 to 1.0 = nyquist. Key tracking should be reviewed,
		 * the value should be quite linear for the filter, 0..Nyquist
		 *
		 * If we take midi key 0 = 8Hz and 127 = 12658.22 Hz then using
		 * our samplerate we should be able to fix some tuning:
		 *
		 * (voice->key.key * 12650.22 + 8) / (127 * srate)
		 *
		 * We want to position param[3] such that it tunes at 0.5 and can then
		 * be notched in the GUI, hence srate/4 rathern than /2.
		 *
		 * Needed some changes to key tracking, it was a bit bipolar.
		coff = (param->param[0].float_val * param->param[0].float_val)
			* (1.0f - param->param[3].float_val)
			* 20000 / srate
				+
			param->param[3].float_val * 4 * voice->cfreq / srate;
		coff = getcoff(param->param[0].float_val, param->param[3].float_val);
		 */
		if (param->param[3].float_val == 0)
			coff = param->param[0].float_val * param->param[0].float_val
				* 20000 / srate;
		else
			coff = param->param[3].float_val * param->param[0].float_val
				* 4 * voice->cfreq / srate;

		 /*
		if (param->param[3].int_val == 0)
			coff = param->param[0].float_val * param->param[0].float_val
				* 20000 / srate;
		else // Fraction of the current frequency by tracking by cutoff.
			coff = param->param[3].float_val * param->param[0].float_val * 4 *
				voice->cfreq / srate;
		 */

//			* voice->cfreq * param->param[3].float_val / srate;
//printf("%f %f %f\n", voice->cfreq, coff, param->param[3].float_val);

		for (; count > 0; count--)
		{
			/*
			 * The 0.83 is arbitrary, it is used to prevent the filter being
			 * overdriven however I think 0.5 would be a better value seeing
			 * as the filter is 2x oversampling?
			 *
			 * We should really interpret coff (the configured frequency) as
			 * a function up to about 20KHz whatever the resampling rate.
			 */
			if ((kfc = coff + *mb++ * Mod) > 0.5)
				kfc = 0.5;
			else if (kfc < 1e-10f)
				kfc = 1e-10f;

			// frequency & amplitude correction
			kfcr = kfc * ( kfc * (1.8730 * kfc + 0.4955) - 0.6490) + 0.9988;
			kacr = kfc * (-3.9364 * kfc + 1.8409) + 0.9968;

			// filter tuning
			k2vg = (1 - expf(-2.0 * M_PI * kfcr * kfc * 0.5));

			// cascade of 4 1st order sections
			ay1  = az1 + k2vg * (TANHFEED((*ib*OV2 - 4*resonance*amf*kacr))
				- TANHF(az1));
			az1  = ay1;

			ay2  = az2 + k2vg * (TANHF(ay1) - TANHF(az2));
			az2  = ay2;

			ay3  = az3 + k2vg * (TANHF(ay2) - TANHF(az3));
			az3  = ay3;

			ay4  = az4 + k2vg * (TANHF(ay3) - TANHF(az4));
			az4  = ay4;

			// 1/2-sample delay for phase compensation
			amf  = (ay4+az5)*0.5;
			az5  = ay4;

			// oversampling (repeat same block) and inject noise for denormals
			dngx1 ^= dngx2;
			ay1  = az1 + k2vg * (TANHFEED((*ib +dngx2 * scale) * OV2
				- 4*resonance*amf*kacr) - TANHF(az1));
			dngx2 += dngx1;
			az1  = ay1;

			ay2  = az2 + k2vg * (TANHF(ay1) - TANHF(az2));
			az2  = ay2;

			ay3  = az3 + k2vg * (TANHF(ay2) - TANHF(az3));
			az3  = ay3;

			ay4  = az4 + k2vg * (TANHF(ay3) - TANHF(az4));
			az4  = ay4;

			// 1/2-sample delay for phase compensation
			amf  = (ay4+az5)*0.5;
			az5  = ay4;

			*ob++ += amf * V2 * 0.5;
			ib++;
		}

		local->az1 = az1;
		local->az2 = az2;
		local->az3 = az3;
		local->az4 = az4;
		local->az5 = az5;
		local->ay1 = ay1;
		local->ay2 = ay2;
		local->ay3 = ay3;
		local->ay4 = ay4;
		local->amf = amf;

	} else {
		/* The chamberlain */
		register float freqcut, highpass, qres,
			delay1 = local->delay1,
			delay2 = local->delay2, 
			delay3 = local->delay3,
			delay4 = local->delay4;
		register int hp = param->param[6].int_val;

		Mod = param->param[2].float_val;

		if (voice->flags & BRISTOL_KEYON)
		{
			delay1 = 0;
			delay2 = 0;
			delay3 = 0;
			delay4 = 0;
		}

		qres = 2.0f - param->param[1].float_val * 1.97f;
		cutoff = param->param[0].float_val;
		gain = param->param[5].float_val * 0.02513;

		/*
		 * The following was for on/off keytracking, needs to be continuous.
		if (param->param[3].int_val)
			cutoff += voice->dFreq / 128.0f;
		 */
		cutoff += param->param[3].float_val * voice->key.key / 512.0f;

		for (; count > 0; count-=1)
		{
			/*
			 * Hal Chamberlin's state variable filter. These are cascaded low
			 * pass (rooney type) filters with feedback. They are not bad - 
			 * they are resonant with a reasonable dB/octave, but the
			 * resonance is not as warm as an analogue equivalent.
			 */
			freqcut = cutoff * 2.0f + *mb++ * Mod / 12.0f;

			if (freqcut > VCF_FREQ_MAX)
				freqcut = VCF_FREQ_MAX;
			else if (freqcut <= 0.000001)
				freqcut = 0.000001;

			/* delay2/4 = lowpass output */
			delay2 = delay2 + freqcut * delay1;

			highpass = *ib++ * 16 - delay2 - qres * delay1;

			/* delay1/3 = bandpass output */
			delay1 = freqcut * highpass + delay1;

			delay4 = delay4 + freqcut * delay3;
			highpass = delay2 - delay4 - qres * delay3;
			delay3 = freqcut * highpass + delay3;

			/* mix filter output into output buffer */
			switch (hp) {
				default: /* LP */
					*ob++ += delay4 * gain;
					break;
				case 1: /* BP */
					*ob++ += delay3 * gain;
					break;
				case 2: /* HP */
					*ob++ += highpass * gain;
					break;
			}
		}

		local->delay1 = delay1;
		local->delay2 = delay2;
		local->delay3 = delay3;
		local->delay4 = delay4;

	}

	return(0);
}

/*
 * Setup any variables in our OP structure, in our IO structures, and malloc
 * any memory we need.
 */
bristolOP *
filterinit(bristolOP **operator, int index, int samplerate, int samplecount)
{
	bristolFILTER *specs;

#ifdef BRISTOL_DBG
	printf("filterinit(%x(%x), %i, %i, %i)\n",
		operator, *operator, index, samplerate, samplecount);
#endif

	srate = samplerate;

	*operator = bristolOPinit(operator, index, samplecount);

	/*
	 * Then the local parameters specific to this operator. These will be
	 * the same for each operator, but must be inited in the local code.
	 */
	(*operator)->operate = operate;
	(*operator)->destroy = destroy;
	(*operator)->reset = reset;
	(*operator)->param = param;

	specs = (bristolFILTER *) bristolmalloc0(sizeof(bristolFILTER));
	(*operator)->specs = (bristolOPSpec *) specs;
	(*operator)->size = sizeof(bristolFILTER);

	/*
	 * These are specific to this operator, and will need to be altered for
	 * each operator.
	 */
	specs->spec.opname = OPNAME;
	specs->spec.description = OPDESCRIPTION;
	specs->spec.pcount = PCOUNT;
	specs->spec.iocount = IOCOUNT;
	specs->spec.localsize = sizeof(bristolFILTERlocal);

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

	specs->spec.param[4].pname = "filter type";
	specs->spec.param[4].description = "Chamberlain or Rooney filter";
	specs->spec.param[4].type = BRISTOL_TOGGLE;
	specs->spec.param[4].low = 0;
	specs->spec.param[4].high = 1;
	specs->spec.param[4].flags = BRISTOL_BUTTON;

	specs->spec.param[5].pname = "gain";
	specs->spec.param[5].description = "Filter gain";
	specs->spec.param[5].type = BRISTOL_FLOAT;
	specs->spec.param[5].low = 0;
	specs->spec.param[5].high = 1;
	specs->spec.param[5].flags = BRISTOL_ROTARY|BRISTOL_SLIDER;

	specs->spec.param[6].pname = "LP/BP/HP";
	specs->spec.param[6].description = "Low pass or high pass (type-0 only)";
	specs->spec.param[6].type = BRISTOL_INT;
	specs->spec.param[6].low = 0;
	specs->spec.param[6].high = 2;
	specs->spec.param[6].flags = BRISTOL_ROTARY|BRISTOL_SLIDER;

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

