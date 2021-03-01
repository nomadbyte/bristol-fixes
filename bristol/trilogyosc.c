
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
 *	trilogyoscinit()
 *	operate()
 *	reset()
 *	destroy()
 *
 *	destroy() is in the library.
 *
 * Operate will be called when all the inputs have been loaded, and the result
 * will be an output buffer written to the next operator.
 */

#include <stdlib.h>
#include <math.h>

#include "bristol.h"
#include "bristolblo.h"
#include "trilogyosc.h"

float note_diff;
int samplecount;

static void fillWave(float *, int, int);
static void buildTrilogySound(bristolOP *, bristolOPParams *);
static float *zerobuf;

/*
 * Use of these mean that we can only product a single trilogyosc oscillator.
 * Even though it is reasonably flexible this does lead to problems, and if
 * we move to a dual manual Trilogy it will need to be resolved.
 *
 * The same is true of using single global result waves, wave[6] and wave[7].
 * These need to be moved to private address space, ie, we need more 
 * instantiation.
 */

/*
 * This can be a single list, it is used to generate the different pipes.
 *
 * We have 16, 8, 4, 2.
 */
static float sweeps[16] = {2, 4, 8, 16, 1, 32, 12, 20, 24, 32, 0,0,0,0,0,0};

/*
 * The name of this operator, IO count, and IO names.
 */
#define OPNAME "Trilogy"
#define OPDESCRIPTION "Trilogy Digitally Controlled Oscillator"
#define PCOUNT 13
#define IOCOUNT 4

#define TRILOGY_IN_IND 0
#define TRILOGY_OUT1_IND 1
#define TRILOGY_OUT2_IND 2
#define TRILOGY_PWM_IND 3
#define TRILOGY_SYNC_IND 4

/*
 * Reset any local memory information.
 */
static int destroy(bristolOP *operator)
{
#ifdef BRISTOL_DBG
	printf("trilogyoscdestroy(%x)\n", operator);
#endif

	/*
	 * Unmalloc anything we added to this structure
	 */
	bristolfree(((bristolTRILOGY *) operator)->wave[0]);
	bristolfree(((bristolTRILOGY *) operator)->wave[1]);
	bristolfree(((bristolTRILOGY *) operator)->wave[2]);
	bristolfree(((bristolTRILOGY *) operator)->wave[3]);
	bristolfree(((bristolTRILOGY *) operator)->wave[4]);
	bristolfree(((bristolTRILOGY *) operator)->wave[5]);
	bristolfree(((bristolTRILOGY *) operator)->wave[6]);
	bristolfree(((bristolTRILOGY *) operator)->wave[7]);

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
	printf("trilogyoscreset(%x, %x)\n", operator, param);
#endif

	/*
	 * For the trilogyosc we want to put in a few local memory variables for this
	 * operator specifically.
	 *
	 * These are sineform, waveindex, wavelevel, and percussions, plus two
	 * unique wavetables.
	 */
	if (param->param[0].mem != NULL)
		bristolfree(param->param[0].mem);
	param->param[0].mem = bristolmalloc0(sizeof(float) * TRILOGY_WAVE_SZE);

	param->param[0].float_val = 1.0;
	param->param[1].float_val = 0.2;
	param->param[2].float_val = 0.2;
	param->param[3].float_val = 1.0;
	param->param[4].float_val = 1.0;
	param->param[5].float_val = 0.5;
	param->param[6].float_val = 1.0;
	param->param[7].int_val = 0;
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
printf("	trilogyoscparam(%x, %x, %i, %f)\n", (size_t) operator, (size_t) param, index,
value);
#endif

	switch (index) {
		/* The first four are the harmonics */
		case 0:
		case 1:
		case 2:
		case 3:
			param->param[index].float_val = value;
			break;
		case 4:
			/* Waveform */
			param->param[index].float_val = value;
			buildTrilogySound(operator, param);
			break;
		case 5:
			/* Phase separation */
			param->param[index].float_val = value * value * value * 3;
			break;
		case 7:
			/* Waveform selections */
			param->param[index].float_val = value;
			param->param[index].int_val = value == 0? 0:1;
			break;
		case 6:
			/* Modulation depth */
			param->param[index].float_val = value * value * value * 3;
			break;
		case 11:
			param->param[index].float_val = value;
			buildTrilogySound(operator, param);
			break;
		case 12:
			/* Subharmonic, will be used for the invariable string harmonic */
			param->param[index].float_val = value;
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
				 * Up or down 7 notes.
				 */
				if (tune > 0)
					notes *= note_diff;
				else
					notes /= note_diff;

				if (tune > 0)
					param->param[index].float_val = 1.0 + (notes - 1) * tune;
				else
					param->param[index].float_val = 1.0 - (1 - notes) * -tune;

				break;
			}
			break;
		case 10:
			param->param[index].float_val = value;
			break;
	}

	return(0);
}

/*
 * This takes a lot of stacked parameters and could be optimised by putting
 * them in registers.
	genWave(ob, ib, sb, pwb, specs->wave[1], 1.0, 2.0, wtp, lsv, count, 512);
 */
static float
genWave(register float *ob, register float *ib, register float *sb,
register float *modbuf, register float *wt1, register float gain,
register float tune, register float wtp, register float lsv,
register int count, register float inv)
{
	register int obp;

	if (gain == 0.0)
		return(wtp);

	while (wtp >= TRILOGY_WAVE_SZE)
		wtp -= TRILOGY_WAVE_SZE;
	while (wtp < 0)
		wtp += TRILOGY_WAVE_SZE;

	for (obp = 0; obp < count;obp++)
	{
		/*
		 * Take a sample from the wavetable into the output buffer, this is
		 * the combined tri/ramp waveform.
		 * First sync to some input signal. We need to check on how we are
		 * going to resample this since the previous sample has been lost.
		if ((lsv < 0) && (sb[obp] >= 0)) {
			ob[obp] += (wt1[0] * (wtp - ((float) ((int) wtp)))
				+ wt1[(int) wtp] * (1.0 - (wtp - ((float) ((int) wtp)))))
					* gain;
		 */
		if ((((int) wtp) == TRILOGY_WAVE_SZE_M)
			|| ((lsv < 0) && (sb[obp] >= 0)))
			ob[obp] += (wt1[0] * (wtp - ((float) ((int) wtp)))
				+ wt1[(int) wtp] * (1.0 - (wtp - ((float) ((int) wtp)))))
					* gain;
		else
			ob[obp] += (wt1[(int) wtp + 1] * (wtp - ((float) ((int) wtp)))
				+ wt1[(int) wtp] * (1.0 - (wtp - ((float) ((int) wtp)))))
					* gain;

		/*
		 * Roll the buffers forward - need to consider frequency and tuning.
		 */
		if ((lsv < 0) && (sb[obp] >= 0))
			wtp -= (float) ((int) wtp);
//			wtp = 0;
		else {
			if ((wtp += ib[obp] * 0.5 * tune + modbuf[obp] * inv)
				>= TRILOGY_WAVE_SZE)
				while ((wtp -= TRILOGY_WAVE_SZE) >= TRILOGY_WAVE_SZE)
					;
			while (wtp < 0)
				wtp += TRILOGY_WAVE_SZE;
		}

		lsv = sb[obp];
	}

	return(wtp);
}

/*
 * As of the first write, 9/11/01, this takes nearly 20mips to operate a single
 * oscillator. Will work on optimisation of the code, using non-referenced 
 * variables in register workspace.
 *
 *	output buffer pointer.
 *	output buffer index.
 *	input buffer index.
 *	wavetable.
 *	wavetable index.
 *	count.
 *	gain.
 *
 * This oscillator is going to have 4 continous controllers for the signal
 * strengths of the different harmonics, 32/16/8/4. It will have different
 * gain levels for the tri and ramp waves. Finally the phase shifting pulse
 * wave will function probably on the lowest selected harmonic at first since
 * we are not going to recalculate it unless the results of two phase shifting
 * pulse harmonics turns out to be seriously attractive.
 */
static int operate(bristolOP *operator,
	bristolVoice *voice,
	bristolOPParams *param,
	void *lcl)
{
	register bristolTRILOGYlocal *local = lcl;
	register int count;
	register float *ib, *ob, *sb, *pwb, lsv, transp;
	bristolTRILOGY *specs;

	specs = (bristolTRILOGY *) operator->specs;

#ifdef BRISTOL_DBG
	printf("trilogyosc(%x, %x, %x)\n", operator, param, local);
#endif

	count = specs->spec.io[TRILOGY_OUT1_IND].samplecount;
	ib = specs->spec.io[TRILOGY_IN_IND].buf;
	ob = specs->spec.io[TRILOGY_OUT1_IND].buf;
	sb = specs->spec.io[TRILOGY_SYNC_IND].buf;
	pwb = specs->spec.io[TRILOGY_PWM_IND].buf;
	lsv = local->lsv;

	transp = param->param[8].float_val * param->param[9].float_val;

	/*
	 * Here we should put in the pulse wave. The nice thing about making it a
	 * routine is that we can then consider calling it multiple times for the
	 * different harmonics. Quite CPU intensive. The sweeps should be factored
	 * by detune as well however that is not available from the voice.....
	 *
	 * Reparameterize the operator for
	 * 	controllable detune
	 *	controllable spread
	 *	controllable distorts
	 *
	 * Insert the base wave, then the distorted wave based on a single param
	 */
	local->wtppw11 = genWave(specs->spec.io[TRILOGY_OUT1_IND].buf,
		ib, sb, zerobuf, specs->wave[param->param[7].int_val],
		param->param[0].float_val,
		sweeps[0] * transp,
		local->wtppw11, lsv, count, 0.0);
	local->wtppw12 = genWave(specs->spec.io[TRILOGY_OUT2_IND].buf,
		ib, sb, zerobuf, specs->wave[param->param[7].int_val],
		param->param[1].float_val,
		sweeps[1] * transp,
		local->wtppw12, lsv, count, 0.0);
	local->wtppw13 = genWave(specs->spec.io[TRILOGY_OUT2_IND].buf,
		ib, sb, zerobuf, specs->wave[param->param[7].int_val],
		param->param[2].float_val,
		sweeps[2] * transp,
		local->wtppw13, lsv, count, 0.);
	local->wtppw14 = genWave(specs->spec.io[TRILOGY_OUT1_IND].buf,
		ib, sb, zerobuf, specs->wave[param->param[7].int_val],
		param->param[3].float_val,
		sweeps[3] * transp,
		local->wtppw14, lsv, count, 0.0);

	local->wtppw1 = genWave(specs->spec.io[TRILOGY_OUT2_IND].buf,
		ib, sb, pwb, specs->wave[param->param[7].int_val],
		param->param[0].float_val,
		sweeps[0] * transp + -0.11 * voice->detune * param->param[5].float_val,
		local->wtppw1, lsv, count, 0.195 * param->param[6].float_val);
	local->wtppw2 = genWave(specs->spec.io[TRILOGY_OUT1_IND].buf,
		ib, sb, pwb, specs->wave[param->param[7].int_val],
		param->param[1].float_val,
		sweeps[1] * transp + 0.13 * voice->detune * param->param[5].float_val,
		local->wtppw2, lsv, count, -0.19 * param->param[6].float_val);
	local->wtppw3 = genWave(specs->spec.io[TRILOGY_OUT1_IND].buf,
		ib, sb, pwb, specs->wave[param->param[7].int_val],
		param->param[2].float_val,
		sweeps[2] * transp + -0.5 * voice->detune * param->param[5].float_val,
		local->wtppw3, lsv, count, 0.18 * param->param[6].float_val);
	local->wtppw4 = genWave(specs->spec.io[TRILOGY_OUT2_IND].buf,
		ib, sb, pwb, specs->wave[param->param[7].int_val],
		param->param[3].float_val,
		sweeps[3] * transp + 0.7 * voice->detune * param->param[5].float_val,
		local->wtppw4, lsv, count, -0.21 * param->param[6].float_val);

	local->wtppw5 = genWave(specs->spec.io[TRILOGY_OUT1_IND].buf,
		ib, sb, zerobuf, specs->wave[1],
		param->param[12].float_val,
		sweeps[4] * transp,
		local->wtppw5, lsv, count, 0.0);
	local->wtppw15 = genWave(specs->spec.io[TRILOGY_OUT2_IND].buf,
		ib, sb, zerobuf, specs->wave[1],
		param->param[12].float_val,
		sweeps[4] * transp,
		local->wtppw15, lsv, count, 0.0);

	local->lsv = sb[count - 1];

	return(0);
}

/*
 * Setup any variables in our OP structure, in our IO structures, and malloc
 * any memory we need.
 */
bristolOP *
trilogyoscinit(bristolOP **operator, int index, int samplerate, int samplecount)
{
	bristolTRILOGY *specs;

#ifdef BRISTOL_DBG
	printf("trilogyoscinit(%x(%x), %i, %i, %i)\n",
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

	specs = (bristolTRILOGY *) bristolmalloc0(sizeof(bristolTRILOGY));
	(*operator)->specs = (bristolOPSpec *) specs;
	(*operator)->size = sizeof(bristolTRILOGY);

	/*
	 * These are specific to this operator, and will need to be altered for
	 * each operator.
	 */
	specs->spec.opname = OPNAME;
	specs->spec.description = OPDESCRIPTION;
	specs->spec.pcount = PCOUNT;
	specs->spec.iocount = IOCOUNT;
	specs->spec.localsize = sizeof(bristolTRILOGYlocal);

	/*
	 * We are going to assign multiple waves to this oscillator.
	 * sine, ramp, square, triangle?
	 */
	specs->wave[0] = (float *) bristolmalloc(TRILOGY_WAVE_SZE * sizeof(float));
	specs->wave[1] = (float *) bristolmalloc(TRILOGY_WAVE_SZE * sizeof(float));
	specs->wave[2] = (float *) bristolmalloc(TRILOGY_WAVE_SZE * sizeof(float));
	specs->wave[3] = (float *) bristolmalloc(TRILOGY_WAVE_SZE * sizeof(float));
	specs->wave[4] = (float *) bristolmalloc(TRILOGY_WAVE_SZE * sizeof(float));
	specs->wave[5] = (float *) bristolmalloc(TRILOGY_WAVE_SZE * sizeof(float));
	specs->wave[6] = (float *) bristolmalloc(TRILOGY_WAVE_SZE * sizeof(float));
	specs->wave[7] = (float *) bristolmalloc(TRILOGY_WAVE_SZE * sizeof(float));

	zerobuf = (float *) bristolmalloc(TRILOGY_WAVE_SZE * sizeof(float));
	bristolbzero(zerobuf, TRILOGY_WAVE_SZE * sizeof(float));

	/*
	 * FillWave is something that should be called as a parameter change, but
	 * for testing will load it here.
	 */
	fillWave(specs->wave[0], TRILOGY_WAVE_SZE, 0);
	fillWave(specs->wave[1], TRILOGY_WAVE_SZE, 1);
	fillWave(specs->wave[2], TRILOGY_WAVE_SZE, 2);
	fillWave(specs->wave[3], TRILOGY_WAVE_SZE, 3);
	fillWave(specs->wave[4], TRILOGY_WAVE_SZE, 4);
	fillWave(specs->wave[5], TRILOGY_WAVE_SZE, 5);
	fillWave(specs->wave[6], TRILOGY_WAVE_SZE, 6);
	fillWave(specs->wave[7], TRILOGY_WAVE_SZE, 7);

	/*
	 * Now fill in the trilogyosc specs for this operator. These are specific to an
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

	specs->spec.param[7].pname = "Waveform selector";
	specs->spec.param[7].description = "Phase distortions or BLO";
	specs->spec.param[7].type = BRISTOL_INT;
	specs->spec.param[7].low = 0;
	specs->spec.param[7].high = 1;
	specs->spec.param[7].flags = BRISTOL_BUTTON;

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
	 * Now fill in the trilogyosc IO specs.
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

	return(*operator);
}

/*
 * Hm, just in case you could not tell, this is a phase distortion oscillator.
 * It is somewhat warmer than the original trilogyosc.
 */
static void fillPDwave(register float *mem, register int count,
register double reach)
{
	register int i, recalc1 = 1, recalc2 = 1;
	register double j = 0, Count = (double) count, inc = reach;

	for (i = 0;i < count; i++)
	{
		mem[i] = sin(((double) (2 * M_PI * j)) / Count) * TRILOGY_WAVE_GAIN;

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
					mem[i] = -TRILOGY_WAVE_GAIN
						+ ((float) i / (count / 2)) * TRILOGY_WAVE_GAIN * 2;
				for (;i < count; i++)
					mem[i] = TRILOGY_WAVE_GAIN -
						(((float) (i - count / 2) * 2) / (count / 2))
							* TRILOGY_WAVE_GAIN;
		}
			break;
	}
}

#define TRILOGY_WAVE_GAIN 12

/*
 * So, rather than tri waves we also need squares, or perhaps more so. This is
 * going to be a distorted cosine wave: depending on the 'reach' then we
 * scan through the cosine faster, filling the space with "1's".
 */
static void
fillsquarewave(register float *mem, register int count,
register float reach)
{
	register int i, newcount;
	register float increment = 0.0;

	if (reach < 0.025)
		reach = 0.025;

	/* 
	 * at 0.5 this is a pure size, as the value goes down we tend to a pulse.
	 */
	newcount = reach * count;
	reach = 2.0 / (float) newcount;

	for (i = 0;i < newcount; i++)
		mem[i] = cosf(M_PI * (increment += reach)) * TRILOGY_WAVE_GAIN;

	for (;i < count; i++)
		mem[i] = TRILOGY_WAVE_GAIN;
}

static void
fillPDcos(float *mem, int count, float compress)
{
	float j = 0, i, inc1, inc2;
	int comp = compress * count / 4;

	if (comp < 1)
		comp = 1;

	/*
	 * Resample the sine wave as per casio phase distortion algorithms.
	 *
	 * We have to get to M_PI/2 in compress steps, hence
	 *
	 *	 inc1 = M_PI/(2 * compress)
	 *
	 * Then we have to scan M_PI in (count - 2 * compress), hence:
	 *
	 *	 inc2 = M_PI/(count - 2 * compress)
	 */
	inc1 = ((float) M_PI) / (((float) 2) * ((float) comp));
	inc2 = ((float) M_PI) / ((float) (count - 2 * comp));

	for (i = 0;i < count; i++)
	{
		*mem++ = cosf(j) * TRILOGY_WAVE_GAIN;

		if (i < comp * 2)
			j += inc1;
		else
			j += inc2;
	}
}

/*
 * Resample the sine wave as per casio phase distortion algorithms.
 *
 * We have to get to M_PI/2 in compress steps, hence
 *
 *	 inc1 = M_PI/(2 * compress)
 *
 * Then we have to scan M_PI in (count - 2 * compress), hence:
 *
 *	 inc2 = M_PI/(count - 2 * compress)
static void
fillPDsine(float *mem, int count, float compress)
{
	float j = 0, i, inc1, inc2;
	int comp = compress * count / 4;

	if (comp < 1)
		comp = 1;
	inc1 = ((float) M_PI) / (((float) 2) * ((float) comp));
	inc2 = ((float) M_PI) / ((float) (count - 2 * comp));

	for (i = 0;i < count; i++)
	{
		*mem++ = sinf(j) * TRILOGY_WAVE_GAIN;

		if (i < comp)
			j += inc1;
		else if (i > (count - 2 * comp))
			j += inc1;
		else
			j += inc2;
	}
}
 */

static void
buildTrilogySound(bristolOP *operator, bristolOPParams *param)
{
	bristolTRILOGY *specs;

	specs = (bristolTRILOGY *) operator->specs;

#ifdef BRISTOL_DBG
	printf("fillTrilogyWave(%x)\n", operator);
#endif

	/*
	 * Dest is actually the local wave for this operator.
	 */
	bristolbzero(param->param[0].mem, TRILOGY_WAVE_SZE * sizeof(float));
	bristolbzero(specs->wave[0], TRILOGY_WAVE_SZE * sizeof(float));

	if (param->param[4].float_val > 0.5)
		fillPDcos(specs->wave[0],
			TRILOGY_WAVE_SZE,
				1.0 - (param->param[4].float_val - 0.5) * 2);
	else
		fillsquarewave(specs->wave[0],
			TRILOGY_WAVE_SZE, param->param[4].float_val * 2);
}

