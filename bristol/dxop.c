
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
 *	dxopinit()
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
#include "dxop.h"

static float note_diff;

/*
 * The name of this operator, IO count, and IO names.
 */
#define OPNAME "DX OP"
#define OPDESCRIPTION "FM Operator"
#define PCOUNT 12
#define IOCOUNT 2

#define DXOP_IN_IND 0
#define DXOP_OUT_IND 1

#define DXOP_WAVE_COUNT 6

#define DXOP_TRANSPOSE	0
#define DXOP_TUNE		1
#define DXOP_ATTACK		2
#define DXOP_DECAY		3
#define DXOP_SUSTAIN	4
#define DXOP_RELEASE	5
#define DXOP_GAIN		6
#define DXOP_VELOCITY	7
#define DXOP_OGC		8
#define DXOP_L1			9
#define DXOP_ATTACK2	10
#define DXOP_L2			11

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
	bristolfree(((bristolDXOP *) operator)->wave[0]);
	bristolfree(((bristolDXOP *) operator)->wave[1]);
	bristolfree(((bristolDXOP *) operator)->wave[2]);
	bristolfree(((bristolDXOP *) operator)->wave[3]);
	bristolfree(((bristolDXOP *) operator)->wave[4]);
	bristolfree(((bristolDXOP *) operator)->wave[5]);
	bristolfree(((bristolDXOP *) operator)->wave[6]);
	bristolfree(((bristolDXOP *) operator)->wave[7]);

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
	printf("dxreset(%x)\n", operator);
#endif

	param->param[0].float_val = 1.0;
	param->param[1].float_val = 1.0;

	param->param[2].float_val = gainTable[100].rate;
	param->param[3].float_val = 1/gainTable[1000].rate;
	param->param[4].float_val = gainTable[13000].gain * BRISTOL_VPO;
	param->param[5].float_val = 1/gainTable[1000].rate;

	param->param[6].float_val = 1.0;
	return(0);
}

/*
 * Alter an internal parameter of an operator.
 */
static int param(bristolOP *operator, bristolOPParams *param,
	unsigned char index, float value)
{
	int offset = (value * (CONTROLLER_RANGE - 1)) + 1;

	if (offset >= CONTROLLER_RANGE)
		offset = CONTROLLER_RANGE - 1;

#ifdef BRISTOL_DBG
	printf("dxopparam(%x, %x, %i, %f)\n", (size_t) operator, (size_t) param,
		index, value);
#endif

	switch (index) {
		case 0:
			/*
			 * Transpose in octaves. 0 to 5, down 2 up 3.
			 */
			{
				int note = value * CONTROLLER_RANGE;

				switch (note) {
					case 0:
						param->param[index].float_val = 0.25;
						break;
					case 1:
						param->param[index].float_val = 0.50;
						break;
					case 2:
						param->param[index].float_val = 1.0;
						break;
					case 3:
						param->param[index].float_val = 2.0;
						break;
					case 4:
						param->param[index].float_val = 4.0;
						break;
					case 5:
						param->param[index].float_val = 8.0;
						break;
				}
			}
			break;
		case 1: /* Tune in "small" amounts */
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
		case 2: /* Attack */
			//param->param[index].float_val = value * value;
			offset = (value * value * (CONTROLLER_RANGE - 1)) + 1;
			if (offset >= CONTROLLER_RANGE)
				offset = CONTROLLER_RANGE - 1;
			param->param[index].float_val = gainTable[offset].rate;
			break;
		case 3: /* Decay */
			param->param[index].float_val = gainTable[offset].rate;
			break;
		case 4:
			/*
			 * Sustain
			 *
			param->param[index].float_val
				= gainTable[offset].gain * BRISTOL_VPO;
			 *
			 * The gain tables are fine for frequency adjustments, but do 
			 * not work well for gain here. Hence
			 */
			offset = value * (CONTROLLER_RANGE - 1);
			if (offset >= CONTROLLER_RANGE)
				offset = CONTROLLER_RANGE - 1;
			param->param[index].float_val = value * BRISTOL_VPO;
			break;
		case 5:
			/*
			 * Release.
			 */
			param->param[index].float_val = 1/gainTable[offset].rate;
			break;
		case 6:
			/*
			 * Gain
			 */
			param->param[index].float_val = gainTable[offset].gain;
			break;
		case 7:
			/*
			 * Keytracking
			 */
			if (value == 0)
				param->param[index].int_val = 0;
			else
				param->param[index].int_val = 1;
			param->param[index].float_val = value;
			break;
		case 8:
			/*
			 * Output gain control
			 */
			if (value == 0) {
				param->param[index].int_val = 0;
				param->param[index].float_val = 0;
			} else {
				param->param[index].int_val = 1;
				param->param[index].float_val = 1.0;
			}
			break;
		case 9:
			/*
			 * These are L1-A-L2 for envelope. Have to look into how to pass
			 * them through to the envelope as a parameter as we only have
			 * 8 available at the moment.
			offset = value * (CONTROLLER_RANGE - 1);
			param->param[index].float_val = value * BRISTOL_VPO;
			 */
			param->param[index].float_val = value * BRISTOL_VPO;
			break;
		case 10: /* Attack2 */
			//param->param[9].float_val = value * value;
			offset = (value * value * (CONTROLLER_RANGE - 1)) + 1;
			if (offset >= CONTROLLER_RANGE)
				offset = CONTROLLER_RANGE - 1;
			param->param[index].float_val = gainTable[offset].rate;
			break;
		case 11:
			param->param[index].float_val = value * BRISTOL_VPO;
			break;
	}

	return(0);
}

//static float scale = 0.0000000001; // 0.000000001;
static int dngx1 = 0x67452301;
static int dngx2 = 0xefcdab89;

static void
buffermax(float *buf, float max, int count)
{
	for (count--;count >= 0; count--)
	{
		dngx1 ^= dngx2;

		if (buf[count] > max)
			buf[count] = max;
		else if (-buf[count] > max)
			buf[count] = -max;

//		buf[count] += dngx2 * scale;

		dngx2 += dngx1;
	}
}

/*
 * Could use some optimisation, it overloads my P450 at 16 voices.
 * May introduce restrictions on voicecount per synth.
 */
static int operate(bristolOP *operator,
	bristolVoice *voice,
	bristolOPParams *param,
	void *lcl)
{
	bristolDXOPlocal *local = lcl;
	register int obp = 0, count, rampup = -1;
	register float *ib, *ob, *wt, wtp, transp;
	register float cgain, attack, decay, sustain, release, gain, egain, delta;
	/* These are for the 7 stage envelope: */
	register float L1, attack2, L2;
	bristolDXOP *specs;

	specs = (bristolDXOP *) operator->specs;

#ifdef BRISTOL_DBG
	printf("dxop(%x, %x, %x)\n", operator, param, local);
#endif

	count = specs->spec.io[DXOP_OUT_IND].samplecount;
	ib = specs->spec.io[DXOP_IN_IND].buf;
	ob = specs->spec.io[DXOP_OUT_IND].buf;
	wt = specs->wave[0];
	wtp = local->wtp;
	transp = param->param[DXOP_TRANSPOSE].float_val
		* param->param[DXOP_TUNE].float_val;
	wtp = local->wtp;
	cgain = local->cgain;
	egain = local->egain;

	buffermax(ib, DXOP_WAVE_SZE * 4, count);

	if (ib[0] > 10000)
		ib[0] = ib[1];

	attack = param->param[DXOP_ATTACK].float_val;
	decay = param->param[DXOP_DECAY].float_val;
	sustain = 1.0 + param->param[DXOP_SUSTAIN].float_val;
	release = param->param[DXOP_RELEASE].float_val;

	/* For extra stage */
	L1 = param->param[DXOP_L1].float_val;
	attack2 = param->param[DXOP_ATTACK2].float_val;
	L2 = param->param[DXOP_L2].float_val;

	/*
	 * If the first level is zero, go into compatability mode, ADSR
	 */
	if (L1 == 0.0)
	{
		L1 = BRISTOL_VPO;
		attack2 = decay;
		L2 = sustain;
	}

	/*
	 * Adjust the gain depending on the key velocity, and include both channel
	 * and polypressure - we will typically have one or the other, but both
	 * is highly unlikely.
	 *
	 * This gain figure should be related to an option for key tracking?
	 *
	 * 4/4/02 - Velocity tracking is either on or off. Moving to a variable
	 * velocity tracking.
	if (param->param[7].int_val)
		gain = param->param[6].float_val
			* (voice->velocity + voice->press + voice->chanpressure);
	else
		gain = param->param[6].float_val;
	 *
	 */
	gain =
		param->param[DXOP_VELOCITY].float_val
			* voice->velocity * param->param[DXOP_GAIN].float_val
		+ (1 - param->param[DXOP_VELOCITY].float_val)
			* param->param[DXOP_GAIN].float_val;

	/*
	 * Going to adjust gain based on continuous controllers, if we have flagged
	 * ogc as a modified variable.
	 */
	if (param->param[DXOP_OGC].int_val)
		gain *= voice->baudio->contcontroller[1];

	/*
	 * Egain is to smooth over changes to the velocity gain when we retrigger
	 * the voice.
	 */
	egain = local->egain;
	if (egain < gain)
		rampup = 1;
	else if (egain > gain)
		rampup = 0;

	count = specs->spec.io[DXOP_OUT_IND].samplecount;
	cgain = local->cgain;
	ob = specs->spec.io[DXOP_OUT_IND].buf;

	if (voice->flags & BRISTOL_KEYOFF)
		local->cstate = STATE_RELEASE;

	if (voice->flags & BRISTOL_KEYON)
	{
/* printf("note on: %f\n", cgain); */
		cgain = 1.0;
		egain = 1.0;
		rampup = 1;
		wtp = 0;
		local->cstate = STATE_ATTACK;

		if ((~voice->flags & BRISTOL_KEYREON) &&
			(voice->offset > 0) && (voice->offset < count))
		{
			if (voice->baudio->midiflags & BRISTOL_MIDI_DEBUG1)
				printf("envelope trigger offset %i frames\n", voice->offset);

			count -= voice->offset;
			obp += voice->offset;
		}
	} else if (voice->flags & BRISTOL_KEYREON) {
/* printf("note RE on\n"); */
		local->cstate = STATE_ATTACK;
	}

	/*
	 * Go jumping through the wavetable, with each jump defined by the value
	 * given on our input line, making sure we fill one output buffer.

		ob[obp] += wt[(int) wtp] * cgain;
		if ((wtp += ib[obp++] * transp) >= DXOP_WAVE_SZE) wtp -= DXOP_WAVE_SZE;
		if (wtp < 0) wtp += DXOP_WAVE_SZE;

	 */

/*printf("%f %f %f %f %f, %f %f\n", */
/*attack, decay, sustain, release, gain, cgain, egain); */
	/*
	 * We need some state indicator to say when we are leading into the attack.
	 * This is actually in the MIDI voice structure, but we do not have access
	 * to this. We should assume that a MIDI event sets our cstate as necessary?
	 *
	 * As of Jan 2006 this became a 7 stage envelope. It actually only added
	 * one state, but altered the way some of the other states work. The extra
	 * state is the second attack stage, but the alterations include the 
	 * possibility that the initial attack could potentially be a decay if we
	 * have a restrike on a voice when the current gain is greater than L1, and
	 * the decay stage could also be an attack in any general case as we no
	 * longer attack to max gain but to L2 in the new stage ATTACK2, and L2 
	 * may well be lower than the sustain value.
	 *
	 *     L1
	 *     /\   S_______
	 *    /  \  /       \
	 *   /    \/         \
	 * _/_____L2__________\__
	 *
	 * or
	 *
	 *   L1 ___ L2
	 *     /   \
	 *    /     \
	 *   /       \_________
	 * _/________S_________\__
	 *
	 * or
	 *         L2
	 *         /\
	 *        /  \_________
	 *       /    S        \
	 * _____/_______________\__
	 *     L1
	 * or
	 *
	 *      S.________
	 *       |        \
	 *       |         \
	 * _____/_L2________\__
	 *     L1
	 *
	 * This is more inline with the original DX envelopes and allows for things
	 * like double breath on notes, and generally a lot more flexibility.
	 *
	 * Basically, in any given stage, if the current value is different from
	 * the target level (L1, L2 or S) then we tend towards it at the desired
	 * rate (A, A2, D), then change state when we get there.
	 *
	 * These should be backwards compatible, ie, if A2/L2 are zero then skip
	 * the stage? If L1==A2==L2==0 skip the stage:
	 *
	 *	set L1=Max, A2=D and L2=S?
	 *
	 * Perhaps not, may just have to rework the memories (AND RHODES).
	 */
	while (count > 0)
	{
		switch (local->cstate)
		{
			case STATE_RELEASE:
				/*
				 * Release does not change for the extra stage, decay from 
				 * current value to zero
				 */
/*printf("release %i, %f\n", voice->index, cgain); */
				/*
				 * Assume key off - start releasing gain.
				 */
				while (cgain > 1.0)
				{
					if (count-- > 0)
					{
						/*
						 * This was integrated for resampling.
						 *
						ob[obp] += wt[(int) wtp] * (((cgain *= release) - 1)
							* egain);
						 */
						if (wtp >= DXOP_WAVE_SZE_M)
							ob[obp] += (wt[0] * (wtp - ((float) ((int) wtp)))
								+ wt[(int) wtp]
								* (1.0 - (wtp - ((float) ((int) wtp)))))
								* (((cgain *= release) - 1) * egain);
						else
							ob[obp] += (wt[(int) wtp + 1]
								* (wtp - ((float) ((int) wtp)))
								+ wt[(int) wtp]
								* (1.0 - (wtp - ((float) ((int) wtp)))))
								* (((cgain *= release) - 1) * egain);

						if ((wtp += ib[obp++] * transp) >= DXOP_WAVE_SZE)
							while ((wtp -= DXOP_WAVE_SZE) > DXOP_WAVE_SZE)
								;
						while (wtp < 0)
							wtp += DXOP_WAVE_SZE;
					} else
						break;

					if (rampup >= 0) {
						if (rampup)
						{
							if ((egain+=0.01) > gain)
							{
								egain = gain;
								rampup = -1;
							}
						} else {
							if ((egain-=0.01) < gain)
							{
								egain = gain;
								rampup = -1;
							}
						}
					}
				}
				if (cgain <= 1.0)
				{
					cgain = 1.0;
					/*
					 * Correctly spoken, each operator should set a flag to
					 * say that it has finneshed, and when all 6 have then we
					 * can clear the voice.
					 */
					local->cstate = STATE_DONE;
					voice->flags |= BRISTOL_KEYDONE;
				} else
					voice->flags &= ~BRISTOL_KEYDONE;
				break;
			case STATE_START:
			case STATE_ATTACK:
				/*
				 * Just triggered - start attack ramp from current gain level.
				 *
				 * For extra stage, BRISTOL_VPO := L1, change delta to
				 * potentially be up or down, and change next state to ATTACK2.
				 */
				if (cgain <= L1)
				{
					while (cgain < L1)
					{
						if (count-- > 0)
						{
							/*
							ob[obp] += wt[(int) wtp] * (((cgain *= attack) - 1)
								* egain);
							 */
							if (wtp >= DXOP_WAVE_SZE_M)
								ob[obp] += (wt[0]
									* (wtp - ((float) ((int) wtp)))
									+ wt[(int) wtp]
									* (1.0 - (wtp - ((float) ((int) wtp)))))
									* (((cgain *= attack) - 1) * egain);
							else
								ob[obp] += (wt[(int) wtp + 1]
									* (wtp - ((float) ((int) wtp)))
									+ wt[(int) wtp]
									* (1.0 - (wtp - ((float) ((int) wtp)))))
									* (((cgain *= attack) - 1) * egain);

							if ((wtp += ib[obp++] * transp) >= DXOP_WAVE_SZE)
								while ((wtp -= DXOP_WAVE_SZE) > DXOP_WAVE_SZE)
									;
							while (wtp < 0)
								wtp += DXOP_WAVE_SZE;
						} else
							break;

						if (rampup >= 0) {
							if (rampup)
							{
								if ((egain+=0.01) > gain)
								{
									egain = gain;
									rampup = -1;
								}
							} else {
								if ((egain-=0.01) < gain)
								{
									egain = gain;
									rampup = -1;
								}
							}
						}
					}
					if (cgain >= L1)
						local->cstate = STATE_ATTACK2;
				} else {
					delta = 1/attack;
					while (cgain > L1)
					{
						if (count-- > 0)
						{
							/*
							ob[obp] += wt[(int) wtp] * (((cgain *= delta) - 1)
								* egain);
							 */
							if (wtp >= DXOP_WAVE_SZE_M)
								ob[obp] += (wt[0]
									* (wtp - ((float) ((int) wtp)))
									+ wt[(int) wtp]
									* (1.0 - (wtp - ((float) ((int) wtp)))))
									* (((cgain *= delta) - 1) * egain);
							else
								ob[obp] += (wt[(int) wtp + 1]
									* (wtp - ((float) ((int) wtp)))
									+ wt[(int) wtp]
									* (1.0 - (wtp - ((float) ((int) wtp)))))
									* (((cgain *= delta) - 1) * egain);

							if ((wtp += ib[obp++] * transp) >= DXOP_WAVE_SZE)
								while ((wtp -= DXOP_WAVE_SZE) > DXOP_WAVE_SZE)
									;
							while (wtp < 0)
								wtp += DXOP_WAVE_SZE;
						} else
							break;

						if (rampup >= 0) {
							if (rampup)
							{
								if ((egain+=0.01) > gain)
								{
									egain = gain;
									rampup = -1;
								}
							} else {
								if ((egain-=0.01) < gain)
								{
									egain = gain;
									rampup = -1;
								}
							}
						}
					}
					if (cgain <= L1)
						local->cstate = STATE_ATTACK2;
				}

				/*
				 * This just ensures that it is the last envelope in any
				 * given bristolSound chain that decides when to stop the
				 * audio stream.
				 */
				voice->flags &= ~BRISTOL_KEYDONE;
				break;
			case STATE_ATTACK2:
				/*
				 * Just triggered - start attack ramp from current gain level.
				 *
				 * For extra stage, BRISTOL_VPO := L2, change delta to
				 * potentially be up or down, and change next state to ATTACK2.
				 */
				if (cgain <= L2)
				{
					while (cgain < L2)
					{
						if (count-- > 0)
						{
							/*
							ob[obp] += wt[(int) wtp] * (((cgain *= attack2) - 1)
								* egain);
							 */
							if (wtp >= DXOP_WAVE_SZE_M)
								ob[obp] += (wt[0]
									* (wtp - ((float) ((int) wtp)))
									+ wt[(int) wtp]
									* (1.0 - (wtp - ((float) ((int) wtp)))))
									* (((cgain *= attack2) - 1) * egain);
							else
								ob[obp] += (wt[(int) wtp + 1]
									* (wtp - ((float) ((int) wtp)))
									+ wt[(int) wtp]
									* (1.0 - (wtp - ((float) ((int) wtp)))))
									* (((cgain *= attack2) - 1) * egain);

							if ((wtp += ib[obp++] * transp) >= DXOP_WAVE_SZE)
								while ((wtp -= DXOP_WAVE_SZE) > DXOP_WAVE_SZE)
									;
							while (wtp < 0)
								wtp += DXOP_WAVE_SZE;
						} else
							break;

						if (rampup >= 0) {
							if (rampup)
							{
								if ((egain+=0.01) > gain)
								{
									egain = gain;
									rampup = -1;
								}
							} else {
								if ((egain-=0.01) < gain)
								{
									egain = gain;
									rampup = -1;
								}
							}
						}
					}
					if (cgain >= L2)
						local->cstate = STATE_DECAY;
				} else {
					delta = 1/attack2;
					while (cgain > L2)
					{
						if (count-- > 0)
						{
							/*
							ob[obp] += wt[(int) wtp] * (((cgain *= delta)- 1)
								* egain);
							 */

							if (wtp >= DXOP_WAVE_SZE_M)
								ob[obp] += (wt[0]
									* (wtp - ((float) ((int) wtp)))
									+ wt[(int) wtp]
									* (1.0 - (wtp - ((float) ((int) wtp)))))
									* (((cgain *= delta) - 1) * egain);
							else
								ob[obp] += (wt[(int) wtp + 1]
									* (wtp - ((float) ((int) wtp)))
									+ wt[(int) wtp]
									* (1.0 - (wtp - ((float) ((int) wtp)))))
									* (((cgain *= delta) - 1) * egain);

							if ((wtp += ib[obp++] * transp) >= DXOP_WAVE_SZE)
								while ((wtp -= DXOP_WAVE_SZE) > DXOP_WAVE_SZE)
									;
							while (wtp < 0)
								wtp += DXOP_WAVE_SZE;
						} else
							break;

						if (rampup >= 0) {
							if (rampup)
							{
								if ((egain+=0.01) > gain)
								{
									egain = gain;
									rampup = -1;
								}
							} else {
								if ((egain-=0.01) < gain)
								{
									egain = gain;
									rampup = -1;
								}
							}
						}
					}
					if (cgain <= L2)
						local->cstate = STATE_DECAY;
				}

				/*
				 * This just ensures that it is the last envelope in any
				 * given bristolSound chain that decides when to stop the
				 * audio stream.
				 */
				voice->flags &= ~BRISTOL_KEYDONE;
				break;
			case STATE_DECAY:
/*printf("decay %i, %f\n", voice->index, cgain); */
				/*
				 * Decay state. Ramp to sustain level.
				 *
				 * Reworked for extra stage, direction may be up or down.
				 */
				if (cgain >= sustain)
				{
					delta = 1/decay;
					while (cgain > sustain)
					{
						if (count-- > 0)
						{
							/*
							ob[obp] += wt[(int) wtp] * (((cgain *= delta) - 1)
								* egain);
							 */

							if (wtp >= DXOP_WAVE_SZE_M)
								ob[obp] += (wt[0]
									* (wtp - ((float) ((int) wtp)))
									+ wt[(int) wtp]
									* (1.0 - (wtp - ((float) ((int) wtp)))))
									* (((cgain *= delta) - 1) * egain);
							else
								ob[obp] += (wt[(int) wtp + 1]
									* (wtp - ((float) ((int) wtp)))
									+ wt[(int) wtp]
									* (1.0 - (wtp - ((float) ((int) wtp)))))
									* (((cgain *= delta) - 1) * egain);

							if ((wtp += ib[obp++] * transp) >= DXOP_WAVE_SZE)
								while ((wtp -= DXOP_WAVE_SZE) > DXOP_WAVE_SZE)
									;
							while (wtp < 0)
								wtp += DXOP_WAVE_SZE;
						} else
							break;

						if (rampup >= 0) {
							if (rampup)
							{
								if ((egain+=0.01) > gain)
								{
									egain = gain;
									rampup = -1;
								}
							} else {
								if ((egain-=0.01) < gain)
								{
									egain = gain;
									rampup = -1;
								}
							}
						}
					}
					if (cgain <= sustain)
						local->cstate = STATE_SUSTAIN;
				} else {
					while (cgain < sustain)
					{
						if (count-- > 0)
						{
							/*
							ob[obp] += wt[(int) wtp] * (((cgain *= decay) - 1)
								* egain);
							 */

							if (wtp >= DXOP_WAVE_SZE_M)
								ob[obp] += (wt[0]
									* (wtp - ((float) ((int) wtp)))
									+ wt[(int) wtp]
									* (1.0 - (wtp - ((float) ((int) wtp)))))
									* (((cgain *= decay) - 1) * egain);
							else
								ob[obp] += (wt[(int) wtp + 1]
									* (wtp - ((float) ((int) wtp)))
									+ wt[(int) wtp]
									* (1.0 - (wtp - ((float) ((int) wtp)))))
									* (((cgain *= decay) - 1) * egain);

							if ((wtp += ib[obp++] * transp) >= DXOP_WAVE_SZE)
								while ((wtp -= DXOP_WAVE_SZE) > DXOP_WAVE_SZE)
									;
							while (wtp < 0)
								wtp += DXOP_WAVE_SZE;
						} else
							break;

						if (rampup >= 0) {
							if (rampup)
							{
								if ((egain+=0.01) > gain)
								{
									egain = gain;
									rampup = -1;
								}
							} else {
								if ((egain-=0.01) < gain)
								{
									egain = gain;
									rampup = -1;
								}
							}
						}
					}
					if (cgain >= sustain)
						local->cstate = STATE_SUSTAIN;
				}

				voice->flags &= ~BRISTOL_KEYDONE;

				break;
			case STATE_SUSTAIN:
/*printf("sustain %i, %f\n", voice->index, cgain); */
				/*
				 * Sustain - fixed output signal.
				 *
				 * No change for extra stage.
				 */
				cgain = sustain;
				while (count-- > 0)
				{
					/**ob++ = (sustain - 1) * egain; */
					/*
					ob[obp] += wt[(int) wtp] * (sustain - 1) * egain;
					 */
					if (wtp >= DXOP_WAVE_SZE_M)
						ob[obp] += (wt[0]
							* (wtp - ((float) ((int) wtp)))
							+ wt[(int) wtp]
							* (1.0 - (wtp - ((float) ((int) wtp)))))
							* (sustain - 1) * egain;
					else {
						/*
						 * We need to do some bounds checking
						 * And at a few other places probably too.
						 * Its ugly as it could hang here for quite a long
						 * time.
						 */
						while (wtp >= DXOP_WAVE_SZE_M)
							wtp -= DXOP_WAVE_SZE_M;
						while (wtp < 0)
							wtp += DXOP_WAVE_SZE_M;
						ob[obp] += (wt[(int) wtp + 1]
							* (wtp - ((float) ((int) wtp)))
							+ wt[(int) wtp]
							* (1.0 - (wtp - ((float) ((int) wtp)))))
							* (sustain - 1) * egain;
					}

					if ((wtp += ib[obp++] * transp) >= DXOP_WAVE_SZE)
						while ((wtp -= DXOP_WAVE_SZE) > DXOP_WAVE_SZE)
							;
					while (wtp < 0)
						wtp += DXOP_WAVE_SZE;
				}
				voice->flags &= ~BRISTOL_KEYDONE;
				break;
			case STATE_DONE:
			default:
/*printf("done %i, %x\n", voice->index, local->cstate); */
				/*
				 * If we have an unknown state, configure it for OFF.
				 */
				local->cstate = STATE_DONE;
				/*
				 * Wind count to end of iterations.
				 */
				count = 0;
				voice->flags |= BRISTOL_KEYDONE;
				break;
		}
	}
	
	local->wtp = wtp;
	local->cgain = cgain;
	local->egain = egain;
	return(0);
}

/*
 * Setup any variables in our OP structure, in our IO structures, and malloc
 * any memory we need.
 */
bristolOP *
dxopinit(bristolOP **operator, int index, int samplerate, int samplecount)
{
	bristolDXOP *specs;

	*operator = bristolOPinit(operator, index, samplecount);

#ifdef BRISTOL_DBG
	printf("dxopinit(%x(%x), %i, %i, %i)\n",
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

	specs = (bristolDXOP *) bristolmalloc0(sizeof(bristolDXOP));
	(*operator)->specs = (bristolOPSpec *) specs;
	(*operator)->size = sizeof(bristolDXOP);

	/*
	 * These are specific to this operator, and will need to be altered for
	 * each operator.
	 */
	specs->spec.opname = OPNAME;
	specs->spec.description = OPDESCRIPTION;
	specs->spec.pcount = PCOUNT;
	specs->spec.iocount = IOCOUNT;
	specs->spec.localsize = sizeof(bristolDXOPlocal);

	/*
	 * We are going to assign multiple waves to this oscillator.
	 * sine, ramp, square, triangle?
	 */
	specs->wave[0] = (float *) bristolmalloc(DXOP_WAVE_SZE * sizeof(float));
	specs->wave[1] = (float *) bristolmalloc(DXOP_WAVE_SZE * sizeof(float));
	specs->wave[2] = (float *) bristolmalloc(DXOP_WAVE_SZE * sizeof(float));
	specs->wave[3] = (float *) bristolmalloc(DXOP_WAVE_SZE * sizeof(float));
	specs->wave[4] = (float *) bristolmalloc(DXOP_WAVE_SZE * sizeof(float));
	specs->wave[5] = (float *) bristolmalloc(DXOP_WAVE_SZE * sizeof(float));
	specs->wave[6] = (float *) bristolmalloc(DXOP_WAVE_SZE * sizeof(float));
	specs->wave[7] = (float *) bristolmalloc(DXOP_WAVE_SZE * sizeof(float));

	/*
	 * FillWave is something that should be called as a parameter change, but
	 * for testing will load it here.
	 */
	fillWave(specs->wave[0], DXOP_WAVE_SZE, 0);
	fillWave(specs->wave[1], DXOP_WAVE_SZE, 1);
	fillWave(specs->wave[2], DXOP_WAVE_SZE, 2);
	fillWave(specs->wave[3], DXOP_WAVE_SZE, 3);
	fillWave(specs->wave[4], DXOP_WAVE_SZE, 4);
	fillWave(specs->wave[5], DXOP_WAVE_SZE, 5);
	fillWave(specs->wave[6], DXOP_WAVE_SZE, 6);
	fillWave(specs->wave[7], DXOP_WAVE_SZE, 7);

	/*
	 * Now fill in the dxop specs for this operator. These are specific to an
	 * oscillator.
	 */
	specs->spec.param[0].pname = "Transpose";
	specs->spec.param[0].description = "Octave transposition";
	specs->spec.param[0].type = BRISTOL_FLOAT;
	specs->spec.param[0].low = 0;
	specs->spec.param[0].high = 12;
	specs->spec.param[0].flags = BRISTOL_ROTARY|BRISTOL_SLIDER;

	specs->spec.param[1].pname = "Tune";
	specs->spec.param[1].description = "fine tuning of frequency";
	specs->spec.param[1].type = BRISTOL_FLOAT;
	specs->spec.param[1].low = 0;
	specs->spec.param[1].high = 1;
	specs->spec.param[1].flags = BRISTOL_ROTARY|BRISTOL_SLIDER;

	specs->spec.param[2].pname = "Attack";
	specs->spec.param[2].description = "envelope attack rate";
	specs->spec.param[2].type = BRISTOL_FLOAT;
	specs->spec.param[2].low = 0;
	specs->spec.param[2].high = 1;
	specs->spec.param[2].flags = BRISTOL_ROTARY|BRISTOL_SLIDER;

	specs->spec.param[3].pname = "Decay";
	specs->spec.param[3].description = "envelope decay rate";
	specs->spec.param[3].type = BRISTOL_FLOAT;
	specs->spec.param[3].low = 0;
	specs->spec.param[3].high = 1;
	specs->spec.param[3].flags = BRISTOL_ROTARY|BRISTOL_SLIDER;

	specs->spec.param[4].pname = "Sustain";
	specs->spec.param[4].description = "envelope sustain level";
	specs->spec.param[4].type = BRISTOL_FLOAT;
	specs->spec.param[4].low = 0;
	specs->spec.param[4].high = 1;
	specs->spec.param[4].flags = BRISTOL_ROTARY|BRISTOL_SLIDER;

	specs->spec.param[5].pname = "Release";
	specs->spec.param[5].description = "envelope release rate";
	specs->spec.param[5].type = BRISTOL_FLOAT;
	specs->spec.param[5].low = 0;
	specs->spec.param[5].high = 1;
	specs->spec.param[5].flags = BRISTOL_ROTARY|BRISTOL_SLIDER;

	specs->spec.param[6].pname = "gain";
	specs->spec.param[6].description = "output gain on signal";
	specs->spec.param[6].type = BRISTOL_FLOAT;
	specs->spec.param[6].low = 0;
	specs->spec.param[6].high = 2;
	specs->spec.param[6].flags = BRISTOL_ROTARY|BRISTOL_SLIDER|BRISTOL_HIDE;

	specs->spec.param[7].pname = "velocity";
	specs->spec.param[7].description = "envelope tracks key velocity/pressure";
	specs->spec.param[7].type = BRISTOL_INT;
	specs->spec.param[7].low = 0;
	specs->spec.param[7].high = 1;
	specs->spec.param[7].flags = BRISTOL_BUTTON;

	specs->spec.param[8].pname = "OGC";
	specs->spec.param[8].description = "Output gain control";
	specs->spec.param[8].type = BRISTOL_INT;
	specs->spec.param[8].low = 0;
	specs->spec.param[8].high = 1;
	specs->spec.param[8].flags = BRISTOL_BUTTON;

	specs->spec.param[9].pname = "L1";
	specs->spec.param[9].description = "First envelope attack level";
	specs->spec.param[9].type = BRISTOL_FLOAT;
	specs->spec.param[9].low = 0;
	specs->spec.param[9].high = 1;
	specs->spec.param[9].flags = BRISTOL_ROTARY|BRISTOL_SLIDER;

	specs->spec.param[10].pname = "Attack2";
	specs->spec.param[10].description = "Second envelope attack rate";
	specs->spec.param[10].type = BRISTOL_FLOAT;
	specs->spec.param[10].low = 0;
	specs->spec.param[10].high = 1;
	specs->spec.param[10].flags = BRISTOL_ROTARY|BRISTOL_SLIDER;

	specs->spec.param[11].pname = "L2";
	specs->spec.param[11].description = "Second envelope attack level";
	specs->spec.param[11].type = BRISTOL_FLOAT;
	specs->spec.param[11].low = 0;
	specs->spec.param[11].high = 1;
	specs->spec.param[11].flags = BRISTOL_ROTARY|BRISTOL_SLIDER;

	/*
	 * Now fill in the dxop IO specs.
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

	return(*operator);
}
#define BRISTOL_DPO (BRISTOL_VPO * 8)
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
				mem[i] = sin(2 * M_PI * ((double) i) / count) * BRISTOL_DPO;
			return;
		case 1:
		default:
			/* 
			 * This is a square wave.
			 */
			for (i = 0;i < count / 2; i++)
				mem[i] = BRISTOL_DPO * 2 / 3;
			for (;i < count; i++)
				mem[i] = -BRISTOL_DPO * 2 / 3;
			return;
		case 2:
			/* 
			 * This is a pulse wave (we do not have PWM yet).
			 */
			for (i = 0;i < count / 5; i++)
				mem[i] = BRISTOL_DPO * 2 / 3;
			for (;i < count; i++)
				mem[i] = -BRISTOL_DPO * 2 / 3;
			return;
		case 3:
			/* 
			 * This is a ramp wave. We scale the index from -.5 to .5, and
			 * multiply by the range. We go from rear to front to table to make
			 * the ramp wave have a positive leading edge.
			 */
			for (i = count - 1;i >= 0; i--)
				mem[i] = (((float) i / count) - 0.5) * BRISTOL_DPO * 2.0;
			return;
		case 4:
			/*
			 * Triangular wave. From MIN point, ramp up at twice the rate of
			 * the ramp wave, then ramp down at same rate.
			 */
			for (i = 0;i < count / 2; i++)
				mem[i] = -BRISTOL_DPO
					+ ((float) i * 2 / (count / 2)) * BRISTOL_DPO; 
			for (;i < count; i++)
				mem[i] = BRISTOL_DPO -
					(((float) (i - count / 2) * 2) / (count / 2)) * BRISTOL_DPO;
			return;
		case 5:
			/*
			 * Would like to put in a jagged edged ramp wave. Should make some
			 * interesting strings sounds.
			 */
			{
				float accum = -BRISTOL_DPO;

				for (i = 0; i < count / 2; i++)
				{
					mem[i] = accum +
						(0.5 - (((float) i) / count)) * BRISTOL_DPO * 4;
					if (i == count / 8)
						accum = -BRISTOL_DPO * 3 / 4;
					if (i == count / 4)
						accum = -BRISTOL_DPO / 2;
					if (i == count * 3 / 8)
						accum = -BRISTOL_DPO / 4;
				}
				for (; i < count; i++)
					mem[i] = -mem[count - i];
			}
			return;
		case 6:
			/*
			 * Tangiential wave. We limit some of the values, since they do get
			 * excessive. This is only half a tan as well, to maintain the
			 * base frequency.
			 */
			for (i = 0;i < count; i++)
			{
				if ((mem[i] =
					tan(M_PI * ((double) i) / count) * BRISTOL_DPO / 16)
					> BRISTOL_DPO * 8)
					mem[i] = BRISTOL_DPO * 6;
				if (mem[i] < -(BRISTOL_DPO * 6))
					mem[i] = -BRISTOL_DPO * 6;
			}
			return;
		case 7:
			/*
			 * This will be to load a (fixed?) filename. Should probably make
			 * another operator for sample management though, since it would
			 * be better to have pitch shifting incorporated.
			 */
			return;
	}
}

