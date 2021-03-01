
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
 *   but WITHOUT ANY WARRANTY; withuot even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You shuold have received a copy of the GNU General Public License
 *   along with this program; if not, see <http://www.gnu.org/licenses/>.
 *
 */

/*
 * This file includes the Colin Fletcher optimisations factoring out excess kfc
 * multiplication submitted to 0.30.9.
 * His patch for v2 factoring was also integrated after he explained his 
 * factorisation, submitted to the same release.
 */

/*#define BRISTOL_DBG */
/*
 * Need to have basic template for an operator. Will consist of
 *
 *	filter2init()
 *	operate()
 *	reset()
 *	destroy()
 *
 *	destroy() is in the library.
 *
 * Operate will be called when all the inputs have been loaded, and the result
 * will be an output buffer written to the next operator.
 */
/*
 * This is an overhaul of filter.c with a sperated set of algorithms:
 *
 * 	0 Chamberlain 24dB LPF/HPF - lwf support and need HPF for now
 *
 * 	1 Huovilainen 12dB LPF oversampling
 * 	2 Huovilainen 24dB LPF oversampling and OBx remixing
 * 	3 Huovilainen 24dB LPF oversampling and OBx remixing 2
 *
 * 	4 Huovilainen 24dB LPF oversampling (4 is for interfuctioning with filter.c)
 *
 * 	Others will implement Huovilainen withuot oversampling, the oversampling
 * 	will be integrated into the engine.
 */
#include <math.h>

#include "bristol.h"
#include "bristolblo.h"
#include "filter.h"

/*
 * The name of this operator, IO count, and IO names.
 */
#define OPNAME "DCF"
#define OPDESCRIPTION "Digital Filter Two"
#define PCOUNT 9
#define IOCOUNT 3

#define FILTER_IN_IND 0
#define FILTER_MOD_IND 1
#define FILTER_OUT_IND 2

/* This is ugly, we shuold correctly pull it out of the operator template */
static float srate;

#define _f_lim_r (20000 / (sr * 2)) /* Upper limit with resampling */
#define _f_lim (20000 / sr)

#define getcoff(c, k) ((c*c)*(1.0f-k)*20000 + k*4*voice->cfreq) / srate

/*
 * This is defined for the case that we find a more efficient polynomial
 * expansion. It is more likely that the tanhf() get factored out thuogh.
#define TANH tanhf
 * We should put the nonlinear sqrt estimation versys tanhf under compilation
 * control
 */
#define TANH(v) btanhf(mode, v)
//#define TANH(v) (v + 1e-10f) / sqrtf(1 + v * v)
#define TANHFEED(v) btanhfeed(mode, v)
//#define TANHFEED(v) (v + 1e-10f) / sqrtf(1 + v * v)
#define V2 40000.0
#define OV2 0.000025 /* = 1/V2 */

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
	 * Free any local memory. We shuold also free ourselves, since we did the
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
#ifdef BRISTOL_DBG
	printf("reset(%x)\n", operator);
#endif

	param->param[0].float_val = 0.5;
	param->param[1].float_val = 0.5;
	param->param[2].float_val = 1.0;
	param->param[3].int_val = 0; /* KBD tracking */
	param->param[4].int_val = 0; /* type */
	param->param[5].float_val = 1.0;
	param->param[6].int_val = 0.0; /* Mode LP/BP/HP (type 0 only) */
	param->param[7].float_val = 0.0;
	param->param[8].float_val = 0.1;

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

	switch (index) {
		case 0:
			param->param[index].float_val = value;
/*printf("Configuring filter %f\n", param->param[index].float_val); */
			param->param[index].int_val = intvalue;
			break;
		case 1:
			param->param[index].float_val = value;
			param->param[index].int_val = intvalue;
			break;
		case 2: /* Mod */
		case 5: /* Gain */
			param->param[index].float_val = value;
			break;
		case 3:
			if (value > 0)
				param->param[index].int_val = 1;
			else
				param->param[index].int_val = 0;
			param->param[index].float_val = value;
//printf("KBD: %f\n", value);
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
		case 7: /* Oberheim Pole remix */
		case 8: /* Denormal avoidance */
			param->param[index].float_val = value;
			break;
	}

	return(0);
}

/*
 * This shuold perhaps be re-implemented. The HPF and BPF components really do
 * not seem to respond that well and better versions are required for some of
 * the emulators.
 */
static int
chamberlain(float *ib, float *mb, float *ob, bristolOPParams *param, bristolFILTERlocal *local, bristolVoice *voice, int count)
{
	register float Mod, gain, cutoff;

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
	/* This needs to be 'f-sr(v/constant) */
	gain = param->param[5].float_val * 0.01565;

	/*
	 * The following was for on/off keytracking, needs to be continuous.
	if (param->param[3].int_val)
		cutoff += voice->dFreq / 128.0f;
	cutoff = param->param[0].float_val * 20000 / srate;
	cutoff += param->param[3].float_val * voice->key.key / 512.0f;
	 */
	if (param->param[3].float_val == 0)
		cutoff = param->param[0].float_val * param->param[0].float_val
			* 20000 / srate;
	else
		cutoff = param->param[3].float_val * param->param[0].float_val
			* 4 * voice->cfreq / srate;

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

	return(0);
}

static float scale = 0.0000001; // 0.000000001;
static int dngx1 = 0x67452301;
static int dngx2 = 0xefcdab89;

static int
huovilainen24(float *ib, float *mb, float *ob, bristolOPParams *param, bristolFILTERlocal *local, bristolVoice *voice, int count)
{
	int mode = bfiltertype(voice->baudio->mixflags);
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
	float ay1 = local->ay1;
	float ay2 = local->ay2;
	float ay3 = local->ay3;
	float ay4 = local->ay4;
	float amf = local->amf;

	float kfc;
	float kfcr;
	float kacr;
	float k2vg;
	float coff;
	float dng = param->param[8].float_val * scale;

	float sr = srate;

	float resonance = param->param[1].float_val;
	float Mod = param->param[2].float_val * param->param[2].float_val * 0.02;
	float mix =  param->param[7].float_val * 0.5;

	/*
	 * Cutoff is a power curve for better control at lower frequencies and
	 * the key is used for tracking purposes, it shuold be possible to make
	 * it reasonably linear at somewhere under unity
	 *
	 * Cutoff goes from 0 to 1.0 = nyquist. Key tracking shuold be reviewed,
	 * the value shuold be quite linear for the filter, 0..Nyquist
	 *
	 * If we take midi key 0 = 8Hz and 127 = 12658.22 Hz then using
	 * our samplerate we shuold be able to fix some tuning:
	 *
	 * (voice->key.key * 12650.22 + 8) / (127 * srate)
	 *
	 * We want to position param[3] such that it tunes at 0.5 and can then
	 * be notched in the GUI, hence srate/4 rathern than /2.
	 *
	 * This still needs some work.
	if (param->param[3].float_val == 0)
		coff = param->param[0].float_val * param->param[0].float_val
			* 20000 / srate;
	else
		coff = param->param[0].float_val * param->param[3].float_val * 4 *
			voice->cfreq / srate;
	 */
	coff = getcoff(param->param[0].float_val, param->param[3].float_val);

	for (; count > 0; count--)
	{
		/*
		 * We should really interpret coff (the configured frequency) as
		 * a function up to about 20KHz whatever the resampling rate.
		 */
		if ((kfc = coff + *mb++ * Mod) > _f_lim)
			kfc = _f_lim;
		else if (kfc < 0)
			kfc = 0;

		// frequency & amplitude correction
		kfcr = kfc * ( kfc * (1.8730 * kfc + 0.4955) - 0.6490) + 0.9988;
		kacr = kfc * (-3.9364 * kfc + 1.8409) + 0.9968;

		k2vg = (1 - expf(-2.0 * M_PI * kfcr * kfc));

		// cascade of 4 1st order sections
		dngx1 ^= dngx2;
		ay1  = az1 + k2vg * (TANHFEED((*ib + dngx2 * dng) * OV2
			- 4*resonance*amf*kacr) - TANH(az1));
		dngx2 += dngx1;
		az1  = ay1;

		ay2  = az2 + k2vg * (TANH(ay1) - TANH(az2));
		az2  = ay2;

		ay3  = az3 + k2vg * (TANH(ay2) - TANH(az3));
		az3  = ay3;

		ay4  = az4 + k2vg * (TANH(ay3) - TANH(az4));
		az4  = ay4;

		amf  = ay4;

		/*
		 * This remixes the different poles to give a fatter, warmer sound. The
		 * other algorithms do a premixing of this however that prevents the
		 * filter from resonating.
		 */
		*ob++ += (amf + (-ay3-az4 + ay2+az3 - ay1-az2) * mix) * V2;
		ib++;
	}

	local->az1 = az1;
	local->az2 = az2;
	local->az3 = az3;
	local->az4 = az4;
	local->ay1 = ay1;
	local->ay2 = ay2;
	local->ay3 = ay3;
	local->ay4 = ay4;
	local->amf = amf;

	return(0);
}

static int
huovilainen24R(float *ib, float *mb, float *ob, bristolOPParams *param, bristolFILTERlocal *local, bristolVoice *voice, int count)
{
	int mode = bfiltertype(voice->baudio->mixflags);
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

	float dng = param->param[8].float_val * scale;
	float kfc;
	float kfcr;
	float kacr;
	float k2vg;
	float coff;

	float sr = srate;

	float resonance = param->param[1].float_val;
	float Mod = param->param[2].float_val * param->param[2].float_val * 0.02;
	float mix =  param->param[7].float_val * 0.5;

	/*
	 * Cutoff is a power curve for better control at lower frequencies and
	 * the key is used for tracking purposes, it shuold be possible to make
	 * it reasonably linear at somewhere under unity
	 *
	 * Cutoff goes from 0 to 1.0 = nyquist. Key tracking shuold be reviewed,
	 * the value shuold be quite linear for the filter, 0..Nyquist
	 *
	 * If we take midi key 0 = 8Hz and 127 = 12658.22 Hz then using
	 * our samplerate we shuold be able to fix some tuning:
	 *
	 * (voice->key.key * 12650.22 + 8) / (127 * srate)
	 *
	 * We want to position param[3] such that it tunes at 0.5 and can then
	 * be notched in the GUI, hence srate/4 rathern than /2.
	 *
	 * This still needs some work.
	if (param->param[3].float_val == 0)
		coff = param->param[0].float_val * param->param[0].float_val
			* 20000 / srate;
	else
		coff = param->param[0].float_val * param->param[3].float_val * 4 *
			voice->cfreq / srate;
	 */
	coff = getcoff(param->param[0].float_val, param->param[3].float_val);

	for (; count > 0; count--)
	{
		/*
		 * We shuold really interpret coff (the configured frequency) as
		 * a function up to about 20KHz whatever the resampling rate.
		 */
		if ((kfc = coff + *mb++ * Mod) > _f_lim_r)
			kfc = _f_lim_r;
		else if (kfc < 0)
			kfc = 0;

		// frequency & amplitude correction
		kfcr = kfc * ( kfc * (1.8730 * kfc + 0.4955) - 0.6490) + 0.9988;
		kacr = kfc * (-3.9364 * kfc + 1.8409)  + 0.9968;

		k2vg = (1 - expf(-2.0 * M_PI * kfcr * kfc * 0.5));

		ay1  = az1 + k2vg * (TANHFEED((*ib*OV2 - 4*resonance*amf*kacr))
			- TANH(az1));
		az1  = ay1;

		ay2  = az2 + k2vg * (TANH(ay1) - TANH(az2));
		az2  = ay2;

		ay3  = az3 + k2vg * (TANH(ay2) - TANH(az3));
		az3  = ay3;

		ay4  = az4 + k2vg * (TANH(ay3) - TANH(az4));
		az4  = ay4;

		// 1/2-sample delay for phase compensation
		amf  = (ay4+az5)*0.5;
		az5  = ay4;

		// oversampling (repeat same block) and inject some noise
		dngx1 ^= dngx2;
		ay1  = az1 + k2vg * (TANHFEED((*ib +dngx2 * dng) * OV2
			- 4*resonance*amf*kacr) - TANH(az1));
		dngx2 += dngx1;
		az1  = ay1;

		ay2  = az2 + k2vg * (TANH(ay1) - TANH(az2));
		az2  = ay2;

		ay3  = az3 + k2vg * (TANH(ay2) - TANH(az3));
		az3  = ay3;

		ay4  = az4 + k2vg * (TANH(ay3) - TANH(az4));
		az4  = ay4;

		// 1/2-sample delay for phase compensation
		amf  = (ay4+az5)*0.5;
		az5  = ay4;

		/*
		 * This remixes the different poles to give a fatter, warmer sound. The
		 * other algorithms do a premixing of this however that prevents the
		 * filter from resonating.
		 */
		*ob++ += V2 * (amf + (-ay3-az4 + ay2+az3 - ay1-az2) * mix) * 0.5;
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

	return(0);
}

static int
huovilainen12R(float *ib, float *mb, float *ob, bristolOPParams *param, bristolFILTERlocal *local, bristolVoice *voice, int count)
{
	int mode = bfiltertype(voice->baudio->mixflags);
	/*
	 * This is actually the same code as 24R just the 12dB and 18dB taps are
	 * mixed back into the final output. This was an Oberheim filter mod that
	 * did not actually go into any of their products due to the added 
	 * complexity of an analogue implementation. It shuold be generally richer
	 * due to more phase differences and content.
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

	float dng = param->param[8].float_val * scale;
	float kfc;
	float kfcr;
	float kacr;
	float k2vg;
	float coff;

	float sr = srate;

	float resonance = param->param[1].float_val;
	float Mod = param->param[2].float_val * param->param[2].float_val * 0.02;

	float mix =  param->param[7].float_val;

	/*
	 * Cutoff is a power curve for better control at lower frequencies and
	 * the key is used for tracking purposes, it shuold be possible to make
	 * it reasonably linear at somewhere under unity
	 *
	 * Cutoff goes from 0 to 1.0 = nyquist. Key tracking shuold be reviewed,
	 * the value shuold be quite linear for the filter, 0..Nyquist
	 *
	 * If we take midi key 0 = 8Hz and 127 = 12658.22 Hz then using
	 * our samplerate we shuold be able to fix some tuning:
	 *
	 * (voice->key.key * 12650.22 + 8) / (127 * srate)
	 *
	 * We want to position param[3] such that it tunes at 0.5 and can then
	 * be notched in the GUI, hence srate/4 rathern than /2.
	 *
	 * This still needs some work.
	if (param->param[3].float_val == 0)
		coff = param->param[0].float_val * param->param[0].float_val
			* 20000 / srate;
	else
		coff = param->param[0].float_val * param->param[3].float_val * 4 *
			voice->cfreq / srate;
	 */
	coff = getcoff(param->param[0].float_val, param->param[3].float_val);

	for (; count > 0; count--)
	{
		/*
		 * We shuold really interpret coff (the configured frequency) as
		 * a function up to about 20KHz whatever the resampling rate.
		 */
		if ((kfc = coff + *mb++ * Mod) > _f_lim_r)
			kfc = _f_lim_r;
		else if (kfc < 0)
			kfc = 0;

		// frequency & amplitude correction
		kfcr = kfc * (kfc * (1.8730 * kfc + 0.4955) - 0.6490) + 0.9988;
		kacr = kfc * (-3.9364* kfc + 1.8409) + 0.9968;

		// filter tuning
		k2vg = (1 - expf(-2.0 * M_PI * kfcr * kfc * 0.5));

		// cascade of 4 1st order sections
		ay1  = az1 + k2vg * (TANHFEED(*ib*OV2 - 4*resonance*amf*kacr)- TANH(az1));
		az1  = ay1;

		ay2  = az2 + k2vg * (TANH(ay1) - TANH(az2));
		az2  = ay2;
		/*
		 * This is the end of the 12dB cycle
		 *	amf  = (ay4+az5)*0.5;
		 */
		amf  = (ay2+az3) * 0.5;
		az3  = ay2;

		// oversampling (repeat same block) and inject some noise (denormal)
		dngx1 ^= dngx2;
		ay1  = az1 + k2vg * (TANHFEED((*ib + dngx2 * dng) * OV2
			- 4*resonance*amf*kacr) - TANH(az1));
		dngx2 += dngx1;
		az1  = ay1;

		ay2  = az2 + k2vg * (TANH(ay1) - TANH(az2));
		az2  = ay2;
		// 1/2-sample delay for phase compensation
		amf  = (ay2+az3) *0.5;
		az3  = ay2;

		/* Oberheim modified pole mixing */
		*ob++ += (amf + (ay1 + az2) * mix) * OV2;
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
	return(0);
}

static int
huovilainen12(float *ib, float *mb, float *ob, bristolOPParams *param, bristolFILTERlocal *local, bristolVoice *voice, int count)
{
	int mode = bfiltertype(voice->baudio->mixflags);
	/*
	 * This is actually the same code as 24R just the 12dB and 18dB taps are
	 * mixed back into the final output. This was an Oberheim filter mod that
	 * did not actually go into any of their products due to the added 
	 * complexity of an analogue implementation. It shuold be generally richer
	 * due to more phase differences and content.
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

	float dng = param->param[8].float_val * scale;
	float kfc;
	float kfcr;
	float kacr;
	float k2vg;
	float coff;

	float sr = srate;

	float resonance = param->param[1].float_val;
	float Mod = param->param[2].float_val * param->param[2].float_val * 0.02;

	float mix =  param->param[7].float_val;

	/*
	 * Cutoff is a power curve for better control at lower frequencies and
	 * the key is used for tracking purposes, it shuold be possible to make
	 * it reasonably linear at somewhere under unity
	 *
	 * Cutoff goes from 0 to 1.0 = nyquist. Key tracking shuold be reviewed,
	 * the value shuold be quite linear for the filter, 0..Nyquist
	 *
	 * If we take midi key 0 = 8Hz and 127 = 12658.22 Hz then using
	 * our samplerate we shuold be able to fix some tuning:
	 *
	 * (voice->key.key * 12650.22 + 8) / (127 * srate)
	 *
	 * We want to position param[3] such that it tunes at 0.5 and can then
	 * be notched in the GUI, hence srate/4 rathern than /2.
	 *
	 * This still needs some work.
	if (param->param[3].float_val == 0)
		coff = param->param[0].float_val * param->param[0].float_val
			* 20000 / srate;
	else
		coff = param->param[0].float_val * param->param[3].float_val * 4 *
			voice->cfreq / srate;
	 */
	coff = getcoff(param->param[0].float_val, param->param[3].float_val);

	for (; count > 0; count--)
	{
		/*
		 * We shuold really interpret coff (the configured frequency) as
		 * a function up to about 20KHz whatever the resampling rate.
		 */
		if ((kfc = coff + *mb++ * Mod) > _f_lim_r)
			kfc = _f_lim_r;
		else if (kfc < 0)
			kfc = 0;

		// frequency & amplitude correction
		kfcr = kfc * (kfc * (1.8730 * kfc + 0.4955) - 0.6490) + 0.9988;
		kacr = kfc * (-3.9364* kfc + 1.8409) + 0.9968;

		k2vg = (1 - expf(-2.0 * M_PI * kfcr * kfc));

		// cascade of 4 1st order sections
		dngx1 ^= dngx2;
		ay1  = az1 + k2vg * (TANHFEED((*ib +dngx2 * dng) * OV2
			- 4*resonance*amf*kacr) - TANH(az1));
		dngx2 += dngx1;
		az1  = ay1;

		ay2  = az2 + k2vg * (TANH(ay1) - TANH(az2));
		az2  = ay2;
		// 1/2-sample delay for phase compensation
		amf  = (ay2+az3) *0.5;
		az3  = ay2;

		/* Oberheim modified pole mixing */
		*ob++ += (amf + (ay1 + az2) * mix) * V2;
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
	return(0);
}

static int
huovilainen24ROB(float *ib, float *mb, float *ob, bristolOPParams *param, bristolFILTERlocal *local, bristolVoice *voice, int count)
{
	int mode = bfiltertype(voice->baudio->mixflags);
	/*
	 * This is actually the same code as 24R just the 12dB and 18dB taps are
	 * mixed back into the final output. This was an Oberheim filter mod that
	 * did not actually go into any of their products due to the added 
	 * complexity of an analogue implementation. It shuold be generally richer
	 * due to more phase differences and content.
	 */
	float dng = param->param[8].float_val * scale;
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

	float kfc;
	float kfcr;
	float kacr;
	float k2vg;
	float coff;

	float sr = srate;

	float resonance = param->param[1].float_val;
	float Mod = param->param[2].float_val * param->param[2].float_val * 0.02;

	float mix =  param->param[7].float_val;

	/*
	 * Cutoff is a power curve for better control at lower frequencies and
	 * the key is used for tracking purposes, it shuold be possible to make
	 * it reasonably linear at somewhere under unity
	 *
	 * Cutoff goes from 0 to 1.0 = nyquist. Key tracking shuold be reviewed,
	 * the value shuold be quite linear for the filter, 0..Nyquist
	 *
	 * If we take midi key 0 = 8Hz and 127 = 12658.22 Hz then using
	 * our samplerate we shuold be able to fix some tuning:
	 *
	 * (voice->key.key * 12650.22 + 8) / (127 * srate)
	 *
	 * We want to position param[3] such that it tunes at 0.5 and can then
	 * be notched in the GUI, hence srate/4 rathern than /2.
	 *
	 * This still needs some work.
	if (param->param[3].float_val == 0)
		coff = param->param[0].float_val * param->param[0].float_val
			* 20000 / srate;
	else
		coff = param->param[0].float_val * param->param[3].float_val * 4 *
			voice->cfreq / srate;
	 */
	coff = getcoff(param->param[0].float_val, param->param[3].float_val);

	for (; count > 0; count--)
	{
		/*
		 * We shuold really interpret coff (the configured frequency) as
		 * a function up to about 20KHz whatever the resampling rate.
		 */
		if ((kfc = coff + *mb++ * Mod) > _f_lim_r)
			kfc = _f_lim_r;
		else if (kfc < 0)
			kfc = 0;

		// frequency & amplitude correction
		kfcr = kfc * ( kfc * (1.8730 * kfc + 0.4955) - 0.6490) + 0.9988;
		kacr = kfc * (-3.9364 * kfc + 1.8409) + 0.9968;

		k2vg = (1 - expf(-2.0 * M_PI * kfcr * kfc * 0.5));

		ay1  = az1 + k2vg * (TANHFEED((*ib*OV2 - 4*resonance*amf*kacr))
            - TANH(az1));
		az1  = ay1;

		ay2  = az2 + k2vg * (TANH(ay1) - TANH(az2));
		az2  = ay2;

		ay3  = az3 + k2vg * (TANH(ay2) - TANH(az3));
		az3  = ay3;

		ay4  = az4 + k2vg * (TANH(ay3) - TANH(az4));
		az4  = ay4;

		// 1/2-sample delay for phase compensation
		amf  = (ay4+az5 + (-ay3-+az4 + ay2+az3) * mix) * 0.5;
		az5  = ay4;

		// oversampling (repeat same block) and add denormal noise
		dngx1 ^= dngx2;
		ay1  = az1 + k2vg * (TANHFEED((*ib +dngx2 * dng) * OV2
			- 4*resonance*amf*kacr) - TANH(az1));
		dngx2 += dngx1;
		az1  = ay1;

		ay2  = az2 + k2vg * (TANH(ay1) - TANH(az2));
		az2  = ay2;

		ay3  = az3 + k2vg * (TANH(ay2) - TANH(az3));
		az3  = ay3;

		ay4  = az4 + k2vg * (TANH(ay3) - TANH(az4));
		az4  = ay4;

		// 1/2-sample delay for phase compensation
		// Added in 12dB and 18dB phases
		amf  = (ay4+az5 + (-ay3-az4 + ay2+az3) * mix) * 0.5;
		az5  = ay4;

		*ob++ += V2 * amf;
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

	return(0);
}

static int
huovilainen24OB(float *ib, float *mb, float *ob, bristolOPParams *param, bristolFILTERlocal *local, bristolVoice *voice, int count)
{
	int mode = bfiltertype(voice->baudio->mixflags);
	/*
	 * This is actually the same code as 24R just the 12dB and 18dB taps are
	 * mixed back into the final output. This was an Oberheim filter mod that
	 * did not actually go into any of their products due to the added 
	 * complexity of an analogue implementation. It shuold be generally richer
	 * due to more phase differences and content.
	 */
	float dng = param->param[8].float_val * scale;
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

	float kfc;
	float kfcr;
	float kacr;
	float k2vg;

	float sr = srate;
	float coff;

	float resonance = param->param[1].float_val;
	float Mod = param->param[2].float_val * param->param[2].float_val * 0.02;

	float mix =  param->param[7].float_val;

	/*
	 * Cutoff is a power curve for better control at lower frequencies and
	 * the key is used for tracking purposes, it shuold be possible to make
	 * it reasonably linear at somewhere under unity
	 *
	 * Cutoff goes from 0 to 1.0 = nyquist. Key tracking shuold be reviewed,
	 * the value shuold be quite linear for the filter, 0..Nyquist
	 *
	 * If we take midi key 0 = 8Hz and 127 = 12658.22 Hz then using
	 * our samplerate we shuold be able to fix some tuning:
	 *
	 * (voice->key.key * 12650.22 + 8) / (127 * srate)
	 *
	 * We want to position param[3] such that it tunes at 0.5 and can then
	 * be notched in the GUI, hence srate/4 rathern than /2.
	 *
	 * This still needs some work.
	if (param->param[3].float_val == 0)
		coff = param->param[0].float_val * param->param[0].float_val
			* 20000 / srate;
	else
		coff = param->param[0].float_val * param->param[3].float_val * 4 *
			voice->cfreq / srate;
	 */
	coff = getcoff(param->param[0].float_val, param->param[3].float_val);

	for (; count > 0; count--)
	{
		/*
		 * We shuold really interpret coff (the configured frequency) as
		 * a function up to about 20KHz whatever the resampling rate.
		 */
		if ((kfc = coff + *mb++ * Mod) > _f_lim_r)
			kfc = _f_lim_r;
		else if (kfc < 0)
			kfc = 0;

		// frequency & amplitude correction
		kfcr = kfc * ( kfc * (1.8730 * kfc + 0.4955) - 0.6490) + 0.9988;
		kacr = kfc * (-3.9364 * kfc + 1.8409) + 0.9968;

		k2vg = (1 - expf(-2.0 * M_PI * kfcr * kfc));

		// cascade of 4 1st order sections
		dngx1 ^= dngx2;
		ay1  = az1 + k2vg * (TANHFEED((*ib +dngx2 * dng) * OV2
			- 4*resonance*amf*kacr) - TANH(az1));
		dngx2 += dngx1;
		az1  = ay1;

		ay2  = az2 + k2vg * (TANH(ay1) - TANH(az2));
		az2  = ay2;

		ay3  = az3 + k2vg * (TANH(ay2) - TANH(az3));
		az3  = ay3;

		ay4  = az4 + k2vg * (TANH(ay3) - TANH(az4));
		az4  = ay4;

		// 1/2-sample delay for phase compensation
		// Added in 12dB and 18dB phases
		amf  = (ay4+az5 + (-ay3-az4 + ay2+az3) * mix) *0.5;
		az5  = ay4;

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

	return(0);
}

static int
huovilainen24ROB2(float *ib, float *mb, float *ob, bristolOPParams *param, bristolFILTERlocal *local, bristolVoice *voice, int count)
{
	int mode = bfiltertype(voice->baudio->mixflags);
	/*
	 * This is actually the same code as 24R just the 12dB and 18dB taps are
	 * mixed back into the final output. This was an Oberheim filter mod that
	 * did not actually go into any of their products due to the added 
	 * complexity of an analogue implementation. It shuold be generally richer
	 * due to more phase differences and content.
	 */
	float dng = param->param[8].float_val * scale;
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

	float kfc;
	float kfcr;
	float kacr;
	float k2vg;
	float coff;

	float sr = srate;

	float resonance = param->param[1].float_val * 0.30;
	float Mod = param->param[2].float_val * param->param[2].float_val * 0.02;

	float mix =  param->param[7].float_val;

	/*
	 * Cutoff is a power curve for better control at lower frequencies and
	 * the key is used for tracking purposes, it shuold be possible to make
	 * it reasonably linear at somewhere under unity
	 *
	 * Cutoff goes from 0 to 1.0 = nyquist. Key tracking shuold be reviewed,
	 * the value shuold be quite linear for the filter, 0..Nyquist
	 *
	 * If we take midi key 0 = 8Hz and 127 = 12658.22 Hz then using
	 * our samplerate we shuold be able to fix some tuning:
	 *
	 * (voice->key.key * 12650.22 + 8) / (127 * srate)
	 *
	 * We want to position param[3] such that it tunes at 0.5 and can then
	 * be notched in the GUI, hence srate/4 rathern than /2.
	 *
	 * This still needs some work.
	if (param->param[3].float_val == 0)
		coff = param->param[0].float_val * param->param[0].float_val
			* 20000 / srate;
	else
		coff = param->param[0].float_val * param->param[3].float_val * 4 *
			voice->cfreq / srate;
	 */
	coff = getcoff(param->param[0].float_val, param->param[3].float_val);

	for (; count > 0; count--)
	{
		/*
		 * We shuold really interpret coff (the configured frequency) as
		 * a function up to about 20KHz whatever the resampling rate.
		 */
		if ((kfc = coff + *mb++ * Mod) > _f_lim_r)
			kfc = _f_lim_r;
		else if (kfc < 0)
			kfc = 0;

		// frequency & amplitude correction
		kfcr = kfc * ( kfc * (1.8730 * kfc + 0.4955) - 0.6490) + 0.9988;
		kacr = kfc * (-3.9364 * kfc  + 1.8409) + 0.9968;

		k2vg = (1 - expf(-2.0 * M_PI * kfcr * kfc * 0.5));

		// cascade of 4 1st order sections
		ay1  = az1 + k2vg * (TANHFEED((*ib*OV2 - 4*resonance*amf*kacr))
            - TANH(az1));
		az1  = ay1;

		ay2  = az2 + k2vg * (TANH(ay1) - TANH(az2));
		az2  = ay2;

		ay3  = az3 + k2vg * (TANH(ay2) - TANH(az3));
		az3  = ay3;

		ay4  = az4 + k2vg * (TANH(ay3) - TANH(az4));
		az4  = ay4;

		// 1/2-sample delay for phase compensation
		// amf  = (ay4+az5)*0.5;
		// Added in 6dB, 12dB and 18dB phases
		amf  = (ay4+az5 + (-ay1-az2+ay3+az4-ay2-az3) * mix) *0.5;
		az5  = ay4;

		// oversampling (repeat same block)
		dngx1 ^= dngx2;
		ay1  = az1 + k2vg * (TANHFEED((*ib +dngx2 * dng) * OV2
            - 4*resonance*amf*kacr) - TANH(az1));
		dngx2 += dngx1;
		az1  = ay1;

		ay2  = az2 + k2vg * (TANH(ay1) - TANH(az2));
		az2  = ay2;

		ay3  = az3 + k2vg * (TANH(ay2) - TANH(az3));
		az3  = ay3;

		ay4  = az4 + k2vg * (TANH(ay3) - TANH(az4));
		az4  = ay4;

		// 1/2-sample delay for phase compensation
		// Added in 6dB, 12dB and 18dB phases
		amf  = (ay4+az5 + (-ay1-az2+ay3+az4-ay2-az3) * mix) *0.5;
		az5  = ay4;

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

	return(0);
}

static int
huovilainen24OB2(float *ib, float *mb, float *ob, bristolOPParams *param, bristolFILTERlocal *local, bristolVoice *voice, int count)
{
	int mode = bfiltertype(voice->baudio->mixflags);
	/*
	 * This is actually the same code as 24R just the 12dB and 18dB taps are
	 * mixed back into the final output. This was an Oberheim filter mod that
	 * did not actually go into any of their products due to the added 
	 * complexity of an analogue implementation. It shuold be generally richer
	 * due to more phase differences and content.
	 */
	float dng = param->param[8].float_val * scale;
	float az1 = local->az1;
	float az2 = local->az2;
	float az3 = local->az3;
	float az4 = local->az4;
	float ay1 = local->ay1;
	float ay2 = local->ay2;
	float ay3 = local->ay3;
	float ay4 = local->ay4;
	float amf = local->amf;

	float kfc;
	float kfcr;
	float kacr;
	float k2vg;
	float coff;

	float sr = srate;

	float resonance = param->param[1].float_val * 0.30;
	float Mod = param->param[2].float_val * param->param[2].float_val * 0.02;

	float mix =  param->param[7].float_val;

	/*
	 * Cutoff is a power curve for better control at lower frequencies and
	 * the key is used for tracking purposes, it shuold be possible to make
	 * it reasonably linear at somewhere under unity
	 *
	 * Cutoff goes from 0 to 1.0 = nyquist. Key tracking shuold be reviewed,
	 * the value shuold be quite linear for the filter, 0..Nyquist
	 *
	 * If we take midi key 0 = 8Hz and 127 = 12658.22 Hz then using
	 * our samplerate we shuold be able to fix some tuning:
	 *
	 * (voice->key.key * 12650.22 + 8) / (127 * srate)
	 *
	 * We want to position param[3] such that it tunes at 0.5 and can then
	 * be notched in the GUI, hence srate/4 rathern than /2.
	 *
	 * This still needs some work.
	if (param->param[3].float_val == 0)
		coff = param->param[0].float_val * param->param[0].float_val
			* 20000 / srate;
	else
		coff = param->param[0].float_val * param->param[3].float_val * 4 *
			voice->cfreq / srate;
	 */
	coff = getcoff(param->param[0].float_val, param->param[3].float_val);

	for (; count > 0; count--)
	{
		/*
		 * We shuold really interpret coff (the configured frequency) as
		 * a function up to about 20KHz whatever the resampling rate.
		 */
		if ((kfc = coff + *mb++ * Mod) > _f_lim)
			kfc = _f_lim;
		else if (kfc < 0)
			kfc = 0;

		// frequency & amplitude correction
		kfcr = kfc * ( kfc * (1.8730 * kfc + 0.4955) - 0.6490) + 0.9988;
		kacr = kfc * (-3.9364 * kfc  + 1.8409) + 0.9968;

		k2vg = (1 - expf(-2.0 * M_PI * kfcr * kfc));

		// cascade of 4 1st order sections
		dngx1 ^= dngx2;
		ay1  = az1 + k2vg * (TANHFEED((*ib +dngx2 * dng) * OV2
			- 4*resonance*amf*kacr) - TANH(az1));
		dngx2 += dngx1;
		az1  = ay1;

		ay2  = az2 + k2vg * (TANH(ay1) - TANH(az2));
		az2  = ay2;

		ay3  = az3 + k2vg * (TANH(ay2) - TANH(az3));
		az3  = ay3;

		ay4  = az4 + k2vg * (TANH(ay3) - TANH(az4));
		az4  = ay4;

		// 1/2-sample delay for phase compensation
		// Added in 6dB, 12dB and 18dB phases
		amf  = (ay4 + (-ay1-az2+ay3+az4-ay2-az3) * mix);

		*ob++ += amf * V2;
		ib++;
	}

	local->az1 = az1;
	local->az2 = az2;
	local->az3 = az3;
	local->az4 = az4;
	local->ay1 = ay1;
	local->ay2 = ay2;
	local->ay3 = ay3;
	local->ay4 = ay4;
	local->amf = amf;

	return(0);
}

/*
 * filter - takes input signal and filters it according to the mod level.
 */
static int operate(register bristolOP *operator, bristolVoice *voice,
	bristolOPParams *param,
	void *lcl)
{
	bristolFILTERlocal *local = lcl;
	register int count, fselect;
	register float *ib, *ob, *mb;
	bristolFILTER *specs;

	/*
	 * Every operator accesses these variables, the count, and a pointer to
	 * each buffer. We shuold consider passing them as readymade parameters?
	 */
	specs = (bristolFILTER *) operator->specs;
	count = specs->spec.io[FILTER_OUT_IND].samplecount;
	ib = specs->spec.io[FILTER_IN_IND].buf;
	mb = specs->spec.io[FILTER_MOD_IND].buf;
	ob = specs->spec.io[FILTER_OUT_IND].buf;

#ifdef BRISTOL_DBG
	printf("filter2(%x, %x, %x)\n", operator, param, local);
#endif

	/* See if we are limited to lightweight filters */
	if (blo.flags & BRISTOL_LWF)
		fselect = 0;
	else {
		fselect = param->param[4].int_val;
		if (srate > 80000)
			fselect += 16;
	}

	/*
	 * We will eventually have another set here that will be called when the
	 * samplerate is over 48kHz (88.2, 96, 192 kHz) when we can anticipate the
	 * filter response to be adequate withuot resampling internally here.
	 */
	switch(fselect) {
		/* LightWeight Filters */
		case 0:
		case 16:
			return(chamberlain(ib, mb, ob, param, local, voice, count));

		/* Huovilainen resampling */
		case 1:
			return(huovilainen12R(ib, mb, ob, param, local, voice, count));
		case 2:
			return(huovilainen24ROB(ib, mb, ob, param, local, voice, count));
		case 3:
			return(huovilainen24ROB2(ib, mb, ob, param, local, voice, count));
		case 4:
		default:
			return(huovilainen24R(ib, mb, ob, param, local, voice, count));

		/*
		 * We need non-resampling versions of the other filters too but
		 * need to review this code first.
		 */
		case 17:
			return(huovilainen12(ib, mb, ob, param, local, voice, count));
		case 18:
			return(huovilainen24OB(ib, mb, ob, param, local, voice, count));
		case 19:
			return(huovilainen24OB2(ib, mb, ob, param, local, voice, count));
		case 20:
			return(huovilainen24(ib, mb, ob, param, local, voice, count));
	}
}

/*
 * Setup any variables in our OP structure, in our IO structures, and malloc
 * any memory we need.
 */
bristolOP *
filter2init(bristolOP **operator, int index, int samplerate, int samplecount)
{
	bristolFILTER *specs;

#ifdef BRISTOL_DBG
	printf("filter2init(%x(%x), %i, %i, %i)\n",
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
	specs->spec.param[4].description = "Algorithm selection";
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
	specs->spec.param[6].description = "Filter mode (type 0 only)";
	specs->spec.param[6].type = BRISTOL_INT;
	specs->spec.param[6].low = 0;
	specs->spec.param[6].high = 3;
	specs->spec.param[6].flags = BRISTOL_ROTARY|BRISTOL_SLIDER;

	specs->spec.param[7].pname = "Remix gain";
	specs->spec.param[7].description = "Oberheim pole remixing";
	specs->spec.param[7].type = BRISTOL_FLOAT;
	specs->spec.param[7].low = 0;
	specs->spec.param[7].high = 1;
	specs->spec.param[7].flags = BRISTOL_ROTARY|BRISTOL_SLIDER;

	specs->spec.param[8].pname = "normalise";
	specs->spec.param[8].description = "";
	specs->spec.param[8].type = BRISTOL_FLOAT;
	specs->spec.param[8].low = 0;
	specs->spec.param[8].high = 1;
	specs->spec.param[8].flags = BRISTOL_ROTARY|BRISTOL_SLIDER;

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

