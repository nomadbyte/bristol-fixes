
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

#include <math.h>
#include <stdlib.h>

#include "bristol.h"
#include "bristolarpeggiation.h"

extern bristolMidiHandler bristolMidiRoutines;

/* This is required for 2602222, voices left hanging when terminating */
static void desequence(audioMain *am, Baudio *ba)
{
	bristolVoice *voice = am->playlist;

	while (voice != NULL) {
		if (voice->baudio == ba)
			voice->flags |= BRISTOL_KEYDONE;

		voice = voice->next;
	}
}

/*
 * This is sent arpeggiator messages via controller BRISTOL_ARPEGGIATOR (125)
 * and does the configuration of the arpeggiator.
 *
 * The code supports sequencing where steps can be learnt from the keyboard
 * and then stepped through them sequencially, chording where a similar learn
 * process is executed and then each note can play the chord singularly, and
 * arpeggiation where we scan through the active (pressed) notes.
 */
void
bristolArpeggiator(audioMain *audiomain, bristolMidiMsg *msg)
{
	float value;
	int tval;
	Baudio *baudio = findBristolAudio(audiomain->audiolist,
		msg->params.bristol.channel, 0);

	if (baudio == NULL)
		return;

	tval = (msg->params.bristol.valueMSB << 7) + msg->params.bristol.valueLSB;
	value = ((float) tval) / CONTROLLER_RANGE;

	/*
	 * Arpeggio parameters. There are 4, on/off, direction, range, rate. I am
	 * going to use a 5th for envelope retriggering. These could probably be
	 * moved into the baudio structure for general use?
	 */
	switch (msg->params.bristol.controller) {
		case BRISTOL_ARPEG_ENABLE:
			/* on/off */
			if (value != 0) 
			{
				int i;

				if (baudio->arpeggio.a.span == 0)
					baudio->arpeggio.a.span = 1;

				/*
				 * Clear out the known key list
				 */
				if ((baudio->mixflags & BRISTOL_ARPEGGIATE) == 0)
					for (i = 0; i < 127; i++)
						baudio->arpeggio.a.notes[i].k
							= baudio->arpeggio.a.notes[i].v = -1;

				baudio->arpeggio.a.current = 0;

				/* Put in some exclusion */
				baudio->mixflags &= ~BRISTOL_SEQUENCE;
				baudio->mixflags |= BRISTOL_ARPEGGIATE;
			} else {
				/*
				 * This is a little bit abrupt: arpeggiation is stopped but
				 * since the arpeggiated/sequenced keys are still active they
				 * all sound at once.
				 * MARK: 2602222
				 */
				desequence(audiomain, baudio);
				baudio->mixflags &= ~BRISTOL_ARPEGGIATE;
			}
			break;
		case BRISTOL_ARPEG_DIR:
			/* direction up/down/up+down/rnd */
			switch (tval) {
				case BRISTOL_ARPEG_UP:
				default:
					baudio->arpeggio.a.dir = BRISTOL_ARPEG_UP;
					baudio->arpeggio.a.step = 1;
					baudio->arpeggio.a.count = baudio->arpeggio.a.rate;
					baudio->arpeggio.a.current = 0;
					break;
				case BRISTOL_ARPEG_DOWN:
					baudio->arpeggio.a.dir = BRISTOL_ARPEG_DOWN;
					baudio->arpeggio.a.step = -1;
					baudio->arpeggio.a.count = baudio->arpeggio.a.rate;
					baudio->arpeggio.a.current = 0;
					break;
				case BRISTOL_ARPEG_UD:
					baudio->arpeggio.a.dir = BRISTOL_ARPEG_UD;
					baudio->arpeggio.a.step = 1;
					baudio->arpeggio.a.count = baudio->arpeggio.a.rate;
					baudio->arpeggio.a.current = 0;
					break;
				case BRISTOL_ARPEG_RND:
					baudio->arpeggio.a.dir = BRISTOL_ARPEG_RND;
					baudio->arpeggio.a.step = 1;
					baudio->arpeggio.a.count = baudio->arpeggio.a.rate;
					baudio->arpeggio.a.current = 0;
					break;
			}
			break;
		case BRISTOL_ARPEG_RANGE:
			/* range in octaves */
			baudio->arpeggio.a.span = tval + 1;
			break;
		case BRISTOL_ARPEG_CLOCK:
			if (value != 0)
				baudio->arpeggio.flags |= BRISTOL_A_CLOCK;
			else
				baudio->arpeggio.flags &= ~BRISTOL_A_CLOCK;
			break;
		case BRISTOL_ARPEG_RATE:
			/* rate */
			baudio->arpeggio.a.rate = 2000 + (1.0 - value) *
				audiomain->samplerate;
			if (baudio->arpeggio.a.rate < baudio->arpeggio.a.count)
				baudio->arpeggio.a.count = baudio->arpeggio.a.rate;
			break;
		case BRISTOL_ARPEG_TRIGGER:
			/* retrigger */
			if (value != 0)
				baudio->arpeggio.flags |= BRISTOL_A_TRIGGER;
			else
				baudio->arpeggio.flags &= ~BRISTOL_A_TRIGGER;
			break;
		case BRISTOL_ARPEG_POLY_2:
			if (value != 0) {
				baudio->arpeggio.flags |= BRISTOL_POLY_2;
				baudio->mixflags &= ~BRISTOL_SEQUENCE;
				baudio->mixflags |= BRISTOL_V_UNISON|BRISTOL_ARPEGGIATE;
			} else {
				baudio->arpeggio.flags &= ~BRISTOL_POLY_2;
				baudio->mixflags &= ~(BRISTOL_V_UNISON|BRISTOL_ARPEGGIATE);
			}
			break;

		case BRISTOL_SEQ_DIR:
			/* direction up/down/up+down/rnd */
			switch (tval) {
				case BRISTOL_ARPEG_UP:
				default:
					baudio->arpeggio.s.dir = BRISTOL_ARPEG_UP;
					baudio->arpeggio.s.step = 1;
					baudio->arpeggio.s.count = baudio->arpeggio.s.rate;
					baudio->arpeggio.s.current = 0;
					break;
				case BRISTOL_ARPEG_DOWN:
					baudio->arpeggio.s.dir = BRISTOL_ARPEG_DOWN;
					baudio->arpeggio.s.step = -1;
					baudio->arpeggio.s.count = baudio->arpeggio.s.rate;
					baudio->arpeggio.s.current = 0;
					break;
				case BRISTOL_ARPEG_UD:
					baudio->arpeggio.s.dir = BRISTOL_ARPEG_UD;
					baudio->arpeggio.s.step = 1;
					baudio->arpeggio.s.count = baudio->arpeggio.s.rate;
					baudio->arpeggio.s.current = 0;
					break;
				case BRISTOL_ARPEG_RND:
					baudio->arpeggio.s.dir = BRISTOL_ARPEG_RND;
					baudio->arpeggio.s.step = 1;
					baudio->arpeggio.s.count = baudio->arpeggio.s.rate;
					baudio->arpeggio.s.current = 0;
					break;
			}
			break;
		case BRISTOL_SEQ_RANGE:
			/* range in octaves */
			baudio->arpeggio.s.span = tval + 1;
			break;
		case BRISTOL_SEQ_CLOCK:
			if (value != 0)
				baudio->arpeggio.flags |= BRISTOL_S_CLOCK;
			else
				baudio->arpeggio.flags &= ~BRISTOL_S_CLOCK;
			break;
		case BRISTOL_SEQ_RATE:
			/* rate */
//			tval = gainTable[C_RANGE_MIN_1 - tval].gain * CONTROLLER_RANGE;
//			baudio->arpeggio.s.rate = baudio->arpeggio.s.count = tval * 5;
			baudio->arpeggio.s.rate = 2000 + (1.0 - value) *
				audiomain->samplerate;
			if (baudio->arpeggio.s.rate < baudio->arpeggio.s.count)
				baudio->arpeggio.s.count = baudio->arpeggio.s.rate;
			break;
		case BRISTOL_SEQ_TRIGGER:
			/* retrigger */
			if (value != 0)
				baudio->arpeggio.flags |= BRISTOL_S_TRIGGER;
			else
				baudio->arpeggio.flags &= ~BRISTOL_S_TRIGGER;
			break;

		case BRISTOL_SEQ_ENABLE:
			/* on/off */
			if (value != 0) {
				baudio->mixflags &= ~BRISTOL_ARPEGGIATE;
				baudio->arpeggio.flags &= ~BRISTOL_SEQ_LEARN;
				if (baudio->arpeggio.s.span == 0)
					baudio->arpeggio.s.span = 1;

				baudio->mixflags |= BRISTOL_SEQUENCE;
			} else {
				/*
				 * This is a little bit abrupt: arpeggiation is stopped but
				 * since the arpeggiated/sequenced keys are still active they
				 * all sound at once.
				 * MARK: 2602222
				 */
				desequence(audiomain, baudio);
				baudio->mixflags &= ~BRISTOL_SEQUENCE;
			}
			break;
		case BRISTOL_SEQ_RESEQ:
			/*
			 * We should reset the MAX count and start to learn new sequence.
			 *
			 * To prevent any damage we should consider either stopping the
			 * arpeggiator or putting in some silly long step time?
			 *
			 * When we have requested a resequence then we start accepting
			 * midi note events in our hook and also key requests from the GUI.
			 * At the moment key requests do not carry velocity, that is for
			 * future study since we don't even use it here, we just maintain
			 * it in the engine. Velocity support in the engine would require
			 * some changes in key event dispatching in the GUI too since it
			 * does the non-volatile memory and it only receives key id at the
			 * moment.
			 */
			if (tval == 0)
			{
				if (baudio->arpeggio.s.max == 0)
					baudio->arpeggio.s.max = BRISTOL_SEQ_MIN;
				baudio->arpeggio.flags &= ~BRISTOL_SEQ_LEARN;
				return;
			} else {
				baudio->arpeggio.flags |= BRISTOL_SEQ_LEARN;
				baudio->mixflags &= ~BRISTOL_SEQUENCE;
				baudio->arpeggio.s.current = 0;
				baudio->arpeggio.s.max = 0;
			}
			break;
		case BRISTOL_SEQ_KEY:
			/*
			 * This should contain another key to put into the next location
			 * in our step list. There is a hook from midinote.c to get this
			 * from a master keyboard - NoteEvent() below.
			 *
			 * So, find the key and put it into the current MAX position then
			 * move MAX forward. This will be used to reprogram the arpeggiator
			 * from the GUI memories.
			 */
			if (baudio->arpeggio.flags & BRISTOL_SEQ_LEARN) {
				if (baudio->arpeggio.s.max == 0)
					baudio->arpeggio.s.dif = tval;
				baudio->arpeggio.s.notes[baudio->arpeggio.s.max].k
					= tval - baudio->arpeggio.s.dif;
				baudio->arpeggio.s.notes[baudio->arpeggio.s.max++].v = 120;
				if (baudio->arpeggio.s.max > BRISTOL_SEQ_MAX)
					baudio->arpeggio.s.max = BRISTOL_SEQ_MAX;
//printf("insert %i\n", tval - baudio->arpeggio.s.dif);
			}
			break;
		case BRISTOL_CHORD_ENABLE:
			/* on/off */
			if (value != 0)
				baudio->mixflags |= BRISTOL_CHORD;
			else
				baudio->mixflags &= ~BRISTOL_CHORD;
			break;
		case BRISTOL_CHORD_RESEQ:
			/*
			 * We should reset the MAX count and start to learn new sequence.
			 *
			 * To prevent any damage we should consider either stopping the
			 * arpeggiator or putting in some silly long step time?
			 */
			if (tval == 0)
			{
				baudio->arpeggio.flags &= ~BRISTOL_CHORD_LEARN;
				return;
			} else {
				baudio->arpeggio.flags |= BRISTOL_CHORD_LEARN;
				baudio->mixflags &= ~BRISTOL_CHORD;
				baudio->arpeggio.c.max = 0;
			}
			break;
		case BRISTOL_CHORD_KEY:
			/*
			 * This should contain another key to put into the next location
			 * in our step list. There is a hook from midinote.c to get this
			 * from a master keyboard - NoteEvent() below.
			 *
			 * So, find the key and put it into the current MAX position then
			 * move MAX forward. This will be used to reprogram the chord
			 * from the GUI memories.
			 */
			if (baudio->arpeggio.flags & BRISTOL_CHORD_LEARN) {
				baudio->arpeggio.c.notes[baudio->arpeggio.c.max].k = tval;
				baudio->arpeggio.c.notes[baudio->arpeggio.c.max].v = 120;
				if (++baudio->arpeggio.c.max > BRISTOL_CHORD_MAX)
					baudio->arpeggio.c.max = BRISTOL_CHORD_MAX;
			}
			break;
	}
}

static void
arpeggiatorRePoly2(arpSeq *seq, int voicecount)
{
	int i, max = seq->max, j = 0, vc = voicecount;

	/*
	 * We minimally want to get one voice per key in our table. If we have 
	 * more keys than voices we will flag an error, it is only going to be
	 * done with note on/off but the table should only contain entries up to
	 * voicecount. It does not break anything, the keys at the start of the
	 * list will not get voices.
	 *
	 * We need to make sure that:
	 *
	 *	All voices get allocated.
	 *	Voice do not get overallocated.
	 *	If we have more voices than keys then they are piled at the end.
	 *
	 * Also, we use an upper part of the table to allocate a full list of
	 * notes. We could better stuff the '.v' with this information though.
	 */
	for (i = 0; i < max; i++) {
		vc -= (seq->notes[i].v = vc / (max - i));
		for (;j < (voicecount - vc); j++)
			seq->notes[j + BRISTOL_ARPEG_MAX].k = seq->notes[i].k;
	}
}

/*
 * This is used to initialise the arpeggiator settings when a new note is
 * assigned. We will hijack the operation as well if we are recording a new
 * sequence.
 *
 * The key has not yet been transposed.
 *
 * This is always called for NOTE ON, irrespective of SEQUENCER status.
 */
void
bristolArpeggiatorNoteEvent(Baudio *baudio, bristolMidiMsg *msg)
{
	/*
	 * The code could be speeded up by putting the sequencer pointer into a
	 * buffer register.
	 */
	register arpeggiator *arpeggio = &baudio->arpeggio;

	if ((msg->command & MIDI_COMMAND_MASK) == MIDI_NOTE_OFF)
	{
		/*
		 * This will be used by the arpeggiator to maintain its note table.
		 *
		 * All arpeggiated note on events will go into the table, we scan 
		 * through that up or down once per configured octave. We should keep
		 * a count of the number of arpeggiated notes so that we can make the
		 * random work correctly.
		 *
		 * Typically we would only want to keep a known keylist if we are
		 * actively arpeggiating however it could be used in other cases. It
		 * can later be considered to run continuously.
		 */
		if (baudio->mixflags & BRISTOL_ARPEGGIATE) {
			int i;

			/*
			 * Key off are a little painful. We assume we lost a key from
			 * the list, we need to search for it and then move the rest of
			 * the list up.
			 */
			for (i = 0; i < 128; i++)
				if ((arpeggio->a.notes[i].k == msg->params.key.key)
					|| (arpeggio->a.notes[i].k == -1))
				{
					for (; i < 127; i++)
					{
						arpeggio->a.notes[i].k = arpeggio->a.notes[i + 1].k;
						arpeggio->a.notes[i].v = arpeggio->a.notes[i + 1].v;
						if (arpeggio->a.notes[i].k == -1)
							break;
					}
					arpeggio->a.max = i;
					break;
				}

			if (arpeggio->a.current >= i)
				arpeggio->a.current = 0;

			if (baudio->arpeggio.flags & BRISTOL_POLY_2)
				arpeggiatorRePoly2(&arpeggio->a, baudio->voicecount);

			/* Print the list for debugging * */
			printf("Note %i, total %i:", msg->params.key.key, arpeggio->a.max);
			for (i = 0; arpeggio->a.notes[i].k > 0; i++)
				printf(" %i/%i ",arpeggio->a.notes[i].k,arpeggio->a.notes[i].v);
			printf("\n");
		}
		return;
	}

	if (baudio->mixflags & BRISTOL_ARPEGGIATE) {
		int i;

		/*
		 * This next section of code produces an unordered list of which keys
		 * were pressed, or correctly stated they are ordered by time not by
		 * key id. For an arpeggiator this is arguably incorrect however it can
		 * be used in the same way as a key ordered list if so played and has
		 * more flexibility.
		 *
		 * We should make sure this list is a reasonable reflection of the
		 * baudio capabilities, ie, if we only have 4 voices assigned then we
		 * should not be letting this list grow beyond that, so if the max
		 * is already the same as our voice count then shift all the voices
		 * up and put the new one at the end.
		 */
		if (arpeggio->a.max >= baudio->voicecount) {
			for (i = 0; i < baudio->voicecount; i++)
				arpeggio->a.notes[i].k = arpeggio->a.notes[i + 1].k;
			arpeggio->a.notes[i].k = -1;
			arpeggio->a.notes[arpeggio->a.max - 1].k = msg->params.key.key;
			arpeggio->a.max = baudio->voicecount - 1;
		} else
			arpeggio->a.notes[arpeggio->a.max].k = msg->params.key.key;

		/* Terminate the list */
		arpeggio->a.notes[++arpeggio->a.max].k = -1;

		if (baudio->arpeggio.flags & BRISTOL_POLY_2)
			arpeggiatorRePoly2(&arpeggio->a, baudio->voicecount);

		/* Print the list for debugging */
		printf("Note On total: %i:", arpeggio->a.max);
		for (i = 0; arpeggio->a.notes[i].k > 0; i++)
			printf(" %i/%i ", arpeggio->a.notes[i].k, arpeggio->a.notes[i].v);
		printf("\n");
	}

	if (baudio->arpeggio.flags & BRISTOL_CHORD_LEARN) {
		if (arpeggio->c.max == 0) {
			arpeggio->c.dif = msg->params.key.key;
			arpeggio->c.vdif = msg->params.key.velocity;
		}
		arpeggio->c.notes[arpeggio->c.max].k
			= msg->params.key.key - arpeggio->c.dif;
		arpeggio->c.notes[arpeggio->c.max].v
			= msg->params.key.velocity - arpeggio->c.vdif;
		if (++arpeggio->c.max > BRISTOL_CHORD_MAX)
			arpeggio->c.max = BRISTOL_CHORD_MAX;
	}

	/*
	 * We should not really be doing both of these at the same time however
	 * from the perspective of the recording it does not have to be an issue.
	 */
	if (arpeggio->flags & BRISTOL_SEQ_LEARN) {
		if (arpeggio->s.max == 0) {
			arpeggio->s.dif = msg->params.key.key;
			arpeggio->s.vdif = msg->params.key.velocity;
		}
		arpeggio->s.notes[arpeggio->s.max].k
			= msg->params.key.key - arpeggio->s.dif;
//printf("live insert %i\n", arpeggio->s.notes[arpeggio->s.max].k);
		arpeggio->s.notes[arpeggio->s.max].v
			= msg->params.key.velocity - arpeggio->s.vdif;
		if (++arpeggio->s.max >= BRISTOL_SEQ_MAX)
			arpeggio->s.max = BRISTOL_SEQ_MAX;
	} else if (baudio->mixflags & BRISTOL_SEQUENCE) {
		if (baudio->lvoices <= 0) {
			arpeggio->s.current = 0;

			if (arpeggio->s.dir == BRISTOL_ARPEG_DOWN)
			{
				arpeggio->s.current = arpeggio->s.max;
				arpeggio->s.octave = arpeggio->s.span;
				arpeggio->s.step = -1;
			} else {
				arpeggio->s.step = 1;
				arpeggio->s.octave = 1;
			}

			arpeggio->s.count = arpeggio->s.rate;
		}
	}
}

/*
 * Take a sequence structure, roll its counters, then see what we have to
 * do with the index selections for key and octave.
 */
static int
arpeggioCounterCheck(arpSeq *seq, int samplecount)
{
	if ((seq->count -= samplecount) <= 0)
	{
		seq->count += seq->rate;

		switch (seq->dir) {
			case BRISTOL_ARPEG_UP:
			default:
				if (++seq->current >= seq->max)
				{
					seq->current = 0;
					/*
					 * See if we need to move up an octave
					 */
					if (++seq->octave > seq->span)
						seq->octave = 1;
				}
				break;
			case BRISTOL_ARPEG_DOWN:
				if (--seq->current < 0)
				{
					seq->current = seq->max - 1;
					/*
					 * See if we need to move down an octave
					 */
					if (--seq->octave <= 0)
						seq->octave = seq->span;
				}
				break;
			case BRISTOL_ARPEG_UD:
				if ((seq->current += seq->step) < 0)
				{
					/*
					 * We have gone through all the notes on the way
					 * down, see if we have got to bottom octave
					 */
					if (--seq->octave <= 0)
					{
						/*
						 * Start going back up. The s.current is moved to
						 * 1 since we have already played zero and I do not
						 * want two strikes on the first and last notes.
						 * We might tweak that idea though, it fails if we 
						 * only have a single note.
						 */
						seq->current = 0;
						seq->step = 1;
						seq->octave = 1;
					} else
						/* Continue down through another octave */
						seq->current = seq->max - 1;
				} else if (seq->current >= seq->max) {
					if (++seq->octave > seq->span)
					{
						/*
						 * Start going back down
						 */
						seq->current = seq->max - 1;
						seq->step = -1;
						seq->octave = seq->span;
					} else
						/* Continue up through this octave */
						seq->current = 0;
				}
				break;
			case BRISTOL_ARPEG_RND:
				/*
				 * We need to take a random note from a random octave
				 */
				seq->current = (rand() & 0x00ff) % seq->max;
				seq->octave = ((rand() & 0x00003) % seq->span) + 1;
				break;
		}
		/*
		 * have wound the counters, tell parent to retrigger
		 */
		return(1);
	}
	return(-1);
}

/*
 * Called by the audio engine to roll through any emulations and see if they
 * need some updates to the arpeggiation parameters.
 *
 * This should be redefined. We have a step sequencer and a chording system
 * however for performance it would be better to have a true arpeggiator:
 *	Out of the available pressed noes scan through them up/down/ud/rnd.
 *
 * This is called once per baudio per frame. Another call, ArpegReFreq() is
 * called per voice.
 */
int
bristolArpegReAudio(audioMain *audiomain, Baudio *baudio)
{
	baudio->arpeggio.flags &= ~BRISTOL_REQ_TRIGGER;

	if (baudio->mixflags & BRISTOL_ARPEGGIATE)
	{
		if (baudio->arpeggio.flags & BRISTOL_POLY_2)
			baudio->arpeggio.a.count = 8;
		else if (arpeggioCounterCheck(&baudio->arpeggio.a,
			audiomain->samplecount) >= 0)
			baudio->arpeggio.flags |= BRISTOL_REQ_TRIGGER;
		baudio->arpeggio.flags &= ~BRISTOL_DONE_FIRST;
	}

	/*
	 * If we are sequencing or arpeggiating then we need to look at their
	 * counters here. The counters are moved just once for however many voices
	 * we have but the counters are separate so we could anticipate sequencing
	 * arpeggios? Hm, perhaps not, but we can garantee exclusion elsewhere.
	 */
	if (baudio->mixflags & BRISTOL_SEQUENCE)
	{
		if (baudio->arpeggio.flags & BRISTOL_SEQ_LEARN)
			return(0);

		if (arpeggioCounterCheck(&baudio->arpeggio.s, audiomain->samplecount)
			>= 0)
			baudio->arpeggio.flags |= BRISTOL_REQ_TRIGGER;
	}

	return(0);
}

int
bristolArpegReVoice(Baudio *baudio, bristolVoice *voice, float sr)
{
	float dFreq = voice->dFreq;
	int key = voice->key.key;

	/*
	 * See if we should move on to the next sequence in the chain.
	 */
	if (baudio->mixflags & BRISTOL_ARPEGGIATE)
	{
		if (baudio->arpeggio.flags & BRISTOL_POLY_2)
		{
			/*
			 * Spread notes across all voices. This is all done with ARPEG,
			 * POLY_2 and UNISON together. The first round of code may be a
			 * bit poppy as voices will not have much consistency in the keys
			 * they are assigned, will have to see.
			 */
			if (--baudio->arpeggio.a.count >= 0)
				key = baudio->arpeggio.a.notes[BRISTOL_ARPEG_MAX
					+ baudio->arpeggio.a.count].k;
			else
				return(-1);
		} else {
			/*
			 * We are only interested in working on the first voice. This may
			 * give us an occasional pop if the first note is released, we then
			 * have to select the next available voice.
			 */
			if (baudio->arpeggio.flags & BRISTOL_DONE_FIRST)
				return(-1);

			baudio->arpeggio.flags |= BRISTOL_DONE_FIRST;
			/*
			 * We are going to use just the first voice in our list and cycle
			 * it through the frequencies of the keys held.
			 */
			if ((key = baudio->arpeggio.a.notes[baudio->arpeggio.a.current].k)
				< 0)
				return(-1);
			key += baudio->arpeggio.a.octave * 12;
		}
	} else if (baudio->mixflags & BRISTOL_SEQUENCE) {
		/*
		 * We have to track two sequence variables, the current note and
		 * then the current octave. I want the process to be similar to the
		 * arpeggiation - play the sequence once in each octave. If the octave
		 * is '1' then it is just the sequence.
		 */
		key += baudio->arpeggio.s.notes[baudio->arpeggio.s.current].k
			+ baudio->arpeggio.s.octave * 12;
	}

	while (key < 0)
		key += 12;
	while (key >= 128)
		key -= 12;

	/*
	 * This is used by arpeggiation to redefine the operating frequency of the
	 * voice. It uses the same code as in the NoteOn request other than that
	 * we have dropped the temperature detune code here.
	 */
	if (baudio->microtonalmap[key].step > 0.0f) {
		/*
		 * This is the synth private microtonal map. This should really 
		 * contain frequencies rather than the internal adjusted values?
		 */
		voice->dFreq = baudio->microtonalmap[key].step;
		voice->dfreq = baudio->microtonalmap[key].freq;
	} else if (bristolMidiRoutines.bmr[7].floatmap[key] > 0.0f) {
		/*
		 * This is the synth global microtonal map
		 */
		voice->dFreq = bristolMidiRoutines.freq[key].step;
		voice->dfreq = bristolMidiRoutines.freq[key].freq;
	} else {
		/*
		 * Otherwise this is the default ET internal map 'currentTable'.
		 */
		voice->dFreq = baudio->ctab[key].step;
		voice->dfreq = baudio->ctab[key].freq;
	}

	/*
	 * This is FFS. We need to alter the way a voice looks for glide. It
	 * is currently based on key numbers however here it needs to be based
	 * on frequencies.
	 *
	 * Now, if the destination frequency changed then we may need to request
	 * a retrigger and reorganise the glissando. The glissando algorithm should
	 * also be based on powers.....
	 */
	if (dFreq != voice->dFreq)
	{
		if (baudio->glide != 0) {
			voice->cFreqstep = (voice->dFreq - voice->cFreq)
				/ (baudio->glide * sr);
			voice->cFreqmult = powf(M_E, logf(voice->dFreq/voice->cFreq)
				/ (baudio->glide * sr));

			voice->cfreqmult = powf(M_E, logf(voice->dfreq/voice->cfreq)
				/ (baudio->glide * sr));
		} else {
			voice->cFreq = voice->dFreq;
			voice->cfreq = voice->dfreq;
		}
	}

	/*
	 * Trigger should be an arpeggiator flag
	 */
	if ((baudio->arpeggio.flags & BRISTOL_A_TRIGGER)
		&& (((voice->flags & BRISTOL_KEYOFF) == 0)
			|| (baudio->arpeggio.a.max >= 1))
		&& (baudio->arpeggio.flags & BRISTOL_REQ_TRIGGER))
		voice->flags |= BRISTOL_KEYREON;
	if ((baudio->arpeggio.flags & BRISTOL_S_TRIGGER)
		&& ((voice->flags & BRISTOL_KEYOFF) == 0)
		&& (baudio->arpeggio.flags & BRISTOL_REQ_TRIGGER))
		voice->flags |= BRISTOL_KEYREON;

	return(0);
}

/*
 * Called from bristolsystem.c when a client sends a HELLO message to start
 * an emulation using the baudio given here.
 */
void
bristolArpeggiatorInit(Baudio *baudio)
{
	int i;

	/*
	 * Clear out the arpeggiator known key list
	 */
	for (i = 0; i < 127; i++)
		baudio->arpeggio.a.notes[i].k = baudio->arpeggio.a.notes[i].v = -1;

	/*
	 * Put in some default arpeggio sequence
	 */
	baudio->arpeggio.s.notes[0].k = 0;
	baudio->arpeggio.s.notes[1].k = 4;
	baudio->arpeggio.s.notes[2].k = 7;
	baudio->arpeggio.s.notes[3].k = 12;
	baudio->arpeggio.s.notes[4].k = 16;
	baudio->arpeggio.s.notes[0].v = 120;
	baudio->arpeggio.s.notes[1].v = 120;
	baudio->arpeggio.s.notes[2].v = 120;
	baudio->arpeggio.s.notes[3].v = 120;
	baudio->arpeggio.s.notes[4].v = 120;

	baudio->arpeggio.s.dir = BRISTOL_ARPEG_UD;
	baudio->arpeggio.s.max = 4;
	baudio->arpeggio.s.span = 1;
	baudio->arpeggio.s.step = 1;
	baudio->arpeggio.s.rate = 400000;
	baudio->arpeggio.s.current = 0;

	baudio->arpeggio.flags = 0;

	baudio->arpeggio.c.notes[0].k = 0;
	baudio->arpeggio.c.notes[1].k = 4;
	baudio->arpeggio.c.notes[2].k = 7;
	baudio->arpeggio.c.notes[3].k = 12;
	baudio->arpeggio.c.notes[0].v = 120;
	baudio->arpeggio.c.notes[1].v = 120;
	baudio->arpeggio.c.notes[2].v = 120;
	baudio->arpeggio.c.notes[3].v = 120;
	baudio->arpeggio.c.max = 4;
}

