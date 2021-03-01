
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

//#define DEBUG

#include <math.h>

#include "bristol.h"
#include "bristolmidi.h"
#include "bristolmidieventnames.h"

extern int midiNoteOn();
extern int midiNoteOff();
extern void allNotesOff();
extern void sustainedNotesOff();
extern void sustainedNotesOff();
extern int bristolSystem(audioMain *, bristolMidiMsg *);

int buildCurrentTable(Baudio *, float);

bristolVoice *
findVoice(bristolVoice *voice, int key, int chan)
{
	if (voice == (bristolVoice *) NULL)
		return(NULL);

/*printf("findVoice(%i, %i, %i) [%x]\n", voice->index,key,chan,voice->baudio); */

	if ((voice->key.key == key)
		&& (voice->baudio != (Baudio *) NULL)
		&& ((voice->baudio->midichannel == chan)
			|| (voice->baudio->midichannel == BRISTOL_CHAN_OMNI)))
		return(voice);

	return(findVoice(voice->next, key, chan));
}

Baudio *
findBristolAudioByChan(Baudio *baudio, int chan)
{
#ifdef DEBUG
	printf("findBristolAudioByChan(%x, %i)\n", baudio, chan);
#endif

	if (baudio == (Baudio *) NULL)
		return(NULL);

	if ((baudio->midichannel == chan)
		|| (baudio->midichannel == BRISTOL_CHAN_OMNI))
		return(baudio);

	return(findBristolAudioByChan(baudio->next, chan));
}

/*
 * This would probably be a lot faster as a while() loop.
 */
Baudio *
findBristolAudio(Baudio *baudio, int id, int channel)
{
#ifdef DEBUG
	printf("findBristolAudio(%x, %i, %i)\n", baudio, id, channel);
#endif

	if (baudio == (Baudio *) NULL)
		return(NULL);

/*	if ((baudio->controlid == id) */
/*		&& (baudio->midichannel == channel)) */

	if (baudio->sid == id)
		return(baudio);

	return(findBristolAudio(baudio->next, id, channel));
}

int
midiPolyPressure(audioMain *audiomain, bristolMidiMsg *msg)
{
	bristolVoice *voice;

#ifdef DEBUG
	printf("midiPolyPressure(%i, %i)\n",
		msg->params.pressure.key, msg->params.pressure.pressure);
#endif

	voice = audiomain->playlist;

	while (voice != NULL)
	{
		if ((voice->baudio != NULL)
			&& ((voice->baudio->midichannel == msg->channel)
				|| (voice->baudio->midichannel == BRISTOL_CHAN_OMNI))
			&& (voice->keyid == msg->params.pressure.key))
		{
			if (voice->baudio->midiflags & BRISTOL_MIDI_DEBUG1)
				printf("midiPolyPressure(%i, %i)\n",
					msg->params.pressure.key, msg->params.pressure.pressure);
			if (voice->baudio->midiflags & BRISTOL_MIDI_DEBUG2)
				bristolMidiPrint(msg);
			voice->pressure.pressure = msg->params.pressure.pressure;
			voice->press = ((float) msg->params.pressure.pressure) / 127;
		}

		voice = voice->next;
	}

	return(0);
}

int
midiControl(audioMain *audiomain, bristolMidiMsg *msg)
{
	int c_id;
	float c_val;
	Baudio *baudio = audiomain->audiolist;

	c_id = msg->params.controller.c_id;
	c_val = (float) msg->params.controller.c_val;

#ifdef DEBUG
	printf("midiControl(%i, %f)\n", c_id, c_val);
#endif

	if ((c_id >= MIDI_ALL_SOUNDS_OFF) && (c_id <= MIDI_POLY_OFF))
	{
		/*
		 * For now just turn off ALL notes. This should arguably be done on
		 * the specified channel, but we are effectively only one multitimbral
		 * synth so the choice is ours - going to do it per channel.
		 */
		allNotesOff(audiomain, msg->channel);
		return(0);
	}

	if ((c_id >= MIDI_CONTROLLER_COUNT) || (c_id < 0))
		return(0);

	while ((baudio = findBristolAudioByChan(baudio, msg->channel)) != NULL)
	{
		if (baudio->midiflags & BRISTOL_MIDI_DEBUG1)
			bristolMidiPrint(msg);

		/*
		 * This is only half the problem. We need to check if this is a few
		 * selected controllers and may have to do some special processing.
		 * The first part of the 'special' processing is to hash in the ones
		 * that have 'coarse/fine' values.
		 *
		 * From the GM-2 specification we need the following:

		Finally, a GM module should also respond to the following MIDI
		controller messages: Modulation (1) (usually hard-wired to control LFO
		amount, ie, vibrato), Channel Volume (7), Pan (10), Expression (11),
		Sustain (64), Reset All Controllers (121), and All Notes Off (123).
		Additionally, the module should respond to these Registered Parameter
		Numbers <../tech/midispec/rpn.htm>: Pitch Wheel Bend Range (0), Fine
		Tuning (1), and Coarse Tuning (2).

		 * To date we have:
		 *
		 *	Modulation
		 *	Sustain
		 *	AllNotesOff
		 *
		 * Volume and Pan may be reserved for the eventual mixing function?
		 *
		 * Most notably is the RP, NRP and DataEntry controllers. I think I am
		 * not going to concern myself too much with RP, or perhaps just have
		 * my own NRP that will reflect them? I think we should also implement
		 * an callback routine, with an option to override it with a call from
		 * the emulation if it want copies of the controllers. We still put the
		 * values into our contcontroller structure even if we have a callback.
		 */
		bristolMidiToGM2(baudio->GM2values, baudio->midimap, baudio->valuemap,
			msg);

		if ((c_id >= 0) && (c_id < MIDI_CONTROLLER_COUNT))
//			baudio->contcontroller[c_id] = ((float) c_val) / 127;
			baudio->contcontroller[c_id] = ((float) msg->params.controller.c_val) / 127.0;

		if (baudio->midiflags & BRISTOL_MIDI_DEBUG1)
			bristolMidiPrintGM2(msg);

		/*
		 * If this is an NRP we need to call the synth as it is probably a 
		 * parameter change. We should have two types of NRP, those dedicated
		 * to the emulation, and those dedicated to bristol general use. The
		 * emulation parameters are just another access method to that used by
		 * the GUI with SLab SYSEX. The general use are for RP like functions
		 * such as detune and gain that are not emulation specific.
		if (baudio->midi != NULL)
			baudio->midi(baudio,
				msg->params.controller.c_id,
				msg->params.controller.c_val);
obxController(Baudio *baudio, u_char operator, u_char controller, float value)
		 */

		/*
		 * If this is a data entry value then send it to the emulation so that
		 * it can be intepreted into one of the emulated controls. We will add
		 * in some method such that continuous controllers can be promoted to
		 * NRP allowing CC to drive the emulation as well.
		 */
		if (msg->params.controller.c_id == 38)
		{
			if (msg->GM2.c_id == MIDI_GM_RP)
			{
				if (msg->GM2.coarse == BRISTOL_NRP_NULL)
					return(0);

/*
printf("	RP codes %i/%i/%i: %i, %f: %i %i\n",
c_id,
msg->GM2.c_id,
msg->GM2.coarse,
msg->GM2.fine,
msg->GM2.value,
msg->GM2.intvalue,
(int) (msg->GM2.value * 16384.0f));
*/

				/* This needs some global registered callback for the synth. */
				if (baudio->midi != NULL)
					baudio->midi(baudio, msg->GM2.coarse, msg->GM2.value);
			} else if (msg->GM2.c_id == MIDI_GM_NRP) {
				c_id = (baudio->GM2values[99] << 7) +
					baudio->GM2values[98];

/*
printf("	NRP codes %i/%i/%i: %i, %f: %i %i\n",
c_id,
msg->GM2.c_id,
msg->GM2.coarse,
msg->GM2.fine,
msg->GM2.value,
msg->GM2.intvalue,
(int) (msg->GM2.value * 16384.0f));
*/

				if (msg->GM2.coarse < MIDI_NRP_PW) {
					/*
					 * These have to be enabled
					 */
					if (~baudio->midiflags & BRISTOL_MIDI_NRP_ENABLE)
					{
						baudio = baudio->next;
						continue;
					}

					baudio->param(baudio,
						msg->GM2.coarse >> 7, msg->GM2.coarse & 0x7f,
							msg->GM2.value);
				} else
					/*
					 * These are bristol RP - they don't have an enable flag
					 * since they only recognise a specific set of requests,
					 * see bristolMidiController() code in audiothread.c
					 */ 
					if (baudio->midi != NULL)
						baudio->midi(baudio, msg->GM2.coarse, msg->GM2.value);
			}
		}

		baudio = baudio->next;
	}

	if ((c_id == BRISTOL_CC_HOLD1) & (c_val == 0))
	{
		/*
		 * We need to clear any sustained keys.
		 */
		sustainedNotesOff(audiomain, msg->channel);
	}

	return(0);
}

int
midiProgram(audioMain *audiomain, bristolMidiMsg *msg)
{
	/*
	 * Ah, the engine receives this message, probably from rawmidi. What to do
	 * next is not selfevident: the engine has no memories - these are
	 * all handled by the GUI. These (and other) messages should be passed
	 * through to the GUI, or the GUI should register for a midi pipe as well
	 * and do the other necessary things. I think I actually prefer the latter,
	 * since having a passthrough here would only be one special case.
	 *
	 * 0.40.6 should have bidirectional passthrough, removed the message as it
	 * is superfluous.
	printf("MIDI Program changes are in the GUI\n");
	 */

	return(0);
}

int
midiChannelPressure(audioMain *audiomain, bristolMidiMsg *msg)
{
	bristolVoice *voice = audiomain->playlist;
	Baudio *baudio = audiomain->audiolist;

#ifdef DEBUG
	printf("midiChannelPressure(%i)\n", msg->params.channelpress.pressure);
#endif

	while (baudio != NULL)
	{
		if ((baudio->midichannel == msg->channel)
			|| (baudio->midichannel == BRISTOL_CHAN_OMNI))
		{
			baudio->chanPress.pressure = msg->params.channelpress.pressure;
			baudio->chanpressure = ((float) baudio->chanPress.pressure)/127.0f;

			if (baudio->midiflags & BRISTOL_MIDI_DEBUG1)
				printf("midiChannelPressure(%i, %i)\n",
					msg->channel,
					msg->params.channelpress.pressure);
			if (baudio->midiflags & BRISTOL_MIDI_DEBUG2)
				bristolMidiPrint(msg);
		}

		baudio = baudio->next;
	}

	while (voice != NULL)
	{
		if ((voice->baudio != NULL)
			&& ((voice->baudio->midichannel == msg->channel)
				|| (baudio->midichannel == BRISTOL_CHAN_OMNI)))
			voice->chanpressure = voice->baudio->chanpressure;

		voice = voice->next;
	}
	return(0);
}

void
doPitchWheel(Baudio *baudio)
{
	float note = 1.0;
	int i;

	if (baudio->pitchwheel >= 0)
	{
		for (i = 0; i < baudio->midi_pitch; i++)
			note *= baudio->note_diff;

		baudio->midi_pitchfreq = 1.0 + (note - 1.0) * baudio->pitchwheel;
	} else {
		for (i = 0; i < baudio->midi_pitch; i++)
			note /= baudio->note_diff;

		baudio->midi_pitchfreq = 1.0 - (note - 1.0) * baudio->pitchwheel;
	}
}

/*
 * Correctly speaking we should calculate all the interpolative values. We can
 * do this by having a parameter for the spread of the pitchwheel, and taking
 * that octave fraction for any given value. Hm... FFS.
 */
int
midiPitchWheel(audioMain *audiomain, bristolMidiMsg *msg)
{
	float pitch;
	Baudio *baudio = audiomain->audiolist;

	pitch = (float) (msg->params.pitch.lsb + (msg->params.pitch.msb << 7));
/*	
	if (pitch > 0)
		pitch += 128;

 	baudio->pitchwheel = (pitch + 8192) / CONTROLLER_RANGE;
*/

	while ((baudio = findBristolAudioByChan(baudio, msg->channel)) != NULL)
	{
		baudio->pitchwheel = (pitch / C_RANGE_MIN_1 - 0.5) * 2;

		if (baudio->midiflags & BRISTOL_MIDI_DEBUG2)
			bristolMidiPrint(msg);

		if (baudio->gtune == 0)
			baudio->gtune = 1.0;

		/*
		 * Put it in a range of -1 to 1.
		baudio->midi_pitchfreq = pitch / 8192;
		 */

#ifdef DEBUG
		printf("midiPitchWheel(%f) %p %i\n", pitch, baudio, msg->channel);
#endif

		doPitchWheel(baudio);

		buildCurrentTable(baudio, baudio->midi_pitchfreq * baudio->gtune);
		alterAllNotes(baudio);

		baudio = baudio->next;
	}

	return(0);
}

static int midiSystem(audioMain *audiomain, bristolMidiMsg *msg)
{
	float adjusted;
	Baudio *baudio;

	adjusted = ((float) (msg->params.bristol.valueLSB
		+ (msg->params.bristol.valueMSB << 7))) / (CONTROLLER_RANGE - 1);

	if (adjusted > 1.0)
		adjusted = 1.0;
	else if (adjusted < 0)
		adjusted = 0;

	/*
	 * First take a peek to see if this is a "SLab" message.
	 */
	if ((strncmp((const char *) &msg->params.bristol.SysID, "SLab", 4) == 0)
		|| ((msg->params.bristol.SysID == ((audiomain->SysID >> 24)))
			&& (msg->params.bristol.L == ((audiomain->SysID >> 16)&0x00ff))
			&& (msg->params.bristol.a == ((audiomain->SysID >> 8)&0x00ff))
			&& (msg->params.bristol.b == ((audiomain->SysID & 0x00ff)))))
	{
/*		bristolOPParams *params; */

		/*
		 * If so we can look for index (should be zero for now) operator,
		 * controller and value.
		 */
#ifdef DEBUG
		printf("	SLab[%i] %i, %i, %i/%i, %f\n",
			msg->params.bristol.channel,
			msg->params.bristol.operator,
			msg->params.bristol.controller,
			msg->params.bristol.valueMSB,
			msg->params.bristol.valueLSB,
			adjusted);
#endif

		if (msg->params.bristol.operator == BRISTOL_SYSTEM)
		{
			bristolSystem(audiomain, msg);
			return(0);
		}

		if (msg->params.bristol.operator == BRISTOL_ARPEGGIATOR)
		{
			bristolArpeggiator(audiomain, msg);
			return(0);
		}

		/*
		 * LADI is primarily handled in the MIDI thread as it needs to forward
		 * the requests to any attached GUI.
		 */
		if (msg->params.bristol.operator == BRISTOL_LADI)
		{
			switch (msg->params.bristol.controller) {
				case BRISTOL_LADI_SAVE_REQ:
					if (audiomain->debuglevel > 1)
						printf("ladi relooped save request %x/%x\n",
							msg->params.bristol.valueMSB,
							msg->params.bristol.valueLSB);
					return(0);
				case BRISTOL_LADI_LOAD_REQ:
					if (audiomain->debuglevel > 1)
						printf("ladi relooped load request %x/%x\n",
							msg->params.bristol.valueMSB,
							msg->params.bristol.valueLSB);
					return(0);
			}
			return(0);
		}

		/*
		 * Insert some code to check the BRISTOL_ACTIVE_SENSE here. These are
		 * sent from the GUI and if they are found then we will check them 
		 * every few seconds to make sure the GUI has not exit.
		 */
		if (msg->params.bristol.operator == BRISTOL_ACTIVE_SENSE)
		{
			bristolActiveSense(audiomain, msg);
			return(0);
		}

		if (audiomain->atStatus == -1)
			/*
			 * If we do not have an active audiothread, then we will only
			 * accept messages on operator 127 - the bristol system control
			 * operator channel
			 */
			return(0);

		/*
		 * Bristol Sysex messages do contain midi channel information, but 
		 * since we are doing system operation we need to make sure we take the
		 * single bristolAudio associated with this config interface, ie, we
		 * need to consider where the message came from, and look for its ID.
		 * The rest of the midi messages search for midichannel, and may get
		 * several hits when we are working with multitimbral configurations.
		 */
		baudio = findBristolAudio(audiomain->audiolist,
			msg->params.bristol.channel, 0);

		if (baudio == (Baudio *) NULL)
			return(0);

		if (baudio->midiflags & BRISTOL_MIDI_DEBUG1)
			printf("bSYSEX %i %i: %f\n",
				msg->params.bristol.operator,
				msg->params.bristol.controller,
				adjusted);

		/* These are for type 2 messages, not yet to the engine, only from */
		if (msg->params.bristol.msgType > 7)
		{
			if (baudio->midiflags & BRISTOL_MIDI_DEBUG1)
				printf("engine does not handle bristol type2 messages yet\n");
			return(0);
		}

		if (msg->params.bristol.operator < baudio->soundCount) {
			/*
			 * Find out if this is a float val or what? Alternatively, find
			 * out which algo this is, and call the "param()" routing
			 * associated with it - let it sort out its parameter range!
			 *
			 * These are called with operator, parameter and local:
			 */
			audiomain->palette[baudio->sound[
				msg->params.bristol.operator]->index]->param(
					audiomain->palette[
					baudio->sound[msg->params.bristol.operator]->index],
					baudio->sound[msg->params.bristol.operator]->param,
					msg->params.bristol.controller,
					(float) adjusted);
		} else {
			/*
			 * Pass the event on to any global controller registered by
			 * this bristolSound
			 */
			if (baudio->param != NULL)
				baudio->param(baudio, msg->params.bristol.operator,
					msg->params.bristol.controller, adjusted);
		}
	}
	return(0);
}

/*
 * We have a default table which consists of the correct note steps for a 
 * tuned synth. We also have a global tuning, and this applies alterations into
 * the "currentTable". This table is used to direct the frequencies applied to
 * all the oscillators.
 */
int
buildCurrentTable(Baudio *baudio, float gtune)
{
	register int i;

	if (gtune > 10)
		return(0);

	if (gtune == 0)
	{
		if (baudio->gtune == 0)
			return(0);
		gtune = baudio->gtune;
	}

	for (i = 0; i < DEF_TAB_SIZE; i++) {
		baudio->ctab[i].step = defaultTable[i].defnote * gtune;
		baudio->ctab[i].freq = defaultTable[i].rate * gtune;
	}

	/*
	 * We should also consider retuning any active notes at this point, but
	 * since this code should not be concerned with the actual synthesis 
	 * algorithm, it is left to the caller to make such changes. See the code
	 * for the audioEngine, where this routine is called.
	 */

	return(0);
}

/*
 * Calculate a default frequency table for the 127 MIDI keys.
 *
 * This is the rate at which we should step through the 1024 samples of wave
 * table.
 *
 * Taken from the table:
 *
 * A  220.000 57 44=110 32=55 20=27.5 8=13.75
 * A# 233.082 58
 * B  246.942 59
 * C  261.626 60 = Midi Key, Middle C 
 * C# 277.183 61
 * D  293.665 62
 * D# 311.127 63
 * E  329.626 64
 * F  349.228 65
 * F# 369.994 66
 * G  391.995 67
 * G# 415.305 68
 *
 * We can work on putting full calculation into here for other frequency 
 * tables. For correct operation, each semitone is
 *	previous frequency * (2^^(1/12))
 * Since A is constand whole numbers we can calcuate each octave from A.
 */
float samplerate;

int
initFrequencyTable(float rate)
{
	int i;
	float gain_diff, accum = 1.0;

	samplerate = rate;

	/*
	 * For any given frequency, we need given number of cycles per second, and
	 * this is equal to rate/f samples per cycle. We have 1024 samples, so we
	 * can use 1024/(rate/f) to get step rates.
	 */
	defaultTable[57].rate = 220.000;
	defaultTable[58].rate = 233.082;
	defaultTable[59].rate = 246.942;
	defaultTable[60].rate = 261.626;
	defaultTable[61].rate = 277.183;
	defaultTable[62].rate = 293.665;
	defaultTable[63].rate = 311.127;
	defaultTable[64].rate = 329.626;
	defaultTable[65].rate = 349.228;
	defaultTable[66].rate = 369.994;
	defaultTable[67].rate = 391.995;
	defaultTable[68].rate = 415.305;

	defaultTable[57].defnote = ((float) 1024.0)/(rate / defaultTable[57].rate);
	defaultTable[58].defnote = ((float) 1024.0)/(rate / defaultTable[58].rate);
	defaultTable[59].defnote = ((float) 1024.0)/(rate / defaultTable[59].rate);
	defaultTable[60].defnote = ((float) 1024.0)/(rate / defaultTable[60].rate);
	defaultTable[61].defnote = ((float) 1024.0)/(rate / defaultTable[61].rate);
	defaultTable[62].defnote = ((float) 1024.0)/(rate / defaultTable[62].rate);
	defaultTable[63].defnote = ((float) 1024.0)/(rate / defaultTable[63].rate);
	defaultTable[64].defnote = ((float) 1024.0)/(rate / defaultTable[64].rate);
	defaultTable[65].defnote = ((float) 1024.0)/(rate / defaultTable[65].rate);
	defaultTable[66].defnote = ((float) 1024.0)/(rate / defaultTable[66].rate);
	defaultTable[67].defnote = ((float) 1024.0)/(rate / defaultTable[67].rate);
	defaultTable[68].defnote = ((float) 1024.0)/(rate / defaultTable[68].rate);

	/*
	 * The lower keys
	 */
	for (i = 56; i >= 0; i--)
	{
		defaultTable[i].rate = defaultTable[i + 12].rate / 2;
		defaultTable[i].defnote = defaultTable[i + 12].defnote / 2;
	}

	/*
	 * The higher keys
	 */
	for (i = 69; i < DEF_TAB_SIZE; i++)
	{
		defaultTable[i].rate = defaultTable[i - 12].rate * 2;
		defaultTable[i].defnote = defaultTable[i - 12].defnote * 2;
	}

	/*
	 * We now have to build in a gain table, which will be logarithmic,
	 * and a multipication list for attack/decay rates.
	 */
	gain_diff = pow((double) 13, ((double) 1)/DEF_TAB_SIZE);

	for (i = 1; i <= DEF_TAB_SIZE; i++)
	{
		defaultTable[i - 1].gain = (accum *= gain_diff);
	}
	gain_diff = pow((double) CONTROLLER_RANGE, ((double) 1)/CONTROLLER_RANGE);
	accum = 1.0;
	gainTable[0].gain = (accum *= gain_diff) / CONTROLLER_RANGE;
	for (i = 1; i < CONTROLLER_RANGE; i++)
	{
		gainTable[i].gain = (accum *= gain_diff) / CONTROLLER_RANGE -
			gainTable[0].gain;
	}
	gainTable[0].gain = gainTable[1].gain;
	gainTable[CONTROLLER_RANGE - 1].gain = 
		gainTable[CONTROLLER_RANGE - 2].gain;

	gain_diff = pow((double) 2, ((double) 1)/CONTROLLER_RANGE);
	accum = 1.0;
	for (i = 1; i < CONTROLLER_RANGE; i++)
		gainTable[i].rate = powf(12.0,
			1.0/(((float) i) * BRISTOL_RAMP_RATE * rate / ((float) C_RANGE_MIN_1)));
	/* power does not work well for certain values..... */
	gainTable[0].rate = gainTable[1].rate;

	return(0);
}

/*
 * This takes a table of floats that represent frequencies and turns them into
 * our internal table steps.
 */
void
initMicrotonalTable(float table[])
{
	int i;

	if (samplerate == 0)
		samplerate = 44100; /* If still zero set a default */

	/*
	 * For any given frequency, we need given number of cycles per second, and
	 * this is equal to rate/f samples per cycle. We have 1024 samples, so we
	 * can use 1024/(rate/f) to get step rates.
	 */
	if (table[0] <= 0)
		return;

	for (i = 0; i < 128; i++)
		table[i] = ((float) 1024.0) / (samplerate / table[i]);
}

#warning add in initMidiControllerFreqMap
void
initMidiControllerMap(int index, int *map, float *floatmap)
{
	int i, c, f;
	char param[64];
	float tmpmap[128];

	tmpmap[0] = 0.00001;
	tmpmap[127] = 127.0;

	/* Build a keymap, watch out for failed mappings */
	if ((c = bristolGetMap("midicontrollermap",
		&eventNames[index][0], tmpmap, 128, 0)) <= 0)
		for (i = 0; i < 128; i++)
			tmpmap[i] = i;

	for (i = 0; i < 128; i++)
		map[i] = (int) tmpmap[i];

	sprintf(param, "%sFloat", &eventNames[index][0]);
	if (((f = bristolGetMap("midicontrollermap", param, floatmap, 128, 0)) <= 0)
		&& (c > 0))
	{
		for (i = 0; i < 128; i++)
			floatmap[i] = (float) map[i];
	}
}

void
initMidiRoutines(audioMain *audiomain, midiHandler midiRoutines[])
{
	int i;

#ifdef DEBUG
	printf("initMidiRoutines()\n");
#endif

	if (samplerate == 0)
	{
		printf("Fixing samplerate at %i\n", audiomain->samplerate);
		if ((samplerate = audiomain->samplerate) == 0)
			samplerate = audiomain->samplerate = 44100; /* Set default */
	}

	midiRoutines[0].callback = midiNoteOff;
	midiRoutines[1].callback = midiNoteOn;
	midiRoutines[2].callback = midiPolyPressure;
	midiRoutines[3].callback = midiControl;
	midiRoutines[4].callback = midiProgram;
	midiRoutines[5].callback = midiChannelPressure;
	midiRoutines[6].callback = midiPitchWheel;
	midiRoutines[7].callback = midiSystem;

	/*
	 * We are going to initialise all the maps to linear, they get remapped 
	 * later if the midicontrollermap file is found. Not all of these are
	 * used: NoteOn/Off, PolyPressure and ChannelPressure are used here, the
	 * rest are rather superfluous.
	 *
	 * These maps are applied first, before any dispatch is done, and will 
	 * affect all emulations.
	 *
	 * Continuous controllers are managed separately in the GUI, they are not
	 * an engine feature. We might want to make an exception for ModWheel CC_1?
	 */
	for (i = 0; i < 8; i++)
	{
		initMidiControllerMap(i, &midiRoutines[i].map[0],
			&midiRoutines[i].floatmap[0]);
	}

	if (bristolGetMap("midicontrollermap", "midiNoteMap",
		&midiRoutines[7].floatmap[0], 128, 0) <= 0)
		for (i = 0; i < 128; i++)
			midiRoutines[7].floatmap[i] = i;

	for (i = 0; i < 128; i++)
	{
		midiRoutines[7].map[i] = (int) midiRoutines[7].floatmap[i];
		midiRoutines[7].floatmap[i] = -1.0;
	}

	/*
	 * It would be better to have this burried in the baudio so it runs on a
	 * single emulation or midi channel, that is for later study.
	 */
	if (audiomain->microTonalMappingFile != NULL)
	{
		float notes[128];

		int n = bristolParseScala(audiomain->microTonalMappingFile, notes);

		if (n > 0)
		{
			int i, j = 0, k = 440;

			--n;

			/*
			 * This was just for my purposesi
			for (i = 0; i <= n; i++)
				printf("Note: %f\n", notes[i]);
			if (notes[n] != 2.0)
				printf("Scale is not based on octaves\n");
			 */

			/*
			 * We need to do some mapping to the keyboard and these are not
			 * necessarily obvious. If we have 12 notes then map them to the
			 * keyboard sequentially basing the notes around A=440 = 69 on the
			 * midi keyboard. For 20 notes do similar up and down.
			 *
			 * For 7 and 8 notes then just use the whites? 5 Notes just the
			 * blacks? No, leave this until we have implemented MIDI key
			 * dumps and let somebody else decide what mappings they want.
			 *
			 * So, we have N notes that we are going to assume span an octave
			 * so lets go place them onto sequential notes. This could be a
			 * bit more intelligent so perhaps we should consider also having
			 * an implementation of the Scala .kbm file as well?
			 */
			for (i = 70; i < 128; i++)
			{
				midiRoutines[7].floatmap[i] = k * notes[j];
/* printf("i %i, j %i, k %i: %f\n", i, j, k, midiRoutines[7].floatmap[i]); */
				if (++j > n)
				{
					k *= notes[n]; 
					j = 0;
				}
			}
			k = 440 / notes[n];
			j = n;
			for (i = 69; i >= 0; i--)
			{
				midiRoutines[7].floatmap[i] = k * notes[j];
/* printf("i %i, j %i, k %i: %f\n", i, j, k, midiRoutines[7].floatmap[i]); */
				if (--j < 0)
				{
					j = n;
					k /= notes[n]; 
				}
			}
		}
	} else
		bristolGetMap("midicontrollermap", "midiMicroTonalMap",
			&midiRoutines[7].floatmap[0], 128, 0);

	/*
	 * The microTonal mapping is a list of frequencies (there may end up being
	 * more options. Either way, these need to be converted into wave table
	 * steps.
	 */
	initMicrotonalTable(&midiRoutines[7].floatmap[0]);

	initFrequencyTable((float) audiomain->samplerate);
}

