
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
 *	envelopeinit()
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
#include "envelope.h"

/*
 * The name of this operator, IO count, and IO names.
 */
#define OPNAME "ENV"
#define OPDESCRIPTION "Digital ADSR Envelope Generator"
#define PCOUNT 9
#define IOCOUNT 1

#define ENV_OUT_IND 0

/*
 * Reset any local memory information.
 */
static int destroy(bristolOP *operator)
{
#ifdef BRISTOL_DBG
	printf("reset(%x)\n", operator);
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

/*
 * Reset any local memory information.
 */
static int reset(bristolOP *operator, bristolOPParams *param)
{
#ifdef BRISTOL_DBG
	printf("reset(%x)\n", operator);
#endif

	param->param[4].float_val = 1.0;
	param->param[5].float_val = 1.0; /* Full velocity tracking */
	param->param[5].int_val = 1;
	/* Memory moog options - rezero/condition/velocity_track */
	param->param[6].float_val = 0;
	param->param[7].float_val = 0;
	param->param[8].float_val = 0;
	return(0);
}

/*
 * Alter an internal parameter of an operator.
 */
static int param(bristolOP *operator, bristolOPParams *param,
	unsigned char index, float value)
{
	int offset = (value * (CONTROLLER_RANGE - 1)) + 1;
	bristolENV *specs;

	specs = (bristolENV *) operator->specs;

	if (offset >= CONTROLLER_RANGE)
		offset = C_RANGE_MIN_1;
	else if (offset < 0)
		offset = 0;

#ifdef BRISTOL_DBG
	printf("env param(%i, %f)\n", index, value);
#endif

	switch (index) {
		case 0:
			/*
			 * Attack. This gets tweaked into a rate at operational time as
			 * per decay and release.
			 */
			param->param[index].float_val = value;
			break;
		case 1:
		case 3:
			/*
			 * Decay and Release. We have our overall duration, we need to find
			 * out how many multiplies we need to decay the target number of
			 * levels over duration*samplerate samples.
			param->param[index].float_val = 1/gainTable[offset].rate;
			 */
			param->param[index].float_val =
				1/powf(13.0, 1.0/
					(2.0 + value*value*value
						* specs->samplerate*specs->duration));
			break;
		case 2:
			offset = value * (CONTROLLER_RANGE - 1);
			/*
			 * Sustain
			 *
			param->param[index].float_val = gainTable[offset].gain
				* BRISTOL_VPO;
			 *
			 * The gain tables are fine for frequency adjustments, but do 
			 * not work well for gain here. Hence
			 */
			param->param[index].float_val = value * BRISTOL_VPO;
			break;
		case 4:
			/*
			 * Gain
			param->param[index].float_val = gainTable[offset].gain;
			 */
			param->param[index].float_val = value;
			break;
		case 5:
			/*
			 * Velocity tracking gain. The lack of a break is intentional.
			 */
			if (value == 0)
				param->param[index].int_val = 0;
			else
				param->param[index].int_val = 1;
		case 6:
		case 7:
			/*
			 * A few options are required for the Memory Moog envelope. The
			 * default is that they have the value 0. These are:
			 *	6: Rezero - a retrigger should return to zero gain.
			 *	7: Condition - continue to full gain even if keyoff happens.
			 */
			param->param[index].float_val = value;
			break;
		case 8:
			/*
			 *	8: Velocity tracking for attack/decay/release values. We
			 *	should recalculate ARSR when this changes.
			 */
			param->param[index].float_val = value;
			break;
		case 9:
			if ((specs->duration = offset - 1) < 1)
				specs->duration = 10;
			break;
		case 10:
			if (value != 0)
				specs->flags |= LINEAR_ATTACK;
			else
				specs->flags &= ~LINEAR_ATTACK;
			break;
	}
#ifdef BRISTOL_DBG
	printf("a %f, d %f, s %0.10f r %f g %f %i, %i\n",
		param->param[0].float_val, param->param[1].float_val,
		param->param[2].float_val, param->param[3].float_val,
		param->param[4].float_val, param->param[5].int_val, offset);
#endif

	return(0);
}

#define MM_PARAM (1.0f - voice->velocity * param->param[8].float_val) * (1.0f - voice->velocity * param->param[8].float_val)
/*
 * Pure output signal generator - will drive amps, filters, DCOs, whatever.
 */
static int operate(register bristolOP *operator, bristolVoice *voice,
	bristolOPParams *param, void *lcl)
{
	register bristolENVlocal *local = lcl;
	register float cgain, attack, decay, sustain, release, *ob, gain, egain;
	register int count, rampup = -1;
	bristolENV *specs;

	specs = (bristolENV *) operator->specs;

#ifdef BRISTOL_DBG
	printf("envelope(%x, %x, %x, %x)\n", operator, voice, param, local);
#endif

	if (specs->flags & LINEAR_ATTACK)
	{
		if (param->param[8].float_val == 0.0)
			//attack = gainTable[
			//	(int) (C_RANGE_MIN_1 * param->param[0].float_val)].rate;
			attack = 12.0 / (specs->samplerate * 0.0005 + specs->duration * specs->samplerate
				* param->param[0].float_val
				* param->param[0].float_val
				* param->param[0].float_val);
		else
			/*
			 * Memory Moog uses this for keyboard controlled envelope. The
			 * 'strict' specification was that parameters changed by note, not
			 * velocity, but that was largely because it did not have velocity.
			 *
			 * This is going to follow the crumar model - velocity affects only
			 * the Attack, not decay, sustain or release.
			 * 
			 * For the Bit-1/99 this will be changed such that it is not simply
			 * on or off but scalable.
			attack = param->param[0].float_val * MM_PARAM;
			attack = gainTable[(int) (attack * C_RANGE_MIN_1 + 1)].rate;
			 */
			attack = 12.0 / (specs->samplerate * 0.0005 + specs->duration * specs->samplerate
				* param->param[0].float_val * MM_PARAM);
	} else {
		if (param->param[8].float_val == 0.0)
			attack = gainTable[
				(int) (C_RANGE_MIN_1 * param->param[0].float_val
					* param->param[0].float_val)].rate;
		else {
			if ((attack = param->param[0].float_val
				* param->param[0].float_val * MM_PARAM) > 1.0)
				attack = 1.0;
			else if (attack < 0)
				attack = 0;
			attack = gainTable[(int) (attack * C_RANGE_MIN_1)].rate;
		}
	}

	decay = param->param[1].float_val;
	sustain = 1.0 + param->param[2].float_val;
	release = param->param[3].float_val;

	/*
	 * Adjust the gain depending on the key velocity, and include both channel
	 * and polypressure - we will typically have one or the other, but both
	 * is highly unlikely.
	 *
	 * This gain figure is related to an option for key tracking?
	 */
	if (param->param[5].int_val)
		gain = param->param[4].float_val
			* (voice->velocity + voice->press + voice->chanpressure)
			* param->param[5].float_val + (1.0 - param->param[5].float_val);
	else
		gain = param->param[4].float_val;

	/*
	 * Egain is to smooth over changes to the velocity gain when we retrigger
	 * the voice.
	 */
	egain = local->egain;
	if (egain < gain)
		rampup = 1;
	else if (egain > gain)
		rampup = 0;

	count = specs->spec.io[ENV_OUT_IND].samplecount;
	cgain = local->cgain;
	ob = specs->spec.io[ENV_OUT_IND].buf;

	/*
	 * The offsets are only provided once so if we have it then it's function is
	 * actually similar to note_off. Anyway.
	if ((voice->offset >= 0) && (voice->flags & BRISTOL_KEYOFF))
		printf("%i: env off offset %i\n", voice->key.key, voice->offset);
	 */

	if (voice->flags & BRISTOL_KEYOFF)
	{
		voice->flags |= BRISTOL_KEYOFFING;
		if (param->param[7].float_val != 0)
		{
			if (local->cstate != STATE_ATTACK)
				local->cstate = STATE_RELEASE;
		} else
			local->cstate = STATE_RELEASE;
	}

	if ((voice->flags & BRISTOL_KEYOFFING)
		&& (param->param[7].float_val != 0)
		&& (local->cstate != STATE_ATTACK))
		local->cstate = STATE_RELEASE;

	if (voice->flags & BRISTOL_KEYON)
	{
		/* There is some issue here - do we want to rezero? */
		if ((voice->baudio->voicecount > 1) || (param->param[6].float_val != 0)
			|| (cgain < 1.0f))
			cgain = 1.0;

		local->cstate = STATE_ATTACK;
		/*
		 * This is for Jack Sample Accurate support. It is a compromise as I am
		 * not a great fan of the feature: the envelope will delay its attack
		 * event by the number of frames indicated in the voice offset, that
		 * offset is taken from the Jack event (it is zero for all other bristol
		 * MIDI interface drivers).
		 *
		 * State changes will not be sample accurate in the first version - they
		 * are more work as the current operation has to continue until the time
		 * of the event. Note off is not such a big deal as having a bit of 
		 * decay will mask out the exact point of the note. One consideration is
		 * application to REON event but a REON is going to be masked by the
		 * current output anyway. Since all these events are related to Jack
		 * then NOTE_OFF and REON are still accurate to the period, ie, they 
		 * will behave consistently across different plays and when mastering
		 * at max speed.
		 *
		 * Features other than envelope sync (and the existing LFO sync) are 
		 * not likely to get considered - this is an analogue emulator and I
		 * don't really want things like general oscillator sync.
		 */
		if ((~voice->flags & BRISTOL_KEYREON) &&
			(voice->offset > 0) && (voice->offset < count))
		{
			if (voice->baudio->midiflags & BRISTOL_MIDI_DEBUG1)
				printf("envelope trigger offset %i frames\n", voice->offset);

			memset(ob, 0.0f, voice->offset * sizeof(float));
			ob += voice->offset;
			count -= voice->offset;
		}
	} else if (voice->flags & BRISTOL_KEYREON)
		local->cstate = STATE_ATTACK;

	if (voice->flags & (BRISTOL_KEYON|BRISTOL_KEYREON))
	{
		/*
		 * This accepts note on offsets from Jack. Note off offsets will be
		 * based on period size for now.
		printf("%i: env on  offset %i\n", voice->key.key, voice->offset);
		 */
		/* Rezero gain */
		if (param->param[6].float_val != 0)
			cgain = 1.0;

		local->cstate = STATE_ATTACK;
		voice->flags &= ~(BRISTOL_KEYDONE|BRISTOL_KEYOFFING);
	}

	/*
	 * We need some state indicator to say when we are leading into the attack.
	 * This is actually in the MIDI voice structure, but we do not have access
	 * to this. We should assume that a MIDI event sets our cstate as necessary?
	 */
	while (count > 0)
	{
		switch (local->cstate)
		{
			case STATE_RELEASE:
				/*
				 * Assume key off - start releasing gain.
				 */
				while (cgain > 1.0)
				{
					if (count-- > 0)
						*ob++ = ((cgain *= release) - 1) * egain;
					else
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
/* printf("keydone: %i, %i, %x\n", voice->key.key, local->cstate, voice); */
					cgain = 1.0;
					local->cstate = STATE_DONE;
					if (~voice->flags & (BRISTOL_KEYON|BRISTOL_KEYREON))
						voice->flags |= BRISTOL_KEYDONE;
					voice->flags &= ~BRISTOL_KEYOFFING;
				} else
					voice->flags &= ~(BRISTOL_KEYDONE);
				break;
			case STATE_START:
			case STATE_ATTACK:
				/*
				 * Just triggered - start attack ramp from current gain level.
				 */
				if (specs->flags & LINEAR_ATTACK) {
					while (cgain < BRISTOL_VPO)
					{
						if (count-- > 0)
							*ob++ = ((cgain += attack) - 1) * egain;
						else
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
				} else {
					while (cgain < BRISTOL_VPO)
					{
						if (count-- > 0)
							*ob++ = ((cgain *= attack) - 1) * egain;
						else
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
				}

				if (cgain >= BRISTOL_VPO) local->cstate = STATE_DECAY;
				/*
				 * This just ensures that it is the last envelope in any given
				 * bristolSound chain that decides when to stop the audio
				 * stream.
				 */
				voice->flags &= ~(BRISTOL_KEYDONE);
				break;
			case STATE_DECAY:
				/*
				 * Decay state. Ramp down to sustain level.
				 */
				while (cgain > sustain)
				{
					if (count-- > 0)
						*ob++ = ((cgain *= decay) - 1) * egain;
					else
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
				voice->flags &= ~(BRISTOL_KEYDONE);
				break;
			case STATE_SUSTAIN:
				/*
				 * Sustain - fixed output signal.
				 */
				cgain = sustain;
				while (count-- > 0)
				{
					*ob++ = (sustain - 1) * egain;
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
				voice->flags &= ~(BRISTOL_KEYDONE);
				break;
			default:
				/*
				 * If we have an unknown state, configure it for OFF.
printf("unknown state: %i, %i\n", voice->key.key, local->cstate);
				 */
			case STATE_DONE:
				local->cstate = STATE_DONE;
				while (count-- > 0)
					*ob++ = 0.0;
				if (~voice->flags & (BRISTOL_KEYON|BRISTOL_KEYREON))
					voice->flags |= (BRISTOL_KEYDONE);
				
				break;
		}
	}

	local->cgain = cgain;
	local->egain = egain;

	return(0);
}

/*
 * Setup any variables in our OP structure, in our IO structures, and malloc
 * any memory we need.
 */
bristolOP *
envinit(bristolOP **operator, int index, int samplerate, int samplecount)
{
	bristolENV *specs;

#ifdef BRISTOL_DBG
	printf("envelopeinit(%x(%x), %i, %i, %i)\n",
		operator, *operator, index, samplerate, samplecount);
#endif

	*operator = bristolOPinit(operator, index, samplecount);

	/*
	 * Then the local parameters specific to this operator. These will be
	 * the same for each operator, but must be inited in the local code.
	 */
	(*operator)->operate = operate;
	(*operator)->destroy = destroy;
	(*operator)->reset = reset;
	(*operator)->param = param;

	specs = (bristolENV *) bristolmalloc0(sizeof(bristolENV));
	specs->samplerate = samplerate;
	specs->duration = BRISTOL_RAMP_RATE;
#ifdef BRISTOL_LIN_ATTACK
	specs->flags |= LINEAR_ATTACK;
#endif
	(*operator)->specs = (bristolOPSpec *) specs;
	(*operator)->size = sizeof(bristolENV);

	/*
	 * These are specific to this operator, and will need to be altered for
	 * each operator.
	 */
	specs->spec.opname = OPNAME;
	specs->spec.description = OPDESCRIPTION;
	specs->spec.pcount = PCOUNT;
	specs->spec.iocount = IOCOUNT;
	specs->spec.localsize = sizeof(bristolENVlocal);

	/*
	 * Now fill in the envelope specs for this operator. These are specific to
	 * an ADSR.
	 */
	specs->spec.param[0].pname = "attack";
	specs->spec.param[0].description = "Initial ramp up rate";
	specs->spec.param[0].type = BRISTOL_FLOAT;
	specs->spec.param[0].low = 0;
	specs->spec.param[0].high = 1;
	specs->spec.param[0].flags = BRISTOL_ROTARY|BRISTOL_SLIDER;

	specs->spec.param[1].pname = "decay";
	specs->spec.param[1].description = "Decay to sustain rate";
	specs->spec.param[1].type = BRISTOL_FLOAT;
	specs->spec.param[1].low = 0;
	specs->spec.param[1].high = 1;
	specs->spec.param[1].flags = BRISTOL_ROTARY|BRISTOL_SLIDER;

	specs->spec.param[2].pname = "sustain";
	specs->spec.param[2].description = "Output level steady state";
	specs->spec.param[2].type = BRISTOL_FLOAT;
	specs->spec.param[2].low = 0;
	specs->spec.param[2].high = 1;
	specs->spec.param[2].flags = BRISTOL_ROTARY|BRISTOL_SLIDER;

	specs->spec.param[3].pname = "release";
	specs->spec.param[3].description = "Final decay rate";
	specs->spec.param[3].type = BRISTOL_FLOAT;
	specs->spec.param[3].low = 0;
	specs->spec.param[3].high = 1;
	specs->spec.param[3].flags = BRISTOL_ROTARY|BRISTOL_SLIDER;

	specs->spec.param[4].pname = "gain";
	specs->spec.param[4].description = "Overall signal level";
	specs->spec.param[4].type = BRISTOL_FLOAT;
	specs->spec.param[4].low = 0;
	specs->spec.param[4].high = 1;
	specs->spec.param[4].flags = BRISTOL_ROTARY|BRISTOL_SLIDER;

	specs->spec.param[5].pname = "velocity";
	specs->spec.param[5].description = "velocity sensitive gain";
	specs->spec.param[5].type = BRISTOL_INT;
	specs->spec.param[5].low = 0;
	specs->spec.param[5].high = 1;
	specs->spec.param[5].flags = BRISTOL_BUTTON;

	specs->spec.param[6].pname = "rezero";
	specs->spec.param[6].description = "reset gain on retrigger";
	specs->spec.param[6].type = BRISTOL_INT;
	specs->spec.param[6].low = 0;
	specs->spec.param[6].high = 1;
	specs->spec.param[6].flags = BRISTOL_BUTTON;

	specs->spec.param[7].pname = "conditional";
	specs->spec.param[7].description = "always gain to max";
	specs->spec.param[7].type = BRISTOL_INT;
	specs->spec.param[7].low = 0;
	specs->spec.param[7].high = 1;
	specs->spec.param[7].flags = BRISTOL_BUTTON;

	specs->spec.param[8].pname = "velocity";
	specs->spec.param[8].description = "velocity sensitive ADR";
	specs->spec.param[8].type = BRISTOL_FLOAT;
	specs->spec.param[8].low = 0;
	specs->spec.param[8].high = 1;
	specs->spec.param[8].flags = BRISTOL_BUTTON;

	/*
	 * Now fill in the dco IO specs.
	 */
	specs->spec.io[0].ioname = "output";
	specs->spec.io[0].description = "ADSR Envelope Output Signal";
	specs->spec.io[0].samplerate = samplerate;
	specs->spec.io[0].samplecount = samplecount;
	specs->spec.io[0].flags = BRISTOL_DC|BRISTOL_OUTPUT;

	return(*operator);
}

