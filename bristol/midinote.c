
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

/* #define DEBUG */

#include <stdlib.h>
#include <math.h>
#include "bristol.h"

extern bristolMidiHandler bristolMidiRoutines;

int rbMidiNoteOn(audioMain *, bristolMidiMsg *);
int rbMidiNoteOff(audioMain *, bristolMidiMsg *);

/*
 * Monophonic voice logic
 */
static int
doMNL(bristolVoice *voice, int key, int velocity, int action, int offset)
{
	Baudio *baudio = voice->baudio;
	int mn = -1;

offset = 99;
	if (voice->baudio == NULL)
		return(0);

	if (voice->baudio->midiflags & BRISTOL_MIDI_DEBUG1)
		printf("monophonic note logic (%i %i)\n", key, action);

	if (action == BRISTOL_ALL_NOTES_OFF)
	{
		/*
		 * Clear out our table
		 */
		for (mn = 0; mn < 128; mn++)
			baudio->notemap.key[mn] = 0;
		baudio->notemap.count = 0;
		voice->flags &= ~BRISTOL_KEYSUSTAIN;
		return(0);
	}

	if ((voice->baudio->notemap.flags & (BRISTOL_MNL_LNP|BRISTOL_MNL_HNP)) == 0)
		return(-1);

	/* We never use this, the key actually stays on all the time anyway */
	voice->flags &= ~BRISTOL_KEYSUSTAIN;

	baudio->notemap.key[key] = action * key;
	baudio->notemap.velocity[key] = velocity;

	baudio->notemap.low = 127;
	baudio->notemap.high = 0;
	baudio->notemap.count = 0;

	for (mn = 0; mn < 128; mn++)
	{
		if (baudio->notemap.key[mn] != 0)
		{
			baudio->notemap.count++;
			if (mn < baudio->notemap.low) baudio->notemap.low = mn;
			if (mn > baudio->notemap.high) baudio->notemap.high = mn;
		}
	}

	if (baudio->notemap.flags & BRISTOL_MNL_LNP)
	{
		if ((baudio->notemap.extreme <= 0)
			|| (baudio->notemap.extreme < baudio->notemap.high))
			baudio->notemap.extreme = baudio->notemap.high;

		if ((baudio->notemap.count == 0)
			&& (baudio->contcontroller[BRISTOL_CC_HOLD1] < 0.5))
		{
			voice->flags |= BRISTOL_KEYOFF;
			return(-1);
		}

		if (key > baudio->notemap.low)
		{
			/*
			 * There is an issue here: If I get a note on then a note off in 
			 * the same sample period then the first note correctly sets the
			 * keyon flags, the second one incorrectly clears them. Now I am 
			 * in the audio thread here so there is nothing to signal to the
			 * audio process to find out if it has seen my flags yet.
			 * The question is, why would I clear these flags on note off that
			 * is not in my key range?
			voice->flags &= ~(BRISTOL_KEYON|BRISTOL_KEYREON);
			 */
			return(-1);
		}

		mn = baudio->notemap.low;
	} else {
		if ((baudio->notemap.extreme <= 0)
			|| (baudio->notemap.extreme > baudio->notemap.low))
			baudio->notemap.extreme = baudio->notemap.low;

		if ((baudio->notemap.count == 0)
			&& (baudio->contcontroller[BRISTOL_CC_HOLD1] < 0.5))
		{
			voice->flags |= BRISTOL_KEYOFF;
			return(-1);
		}

		if (key < baudio->notemap.high)
		{
			/*
			 * See above notes - this key should not do anything, at all
			voice->flags &= ~(BRISTOL_KEYON|BRISTOL_KEYREON);
			*/
			return(-1);
		}

		mn = baudio->notemap.high;
	}

	if (baudio->notemap.count == 0)
		mn = baudio->notemap.extreme;

	if (voice->baudio->midiflags & BRISTOL_MIDI_DEBUG2)
		printf("(%i) mn %i, key %i\n", baudio->notemap.count, mn, key);

	mn += baudio->transpose;

	while (mn > 127)
		mn -= 12;
	while (mn < 0)
		mn += 12;

	voice->lastkey = voice->key.key;
	voice->keyid = key;
	voice->key.key = mn;

	if (baudio->microtonalmap[mn].step > 0.0) {
		voice->dFreq = baudio->microtonalmap[mn].step;
		voice->dfreq = baudio->microtonalmap[mn].freq;
	} else if (bristolMidiRoutines.bmr[7].floatmap[mn] > 0.0) {
		/*
		 * This is the synth private microtonal map (eg, scala file)
		voice->dFreq = bristolMidiRoutines.freq[mn].step;
		voice->dfreq = bristolMidiRoutines.freq[mn].freq;
		 */
		voice->dFreq = bristolMidiRoutines.bmr[7].floatmap[mn];
		voice->dfreq = baudio->samplerate * voice->dFreq / ((float) 1024.0);
	} else {
		voice->dFreq = baudio->ctab[mn].step;
		voice->dfreq = baudio->ctab[mn].freq;
	}

	if (baudio->glide == 0)
	{
		voice->cFreq = voice->dFreq;
		voice->cfreq = voice->dfreq;
		voice->cFreqmult = voice->cfreqmult = 0;
	} else {
		voice->cFreqmult = powf(M_E, logf(voice->dFreq/voice->cFreq)
			/ (baudio->glide * baudio->samplerate));
		voice->cfreqmult = powf(M_E, logf(voice->dfreq/voice->cfreq)
			/ (baudio->glide * baudio->samplerate));
	}

	if (action == 1) {
		/*
		 * See if this was the first voice in which case we need to flag 'on'
		 * rather than a restrike. Also configure the velocity if we are set to
		 * do legato velocity.
		 */
		if (baudio->notemap.count == 1)
		{
			/* Legato velocity, only configure it on the first keystroke */
			if (baudio->notemap.flags & BRISTOL_MNL_VELOC)
			{
				voice->key.velocity = velocity;
				voice->velocity = baudio->velocitymap[velocity];
			}

			voice->flags |= BRISTOL_KEYON|BRISTOL_KEYREON;

			/* Pro-1 does not glide on first note, if selected */
			if (baudio->mixflags & BRISTOL_GLIDE_AUTO)
			{
				voice->cFreq = voice->dFreq;
				voice->cfreq = voice->dfreq;
				voice->cFreqmult = voice->cfreqmult = 0;
			}
		} else {
			if (baudio->notemap.flags & BRISTOL_MNL_TRIG)
				voice->flags |= BRISTOL_KEYREON;

			/*
			 * This means legato notes will follow each note velocity. Not sure
			 * if that is really desirable, it might be better to have just the
			 * first note define velocity?
			 */
			if ((baudio->notemap.flags & BRISTOL_MNL_VELOC) == 0)
			{
				voice->key.velocity = velocity;
				voice->velocity = baudio->velocitymap[velocity];
			}
		}
		voice->flags &= ~(BRISTOL_KEYOFF|BRISTOL_KEYOFFING|BRISTOL_KEYDONE);
	} else if (baudio->notemap.flags & BRISTOL_MNL_TRIG) {
		/* If we don't have legato velocity we apply velocity here */
		if ((baudio->notemap.flags & BRISTOL_MNL_VELOC) == 0)
		{
			voice->key.velocity = baudio->notemap.velocity[mn];
			voice->velocity = baudio->velocitymap[voice->key.velocity];
		}
		/* Flag note off events as a gate for envelope retrigger */
		voice->flags |= BRISTOL_KEYREON;
	}

	if ((offset >= baudio->samplecount) || (offset < 0))
		voice->offset = 0;
	else
		voice->offset = offset;

	return(mn);
}

void
printPlayList(audioMain *audiomain)
{
	bristolVoice *v = audiomain->playlist;

	printf("printPlayList(%p, %p)\n", audiomain->playlist, audiomain->playlast);

	while (v != NULL)
	{
		printf("%p<-%p->%p: %i, %i, 0x%x (%p)\n",
			v->last, v, v->next, v->index, v->key.key, v->flags, v->baudio);

		v = v->next;
	}

	if ((v = audiomain->newlist) != NULL)
	{
		printf("printNewlist(%p)\n", v);
		while (v != NULL)
		{
			printf("%p<-%p->%p: %i, %i, 0x%x\n",
				v->last,
				v,
				v->next,
				v->index,
				v->key.key,
				v->flags
			);

			v = v->next;
		}
	}
}

/*
 * There will be a lot of processing involved with MIDI note on/off management,
 * so it has been pulled into a separate file.
 */
int
midiNoteOff(audioMain *audiomain, bristolMidiMsg *msg)
{

	/*
printf("midiNoteOff %i\n", msg->params.key.key);
	 * If we get a note off, then find that note, and configure it for done. If
	 * we have sustain active then just set the BRISTOL_SUSTAIN flag, the key
	 * will be released when the sustain pedal is released.
	 *
	 * We need to take the long semaphore here to prevent the engine from
	 * doing anything we don't want it to with the playlist or newlist. Since
	 * we may be playing with newlist we minimally need the short semaphore, and
	 * if we want to take things from the newlist to the freelist then we need
	 * the long semaphore too. Hence we just request the long one as it covers
	 * both needs.
	 */
#ifdef BRISTOL_SEMAPHORE
	bristolVoice *voice, *v;
	sem_wait(audiomain->sem_long);
#else
	if ((msg->params.key.flags & BRISTOL_KF_JACK)
		&& (~audiomain->flags & BRISTOL_JACK_DUAL))
	{
		/*
		 * The message came from Jack and we have selected a single registration
		 * which means we are actually in the audio thread and can conginue with
		 * the note on event.
		 */
	    if ((audiomain->debuglevel & BRISTOL_DEBUG_MASK) > BRISTOL_DEBUG2)
			printf("direct off call\n");
		return(rbMidiNoteOff(audiomain, msg));
	} if (jack_ringbuffer_write_space(audiomain->rb) >= sizeof(bristolMidiMsg))
		jack_ringbuffer_write(audiomain->rb, (char *) msg,
			sizeof(bristolMidiMsg));
	else
		printf("ringbuffer exhausted, note off event dropped\n");

	return(0);
}

int
rbMidiNoteOff(audioMain *audiomain, bristolMidiMsg *msg)
{
	bristolVoice *voice, *v;
#endif

msg->offset = 101;
	voice = audiomain->playlist;

	while (voice != NULL)
	{
		if ((voice->baudio == NULL)
			|| ((voice->baudio->mixflags & (BRISTOL_HOLDDOWN|BRISTOL_REMOVE)))
			|| (voice->baudio->mixflags & BRISTOL_KEYHOLD))
		{
			voice = voice->next;
			continue;
		}

		/*
		 * Check monophonic note logic. If we have monophonic note logic
		 * selected then see if there is another note currently on, then
		 * rather than remove this note from the list just modify its
		 * parameters.
		 */
		if ((voice->baudio->voicecount == 1)
			&& ((voice->baudio->midichannel == msg->channel)
				|| (voice->baudio->midichannel == BRISTOL_CHAN_OMNI))
			&& (voice->baudio->notemap.flags & (BRISTOL_MNL_LNP|BRISTOL_MNL_HNP)))
		{
			if (voice->baudio->midiflags & BRISTOL_MIDI_DEBUG1)
				bristolMidiPrint(msg);
			doMNL(voice, msg->params.key.key, -1, 0, msg->offset);
			voice = voice->next;
			continue;
		}

		if ((voice->keyid == msg->params.key.key)
			&& ((voice->baudio->midichannel == msg->channel)
				|| (voice->baudio->midichannel == BRISTOL_CHAN_OMNI)))
		{
			/* can't print the message until we know if there is debug on */
			if (voice->baudio->midiflags & BRISTOL_MIDI_DEBUG1)
				bristolMidiPrint(msg);

			if ((voice->baudio->contcontroller[BRISTOL_CC_HOLD1] > 0.5)
				&& (voice->baudio->voicecount > 1))
				voice->flags |= BRISTOL_KEYSUSTAIN;
			else {
				/*
				 * Do not put the note off velocity in the key velocity, it can
				 * cause pops and clicks if it used to control the gain.
				 * It should be seen as a separate control parameter.
				 *
				 * Took the next line out. It is basically a damaged part of
				 * the specification - if the note off is sent with default
				 * velocity of 64, or for that matter any other velocity, it
				 * can cause jumps in the envelope tracking and any other 
				 * code using velocity. We should merge it into another 
				 * location.
				voice->velocity =
					voice->baudio->velocitymap[msg->params.key.velocity];
				 */
				voice->keyoff.velocity = msg->params.key.velocity;
				voice->flags |= BRISTOL_KEYOFF;

				bristolArpeggiatorNoteEvent(voice->baudio, msg);
			}
		}

		voice = voice->next;
	}

	/*
	 * Scan the newlist to make sure this was not a spurious short event,
	 * prevents us from having notes sticking.
	 */
	voice = audiomain->newlist;
	while (voice != NULL)
	{
		if ((voice->baudio == NULL)
			|| (voice->baudio->mixflags & BRISTOL_KEYHOLD))
		{
			voice = voice->next;
			continue;
		}

		if ((voice->keyid == msg->params.key.key)
/*				|| (voice->baudio->voicecount == 1)) This caused Mono issues */
			&& ((voice->baudio->midichannel == msg->channel)
				|| (voice->baudio->midichannel == BRISTOL_CHAN_OMNI)))
		{
			/*
			 * Just take it off the newlist and put on the freelist. This was
			 * damaged for a while as it assumed we were looking at the head
			 * of the newlist and it was not doubly linked.
			 *
			 * I think this causes clicks due to forced close of the envelope.
			 * Changed the logic from remove from list to KEYOFF
			 */
			voice->flags &= ~(BRISTOL_KEYON|BRISTOL_KEYREON);

			if (voice->baudio->voicecount > 1)
			{
				v = voice->next;

				if (voice->next != NULL)
					voice->next->last = voice->last;
				else
					audiomain->newlast = voice->last;
				if (voice->last != NULL)
					voice->last->next = voice->next;
				else
					audiomain->newlist = voice->next;

				if ((voice->next = audiomain->freelist) == NULL)
					audiomain->freelast = voice;
				else
					voice->next->last = voice;
				voice->last = NULL;
				audiomain->freelist = voice;

				if (voice->baudio->midiflags & BRISTOL_MIDI_DEBUG2)
					printf("mt: off: removed %p/%i from newlist\n",
						voice, voice->keyid);

				voice = v;
				continue;
			} else {
				voice->flags |=
					BRISTOL_KEYOFF|BRISTOL_KEYSUSTAIN|BRISTOL_KEYREOFF;
				if (voice->baudio->midiflags & BRISTOL_MIDI_DEBUG2)
					printf("mt: off: %p->%p/%i was on newlist\n",
						voice, voice->next, voice->keyid);
			}
		}

		voice = voice->next;
	}

#ifdef BRISTOL_SEMAPHORE
	sem_post(audiomain->sem_long);
#endif

//printf("sl: ");printPlayList(audiomain);

	return(0);
}

static int arbitrary = 0x12365302;
static int nx1 = 0x67452301;
static int nx2 = 0xefcdab89;

/*
 * Find a free voice, or take one off the current playlist.
 *
 * Need to converge in the correct locals list before we can assign a voice, 
 * and find an associated baudio structure. Consideration will have to be made
 * for the ability to do layering.
 */
int
doMidiNoteOn(audioMain *audiomain, bristolMidiMsg *msg, Baudio *baudio, int key)
{
	bristolVoice *voice, *last = NULL, *donevoice = NULL;
	int lastkey, transposedkey, velocity, offset;
	float cFreq, dFreq, dTune = 0.0, cFreqmult = 0, mappedvelocity;
	float cfreq, dfreq, cfreqmult;

	/*
	 *	locals
	 *	Keyid
	 *	Transposed key id, check for bounds
	 *	Velocity and mapped velocity
	 *	dFreq/dfreq from frequency maps
	 *	dTune
	 *	cFreqmult
	 */
	if (baudio->midiflags & BRISTOL_MIDI_DEBUG1)
	{
		printf("doMidiNoteOn()\n");
		bristolMidiPrint(msg);
	}

	if (baudio->midiflags & BRISTOL_MIDI_DEBUG2)
		printPlayList(audiomain);

	/*
	 * We may have to do some foldback however steps of 12 would only work for
	 * western scales so there may be scala file issues. This only happens at
	 * the edges though.
	 */
	if ((transposedkey = msg->params.key.key + baudio->transpose) > 127)
		while (transposedkey > 127)
			transposedkey -= 12;

	while (transposedkey < 0)
		transposedkey += 12;

	if ((offset = msg->offset) >= audiomain->samplecount)
		offset = 0;

	/*
	 * Get some operational values ready for the voice structures.
	 */
	velocity = msg->params.key.velocity;
	mappedvelocity = baudio->velocitymap[msg->params.key.velocity];

	/*
	 * Stack up the last key structure for portamento. Glide starts at the
	 * current glide of the last played note. This will fail in multitimbral
	 * mode, since next key may be for a different voice!
	 * Also need to cater for single voice synths.
	 *
	 * These need to come from baudio.
	 */
	lastkey = baudio->lastkey;
	baudio->lastkey = transposedkey;

	/*
	 * Have been requested to implement this feature as a part of Unison/Mono
	 * mode of the diverse poly instruments. This should be possible with an
	 * OR operation on the UNISON flag however this is currently handled in the
	 * calling routine, not here.
	 */
	if ((baudio->voicecount == 1) && (baudio->lvoices != 0)
		&& ((voice = audiomain->playlist) != NULL)
		&& (baudio->notemap.flags & (BRISTOL_MNL_LNP|BRISTOL_MNL_HNP)))
	{
		while (voice->baudio != baudio)
			if ((voice = voice->next) == NULL)
				return(0);

		doMNL(voice, key, velocity, 1, msg->offset);
		return(0);
	}

	/* This needs to be stuff the first time around only really */
	baudio->notemap.key[key] = key;
	baudio->notemap.velocity[key] = msg->params.key.velocity;
	baudio->notemap.count++;

	/*
	 * This is the main point at which voice note settings take place. We are
	 * going to introduce a random detune per voice that will vary each time 
	 * a note is played within a reasonably small (configurable) range. This
	 * will be used to emulate temperature sensitivity of the old synths.
	 * It should be configurable per synth so perhaps we will make it a MIDI
	 * unassigned opererator? The engine should respond to this operator, the
	 * GUI will sent it, and any other arbitrary control may send it to the 
	 * engine as well.
	 */
	if (baudio->microtonalmap[transposedkey].step > 0.0) {
		/*
		 * This is the synth private microtonal map. This should really 
		 * contain frequencies rather than the internal adjusted values?
		 */
		dFreq = baudio->microtonalmap[transposedkey].step;
		dfreq = baudio->microtonalmap[transposedkey].freq;
	} else if (bristolMidiRoutines.bmr[7].floatmap[transposedkey] > 0.0) {
		/*
		 * This is the synth global microtonal map
		dFreq = bristolMidiRoutines.freq[transposedkey].step;
		dfreq = bristolMidiRoutines.freq[transposedkey].freq;
		 */
		dfreq = bristolMidiRoutines.bmr[7].floatmap[transposedkey];
		dFreq = ((float) 1024.0) * dfreq / baudio->samplerate;
	} else {
		/*
		 * Otherwise this is the default ET internal map 'currentTable'.
		 */
		dFreq = baudio->ctab[transposedkey].step;
		dfreq = baudio->ctab[transposedkey].freq;
	}

	if (baudio->detune != 0)
	{
		/*
		 * We take a random number. If it is < RAND_MAX/2 then it will be used
		 * to reduce the frequency by a fraction of the dTune, otherwise it will
		 * increase the frequency
		arbitrary = rand();
		 *
		 * This is a noise generator and is cheaper than rand?
		 */
		nx1 ^= nx2;
		arbitrary += nx2;
		nx2 += nx1;

		if (arbitrary > (RAND_MAX>>1))
		{
			dTune = 1.0 + baudio->detune
				* ((float) arbitrary) / ((float) RAND_MAX);
		} else {
			dTune = 1.0 - baudio->detune
				* ((float) arbitrary) / ((float) RAND_MAX);
		}

		dFreq *= dTune;
		dfreq *= dTune;

/*printf("tempering key by %1.4f, was %1.4f, is %1.4f\n", */
/*dTune, voice->baudio->ctab[transposedkey].step, voice->dFreq); */
	}
/* printf("		detune %f\n", baudio->detune); */

	if ((baudio->voicecount == 1) || (audiomain->playlist == NULL)) {
		cFreq = baudio->ctab[lastkey].step;
		cfreq = baudio->ctab[lastkey].freq;
	} else {
		cFreq = audiomain->playlist->cFreq;
		cfreq = audiomain->playlist->cfreq;
	}

	/*
	 * This is the Pro-1 Glide option:
	 *	If this is the first key and we are Auto then skip the glide.
	 *
	 */
	if ((baudio->glide == 0) ||
		((baudio->lvoices == 0) && (baudio->mixflags & BRISTOL_GLIDE_AUTO)))
	{
		cFreq = dFreq;
		cfreq = dfreq;
		cFreqmult = cfreqmult = 0;
	} else {
		/*
		 * cFreqstep was an incrementer to the cFreq float, which gave constant
		 * glide times but it is also arguably incorrect as it should be a
		 * multiplier that is applied a given number of times - that way the
		 * glide is also constant per octave, not the case with the original
		 * linear incrementor:
		 */
		cFreqmult = powf(M_E, logf(dFreq/cFreq)
			/ (baudio->glide * audiomain->samplerate));
		cfreqmult = powf(M_E, logf(dfreq/cfreq)
			/ (baudio->glide * audiomain->samplerate));
	}

	/*
	 * We have evaluated all the parameters we want for this voice:
	 *
	 *	baudio
	 *	locals
	 *	Lastkey for portamento
	 *	Keyid
	 *	Transposed key id, check for bounds
	 *	Velocity and mapped velocity
	 *	dFreq from frequency maps
	 *	dTune
	 *	cFreqmult
	 *
	 * See how many voices were last running on this emulator.
	 *
	 * 	If under the voice limit and freelist is not empty
	 *		Request semaphore-1: the short semaphore
	 *			Unlink the head of the freelist, relink that list
	 *			Stuff the pre-evaluated parameters
	 *			Link to tail of the newlist
	 *		Relinquish semaphore-1
	 *		Return;
	 *
	 *	Request semaphore-2: the long semaphore over the Poly process
	 *		take last playlist entry for this emulator or bump the last entry
	 *		Stuff the pre-evaluated parameters
	 *		Unlink the selected voice and rejoin the list it was taken from.
	 *		Link to tail of the newlist
	 *	Relinquish semaphore-2
	 *
	 * Another case we may need to look at is whether this note is already
	 * audible and then take case #2.
	 */
	if ((baudio->lvoices < baudio->voicecount) && (audiomain->freelist != NULL))
	{
		/*
		 * Stuff the new information into the head of the freelist, then wait
		 * for the semaphore to put it on the newlist.
		 */
		voice = audiomain->freelist;
		voice->baudio = baudio;
		voice->flags &= ~(BRISTOL_KEYON|BRISTOL_KEYREON);
		voice->offset = offset;
		voice->locals = baudio->locals;
		voice->keyid = key;
		voice->key.key = transposedkey;
		voice->key.velocity = velocity;
		voice->velocity = mappedvelocity;
		voice->lastkey = lastkey;
		voice->dFreq = dFreq;
		voice->cFreq = cFreq;
		voice->cFreqmult = cFreqmult;
		voice->dfreq = dfreq;
		voice->cfreq = cfreq;
		voice->cfreqmult = cfreqmult;
		voice->detune = dTune;

#ifdef BRISTOL_SEMAPHORE
		sem_wait(audiomain->sem_short);
#endif

		/*
		 * Short piece of critical code
		 *
		 * Relink the head of the freelist (which may now be empty)
		 */
		if ((audiomain->freelist = voice->next) != NULL)
			voice->next->last = NULL;
		else
			audiomain->freelast = NULL;

		/*
		 * Link voice into the newlist
		 */
		if ((voice->next = audiomain->newlist) != NULL)
			voice->next->last = voice;
		else
			audiomain->newlast = voice;
		audiomain->newlist = voice;
		voice->last = NULL;

//printf("ss: ");printPlayList(audiomain);

#ifdef BRISTOL_SEMAPHORE
		sem_post(audiomain->sem_short);
#endif

		return(0);
	}

#ifdef BRISTOL_SEMAPHORE
	sem_wait(audiomain->sem_long);
#endif

	/*
	 * We have the semaphore, search for the voice. There are several cases
	 * that we should check for: single voice, last voice, matching key/baudio.
	 */
	voice = audiomain->playlist;
	while (voice != NULL)
	{
		if (voice->baudio == NULL) {
			voice = voice->next;
			continue;
		}

		/*
		 * Then see if it's also my key. If it is my key break, otherwise take
		 * this as the 'last' entry and continue. We do not need to check the
		 * channel as that has already been done.
		 */
		if (voice->baudio == baudio)
		{
			/*
			 * So, it's my baudio but not the same note, keep it as possible
			 * 'last' key to be stolen later.
			 */
			last = voice;

			/*
			 * If we only have a single voice available and this is it, use it
			 */
			if (baudio->voicecount == 1)
			{
				voice->flags &= ~(BRISTOL_KEYON|BRISTOL_KEYDONE|BRISTOL_KEYOFF);
				//|BRISTOL_KEYOFFING);
				voice->flags |= BRISTOL_KEYREON;

				voice->keyid = key;
				voice->offset = offset;
				voice->key.key = transposedkey;
				voice->key.velocity = velocity;
				voice->velocity = mappedvelocity;
				voice->lastkey = lastkey;
				voice->dFreq = dFreq;
				voice->cFreq = cFreq;
				voice->cFreqmult = cFreqmult;
				voice->dfreq = dfreq;
				voice->cfreq = cfreq;
				voice->cfreqmult = cfreqmult;
				voice->detune = dTune;

#ifdef BRISTOL_SEMAPHORE
				sem_post(audiomain->sem_long);
#endif
				return(0);
			}

			if (voice->keyid == msg->params.key.key)
				break;
		}


		if (voice->flags & (BRISTOL_DONE|BRISTOL_KEYOFFING))
			donevoice = voice;

		voice = voice->next;
	}

	/*
	 * If we did not find a voice see if we found a 'last' voice, if not just
	 * take the last playlist entry
	 */
	if (voice == NULL) {
		if (donevoice != NULL) // Take a voice that is going off
			voice = donevoice;
		else if (last != NULL) // Take the last voice for this synth
			voice = last;
		else // Steal the oldest note
			voice = audiomain->playlast;
	}

	if (voice != NULL)
	{
		/*
		 * Unlink this voice, refill it, put it on the head of the newlist.
		 */
		if (voice->next != NULL)
			voice->next->last = voice->last;
		else
			audiomain->playlast = voice->last;

		if  (voice->last != NULL)
			voice->last->next = voice->next;
		else
			audiomain->playlist = voice->next;

		if ((voice->next = audiomain->newlist) != NULL)
			voice->next->last = voice;
		else
			audiomain->newlast = voice;
		audiomain->newlist = voice;
		voice->last = NULL;

		voice->flags &= ~(BRISTOL_KEYON|BRISTOL_KEYREON);
		if (voice->baudio == baudio)
			voice->flags |= BRISTOL_KEYREON;

		voice->keyid = key;
		voice->offset = offset;
		voice->key.key = transposedkey;
		voice->key.velocity = velocity;
		voice->velocity = mappedvelocity;
		voice->lastkey = lastkey;
		voice->dFreq = dFreq;
		voice->cFreq = cFreq;
		voice->cFreqmult = cFreqmult;
		voice->dfreq = dfreq;
		voice->cfreq = cfreq;
		voice->cfreqmult = cfreqmult;
		voice->detune = dTune;
	} else
		if (baudio->midiflags & BRISTOL_MIDI_DEBUG2)
			printf("voicecount exceeded\n");

#ifdef BRISTOL_SEMAPHORE
	sem_post(audiomain->sem_long);
#endif

//printf("sl: ");printPlayList(audiomain);

	return(0);
}

int
midiNoteOn(audioMain *audiomain, bristolMidiMsg *msg)
{
#ifdef BRISTOL_SEMAPHORE
	Baudio *baudio = audiomain->audiolist;
#else

	if (msg->params.key.velocity == 0)
	{
		msg->command = MIDI_NOTE_OFF|(msg->command & MIDI_CHAN_MASK);
		msg->params.key.velocity = 64;
		return(midiNoteOff(audiomain, msg));
	}

	/*
	 * So, if this came from a jack handle, and we are single open then don't
	 * dump this in the ringbuffer - call the handler directly
	 *
	 * The JDO flags is straightforward. We then need to check if 'from' is 
	 * a jack handle.
	 */
	if ((msg->params.key.flags & BRISTOL_KF_JACK)
		&& (~audiomain->flags & BRISTOL_JACK_DUAL))
	{
		/*
		 * The message came from Jack and we have selected a single registration
		 * which means we are actually in the audio thread and can conginue with
		 * the note on event.
		 */
	    if ((audiomain->debuglevel & BRISTOL_DEBUG_MASK) > BRISTOL_DEBUG2)
			printf("direct on call\n");
		return(rbMidiNoteOn(audiomain, msg));
	} if (jack_ringbuffer_write_space(audiomain->rb) >= sizeof(bristolMidiMsg))
		jack_ringbuffer_write(audiomain->rb, (char *) msg,
			sizeof(bristolMidiMsg));
	else
		printf("ringbuffer exhausted, note on event dropped\n");

	return(0);
}

int
rbMidiNoteOn(audioMain *audiomain, bristolMidiMsg *msg)
{
	Baudio *baudio = audiomain->audiolist;
#endif
	/*
	 * Hm, if we have already applied a velocity curve then this value that
	 * was zero can now be something else, and that would lead to unexpected
	 * results
	 */
	if (msg->params.key.velocity == 0)
	{
		msg->command = MIDI_NOTE_OFF|(msg->command & MIDI_CHAN_MASK);
		msg->params.key.velocity = 64;
#ifndef BRISTOL_SEMAPHORE
		return(rbMidiNoteOff(audiomain, msg));
#endif
	}

	/*
	 * Find an associated baudio structure. We have to go through the
	 * baudio lists, and find the correct midi channel. Link up the locals
	 * list.
	 */
	while (baudio != (Baudio *) NULL)
	{
		if (baudio->mixflags & (BRISTOL_HOLDDOWN|BRISTOL_REMOVE))
		{
			baudio = baudio->next;
			continue;
		}

		if (((baudio->midichannel == msg->channel)
				|| (baudio->midichannel == BRISTOL_CHAN_OMNI))
			&& (baudio->lowkey <= msg->params.key.key)
			&& (baudio->highkey >= msg->params.key.key))
		{
			int voices = 1;
			int key = msg->params.key.key;
			int chordindex = 0;

			/*
			 * If we have Unison then we will send the same note on event to
			 * every voice on this emulation. If we have Chord enabled then we
			 * will send a different key number to each of the voices.
			 */
			if (baudio->mixflags & (BRISTOL_V_UNISON|BRISTOL_CHORD))
				voices = baudio->voicecount;

			bristolArpeggiatorNoteEvent(baudio, msg);

			/*
			 * We could avoid this, since we have the correct baudio 
			 * pointer.
			 */
			for (;voices > 0; voices--)
			{
				if (baudio->mixflags & BRISTOL_CHORD)
				{
					/*
					 * There is no key bounds check here, that happens when the
					 * note gets allocated a voice later, as a part of the
					 * transpose operation.
					 */
					msg->params.key.key
						= key + baudio->arpeggio.c.notes[chordindex].k;

					/*
					 * We could consider two modes here:
					 *
					 *	Roll around the notes list and assign something to
					 *	every available voice.
					 *
					 *	Break such that we just have one voice assigned per
					 *	arpeggiator note (assuming we have enough voices).
					 *
					 * I prefer the latter, we can build chord of 4 notes, for
					 * example, and still have polyphony of these chords.
					 * 
					 * This should actually be a call into the arpeggiator.c
					 * code, it would be a lot cleaner. Overhead is not great
					 * as its only for note on events. There is no reason why
					 * the arpeggiator should not actually call the shots here
					 * and request the doMidiNoteOn() back to this code.
						printf("chord: %x %i %i\n",
							(size_t) baudio,
							baudio->arpeggio.c.max,
							baudio->arpeggio.c.notes[chordindex - 1].k);
					 */
					if (++chordindex > baudio->arpeggio.c.max)
						break;
				}
				doMidiNoteOn(audiomain, msg, baudio, key);
			}
		}

		baudio = baudio->next;
	}
	return(0);
}

void
sustainedNotesOff(audioMain *audiomain, int channel)
{
	bristolVoice *voice = audiomain->playlist;

	while (voice != NULL) 
	{
		if ((channel == -1)
			|| ((voice->baudio != NULL)
				&& (voice->flags & BRISTOL_KEYSUSTAIN)
				&& ((voice->baudio->midichannel == channel)
				|| (voice->baudio->midichannel == BRISTOL_CHAN_OMNI))))
			voice->flags |= BRISTOL_KEYOFF;
		voice = voice->next;
	}

	if ((voice != NULL) && (voice->baudio->midiflags & BRISTOL_MIDI_DEBUG2))
		printf("midi sustained notes off\n");
}

void
allNotesOff(audioMain *audiomain, int channel)
{
	bristolVoice *voice = audiomain->playlist;

	while (voice != NULL) 
	{
		if ((channel == -1)
			|| ((voice->baudio != NULL)
				&& ((voice->baudio->midichannel == channel)
					|| (voice->baudio->midichannel == BRISTOL_CHAN_OMNI))))
		{
			voice->flags = BRISTOL_KEYOFF|BRISTOL_KEYDONE;
			//voice->flags &= ~(BRISTOL_KEYSUSTAIN;
		}

		doMNL(voice, 0, 0, BRISTOL_ALL_NOTES_OFF, 0);
		voice = voice->next;
	}

	if ((voice != NULL) && (voice->baudio->midiflags & BRISTOL_MIDI_DEBUG2))
		printf("midi all notes off\n");
}

#ifndef BRISTOL_SEMAPHORE
void
rbMidiNote(audioMain *audiomain, bristolMidiMsg *msg)
{
	if ((audiomain->debuglevel & BRISTOL_DEBUG_MASK) > BRISTOL_DEBUG2)
		printf("rbMidiNote(%x) %x\n", msg->command, msg->params.key.flags);

	if ((msg->command & MIDI_COMMAND_MASK) == MIDI_NOTE_ON)
		rbMidiNoteOn(audiomain, msg);
	else if ((msg->command & MIDI_COMMAND_MASK) == MIDI_NOTE_OFF)
		rbMidiNoteOff(audiomain, msg);
}
#endif

