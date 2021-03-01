
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

#include "bristol.h"
#include "envelope.h"

static float sr;

/*
 * The name of this operator, IO count, and IO names.
 */
#define OPNAME "ENV"
#define OPDESCRIPTION "Digital ADSR Envelope Generator"
#define PCOUNT 6
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

	param->param[0].float_val = 0.5;
	param->param[1].float_val = 1.0;
	param->param[2].float_val = 0.1;
	param->param[3].float_val = 0.1;
	param->param[4].float_val = 0.1;
	param->param[5].float_val = 1.0;
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
		offset = C_RANGE_MIN_1;
	else if (offset < 0)
		offset = 0;

#ifdef BRISTOL_DBG
	printf("param(%x, %f)\n", operator, value);
#endif

	switch (index) {
		case 0:
		case 1:
			/* IL/AL Levels */
			param->param[index].float_val = value;
			break;
		case 2:
		case 3:
		case 4:
			/*
			 * A/D/R
			 * I want this to be linear 0 to 5 seconds.
			 */
			if (value <= 0)
				value = 1.0 / CONTROLLER_RANGE;
			param->param[index].float_val = 5.0 / (value * sr);
			break;
		case 5:
			/* gain */
			param->param[index].float_val = value;
			break;
	}
#ifdef BRISTOL_DBG
#endif
	printf("%i: il %f, al %f, a %f d %f r %f %f\n", index,
		param->param[0].float_val, param->param[1].float_val,
		param->param[2].float_val, param->param[3].float_val,
		param->param[4].float_val, param->param[5].float_val);

	return(0);
}

#define MM_PARAM voice->velocity * param->param[8].float_val
/*#define MM_PARAM voice->baudio->contcontroller[1] */
/*
 * Pure output signal generator - will drive amps, filters, DCOs, whatever.
 */
static int operate(register bristolOP *operator, bristolVoice *voice,
	bristolOPParams *param, void *lcl)
{
	register bristolENVlocal *local = lcl;
	register float cgain, attack, al, decay, il, release, *ob;
	register int count;
	bristolENV *specs;

	specs = (bristolENV *) operator->specs;

#ifdef BRISTOL_DBG
	printf("envelope(%x, %x, %x, %x)\n", operator, voice, param, local);
#endif

	/*
	 * We are going to be governed by 4 different levels selected depending on
	 * our current state.
	 *
	 *	cgain	- current gain level
	 *	il		- initial level set at KEYON/REON and sustain at steady state
	 *	al		- level to which we attack (may decay depending on level)
	 *	0		- decay level on KEYOFF (il = 0)
	 *
	 * The envelope is not touch sensitive
	 */
	cgain = local->cgain;
	il = param->param[0].float_val;
	al = param->param[1].float_val;

	attack = gainTable[(int) (C_RANGE_MIN_1 * param->param[2].float_val)].rate;
	if (attack == 0.0)
		attack = 1/CONTROLLER_RANGE;
	decay = param->param[3].float_val;
	release = param->param[4].float_val;

	count = specs->spec.io[ENV_OUT_IND].samplecount;
	ob = specs->spec.io[ENV_OUT_IND].buf;

	if (voice->flags & BRISTOL_KEYOFF)
	{
		voice->flags |= BRISTOL_KEYOFFING;
		local->cstate = STATE_RELEASE;
	}

	if ((voice->flags & BRISTOL_KEYOFFING) && (local->cstate != STATE_ATTACK))
		local->cstate = STATE_RELEASE;

	if (voice->flags & BRISTOL_KEYREON)
		local->cstate = STATE_ATTACK;
	else if (voice->flags & BRISTOL_KEYON) {
		local->cstate = STATE_ATTACK;
		/* At keyon we really need to stuff a starting value */
		cgain = param->param[0].float_val;

		if ((~voice->flags & BRISTOL_KEYREON) &&
			(voice->offset > 0) && (voice->offset < count))
		{
			if (voice->baudio->midiflags & BRISTOL_MIDI_DEBUG1)
				printf("envelope trigger offset %i frames\n", voice->offset);

			memset(ob, 0.0f, voice->offset * sizeof(float));
			ob += voice->offset;
			count -= voice->offset;
		}
	}

	/*
	 * We should only need a state and consequent target gain. We then trend
	 * the current gain towards the target until we meet it at which point we
	 * change the state.
	 */
	while (count > 0)
	{
		switch (local->cstate)
		{
			case STATE_RELEASE:
				/*
printf("State release %f %f\n", cgain, 0.0);
				 * Assume key off - start releasing gain.
				 */
				while (cgain > 0.0)
				{
					if (count-- > 0)
						*ob++ = (cgain -= release);
					else
						break;
				}
				if (cgain <= 0.0)
				{
					cgain = 0.0;
					local->cstate = STATE_DONE;
					voice->flags &= ~BRISTOL_KEYOFFING;
				}
				break;
			case STATE_START:
			case STATE_ATTACK:
				/*
printf("State attack %f %f\n", cgain, al);
				 * Just triggered - start attack ramp from current gain level.
				 */
				if (cgain < al) {
					while (cgain < al)
					{
						if (count-- > 0)
							*ob++ = (cgain += attack);
						else
							break;
					}
					if (cgain >= al) local->cstate = STATE_DECAY;
				} else {
					while (cgain > al)
					{
						if (count-- > 0)
							*ob++ = (cgain -= attack);
						else
							break;
					}
					if (cgain <= al) local->cstate = STATE_DECAY;
				}
				/*
				 * This just ensures that it is the last envelope in any given
				 * bristolSound chain that decides when to stop the audio
				 * stream.
				 */
				break;
			case STATE_DECAY:
				/*
printf("State decay %f %f\n", cgain, il);
				 * Decay state. Ramp down to il level.
				 */
				if (cgain > il) {
					while (cgain > il)
					{
						if (count-- > 0)
							*ob++ = (cgain -= decay);
						else
							break;
					}
					if (cgain <= il) local->cstate = STATE_SUSTAIN;
				} else {
					while (cgain < il)
					{
						if (count-- > 0)
							*ob++ = (cgain += decay);
						else
							break;
					}
					if (cgain >= il) local->cstate = STATE_SUSTAIN;
				}
				break;
			case STATE_SUSTAIN:
				/*
				 * Sustain - fixed output signal.
				 */
				cgain = il;
				while (count-- > 0)
					*ob++ = il;
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
/* printf("state done: %i, %i\n", voice->key.key, local->cstate); */
				break;
		}
	}
	local->cgain = cgain;
	return(0);
}

/*
 * Setup any variables in our OP structure, in our IO structures, and malloc
 * any memory we need.
 */
bristolOP *
cs80envinit(bristolOP **operator, int index, int samplerate, int samplecount)
{
	bristolENV *specs;

	sr = samplerate;

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
	specs->spec.param[0].pname = "IL";
	specs->spec.param[0].description = "Initial Envelope Level";
	specs->spec.param[0].type = BRISTOL_FLOAT;
	specs->spec.param[0].low = 0;
	specs->spec.param[0].high = 1;
	specs->spec.param[0].flags = BRISTOL_ROTARY|BRISTOL_SLIDER;

	specs->spec.param[1].pname = "AL";
	specs->spec.param[1].description = "Attach Level";
	specs->spec.param[1].type = BRISTOL_FLOAT;
	specs->spec.param[1].low = 0;
	specs->spec.param[1].high = 1;
	specs->spec.param[1].flags = BRISTOL_ROTARY|BRISTOL_SLIDER;

	specs->spec.param[2].pname = "Attack";
	specs->spec.param[2].description = "Output level steady state";
	specs->spec.param[2].type = BRISTOL_FLOAT;
	specs->spec.param[2].low = 0;
	specs->spec.param[2].high = 1;
	specs->spec.param[2].flags = BRISTOL_ROTARY|BRISTOL_SLIDER;

	specs->spec.param[3].pname = "Decay";
	specs->spec.param[3].description = "decay rate";
	specs->spec.param[3].type = BRISTOL_FLOAT;
	specs->spec.param[3].low = 0;
	specs->spec.param[3].high = 1;
	specs->spec.param[3].flags = BRISTOL_ROTARY|BRISTOL_SLIDER;

	specs->spec.param[4].pname = "Release";
	specs->spec.param[4].description = "final Decay Rate";
	specs->spec.param[4].type = BRISTOL_FLOAT;
	specs->spec.param[4].low = 0;
	specs->spec.param[4].high = 1;
	specs->spec.param[4].flags = BRISTOL_ROTARY|BRISTOL_SLIDER;

	specs->spec.param[5].pname = "gain";
	specs->spec.param[5].description = "Overall signal level";
	specs->spec.param[5].type = BRISTOL_FLOAT;
	specs->spec.param[5].low = 0;
	specs->spec.param[5].high = 1;
	specs->spec.param[5].flags = BRISTOL_ROTARY|BRISTOL_SLIDER;

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

