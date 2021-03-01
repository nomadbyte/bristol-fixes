
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
 *	aksenvinit()
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
#include "aksenv.h"

/*
 * The name of this operator, IO count, and IO names.
 */
#define OPNAME "AKSENV"
#define OPDESCRIPTION "Digital ADSR Envelope Generator"
#define PCOUNT 4
#define IOCOUNT 3

#define AKSENV_OUT_IND 0

static float rate;

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

	param->param[0].float_val = 0.01;
	param->param[1].int_val = 50000; /* Hm, about one second! */
	param->param[2].float_val = 0.01;
	param->param[3].float_val = 50000; /* Hm, about one second! */
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
	else if (offset < 0)
		offset = 0;

#ifdef BRISTOL_DBG
	printf("param(%x, %f)\n", operator, value);
#endif

	/*
	 * Attack	2m-1s
	 * On		0-2.5s
	 * Decay	3ms-15.s
	 * off		10ms-5s + off
	 */
	switch (index) {
		case 0:
			/*
			 * Attack value must go from 88 to one secondsworth samples
			 */
			if (value < 0.002)
				value = 0.002;
			param->param[index].float_val = 1.0 / (value * rate);
/*printf("%i is %f\n", index, param->param[index].float_val); */
			break;
		case 1:
			/*
			 * On from 0 to 2.5 * rate
			 */
			param->param[index].float_val = value * rate * 2.5f;
/*printf("%i is %f\n", index, param->param[index].float_val); */
			break;
		case 2:
			/*
			 * Decay
			 */
			if (value < ((float) 3.0f / 15000.0f))
				value = ((float) 3.0) / 15000.0f;
			param->param[index].float_val = 1.0f / (value * rate * 15);
/*printf("%i is %f\n", index, param->param[index].float_val); */
			param->param[index].float_val = value;
			break;
		case 3:
			/*
			 * For manual trigger
			 */
			if (value == 1.0)
				param->param[index].int_val = 0;
			else
				param->param[index].int_val = 1;

			if ((value = value * rate * 5) < 440)
				value = 440;

			param->param[index].float_val =  value;
/*printf("%i is %f\n", index, param->param[index].float_val); */
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

#define MM_PARAM voice->velocity
/*#define MM_PARAM voice->baudio->contcontroller[1] */
/*
 * Pure output signal generator - will drive amps, filters, DCOs, whatever.
 */
static int operate(register bristolOP *operator, bristolVoice *voice,
	bristolOPParams *param, void *lcl)
{
	register bristolAKSENVlocal *local = lcl;
	register float cgain, attack, on, decay, off, *ob, egain, oncount;
	register int count, rampup = -1;
	bristolAKSENV *specs;

	specs = (bristolAKSENV *) operator->specs;

#ifdef BRISTOL_DBG
	printf("aksenv(%x, %x, %x, %x)\n", operator, voice, param, local);
#endif

	attack = param->param[0].float_val;
	on = param->param[1].float_val;
	off = param->param[3].float_val;
/*printf("%f %f %f %f\n", attack, on, decay, off); */

	/*
	 * We are only going to stuff the decay modifier once per call. This is
	 * a change to a slow parameter so the coarse adjustment should not have
	 * a major effect on the result?
	decay = param->param[2].float_val;
	 */
	if ((cgain = specs->spec.io[1].buf[0] * 0.01 + param->param[2].float_val)
		< ((float) 3.0f / 15000.0f))
		cgain = ((float) 3.0) / 15000.0f;
	decay = 1.0f / (cgain * rate * 15);

	/*
	 * Egain is to smooth over changes to the velocity gain when we retrigger
	 * the voice.
	 */
	cgain = local->cgain;
	egain = local->egain;
	oncount = local->oncount;

	if (egain < cgain)
		rampup = 1;
	else if (egain > cgain)
		rampup = 0;

	count = specs->spec.io[AKSENV_OUT_IND].samplecount;
	ob = specs->spec.io[AKSENV_OUT_IND].buf;

	if (voice->flags & BRISTOL_KEYON)
	{
		cgain = 0.0;
		local->cstate = STATE_ATTACK;
		voice->flags &= ~BRISTOL_KEYON;

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
	 * See if we need to retrigger. If we are triggered already then reset
	 * the trigger if signal is zero. If we are not triggered then may set state
	 */
	if (local->estate == 0)
	{
		if (specs->spec.io[2].buf[0] > 0)
		{
			local->cstate = STATE_ATTACK;
			local->estate = 1;
		}
	} else {
		if (specs->spec.io[2].buf[0] <= 0)
			local->estate = 0;
	}

	/*
	 * Attack	2m-1s
	 * On		0-2.5s
	 * Decay	3ms-15s
	 * off		10ms-5s + off
	 *
	 * We need some state indicator to say when we are leading into the attack.
	 * This is actually in the MIDI voice structure, but we do not have access
	 * to this. We should assume that a MIDI event sets our cstate as necessary?
	 */
	while (count > 0)
	{
		/*
		 * We have parameters for rate of attack, on time, rate of decay and
		 * off time. If offtime is 1.0 then we do not have automatic retrigger
		 * but wait for a KEYON flag.
		 */
		switch (local->cstate)
		{
			case STATE_ATTACK:
				while ((cgain += attack) < 1.0f)
				{
					*ob++ = cgain;
					if (count-- <= 0)
						break;
				}
				if (cgain >= 1.0)
				{
					local->cstate = STATE_ON;
					oncount = on;
				}
				break;
			case STATE_ON:
				while (--oncount > 0)
				{
					*ob++ = 1.0f;
					if (count-- <= 0)
						break;
				}
				if (oncount <= 0)
					local->cstate = STATE_DECAY;
				break;
			case STATE_DECAY:
				while ((cgain -= decay) > 0.0f)
				{
					*ob++ = cgain;
					if (count-- <= 0)
						break;
				}
				if (cgain <= 0.0f)
				{
					local->cstate = STATE_OFF;
					oncount = off;
				}
				break;
			case STATE_OFF:
				while (--oncount > 0)
				{
					*ob++ = 0.0f;
					if (count-- <= 0)
						break;
				}
				if (oncount <= 0)
				{
					if (param->param[3].int_val != 0)
						local->cstate = STATE_ATTACK;
				}
				break;
		}
	}
	local->cgain = cgain;
	local->egain = egain;
	local->oncount = oncount;
	return(0);
}

/*
 * Setup any variables in our OP structure, in our IO structures, and malloc
 * any memory we need.
 */
bristolOP *
aksenvinit(bristolOP **operator, int index, int samplerate, int samplecount)
{
	bristolAKSENV *specs;

#ifdef BRISTOL_DBG
	printf("aksenvinit(%x(%x), %i, %i, %i)\n",
		operator, *operator, index, samplerate, samplecount);
#endif

	*operator = bristolOPinit(operator, index, samplecount);

	rate = samplerate;

	/*
	 * Then the local parameters specific to this operator. These will be
	 * the same for each operator, but must be inited in the local code.
	 */
	(*operator)->operate = operate;
	(*operator)->destroy = destroy;
	(*operator)->reset = reset;
	(*operator)->param = param;

	specs = (bristolAKSENV *) bristolmalloc0(sizeof(bristolAKSENV));
	(*operator)->specs = (bristolOPSpec *) specs;
	(*operator)->size = sizeof(bristolAKSENV);

	/*
	 * These are specific to this operator, and will need to be altered for
	 * each operator.
	 */
	specs->spec.opname = OPNAME;
	specs->spec.description = OPDESCRIPTION;
	specs->spec.pcount = PCOUNT;
	specs->spec.iocount = IOCOUNT;
	specs->spec.localsize = sizeof(bristolAKSENVlocal);

	/*
	 * Now fill in the aksenv specs for this operator. These are specific to
	 * an ADSR.
	 */
	specs->spec.param[0].pname = "attack";
	specs->spec.param[0].description = "Initial ramp up rate";
	specs->spec.param[0].type = BRISTOL_FLOAT;
	specs->spec.param[0].low = 0;
	specs->spec.param[0].high = 1;
	specs->spec.param[0].flags = BRISTOL_ROTARY|BRISTOL_SLIDER;

	specs->spec.param[1].pname = "on";
	specs->spec.param[1].description = "Decay to sustain rate";
	specs->spec.param[1].type = BRISTOL_FLOAT;
	specs->spec.param[1].low = 0;
	specs->spec.param[1].high = 1;
	specs->spec.param[1].flags = BRISTOL_ROTARY|BRISTOL_SLIDER;

	specs->spec.param[2].pname = "decay";
	specs->spec.param[2].description = "Output level steady state";
	specs->spec.param[2].type = BRISTOL_FLOAT;
	specs->spec.param[2].low = 0;
	specs->spec.param[2].high = 1;
	specs->spec.param[2].flags = BRISTOL_ROTARY|BRISTOL_SLIDER;

	specs->spec.param[3].pname = "off";
	specs->spec.param[3].description = "Final decay rate";
	specs->spec.param[3].type = BRISTOL_FLOAT;
	specs->spec.param[3].low = 0;
	specs->spec.param[3].high = 1;
	specs->spec.param[3].flags = BRISTOL_ROTARY|BRISTOL_SLIDER;

	/*
	 * Now fill in the dco IO specs.
	 */
	specs->spec.io[0].ioname = "output";
	specs->spec.io[0].description = "ADSR Envelope Output Signal";
	specs->spec.io[0].samplerate = samplerate;
	specs->spec.io[0].samplecount = samplecount;
	specs->spec.io[0].flags = BRISTOL_DC|BRISTOL_OUTPUT;

	specs->spec.io[1].ioname = "decay";
	specs->spec.io[1].description = "decay parameter modifier";
	specs->spec.io[1].samplerate = samplerate;
	specs->spec.io[1].samplecount = samplecount;
	specs->spec.io[1].flags = BRISTOL_DC|BRISTOL_OUTPUT;

	specs->spec.io[2].ioname = "trigger";
	specs->spec.io[2].description = "trigger gate";
	specs->spec.io[2].samplerate = samplerate;
	specs->spec.io[2].samplecount = samplecount;
	specs->spec.io[2].flags = BRISTOL_DC|BRISTOL_OUTPUT;

	return(*operator);
}

