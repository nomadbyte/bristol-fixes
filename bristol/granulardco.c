
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
 *	quantuminit()
 *	operate()
 *	reset()
 *	destroy()
 *
 *	destroy() is in the library.
 *
 * Operate will be called when all the inputs have been loaded, and the result
 * will be an output buffer written to the next operator.
 */

#include <math.h>

#include "bristol.h"
#include "granulardco.h"

float note_diff;
float samplerate;

#define BRISTOL_SQR 4

/*
 * The name of this operator, IO count, and IO names. This is where possible
 * identical to the Prophet DCO, but we need separated outputs. We could use
 * it for the Prophet as well but that would incur the cost of extra mixing
 * code.
 */
#define OPNAME "DCO"
#define OPDESCRIPTION "Digitally Controlled Granular Oscillator"
#define PCOUNT 8
#define IOCOUNT 12

#define DCO_IN_FREQ		0
#define DCO_OUT_LEFT	1
#define DCO_OUT_RIGHT	2

#define MODBUS1			3
#define MODBUS2			4
#define MODBUS3			5
#define MODBUS4			6
#define MODBUS5			7
#define MODBUS6			8
#define MODBUS7			9
#define MODBUS8			10
#define MODBUS9			11

/*
 * MOD BUSSES: Gain, Frequency, Delay, Duration
 */
#define GAIN_MODBUS			3
#define GAIN_SCATTERBUS		4
#define FREQ_MODBUS			5
#define FREQ_SCATTERBUS		6
#define DELAY_MODBUS		7
#define DELAY_SCATTERBUS	8
#define DURATION_MODBUS		9
#define DURATION_SCATTERBUS	10

/*
 * OPERATOR PARAMETERS:
 */
#define GAIN_VALUE			0
#define GAIN_SCATTER		1
#define FREQUENCY_VALUE		2
#define FREQUENCY_SCATTER	3
#define DELAY_VALUE			4
#define DELAY_SCATTER		5
#define DURATION_VALUE		6
#define DURATION_SCATTER	7

#define GDCO_TRANSPOSE		8
#define GDCO_THREADS		9
#define GDCO_WAVEFORM		10
#define GDCO_BANKSEL		11

static void fillWave();

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
	bristolfree(((bristolGRANULARDCO *) operator)->wave[0]);
	bristolfree(((bristolGRANULARDCO *) operator)->wave[1]);
	bristolfree(((bristolGRANULARDCO *) operator)->wave[2]);
	bristolfree(((bristolGRANULARDCO *) operator)->wave[3]);
	bristolfree(((bristolGRANULARDCO *) operator)->wave[4]);
	bristolfree(((bristolGRANULARDCO *) operator)->wave[5]);
	bristolfree(((bristolGRANULARDCO *) operator)->wave[6]);
	bristolfree(((bristolGRANULARDCO *) operator)->wave[7]);

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
	printf("granularreset(%x)\n", operator);
#endif

	param->param[0].float_val = 1.0;
	param->param[1].float_val = 1.0;
	param->param[2].float_val = 1.0;
	param->param[3].float_val = 1.0;
	param->param[3].int_val = 2.0;

	param->param[4].float_val = 1.0;
	param->param[5].float_val = 1.0;
	param->param[6].float_val = 1.0;
	param->param[7].float_val = 1.0;

	param->param[8].float_val = 1.0;
	param->param[9].float_val = 1.0;
	param->param[10].float_val = 1.0;
	param->param[11].float_val = 1.0;

	param->param[12].float_val = 1.0;
	param->param[13].float_val = 1.0;
	param->param[14].float_val = 1.0;
	param->param[15].float_val = 1.0;

	return(0);
}

/*
 * Alter an internal parameter of an operator. For the granular oscillator we
 * have rather unique parameters.
 *
 * After that we have several parameters that each have two options, those being
 * depth and variance: gain, frequency, duration.
 * These are applied to each grain as it is generated, one parameter defines
 * the width of the value - its maximum, and the second one defines the variance
 * from that value. With no variance we only take the connfigured value, and as
 * variance is raised we introduce more randomness into the reproduction.
 */
static int param(bristolOP *operator, bristolOPParams *param,
	unsigned char index, float value)
{
#ifdef BRISTOL_DBG
	printf("granulardcoparam(%x, %x, %i, %f)\n", operator, param, index, value);
#endif
printf("granulardcoparam(%i, %f)\n", index, value);

	switch (index) {
		case GAIN_VALUE:
		case GAIN_SCATTER:
			param->param[index].float_val = value;
			break;
		case FREQUENCY_SCATTER:
			param->param[index].float_val = value;
			break;
		case DELAY_VALUE:
			param->param[index].float_val = value;
			break;
		case DELAY_SCATTER:
			param->param[index].float_val = value;
			break;
		case DURATION_VALUE:
			/*
			 * Duration, range 2 to 100ms
			 *
			 *	 2ms = 88  samples @ 44100
			 * 100ms = 4410 samples @ 44100
			 *
			 * samplerate is in specs->spec.io[0].samplerate, however we had
			 * to save that as I don't have my specs available here.....
			 *
			 * This is the step rate through the gain table.
			 */
			param->param[index].float_val = value;
			break;
		case DURATION_SCATTER:
			param->param[index].float_val = value;
			break;
		case GDCO_THREADS:
			if ((param->param[index].int_val
				= (value * CONTROLLER_RANGE + 1) * 2) >= MAX_GRAIN_THREAD)
				param->param[index].int_val = MAX_GRAIN_THREAD;
			if (param->param[index].int_val <= 0)
				param->param[index].int_val = 2;
			param->param[index].float_val = (float) param->param[index].int_val;
printf("threads %i\n", param->param[index].int_val);
		case GDCO_TRANSPOSE:
			/*
			 * Transpose in octaves. 0 to 5, down 2 up 3.
			 */
			{
				int note = value * CONTROLLER_RANGE;

				switch (note) {
					case 0:
						param->param[index].float_val = 0.015625;
						break;
					case 1:
						param->param[index].float_val = 0.03125;
						break;
					case 2:
						param->param[index].float_val = 0.0625;
						break;
					case 3:
						param->param[index].float_val = 0.125;
						break;
					case 4:
						param->param[index].float_val = 0.25;
						break;
					case 5:
						param->param[index].float_val = 0.50;
						break;
					case 6:
						param->param[index].float_val = 1.0;
						break;
					case 7:
						param->param[index].float_val = 2.0;
						break;
					case 8:
						param->param[index].float_val = 4.0;
						break;
					case 9:
						param->param[index].float_val = 8.0;
						break;
				}
			}
			break;
		case FREQUENCY_VALUE: /* Tune in "small" amounts */
			{
				float tune, notes = 1.0;
				int i;

				tune = (value - 0.5) * 2;

				/*
				 * Up or down 7 notes.
				 */
				for (i = 0; i < 7;i++)
				{
					if (tune > 0)
						notes *= note_diff;
					else
						notes /= note_diff;
				}

				if (tune > 0)
					param->param[index].float_val =
						1.0 + (notes - 1) * tune;
				else
					param->param[index].float_val =
						1.0 - (1 - notes) * -tune;

				break;
			}
		case GDCO_BANKSEL:
			/*
			 * Memory change - use it to reload the waveforms. We need to be
			 * careful as the oscillator, running as a separate thread, may
			 * have pointers to the existing waveforms.
			 */
			printf("Memory selection not implemented: %i\n",
				(int) (((float) C_RANGE_MIN_1) * value));
			break;
		case GDCO_WAVEFORM:
			/*
			 * Waveform selection: Nine values. First 8 are the configured
			 * waves and the ninth is random. Hm, that is a bit limited, this
			 * will be a bitmask of the 8 waves plus random.
			 */
			if ((param->param[index].int_val = (int) (value * CONTROLLER_RANGE))
				<= 0)
				param->param[index].int_val = -1;
			break;
	}

	return(0);
}

#define S_DEC 0.999f

/*
 * Most of these, not all, need to be defined globally to the operator. That
 * does obviously damage any possibility of it being multithreaded/reentrant.
 * It could be fixed, the issue is that 'regrain()' needs to change the run
 * time parameters and this was an easy option for now.
 */
static float gg;
static float *cwt;
static float gtp;
static float cdt;
static float gts;
static float wtp;
static float wtr;
static float wts;
static float *gmb;
static float *wmb;

static float gg2;
static float cdt2;
static float gtp2;
static float gts2;
static float wtp2;
static float wtr2;

/*
 * The fun bit.
 *
 * When any of the 'n' sympathetic grains have mutually terminated this routine
 * is called to reseed them. The default grain count, n, is 8 paired grains, and
 * with the 'scatter' parameters all set to zero the grains will be identical
 * and pan centrally, as scatter changes so does the stereo separation.
 *
 * This code will seed the pairs. In a present release it may actually work
 * on quad grains, for lots of reasons:
 *
 *	We can still get the stereo pairing.
 *	We can organise graceful morphing between consecutive grains.
 *	We could do some quite wild spacialisation.
 *
 * Each parameter will have 4 inputs. The parameter controller that sets its
 * value, the parameter 'scatter' which is going to be used to define the
 * depth of separation and 2 busses: one defines modification of the parameter,
 * and one that defines modification of the scatter between the sympathetic
 * grains. Some use a quantumrand[] entry that will adjust the scatter,
 * something which may be dropped in preference for just using noise on the
 * bus.
 */
static float
regrain(bristolGRANULARDCOlocal *local, bristolGRANULARDCO *specs,
bristolOPParams *param, int count, int channel)
{
	unsigned int cri = local->runtime[channel].cri, i, wave;

	/*
	 * When we have implemented the wavetables this is going to be either a
	 * run through the grain list or a random selection.
	 */
	local->runtime[channel].cg = 0;

	gg = local->runtime[channel].gain = 1.0;

	/*
	 * And wave selection, same for both grains. If random is selected then
	 * start with random, then search for the first wave flag that is active
	 * and take that wave.
	 *
	 * This is 'almost' correct, however if we have two grains and two waves
	 * where the waves are index 5 and 6, then both grains will play wave 5.
	 * That was not the intention, the fix is to actually find the channel'th
	 * wave that is configured when we do not have RAND selected.
	 */
	if ((param->param[GDCO_WAVEFORM].int_val & GDCO_WAVE_RAND) != 0)
		wave = quantumrand[cri] & GDCO_WAVE_MASK;
	else
		wave = channel & GDCO_WAVE_MASK;

	for (i = 0; i < GDCO_WAVE_COUNT; i++)
	{
		if ((param->param[GDCO_WAVEFORM].int_val & (GDCO_WAVE_1 << wave)) != 0)
			break;
		if (++wave >= GDCO_WAVE_COUNT)
			wave = 0;
	}
	cwt = local->runtime[channel].cwt = specs->wave[wave & GDCO_WAVE_MASK];
	local->runtime[channel].wave = wave;

	cdt = local->runtime[channel].cdt =
		samplerate * 0.1
		* (1.0 + param->param[DELAY_VALUE].float_val
			+ specs->spec.io[DELAY_MODBUS].buf[count])
		* (quantumrand[cri] & 0x01?
			1.0 + param->param[DELAY_SCATTER].float_val
			* specs->spec.io[DELAY_SCATTERBUS].buf[count]:
			1.0 - param->param[DELAY_SCATTER].float_val
			* specs->spec.io[DELAY_SCATTERBUS].buf[count]);

	gtp = gtp2 = local->runtime[channel].gtp = local->runtime[channel + 1].gtp
		= 0;

	if ((cri += channel) >= GRAIN_COUNT)
		cri %= GRAIN_COUNT;
	local->runtime[channel].cri = cri;

	wtp = local->runtime[channel].wtp = 0;
	wts = local->runtime[channel].wts = GRANULARDCO_WAVE_SZE;
	wtr = local->runtime[channel].wtr =
		(quantumrand[cri] & 0x01) == 0?
		specs->grains[0].frequency
			* (1.0 + (param->param[FREQUENCY_SCATTER].float_val
			+ specs->spec.io[FREQ_SCATTERBUS].buf[count]) * 0.1):
		specs->grains[0].frequency
			* (1.0 - (param->param[FREQUENCY_SCATTER].float_val
			+ specs->spec.io[FREQ_SCATTERBUS].buf[count]) * 0.1);

	if ((gts = (quantumrand[cri] & 0x01) == 0?
			GRANULARDCO_WAVE_SZE /
			(88 + (param->param[DURATION_VALUE].float_val
			+ specs->spec.io[DURATION_MODBUS].buf[count]
			+ param->param[DURATION_SCATTER].float_val
			* specs->spec.io[DURATION_SCATTERBUS].buf[count])
				* 0.1 * samplerate):
			GRANULARDCO_WAVE_SZE /
			(88 + (param->param[DURATION_VALUE].float_val
			+ specs->spec.io[DURATION_MODBUS].buf[count]
			- param->param[DURATION_SCATTER].float_val
			* specs->spec.io[DURATION_SCATTERBUS].buf[count])
				* 0.1 * samplerate))
		< 0.227657)
		gts = 0.227657;
	else if (gts > 11.636364)
		gts = 11.636364;

	local->runtime[channel].gts = gts;

	/*
	 * Sympathetic grain
	 */
	gg2 = local->runtime[channel + 1].gain =
		(quantumrand[cri] & 0x01) == 0?
			1.0 + param->param[GAIN_SCATTER].float_val:
			1.0 - param->param[GAIN_SCATTER].float_val;

	cdt2 = local->runtime[channel + 1].cdt =
		samplerate * 0.1
		* (1.0 + param->param[DELAY_VALUE].float_val
			+ specs->spec.io[DELAY_MODBUS].buf[count])
		* (quantumrand[cri] & 0x01?
			1.0 - param->param[DELAY_SCATTER].float_val
			+ specs->spec.io[DELAY_SCATTERBUS].buf[count]:
			1.0 + param->param[DELAY_SCATTER].float_val
			* specs->spec.io[DELAY_SCATTERBUS].buf[count]);

	if (++cri >= GRAIN_COUNT)
		cri = 0;
	local->runtime[channel + 1].cri = cri;

	wtp2 = local->runtime[channel + 1].wtp = 0;
	wtr2 = local->runtime[channel + 1].wtr =
		(quantumrand[cri] & 0x01) == 0?
		specs->grains[0].frequency
			* (1.0 + (param->param[FREQUENCY_SCATTER].float_val
			+ specs->spec.io[FREQ_SCATTERBUS].buf[count]) * 0.1):
		specs->grains[0].frequency
			* (1.0 - (param->param[FREQUENCY_SCATTER].float_val
			+ specs->spec.io[FREQ_SCATTERBUS].buf[count]) * 0.1);

	if ((gts2 = ((quantumrand[cri] & 0x01) == 0?
			GRANULARDCO_WAVE_SZE /
			(88 + (param->param[DURATION_VALUE].float_val
			+ specs->spec.io[DURATION_MODBUS].buf[count]
			- param->param[DURATION_SCATTER].float_val
			* specs->spec.io[DURATION_SCATTERBUS].buf[count])
				* 0.1 * samplerate):
			GRANULARDCO_WAVE_SZE /
			(88 + (param->param[DURATION_VALUE].float_val
			+ specs->spec.io[DURATION_MODBUS].buf[count]
			+ param->param[DURATION_SCATTER].float_val
			* specs->spec.io[DURATION_SCATTERBUS].buf[count])
				* 0.1 * samplerate)))
		< 0.227657)
		gts2 = 0.227657;
	else if (gts2 > 11.636364)
		gts2 = 11.636364;
	local->runtime[channel + 1].gts = gts2;

	return(0.0);
}

/*
 */
static int operate(bristolOP *operator,
	bristolVoice *voice,
	bristolOPParams *param,
	void *lcl)
{
	bristolGRANULARDCOlocal *local = lcl;
	float *lbp, *rbp, gain;
	float tune;
	int count, offset, grain;
	bristolGRANULARDCO *specs;

	specs = (bristolGRANULARDCO *) operator->specs;

#ifdef BRISTOL_DBG
	printf("granulardco(%x, %x, %x)\n", operator, param, local);
#endif

	count = specs->spec.io[DCO_IN_FREQ].samplecount;

	tune = param->param[GDCO_TRANSPOSE].float_val
		* param->param[FREQUENCY_VALUE].float_val;

	/*
	 * We should take a peak for note_on events so that we can reinitialise
	 * all the values.
	if (voice->flags & (BRISTOL_KEYON|BRISTOL_KEYREON))
	 */
	if (voice->flags & BRISTOL_KEYON)
	{
		for (grain = 0; grain < MAX_GRAIN_THREAD; grain+=2)
		{
			/* This should be a sync flag */
			local->runtime[grain].cri = grain * 31 + 512;
			regrain(local, specs, param, 0, grain);
		}
	}

	/*
	 * Dive into grain generation. We should take a peak for note_on events
	 * so that we can reinitialise all the values.
	 *
	 * Run time parameters require
	 *
	 *	gtp = pointer into gain table. float.
	 *	gts = step rate through gain table. float.
	 *	wtp = pointer into wave table. float *.
	 *	wtr = step rate through wave table. float.
	 *	wts = size of wave table. float.
	 *	gmb = gain mod buf. float *.
	 *	wmb = wave frequency mod buf. float *.
	 *
	 * When gtp >= wavetable size then we need to regrain(). We have to do all
	 * this twice as we are stereo. Gain should be modified on the fly from
	 * the gain buffer. Step rate should be modified on the fly from its bus.
	 *
	 * When wtp >= wavesize then we loop.
	 */
	for (grain = 0; grain < param->param[GDCO_THREADS].int_val; grain+=2)
	{
		lbp = specs->spec.io[DCO_OUT_LEFT].buf;
		rbp = specs->spec.io[DCO_OUT_RIGHT].buf;

		gg  = local->runtime[grain].gain
			* param->param[GAIN_VALUE].float_val;
		cdt = local->runtime[grain].cdt;
		cwt = local->runtime[grain].cwt;
		gtp = local->runtime[grain].gtp;
		gts = local->runtime[grain].gts;
		wtp = local->runtime[grain].wtp;
		wtr = local->runtime[grain].wtr;
		wts = local->runtime[grain].wts;
		gmb = specs->spec.io[GAIN_MODBUS].buf;
		wmb = specs->spec.io[FREQ_MODBUS].buf;

		gg2  = local->runtime[grain + 1].gain
			* param->param[GAIN_VALUE].float_val;
		cdt2 = local->runtime[grain + 1].cdt;
		gtp2 = local->runtime[grain + 1].gtp;
		gts2 = local->runtime[grain + 1].gts;
		wtp2 = local->runtime[grain + 1].wtp;
		wtr2 = local->runtime[grain + 1].wtr;

		for (offset = 0; offset < count; offset++)
		{
			/*
			 * Check for end of grain. We have to make sure both 
			 * wave table pointers have passed their full duty cycle.
			 */
			if ((gtp == GRANULARDCO_WAVE_SZE) && (gtp2 == GRANULARDCO_WAVE_SZE))
				gtp = gtp2 = regrain(local, specs, param, offset, grain);

			/*
			 * The next two paragraphs are the sympathetic grain generation
			 * sections. The sympathy is a function of parameterisation, when
			 * the parameters are wide there is little concordance, when the
			 * values are slight there is stereo separation.
			 */
			if (cdt-- < 0) {
				/* find the gain */
				gain = quantumgain[(int) gtp] * *gmb * gg;

				/* Now find our wave from (resampling) the wave table.  */
				*lbp += gain * cwt[(int) wtp];

				/* Check for end of wave */
				wtp += *wmb * wtr * tune;
				while (wtp >= wts) wtp -= wts;

				if ((gtp += gts) > GRANULARDCO_WAVE_SZE)
					gtp = GRANULARDCO_WAVE_SZE;
			}

			if (cdt2-- < 0) {
				/* find the gain for right sympathetic grain */
				gain = quantumgain[(int) gtp2] * *gmb * gg2;

				/* Now find our wave from (resampling) the wave table.  */
				*rbp += gain * cwt[(int) wtp2];

				/* Check for end of wave */
				wtp2 += *wmb * wtr2 * tune;
				while (wtp2 >= wts) wtp2 -= wts;

				if ((gtp2 += gts2) > GRANULARDCO_WAVE_SZE)
					gtp2 = GRANULARDCO_WAVE_SZE;
			}

			lbp++;
			rbp++;
			gmb++;
			wmb++;
		}

		local->runtime[grain].cdt = cdt;
		local->runtime[grain].gtp = gtp;
		local->runtime[grain].wtp = wtp;
		local->runtime[grain].wtr = wtr;
		local->runtime[grain].wts = wts;

		local->runtime[grain + 1].cdt = cdt2;
		local->runtime[grain + 1].gtp = gtp2;
		local->runtime[grain + 1].gts = gts2;
		local->runtime[grain + 1].wtp = wtp2;
		local->runtime[grain + 1].wtr = wtr2;
	}

	return(0);
}

/*
 * Setup any variables in our OP structure, in our IO structures, and malloc
 * any memory we need.
 */
bristolOP *
quantuminit(bristolOP **operator, int index, int samplerate, int samplecount)
{
	bristolGRANULARDCO *specs;

	*operator = bristolOPinit(operator, index, samplecount);

	samplerate = samplerate;

#ifdef BRISTOL_DBG
	printf("quantuminit(%x(%x), %i, %i, %i)\n",
		operator, *operator, index, samplerate, samplecount);
#endif

	note_diff = pow(2, ((double) 1)/12);

	/*
	 * Then the local parameters specific to this operator. These will be
	 * the same for each operator, but must be inited in the local code.
	 */
	(*operator)->operate = operate;
	(*operator)->destroy = destroy;
	(*operator)->reset = reset;
	(*operator)->param= param;

	specs = (bristolGRANULARDCO *) bristolmalloc0(sizeof(bristolGRANULARDCO));
	(*operator)->specs = (bristolOPSpec *) specs;
	(*operator)->size = sizeof(bristolGRANULARDCO);

	/*
	 * These are specific to this operator, and will need to be altered for
	 * each operator.
	 */
	specs->spec.opname = OPNAME;
	specs->spec.description = OPDESCRIPTION;
	specs->spec.pcount = PCOUNT;
	specs->spec.iocount = IOCOUNT;
	specs->spec.localsize = sizeof(bristolGRANULARDCOlocal);

	/*
	 * We are going to assign multiple waves to this oscillator.
	 * sine, ramp, square, triangle? We should also have a set of loadable
	 * samples, potentially loadable by memory using the GUI to send a param
	 * to this oscillator, passing the memory number, and we use that to 
	 * select a location for the new waveforms. We also want to have the
	 * bristol command line pass at least one filename.
	 */
	specs->wave[0] =
		(float *) bristolmalloc(GRANULARDCO_WAVE_SZE * sizeof(float));
	specs->wave[1] =
		(float *) bristolmalloc(GRANULARDCO_WAVE_SZE * sizeof(float));
	specs->wave[2] =
		(float *) bristolmalloc(GRANULARDCO_WAVE_SZE * sizeof(float));
	specs->wave[3] =
		(float *) bristolmalloc(GRANULARDCO_WAVE_SZE * sizeof(float));
	specs->wave[4] =
		(float *) bristolmalloc(GRANULARDCO_WAVE_SZE * sizeof(float));
	specs->wave[5] =
		(float *) bristolmalloc(GRANULARDCO_WAVE_SZE * sizeof(float));
	specs->wave[6] =
		(float *) bristolmalloc(GRANULARDCO_WAVE_SZE * sizeof(float));
	specs->wave[7] =
		(float *) bristolmalloc(GRANULARDCO_WAVE_SZE * sizeof(float));

	specs->grains[0].wave = specs->wave[0];
	specs->grains[1].wave = specs->wave[1];
	specs->grains[2].wave = specs->wave[2];
	specs->grains[3].wave = specs->wave[3];
	specs->grains[4].wave = specs->wave[4];
	specs->grains[5].wave = specs->wave[5];
	specs->grains[6].wave = specs->wave[6];
	specs->grains[7].wave = specs->wave[7];
	specs->grains[0].gain = 1.0;
	specs->grains[0].frequency = 1.0;
	specs->grains[0].gainstep = 0.25;

	/*
	 * FillWave is something that should be called as a parameter change, but
	 * for testing will load it here.
	 */
	fillWave(specs->wave[0], GRANULARDCO_WAVE_SZE, 0);
	fillWave(specs->wave[1], GRANULARDCO_WAVE_SZE, 1);
	fillWave(specs->wave[2], GRANULARDCO_WAVE_SZE, 2);
	fillWave(specs->wave[3], GRANULARDCO_WAVE_SZE, 3);
	fillWave(specs->wave[4], GRANULARDCO_WAVE_SZE, 4);
	fillWave(specs->wave[5], GRANULARDCO_WAVE_SZE, 5);
	fillWave(specs->wave[6], GRANULARDCO_WAVE_SZE, 6);
	fillWave(specs->wave[7], GRANULARDCO_WAVE_SZE, 7);

	/*
	 * Now fill in the dco specs for this operator. These are specific to an
	 * oscillator.
	 */
	specs->spec.param[0].pname = "Pulse";
	specs->spec.param[0].description = "Pulse width";
	specs->spec.param[0].type = BRISTOL_FLOAT;
	specs->spec.param[0].low = 0;
	specs->spec.param[0].high = 1;
	specs->spec.param[0].flags = BRISTOL_ROTARY|BRISTOL_SLIDER;

	specs->spec.param[1].pname = "Transpose";
	specs->spec.param[1].description = "Octave transposition";
	specs->spec.param[1].type = BRISTOL_INT;
	specs->spec.param[1].low = 0;
	specs->spec.param[1].high = 12;
	specs->spec.param[1].flags = BRISTOL_ROTARY|BRISTOL_SLIDER;

	specs->spec.param[2].pname = "Tune";
	specs->spec.param[2].description = "fine tuning of frequency";
	specs->spec.param[2].type = BRISTOL_FLOAT;
	specs->spec.param[2].low = 0;
	specs->spec.param[2].high = 1;
	specs->spec.param[2].flags = BRISTOL_ROTARY|BRISTOL_SLIDER;

	specs->spec.param[3].pname = "gain";
	specs->spec.param[3].description = "output gain on signal";
	specs->spec.param[3].type = BRISTOL_FLOAT;
	specs->spec.param[3].low = 0;
	specs->spec.param[3].high = 2;
	specs->spec.param[3].flags = BRISTOL_ROTARY|BRISTOL_SLIDER|BRISTOL_HIDE;

	specs->spec.param[4].pname = "Ramp";
	specs->spec.param[4].description = "harmonic inclusion of ramp wave";
	specs->spec.param[4].type = BRISTOL_INT;
	specs->spec.param[4].low = 0;
	specs->spec.param[4].high = 1;
	specs->spec.param[4].flags = BRISTOL_BUTTON;

	specs->spec.param[5].pname = "Triangle";
	specs->spec.param[5].description = "harmonic inclusion of triagular wave";
	specs->spec.param[5].type = BRISTOL_INT;
	specs->spec.param[5].low = 0;
	specs->spec.param[5].high = 1;
	specs->spec.param[5].flags = BRISTOL_BUTTON;

	specs->spec.param[6].pname = "PWM";
	specs->spec.param[6].description = "harmonic inclusion of square/pwm wave";
	specs->spec.param[6].type = BRISTOL_INT;
	specs->spec.param[6].low = 0;
	specs->spec.param[6].high = 1;
	specs->spec.param[6].flags = BRISTOL_BUTTON;

	specs->spec.param[7].pname = "Square";
	specs->spec.param[7].description = "harmonic inclusion of square wave";
	specs->spec.param[7].type = BRISTOL_INT;
	specs->spec.param[7].low = 0;
	specs->spec.param[7].high = 1;
	specs->spec.param[7].flags = BRISTOL_BUTTON;

	specs->spec.param[8].pname = "Sine";
	specs->spec.param[8].description = "harmonic inclusion of sine wave";
	specs->spec.param[8].type = BRISTOL_INT;
	specs->spec.param[8].low = 0;
	specs->spec.param[8].high = 1;
	specs->spec.param[8].flags = BRISTOL_BUTTON;

	/*
	 * Now fill in the dco IO specs.
	 */
	specs->spec.io[0].ioname = "input";
	specs->spec.io[0].description = "input rate modulation signal";
	specs->spec.io[0].samplerate = samplerate;
	specs->spec.io[0].samplecount = samplecount;
	specs->spec.io[0].flags = BRISTOL_DC|BRISTOL_INPUT;

	specs->spec.io[1].ioname = "mod";
	specs->spec.io[1].description = "pulse width modulation";
	specs->spec.io[1].samplerate = samplerate;
	specs->spec.io[1].samplecount = samplecount;
	specs->spec.io[1].flags = BRISTOL_DC|BRISTOL_INPUT;

	specs->spec.io[2].ioname = "ramp output";
	specs->spec.io[2].description = "oscillator ramp output signal";
	specs->spec.io[2].samplerate = samplerate;
	specs->spec.io[2].samplecount = samplecount;
	specs->spec.io[2].flags = BRISTOL_AC|BRISTOL_OUTPUT;

	specs->spec.io[3].ioname = "sync";
	specs->spec.io[3].description = "synchronisation signal";
	specs->spec.io[3].samplerate = samplerate;
	specs->spec.io[3].samplecount = samplecount;
	specs->spec.io[3].flags = BRISTOL_DC|BRISTOL_INPUT;

	specs->spec.io[4].ioname = "square output";
	specs->spec.io[4].description = "oscillator pulse/square output signal";
	specs->spec.io[4].samplerate = samplerate;
	specs->spec.io[4].samplecount = samplecount;
	specs->spec.io[4].flags = BRISTOL_AC|BRISTOL_OUTPUT;

	specs->spec.io[5].ioname = "sine output";
	specs->spec.io[5].description = "oscillator sine output signal";
	specs->spec.io[5].samplerate = samplerate;
	specs->spec.io[5].samplecount = samplecount;
	specs->spec.io[5].flags = BRISTOL_AC|BRISTOL_OUTPUT;

	specs->spec.io[6].ioname = "triangle output";
	specs->spec.io[6].description = "oscillator triangle output signal";
	specs->spec.io[6].samplerate = samplerate;
	specs->spec.io[6].samplecount = samplecount;
	specs->spec.io[6].flags = BRISTOL_AC|BRISTOL_OUTPUT;

	return(*operator);
}

static void
fillPDsine(float *mem, int count, int compress)
{
	float j = 0, i, inc1, inc2;

	/*
	 * Resample the sine wave with a phase distortion algorithm.
	 *
	 * We have to get to M_PI/2 in compress steps, hence
	 *
	 *	 inc1 = M_PI/(2 * compress)
	 *
	 * Then we have to scan M_PI in (count - 2 * compress), hence:
	 *
	 *	 inc2 = M_PI/(count - 2 * compress)
	 */
	inc1 = ((float) M_PI) / (((float) 2) * ((float) compress));
	inc2 = ((float) M_PI) / ((float) (count - 2 * compress));

	for (i = 0;i < count; i++)
	{
		*mem++ = sinf(j) * BRISTOL_VPO;

		if (i < compress)
			j += inc1;
		else if (i > (count - 2 * compress))
			j += inc1;
		else
			j += inc2;
	}
}

/*
 * Waves have a range of 24, which is basically two octaves. For larger 
 * differences will have to apply apms.
 */
static void
fillWave(float *mem, int count, int type)
{
	int i;

#ifdef BRISTOL_DBG
	printf("fillWave(%x, %i, %i)\n", mem, count, type);
#endif

	switch (type) {
		case 0:
			/*
			 * This will be a sine wave. We have count samples, and
			 * 2PI radians in a full sine wave. Thus we take
			 * 		(2PI * i / count) * 2048.
			 */
			for (i = 0;i < count; i++)
				mem[i] = sin(2 * M_PI * ((double) i) / count) * BRISTOL_VPO;
			return;
		case 1:
		default:
		{
			float value = BRISTOL_SQR;
			/* 
			 * This is a square wave, with decaying plateaus.
			 */
			for (i = 0;i < count / 2; i++)
				mem[i] = (value * S_DEC);
			value = -BRISTOL_SQR;
			for (;i < count; i++)
				mem[i] = (value * S_DEC);
			return;
		}
		case 2:
			/* 
			 * This is a pulse wave - the granular dco does pwm.
			 */
			for (i = 0;i < count / 5; i++)
				mem[i] = BRISTOL_VPO * 2 / 3;
			for (;i < count; i++)
				mem[i] = -BRISTOL_VPO * 2 / 3;
			return;
		case 3:
			/* 
			 * This is a ramp wave. We scale the index from -.5 to .5, and
			 * multiply by the range. We go from rear to front to table to make
			 * the ramp wave have a positive leading edge.
			for (i = 0; i < count / 2; i++)
				mem[i] = ((float) i / count) * BRISTOL_VPO * 2.0;
			for (; i < count; i++)
				mem[i] = ((float) i / count) * BRISTOL_VPO * 2.0 -
					BRISTOL_VPO * 2;
			for (i = count - 1;i >= 0; i--)
				mem[i] = (((float) i / count) - 0.5) * BRISTOL_VPO * 2.0;
			mem[0] = 0;
			mem[count - 1] = mem[1]
				= BRISTOL_VPO;
			 */
			fillPDsine(mem, count, 5);
			return;
		case 4:
			/*
			 * Triangular wave. From MIN point, ramp up at twice the rate of
			 * the ramp wave, then ramp down at same rate.
			 */
			for (i = 0;i < count / 2; i++)
				mem[i] = -BRISTOL_VPO
					+ ((float) i / (count / 2)) * BRISTOL_VPO * 2;
			for (;i < count; i++)
				mem[i] = BRISTOL_VPO -
					(((float) (i - count / 2) * 2) / (count / 2)) * BRISTOL_VPO;
			return;
		case 5:
		case 6:
			/*
			 * Would like to put in a jagged edged ramp wave. Should make some
			 * interesting strings sounds.
			 */
			{
				float accum = -BRISTOL_VPO;

				for (i = 0; i < count / 2; i++)
				{
					mem[i] = accum +
						(0.5 - (((float) i) / count)) * BRISTOL_VPO * 4;
					if (i == count / 8)
						accum = -BRISTOL_VPO * 3 / 4;
					if (i == count / 4)
						accum = -BRISTOL_VPO / 2;
					if (i == count * 3 / 8)
						accum = -BRISTOL_VPO / 4;
				}
				for (; i < count; i++)
					mem[i] = -mem[count - i];
			}
			return;
		case 7:
			/*
			 * Sine wave - added as a part of the OBX extensions.
			 */
			for (i = 0;i < count; i++)
				mem[i] = sin(2 * M_PI * ((double) i) / count) * BRISTOL_VPO;
			return;
	}
}

