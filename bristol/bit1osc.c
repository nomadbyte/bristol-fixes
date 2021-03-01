
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
 *	bit1oscinit()
 *	operate()
 *	reset()
 *	destroy()
 *
 *	destroy() is in the library.
 *
 * Operate will be called when all the inputs have been loaded, and the result
 * will be an output buffer written to the next operator.
 *
 * This code is a bit kludgy (prior to 0.30.3) since the width of a pulse wave
 * is an integer. It needs to be a resampled subsample increment to smooth out
 * the PWM. Also the sync is sample level and noisy so this oscillator will be
 * the first to be fixed then we will take the changes to the other PWM/Sync
 * oscillators.
 */

#include <stdlib.h>
#include <math.h>

#include "bristol.h"
#include "bristolblo.h"
#include "bit1osc.h"

static float note_diff;
int samplecount;

static void fillWave(float *, int, int);
static void buildBitoneSound(bristolOP *, bristolOPParams *);

/*
 * Use of these mean that we can only produce a single bit1osc oscillator.
 * Even though it is reasonably flexible this does lead to problems, and if
 * we move to a dual manual Bitone it will need to be resolved.
 *
 * The same is true of using single global result waves, wave[6] and wave[7].
 * These need to be moved to private address space, ie, we need more 
 * instantiation.
 */

/*
 * This can be a single list, it is used to generate the different pipes.
 *
 * We have 16, 8, 4, 2, 1
 */
static float sweeps[16] = {2, 4, 8, 16, 1, 32, 12, 20, 24, 32, 0,0,0,0,0,0};

/*
 * The name of this operator, IO count, and IO names.
 */
#define OPNAME "Bitone"
#define OPDESCRIPTION "Bit-1 Digitally Controlled Oscillator"
#define PCOUNT 13
#define IOCOUNT 5

#define BITONE_IN_IND 0
#define BITONE_OUT_IND 1
#define BITONE_PWM_IND 2
#define BITONE_SYNC_IND 3
#define BITONE_SYNC_OUT 4

/*
 * Clean up any local memory information.
 */
static int destroy(bristolOP *operator)
{
#ifdef BRISTOL_DBG
	printf("bit1oscdestroy(%x)\n", operator);
#endif

	/*
	 * Unmalloc anything we added to this structure
	 */
	bristolfree(((bristolBITONE *) operator)->wave[0]);
	bristolfree(((bristolBITONE *) operator)->wave[1]);
	bristolfree(((bristolBITONE *) operator)->wave[2]);
	bristolfree(((bristolBITONE *) operator)->wave[3]);
	bristolfree(((bristolBITONE *) operator)->wave[4]);
	bristolfree(((bristolBITONE *) operator)->wave[5]);
	bristolfree(((bristolBITONE *) operator)->wave[6]);
	bristolfree(((bristolBITONE *) operator)->wave[7]);

	bristolfree(((bristolBITONE *) operator)->sbuf);
	bristolfree(((bristolBITONE *) operator)->null);

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
	printf("bit1oscreset(%x, %x)\n", operator, param);
#endif

	/*
	 * For the bit1osc we want to put in a few local memory variables for this
	 * operator specifically.
	 *
	 * These are sineform, waveindex, wavelevel, and percussions, plus two
	 * unique wavetables.
	 */
	if (param->param[0].mem != NULL)
		bristolfree(param->param[0].mem);
	param->param[0].mem = bristolmalloc0(sizeof(float) * BITONE_WAVE_SZE);

	if (param->param[1].mem != NULL)
		bristolfree(param->param[1].mem);
	param->param[1].mem = bristolmalloc0(sizeof(float) * BITONE_WAVE_SZE);

	param->param[0].float_val = 1.0;
	param->param[1].float_val = 0.2;
	param->param[2].float_val = 0.2;
	param->param[3].float_val = 1.0;
	param->param[4].float_val = 1.0;
	param->param[5].float_val = 0.5;
	param->param[6].float_val = 1.0;
	param->param[7].float_val = 512;
	param->param[8].float_val = 1.0;
	param->param[9].float_val = 1.0;
	param->param[11].float_val = 0.0;
	param->param[12].float_val = 0.0;

	return(0);
}

/*
 * Alter an internal parameter of an operator.
 */
static int param(bristolOP *operator, bristolOPParams *param,
	unsigned char index, float value)
{
#ifdef BRISTOL_DBG
	printf("	bit1oscparam(%x, %x, %i, %x)\n", operator, param, index,
		((int) (value * CONTROLLER_RANGE)));
#endif

	switch (index) {
		/* The first four are the harmonics, then tri and ramp gain. */
		case 0:
		case 1:
		case 2:
		case 3:
		case 4:
		case 5:
			param->param[index].float_val = value;
			buildBitoneSound(operator, param);
			break;
		case 11:
			param->param[index].float_val = value;
			buildBitoneSound(operator, param);
			break;
		case 12:
			param->param[index].float_val = value;
			buildBitoneSound(operator, param);
			break;
		case 6:
			/*
			 * Pulse signal level. If all gains are zero then call the sound
			 * building code as it will probably want to stuff in sines as a
			 * default.
			 */
			if (((param->param[index].float_val = value * 0.7) == 0.0)
				&& (param->param[4].float_val == 0.0)
				&& (param->param[5].float_val == 0.0))
				buildBitoneSound(operator, param);

			break;
		case 7: /* Pulse width */
			param->param[index].float_val = (value * BITONE_WAVE_SZE);
			break;
		case 8: /* Frequency in 12 steps */
			{
				int semitone = value * CONTROLLER_RANGE;

				param->param[index].float_val = 1.0;

				while (semitone-- > 0) {
					param->param[index].float_val *= (note_diff);
				}
			}
			break;
		case 9: /* Tuning +/- 1/2 tone */
			{
				float tune, notes = 1.0;

				tune = (value - 0.5) * 2;

				/*
				 * Up or down one note
				 */
				if (tune > 0)
					notes *= note_diff;
				else
					notes /= note_diff;

				if (tune > 0)
					param->param[index].float_val =
						1.0 + (notes - 1) * tune;
				else
					param->param[index].float_val =
						1.0 - (1 - notes) * -tune;

				break;
			}
			break;
		case 10:
			param->param[index].float_val = value;
			break;
	}

	return(0);
}

static float
genSyncPoints(register float *sbuf, register float *sb, register float lsv,
register int count)
{
	/*
	 * Take the sync buffer and look for zero crossings. When they are found
	 * do a resample to find out where the crossing happened as sample level
	 * resolution results in distortion and shimmer
	 *
	 * Resampling, or rather interpolation to zero, means we have to look at 
	 * the respective above and below zero values.
	 */
	for (; count > 0; count--, lsv = *sb++)
		*sbuf++ = ((lsv < 0) && (*sb >= 0))? lsv / (lsv - *sb) : 0;

	return(lsv);
}

/*
 * This takes a lot of stacked parameters and could be optimised by putting
 * them in registers.
	genPWM(ob, ib, sb, pwmb, specs->wave[1], 1.0, 2.0, wtp, lsv, count, 512);
 */
static float
genPWM(register float *ob, register float *ib, register float *sb,
register float *pwmb, register float *ramp, register float gain,
register float tune, register float wtp, float notused_lsv,
register int count, register float width)
{
	register int obp;
	register float gdelta, pdelta, wtpSqr;

	if (gain == 0.0)
		return(wtp);

	while (wtp >= BITONE_WAVE_SZE)
		wtp -= BITONE_WAVE_SZE;
	while (wtp < 0)
		wtp += BITONE_WAVE_SZE;

	for (obp = 0; obp < count; obp++, pwmb++)
	{
		/*
		 * This is a phased difference between two ramps.
		 *
		 * Fill in the ramp wave, then subtract the same waveform with a
		 * phase offset related to the width.
		 */
		wtpSqr = width + *pwmb;

		if (wtpSqr < BITONE_WAVE_PWMIN)
			wtpSqr = BITONE_WAVE_PWMIN;
		else if (wtpSqr > BITONE_WAVE_PWMAX)
			wtpSqr = BITONE_WAVE_PWMAX;

		if ((wtpSqr = wtp + wtpSqr) >= BITONE_WAVE_SZE)
			wtpSqr -= BITONE_WAVE_SZE;

		gdelta = wtp - ((float) ((int) wtp));
		pdelta = wtpSqr - ((float) ((int) wtpSqr));

		if (wtp >= BITONE_WAVE_SZE_M)
			ob[obp] += (ramp[(int) wtp]
				+ (ramp[0] - ramp[(int) wtp]) * gdelta) * gain;
		else
			ob[obp] += (ramp[(int) wtp]
				+ (ramp[(int) wtp + 1] - ramp[(int) wtp]) * gdelta) * gain;

		if (wtpSqr >= BITONE_WAVE_SZE_M)
			ob[obp] -= (ramp[(int) wtpSqr]
				+ (ramp[0] - ramp[(int) wtpSqr]) * pdelta) * gain;
		else
			ob[obp] -= (ramp[(int) wtpSqr]
				+ (ramp[(int) wtpSqr + 1] - ramp[(int) wtpSqr]) * pdelta) *gain;

		if (sb[obp] != 0)
		{
//			float s;

			gain = -gain;

			/*
			 * Sync point. Resample our target sample back to the new origin
			 * which we are going resample as well from the new wtp offset.
			 */
			wtp = sb[obp];

/*
			if ((wtpSqr = width + *pwmb) < BITONE_WAVE_PWMIN)
				wtpSqr = BITONE_WAVE_PWMIN;
			else if (wtpSqr > BITONE_WAVE_PWMAX)
				wtpSqr = BITONE_WAVE_PWMAX;

			wtpSqr += wtp;

			pdelta = wtpSqr - ((float) ((int) wtpSqr));

			s = ramp[0] * (1.0 - wtp) + ramp[1] * wtp
				- (ramp[(int) wtpSqr]
					+ (ramp[(int) wtpSqr + 1] - ramp[(int) wtpSqr]) * pdelta);

*/
			continue;
		}

		/*
		 * Roll the buffers forward - need to consider frequency and tuning.
		 */
		if ((wtp += ib[obp] * 0.25 * tune) >= BITONE_WAVE_SZE)
			while ((wtp -= BITONE_WAVE_SZE) >= BITONE_WAVE_SZE)
				;
		while (wtp < 0)
			wtp += BITONE_WAVE_SZE;
	}

	return(wtp);
}

static int operate(bristolOP *operator,
	bristolVoice *voice,
	bristolOPParams *param,
	void *lcl)
{
	register bristolBITONElocal *local = lcl;
	register int obp, count;
	register float *ib, *ob, *sob, *sb, *pwmb, *wt1, wtp, lsv, transp, ssg;
	register float *wt2, pw;
	bristolBITONE *specs;

	specs = (bristolBITONE *) operator->specs;

#ifdef BRISTOL_DBG
	printf("bit1osc(%x, %x, %x)\n", operator, param, local);
#endif

	count = specs->spec.io[BITONE_OUT_IND].samplecount;
	ib = specs->spec.io[BITONE_IN_IND].buf;
	ob = specs->spec.io[BITONE_OUT_IND].buf;
	sb = specs->spec.io[BITONE_SYNC_IND].buf;
	if ((sob = specs->spec.io[BITONE_SYNC_OUT].buf) == NULL)
		sob = specs->null;
	pwmb = specs->spec.io[BITONE_PWM_IND].buf;
	wt1 = (float *) param->param[0].mem;
	wt2 = (float *) param->param[1].mem;
	wtp = local->wtp;
	lsv = local->lsv;
	local->lsv = genSyncPoints(specs->sbuf, sb, lsv, count);
	ssg = local->ssg >= 0?param->param[6].float_val:-param->param[6].float_val;
//printf("%f %f %f %f\n", ssg, param->param[3].float_val, param->param[5].float_val, param->param[6].float_val);

	transp = param->param[8].float_val * param->param[9].float_val;

	if (voice->flags & BRISTOL_KEYON)
		local->wtp = wtp = local->wtppw1 = local->wtppw2 =
			local->wtppw3 = local->wtppw4 = local->wtppw5 = 0;

	/*
	 * This is not quite the crumar specification. It used < 50% PW with full
	 * PWM to give 3% to 50% Duty Cycle via velocity hence a widening pulse.
	 * At greater than 50% PW with span over 50% to 3% hence thinning pulse.
	 */
	pw = param->param[7].float_val
		+ (param->param[10].float_val * voice->velocity * BITONE_H_SZE);

	/*
	 * Here we should put in the pulse wave. The nice thing about making it a
	 * routine is that we can then consider calling it multiple times for the
	 * different harmonics. Quite CPU intensive. The sweeps should be factored
	 * by detune as well however that is not available from the voice.....
	 */
	local->wtppw1 = genPWM(ob, ib, specs->sbuf, pwmb, specs->wave[1],
		param->param[0].float_val * ssg,
		sweeps[0] * transp + 0.011 * voice->detune,
		local->wtppw1, lsv, count, pw);
	local->wtppw2 = genPWM(ob, ib, specs->sbuf, pwmb, specs->wave[1],
		param->param[1].float_val * ssg,
		sweeps[1] * transp + 0.013 * voice->detune,
		local->wtppw2, lsv, count, pw);
	local->wtppw3 = genPWM(ob, ib, specs->sbuf, pwmb, specs->wave[1],
		param->param[2].float_val * ssg,
		sweeps[2] * transp + 0.017 * voice->detune,
		local->wtppw3, lsv, count, pw);
	local->wtppw4 = genPWM(ob, ib, specs->sbuf, pwmb, specs->wave[1],
		param->param[3].float_val * ssg,
		sweeps[3] * transp + 0.07 * voice->detune,
		local->wtppw4, lsv, count, pw);
	local->wtppw5 = genPWM(ob, ib, specs->sbuf, pwmb, specs->wave[1],
		param->param[12].float_val * ssg,
		sweeps[4] * transp + 0.019 * voice->detune,
		local->wtppw5, lsv, count, pw);

	for (obp = 0; obp < count;obp++)
	{
		/*
		 * Take a sample from the wavetable into the output buffer, this is
		 * the combined tri/ramp waveform.
		 */
		if (((int) wtp) == BITONE_WAVE_SZE_M) {
			ob[obp] += (wt1[0] * (wtp - ((float) ((int) wtp)))
				+ wt1[(int) wtp] * (1.0 - (wtp - ((float) ((int) wtp)))));
			sob[obp] = (wt2[0] * (wtp - ((float) ((int) wtp)))
				+ wt2[(int) wtp] * (1.0 - (wtp - ((float) ((int) wtp)))));
		} else {
			ob[obp] += (wt1[(int) wtp + 1] * (wtp - ((float) ((int) wtp)))
				+ wt1[(int) wtp] * (1.0 - (wtp - ((float) ((int) wtp)))));
			sob[obp] = (wt2[(int) wtp + 1] * (wtp - ((float) ((int) wtp)))
				+ wt2[(int) wtp] * (1.0 - (wtp - ((float) ((int) wtp)))));
		}


		/* Corrected sync */
		if (specs->sbuf[obp] != 0)
		{
			ssg = -ssg;
			wtp = specs->sbuf[obp];
			continue;
		}

		/*
		 * Roll the buffers forward - need to consider frequency and tuning.
		 */
		if ((wtp += ib[obp] * 0.25 * transp) >= BITONE_WAVE_SZE)
			while ((wtp -= BITONE_WAVE_SZE) >= BITONE_WAVE_SZE)
				;
		while (wtp < 0)
			wtp += BITONE_WAVE_SZE;
	}

	local->wtp = wtp;
	local->ssg = ssg;

	return(0);
}

/*
 * Setup any variables in our OP structure, in our IO structures, and malloc
 * any memory we need.
 */
bristolOP *
bit1oscinit(bristolOP **operator, int index, int samplerate, int samplecount)
{
	bristolBITONE *specs;

#ifdef BRISTOL_DBG
	printf("bit1oscinit(%x(%x), %i, %i, %i)\n",
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

	specs = (bristolBITONE *) bristolmalloc0(sizeof(bristolBITONE));
	(*operator)->specs = (bristolOPSpec *) specs;
	(*operator)->size = sizeof(bristolBITONE);

	specs->sbuf = bristolmalloc(sizeof(float) * samplecount);
	specs->null = bristolmalloc(sizeof(float) * samplecount);

	/*
	 * These are specific to this operator, and will need to be altered for
	 * each operator.
	 */
	specs->spec.opname = OPNAME;
	specs->spec.description = OPDESCRIPTION;
	specs->spec.pcount = PCOUNT;
	specs->spec.iocount = IOCOUNT;
	specs->spec.localsize = sizeof(bristolBITONElocal);

	/*
	 * We are going to assign multiple waves to this oscillator.
	 * sine, ramp, square, triangle?
	 */
	specs->wave[0] = (float *) bristolmalloc(BITONE_WAVE_SZE * sizeof(float));
	specs->wave[1] = (float *) bristolmalloc(BITONE_WAVE_SZE * sizeof(float));
	specs->wave[2] = (float *) bristolmalloc(BITONE_WAVE_SZE * sizeof(float));
	specs->wave[3] = (float *) bristolmalloc(BITONE_WAVE_SZE * sizeof(float));
	specs->wave[4] = (float *) bristolmalloc(BITONE_WAVE_SZE * sizeof(float));
	specs->wave[5] = (float *) bristolmalloc(BITONE_WAVE_SZE * sizeof(float));
	specs->wave[6] = (float *) bristolmalloc(BITONE_WAVE_SZE * sizeof(float));
	specs->wave[7] = (float *) bristolmalloc(BITONE_WAVE_SZE * sizeof(float));

	/*
	 * FillWave is something that should be called as a parameter change, but
	 * for testing will load it here.
	 */
	fillWave(specs->wave[0], BITONE_WAVE_SZE, 0);
	fillWave(specs->wave[1], BITONE_WAVE_SZE, 1);
	fillWave(specs->wave[2], BITONE_WAVE_SZE, 2);
	fillWave(specs->wave[3], BITONE_WAVE_SZE, 3);
	fillWave(specs->wave[4], BITONE_WAVE_SZE, 4);
	fillWave(specs->wave[5], BITONE_WAVE_SZE, 5);
	fillWave(specs->wave[6], BITONE_WAVE_SZE, 6);
	fillWave(specs->wave[7], BITONE_WAVE_SZE, 7);

	/*
	 * Now fill in the bit1osc specs for this operator. These are specific to an
	 * oscillator.
	 */
	specs->spec.param[0].pname = "16' strength";
	specs->spec.param[0].description = "gain of 16' harmonic";
	specs->spec.param[0].type = BRISTOL_FLOAT;
	specs->spec.param[0].low = 0;
	specs->spec.param[0].high = 1;
	specs->spec.param[0].flags = BRISTOL_ROTARY|BRISTOL_SLIDER;

	specs->spec.param[1].pname = "8' strength";
	specs->spec.param[1].description = "gain of 8' harmonic";
	specs->spec.param[1].type = BRISTOL_FLOAT;
	specs->spec.param[1].low = 0;
	specs->spec.param[1].high = 1;
	specs->spec.param[1].flags = BRISTOL_ROTARY|BRISTOL_SLIDER;

	specs->spec.param[2].pname = "4' strength";
	specs->spec.param[2].description = "gain of 4' harmonic";
	specs->spec.param[2].type = BRISTOL_FLOAT;
	specs->spec.param[2].low = 0;
	specs->spec.param[2].high = 1;
	specs->spec.param[2].flags = BRISTOL_ROTARY|BRISTOL_SLIDER;

	specs->spec.param[3].pname = "2' strength";
	specs->spec.param[3].description = "gain of 2' harmonic";
	specs->spec.param[3].type = BRISTOL_FLOAT;
	specs->spec.param[3].low = 0;
	specs->spec.param[3].high = 1;
	specs->spec.param[3].flags = BRISTOL_ROTARY|BRISTOL_SLIDER;

	specs->spec.param[4].pname = "tri' strength";
	specs->spec.param[4].description = "gain of tri' harmonic";
	specs->spec.param[4].type = BRISTOL_FLOAT;
	specs->spec.param[4].low = 0;
	specs->spec.param[4].high = 1;
	specs->spec.param[4].flags = BRISTOL_ROTARY|BRISTOL_SLIDER;

	specs->spec.param[5].pname = "ramp' strength";
	specs->spec.param[5].description = "gain of ramp' harmonic";
	specs->spec.param[5].type = BRISTOL_FLOAT;
	specs->spec.param[5].low = 0;
	specs->spec.param[5].high = 1;
	specs->spec.param[5].flags = BRISTOL_ROTARY|BRISTOL_SLIDER;

	specs->spec.param[6].pname = "pulse' strength";
	specs->spec.param[6].description = "gain of pulse' harmonic";
	specs->spec.param[6].type = BRISTOL_FLOAT;
	specs->spec.param[6].low = 0;
	specs->spec.param[6].high = 1;
	specs->spec.param[6].flags = BRISTOL_ROTARY|BRISTOL_SLIDER;

	specs->spec.param[7].pname = "Pulse Width";
	specs->spec.param[7].description = "Pulse width of square wave";
	specs->spec.param[7].type = BRISTOL_FLOAT;
	specs->spec.param[7].low = 0;
	specs->spec.param[7].high = 1;
	specs->spec.param[7].flags = BRISTOL_ROTARY|BRISTOL_SLIDER;

	specs->spec.param[8].pname = "Freq";
	specs->spec.param[8].description = "12 semitone steps";
	specs->spec.param[8].type = BRISTOL_FLOAT;
	specs->spec.param[8].low = 0;
	specs->spec.param[8].high = 1;
	specs->spec.param[8].flags = BRISTOL_ROTARY|BRISTOL_SLIDER;

	specs->spec.param[9].pname = "Tune";
	specs->spec.param[9].description = "1/2 semitone up or down";
	specs->spec.param[9].type = BRISTOL_FLOAT;
	specs->spec.param[9].low = 0;
	specs->spec.param[9].high = 1;
	specs->spec.param[9].flags = BRISTOL_ROTARY|BRISTOL_SLIDER;

	specs->spec.param[10].pname = "Velocity PWM";
	specs->spec.param[10].description = "PW tracking velocity";
	specs->spec.param[10].type = BRISTOL_FLOAT;
	specs->spec.param[10].low = 0;
	specs->spec.param[10].high = 1;
	specs->spec.param[10].flags = BRISTOL_ROTARY|BRISTOL_SLIDER;

	specs->spec.param[11].pname = "Square Gain";
	specs->spec.param[11].description = "Gain of square wave";
	specs->spec.param[11].type = BRISTOL_FLOAT;
	specs->spec.param[11].low = 0;
	specs->spec.param[11].high = 1;
	specs->spec.param[11].flags = BRISTOL_ROTARY|BRISTOL_SLIDER;

	specs->spec.param[12].pname = "Subharmonic Gain";
	specs->spec.param[12].description = "Gain of 32' wave";
	specs->spec.param[12].type = BRISTOL_FLOAT;
	specs->spec.param[12].low = 0;
	specs->spec.param[12].high = 1;
	specs->spec.param[12].flags = BRISTOL_ROTARY|BRISTOL_SLIDER;

	/*
	 * Now fill in the bit1osc IO specs.
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

	specs->spec.io[2].ioname = "PW Modulating Input";
	specs->spec.io[2].description = "input of signal for PWM process";
	specs->spec.io[2].samplerate = samplerate;
	specs->spec.io[2].samplecount = samplecount;
	specs->spec.io[2].flags = BRISTOL_AC|BRISTOL_OUTPUT;

	specs->spec.io[3].ioname = "Sync Input";
	specs->spec.io[3].description = "input to phase sync process";
	specs->spec.io[3].samplerate = samplerate;
	specs->spec.io[3].samplecount = samplecount;
	specs->spec.io[3].flags = BRISTOL_AC|BRISTOL_OUTPUT;

	specs->spec.io[4].ioname = "Sync Output";
	specs->spec.io[4].description = "simple output signal for driving sync";
	specs->spec.io[4].samplerate = samplerate;
	specs->spec.io[4].samplecount = samplecount;
	specs->spec.io[4].flags = BRISTOL_AC|BRISTOL_OUTPUT;

	return(*operator);
}

/*
 * Hm, just in case you could not tell, this is a phase distortion oscillator.
 * It is somewhat warmer than the original bit1osc.
 */
static void fillPDwave(register float *mem, register int count,
register double reach)
{
	register int i, recalc1 = 1, recalc2 = 1;
	register double j = 0, Count = (double) count, inc = reach;

	for (i = 0;i < count; i++)
	{
		mem[i] = sin(((double) (2 * M_PI * j)) / Count) * BITONE_WAVE_GAIN;

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

#define S_DEC 0.999f

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
		default:
			/*
			 * This will be a sine wave. We have count samples, and
			 * 2PI radians in a full sine wave. Thus we take
			 * 		(2PI * i / count) * 2048.
			 */
			fillPDwave(mem, count, 1.0);
			return;
		case 1:
			/*
			 * Ramp wave
			 */
			if (blo.flags & BRISTOL_BLO)
			{
				int i;

				for (i = 0;i < count; i++)
					mem[i] = bloramp[i];
				return;
			}
			fillPDwave(mem, count, 20.0);
			return;
		case 4:
		{
			float value = 8.0;
			int i;

			/* 
			 * This is a square wave, with decaying plateaus.
			 */
			if (blo.flags & BRISTOL_BLO)
			{
				for (i = 0;i < count; i++)
					mem[i] = blosquare[i];
				return;
			}
			for (i = 0;i < count / 2; i++)
				mem[i] = (value * S_DEC);
			value = -8.0;
			for (;i < count; i++)
				mem[i] = (value * S_DEC);
			return;
		}
		case 2:
			{
				int i;

				if (blo.flags & BRISTOL_BLO)
				{
					for (i = 0;i < count; i++)
						mem[i] = blotriangle[i];
					return;
				}
				for (i = 0;i < count / 2; i++)
					mem[i] = -BITONE_WAVE_GAIN
						+ ((float) i / (count / 2)) * BITONE_WAVE_GAIN * 2;
				for (;i < count; i++)
					mem[i] = BITONE_WAVE_GAIN -
						(((float) (i - count / 2) * 2) / (count / 2))
							* BITONE_WAVE_GAIN;
		}
			break;
	}
}

static void
buildBitoneSound(bristolOP *operator, bristolOPParams *param)
{
	register float gain, *source, *dest, sweep, oscindex;
	register int i, j;
	bristolBITONE *specs;
	int done = 0;

	specs = (bristolBITONE *) operator->specs;

#ifdef BRISTOL_DBG
	printf("fillBitoneWave(%x)\n", operator);
#endif

	/*
	 * Dest is actually the local wave for this operator.
	 */
	bristolbzero(param->param[0].mem, BITONE_WAVE_SZE * sizeof(float));

	/*
	 * Take the tri to the destination wave.
	 */
	source = specs->wave[2];
	for (i = 0; i < 5; i++)
	{
		/* We should try some semi random start indeces? */
		oscindex = 0;

		dest = param->param[0].mem;

		/* Get the gain and beef it up a bit for the triwaves */
		if (i == 4)
			gain = param->param[12].float_val * param->param[4].float_val * 2;
		else
			gain = param->param[i].float_val * param->param[4].float_val * 2;

		sweep = sweeps[i] * 2;

		if (gain == 0)
			continue;

		done++;

		for (j = 0; j < BITONE_WAVE_SZE; j++)
		{
			*dest++ += source[(int) oscindex] * gain;
			if ((oscindex += sweep) >= BITONE_WAVE_SZE)
				oscindex -= BITONE_WAVE_SZE;
		}
	}

	/*
	 * Take the ramp to the destination wave.
	 */
	source = specs->wave[1];
	for (i = 0; i < 5; i++)
	{
		oscindex = 0;

		dest = param->param[0].mem;

		if (i == 4)
			gain = param->param[12].float_val * param->param[5].float_val;
		else
			gain = param->param[i].float_val * param->param[5].float_val;

		sweep = sweeps[i] * 2;

		if (gain == 0)
			continue;

		done++;

		for (j = 0; j < BITONE_WAVE_SZE; j++)
		{
			*dest++ += source[(int) oscindex] * gain;
			if ((oscindex += sweep) >= BITONE_WAVE_SZE)
				oscindex -= BITONE_WAVE_SZE;
		}
	}

	/*
	 * Take the square to the destination wave.
	 */
	source = specs->wave[4];
	for (i = 0; i < 5; i++)
	{
		oscindex = 0;

		dest = param->param[0].mem;

		if (i == 4)
 			gain = param->param[12].float_val * param->param[11].float_val
				* 0.7;
		else
			gain = param->param[i].float_val * param->param[11].float_val
				* 0.7;

		sweep = sweeps[i] * 2;

		if (gain == 0)
			continue;

		done++;

		for (j = 0; j < BITONE_WAVE_SZE; j++)
		{
			*dest++ += source[(int) oscindex] * gain;
			if ((oscindex += sweep) >= BITONE_WAVE_SZE)
				oscindex -= BITONE_WAVE_SZE;
		}
	}

	/*
	 * Take the square to the sync waveform
	 */
	source = specs->wave[4];

	oscindex = 0;

	dest = param->param[1].mem;

	gain = 1.0;

	sweep = sweeps[1] * 2;

	for (j = 0; j < BITONE_WAVE_SZE; j++)
	{
		*dest++ += source[(int) oscindex] * gain;
		if ((oscindex += sweep) >= BITONE_WAVE_SZE)
			oscindex -= BITONE_WAVE_SZE;
	}

	/*
	 * Okay, if we get here there were no ramp or tri requested. Take a peek
	 * at square and if that is also not requested then stuff in some sines.
	 */
	if ((done != 0) || (param->param[6].float_val != 0))
		return;

	source = specs->wave[0];
	for (i = 0; i < 5; i++)
	{
		oscindex = 0;

		dest = param->param[0].mem;


		if (i == 4)
 			gain = param->param[12].float_val;
		else
			gain = param->param[i].float_val;

		sweep = sweeps[i] * 2;

		if (gain == 0)
			continue;

		done++;

		for (j = 0; j < BITONE_WAVE_SZE; j++)
		{
			*dest++ += source[(int) oscindex] * gain;
			if ((oscindex += sweep) >= BITONE_WAVE_SZE)
				oscindex -= BITONE_WAVE_SZE;
		}
	}
}

