
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

#include <unistd.h>

#include <bristol.h>
#include <bristolmidiapi.h>
#include <bristolmidiapidata.h>
#include <bristolmidi.h>

extern void inthandler();
extern void initBristolAudio();
#ifdef BRISTOL_PA
extern int bristolPulseInterface();
#endif
#ifdef _BRISTOL_JACK
extern int bristolJackInterface();
#endif
extern void allNotesOff();

static int initCount = 0;

int
bristolgetsid(Baudio *baudio, int sid)
{
	int tsid;

	if (baudio)
		tsid = bristolgetsid(baudio->next, sid);
	else
		return(sid);

/*	printf("bristolgetsid(%x, %i): %i\n", baudio, sid, tsid); */

	if (baudio->sid == tsid)
		return(++tsid);

/*	printf("nomatch: %i\n", tsid); */

	return(tsid);
}

int
bristolSystem(audioMain *audiomain, bristolMidiMsg *msg)
{
	int result = 0, hclose = -1;

	/*
	*/
	if (audiomain->debuglevel > 1)
		printf("bristolSystem(%i, %i, %i) %x\n",
			msg->params.bristol.operator,
			msg->params.bristol.controller,
			msg->params.bristol.from,
			(msg->params.bristol.valueMSB << 7) + msg->params.bristol.valueLSB);

	/*
	 * We are looking for a "hello" message from the GUI, which should include
	 * an algorithm index that we shall be running. We also need to look out 
	 * for exit requests.
	 */
	if (msg->params.bristol.operator != BRISTOL_SYSTEM)
		return(0);

	if (msg->params.bristol.controller == 0)
	{
		/*
		 * These are system global messages - start/stop/hello.
		 */
		int flags = (msg->params.bristol.valueMSB << 7) +
			msg->params.bristol.valueLSB;

		/*
		 * See if we can activate message connections
		 */
		if (flags & BRISTOL_HELLO)
		{
			int x, y;
			/*
			 * When we get a hello we need to suppress midi processing for a
			 * period of time whilst the new emulator is activated. It will be
			 * reactivated when the algorithm has initted later.
			 */
			bristolMidiOption(0, BRISTOL_NRP_MIDI_GO, 0);

			Baudio *baudio = (Baudio *) bristolmalloc0(sizeof(Baudio));

			baudio->mixflags = (BRISTOL_HOLDDOWN|BRISTOL_SENSE);
			/* Turn on sensing with a long timer, GUI can disable it later */
			baudio->sensecount = 15 * audiomain->samplerate;

			if (audiomain->atStatus == BRISTOL_EXIT)
				audiomain->midiflags |= BRISTOL_HELLO;

			/*
			 * We assume that anyone attempting a hello is going to want access
			 * to a bristolAudio channel. We should allocate this here, and give
			 * it an ID for the connection. This ID is returned in an ack back
			 * to the source.
			 */
			baudio->midichannel = msg->params.bristol.channel;
			baudio->samplerate = audiomain->samplerate;
			baudio->samplecount = audiomain->samplecount;

			/*
			 * Put in some default arpeggio values
			 */
			bristolArpeggiatorInit(baudio);

			baudio->sid = result = bristolgetsid(audiomain->audiolist, 0);

			if (baudio->sid == -1)
				printf("SID failure....\n");
			else if (audiomain->debuglevel > 2)
				printf("	alloc SID %i\n", baudio->sid);

			baudio->voicecount = (BRISTOL_PARAMMASK & flags);
			if (audiomain->voiceCount <= 0)
			{
				if ((audiomain->voiceCount = (BRISTOL_PARAMMASK & flags))
					< BRISTOL_VOICECOUNT)
					audiomain->voiceCount = BRISTOL_VOICECOUNT;
			}
			if (audiomain->voiceCount > BRISTOL_MAXVOICECOUNT)
				audiomain->voiceCount = BRISTOL_MAXVOICECOUNT;
			if (baudio->voicecount > BRISTOL_MAXVOICECOUNT)
				baudio->voicecount = BRISTOL_MAXVOICECOUNT;

			if (audiomain->debuglevel)
				printf("created %i voices: allocated %i to synth\n",
					audiomain->voiceCount, baudio->voicecount);

			if (audiomain->voiceCount <= 1)
				audiomain->voiceCount = 1;

			baudio->controlid = msg->params.bristol.from;

			baudio->next = audiomain->audiolist;
			if (audiomain->audiolist != NULL)
				audiomain->audiolist->last = baudio;
			audiomain->audiolist = baudio;

#ifndef BRISTOL_SEMAPHORE
			jack_ringbuffer_go(audiomain->rb);
#endif
		}

		if ((audiomain->midiflags & BRISTOL_HELLO) == 0)
		{
			printf("no hello, returning\n");
			return(0);
		}

		if (audiomain->debuglevel > 2)
			printf("commask %x\n", flags & BRISTOL_COMMASK);

		/*
		 * See what command we have, if any.
		 */
		switch (flags & BRISTOL_COMMASK)
		{
			case BRISTOL_REQSTART:
				if (audiomain->atStatus == BRISTOL_EXIT) {
					audiomain->atReq |= BRISTOL_REQSTART;

					while (audiomain->atStatus != BRISTOL_OK)
						usleep(100000);
				}
				break;
			case BRISTOL_REQSTOP:
				audiomain->atReq = BRISTOL_REQSTOP;
				break;
			case BRISTOL_DEBUGON:
				printf("Debugging enabled\n");
				audiomain->debuglevel = -1;
				break;
			case BRISTOL_DEBUGOF:
				printf("Debugging disabled\n");
				audiomain->debuglevel = 0;
				break;
			case BRISTOL_VOICES:
			{
				Baudio *baudio;

				if (audiomain->voiceCount <= 0)
					audiomain->voiceCount = (BRISTOL_PARAMMASK & flags);
				if (audiomain->voiceCount <= 1)
					audiomain->voiceCount = 1;

				if (audiomain->debuglevel >= 0)
					printf("configured %i voices\n", audiomain->voiceCount);

				baudio = audiomain->audiolist;
				while (baudio != NULL)
				{
					if ((baudio->sid == msg->params.bristol.channel)
							|| (msg->params.bristol.channel == SYSTEM_CHANNEL))
						break;

					baudio = baudio->next;
				}

				/*
				 * If we found an associated bristolAudio then configure its
				 * voices as well, since it may be fewer than the total number
				 * of configured voices.
				 */
				if (baudio != NULL)
				{
					baudio->voicecount = (BRISTOL_PARAMMASK & flags);
					if (audiomain->debuglevel)
						printf("configured %i channel voices\n",
							baudio->voicecount);
				}

				break;
			}
			case BRISTOL_INIT_ALGO:
			{
				Baudio *baudio;

				if (audiomain->atStatus != BRISTOL_OK)
					break;

				if (audiomain->debuglevel)
					printf("init request nr %i\n", initCount++);

				baudio = audiomain->audiolist;
				allNotesOff(audiomain, msg->params.bristol.channel);

				while (baudio != NULL)
				{
					if (baudio->sid == msg->params.bristol.channel)
						break;

					baudio = baudio->next;
				}

				audiomain->midiflags &= ~BRISTOL_PARAMMASK;
				audiomain->midiflags |= (BRISTOL_PARAMMASK & flags);

				initBristolAudio(audiomain, baudio);
				//bristolMidiOption(0, BRISTOL_NRP_MIDI_STOP, 0);

				break;
			}
			case BRISTOL_EXIT_ALGO:
			{
				Baudio *baudio = findBristolAudio(audiomain->audiolist,
					msg->params.bristol.channel, 0);

				if ((baudio == NULL) || (baudio->mixflags == 0))
					return(0);

				bristolMidiOption(0, BRISTOL_NRP_MIDI_GO, 0);

				/*
				printf("exit algo: %i, %x, %x\n",
					msg->params.bristol.channel,
						(size_t) baudio, (size_t) baudio->mixflags);
				*/

				allNotesOff(audiomain, -1);

				/*
				 * Flag that the audio thread remove the algorithm
				 */
				if (baudio != NULL)
					baudio->mixflags = (BRISTOL_HOLDDOWN | BRISTOL_REMOVE);

				/*
				 * Check if this was the last emulator. If it was then do some
				 * shutdown operations here.
				 */
				if (((audiomain->audiolist == NULL)
					|| ((audiomain->audiolist == baudio)
						&& (baudio->next == NULL)))
					&& (audiomain->flags & BRISTOL_MIDI_WAIT))
				{
					int i = 10;

					printf("empty audio list, cleaning up\n");

					while (audiomain->audiolist != NULL)
					{
						usleep(100000);

						if (audiomain->audiolist != NULL)
							printf("waiting for audiothread cleanup\n");

						if (--i == 0)
						{
							printf("timeout on audio thread cleanup\n");
							break;
						}
					}

					bristolMidiSendMsg(msg->params.bristol.from, 0, 127, 0, 0);

					/*
					 * It looks like this should be called first however we do
					 * still need the audio thread to be called by jack for the
					 * time being: the audio thread has do clean its stuff up
					 * before exit. When it has done its cleanup it returns to
					 * jack immediately if the audiolist is empty so now we can
					 * unregister the interface and clean up the rest of the
					 * app
					 */
					if (audiomain->flags & BRISTOL_JACK)
					{
						audiomain->atReq = BRISTOL_REQSTOP;
#ifdef BRISTOL_JACK_MULTI_CLOSE
#ifdef _BRISTOL_JACK // The bristol idle thread should do this.
						bristolJackInterface(NULL);
#endif
#endif
					}
					if (audiomain->flags & BRISTOL_PULSE)
					{
						audiomain->atReq = BRISTOL_REQSTOP;
#ifdef BRISTOL_PA // The bristol idle thread should do this.
						bristolPulseInterface(NULL);
#endif
					}

					bristolMidiSendMsg(msg->params.bristol.from, 0, 127, 0, 1);
					audiomain->atStatus = BRISTOL_TERM;

					inthandler();

					return(0);
				} else {
					int i = 20;

					while ((baudio->mixflags & BRISTOL_REMOVE) && (--i > 0))
						usleep(100000);
				}

				hclose = msg->params.bristol.from;

				bristolfree(baudio);

				bristolMidiOption(0, BRISTOL_NRP_MIDI_GO, 1);

				break;
			}
			/*
			 * This is broken, or we need to find every synth using this
			 * channel?
			 */
			case (BRISTOL_TRANSPOSE & BRISTOL_COMMASK):
			{
				Baudio *baudio = findBristolAudio(audiomain->audiolist,
					msg->params.bristol.channel, 0);

				if (baudio == NULL)
					break;

				/*
				 * We should also consider restating existing frequencies but
				 * that is for later. We should support negative transposes.
				 *
				 * Negative transposes can be done for 2 octaves only - we only
				 * support +ve numbers in messaging so we added in 24 to the
				 * flag that we remove now.
				 */
				baudio->transpose = ((int) (BRISTOL_PARAMMASK & flags)) - 24;

				if (baudio->midiflags & BRISTOL_MIDI_DEBUG1)
					printf("tranpose now %i\n", baudio->transpose);

				break;
			}
			case BRISTOL_HOLD:
			{
				Baudio *baudio = findBristolAudio(audiomain->audiolist,
					msg->params.bristol.channel, 0);

				if (baudio == NULL)
					break;

				if (BRISTOL_PARAMMASK & flags)
					baudio->mixflags |= BRISTOL_KEYHOLD;
				else {
					baudio->mixflags &= ~BRISTOL_KEYHOLD;
					allNotesOff(audiomain, baudio->midichannel);
				}

				break;
			}
			case BRISTOL_LOWKEY:
			{
				Baudio *baudio = findBristolAudio(audiomain->audiolist,
					msg->params.bristol.channel, 0);

				if (baudio == NULL)
					break;

				baudio->lowkey = (BRISTOL_PARAMMASK & flags);

				break;
			}
			case BRISTOL_HIGHKEY:
			{
				Baudio *baudio = findBristolAudio(audiomain->audiolist,
					msg->params.bristol.channel, 0);

				if (baudio == NULL)
					break;

				baudio->highkey = (BRISTOL_PARAMMASK & flags);
				break;
			}
			case BRISTOL_UNISON:
			{
				Baudio *baudio = findBristolAudio(audiomain->audiolist,
					msg->params.bristol.channel, 0);

				if (msg->params.bristol.valueLSB > 0)
					baudio->mixflags |= BRISTOL_V_UNISON;
				else
					baudio->mixflags &= ~BRISTOL_V_UNISON;
				break;
			}
			case BRISTOL_ALL_NOTES_OFF:
				allNotesOff(audiomain, -1);
				break;
			case BRISTOL_MIDICHANNEL:
			{
				Baudio *baudio;

				allNotesOff(audiomain, -1);

				baudio = audiomain->audiolist;

				while (baudio != NULL)
				{
					if (baudio->sid == msg->params.bristol.channel)
						break;

					baudio = baudio->next;
				}

				/*
				 * We may need to put an all notes off in here.
				 */
				if (baudio != NULL)
					baudio->midichannel = flags & BRISTOL_PARAMMASK;

				break;
			}
			case BRISTOL_REQ_FORWARD:
				bristolMidiOption(msg->params.bristol.from,
					BRISTOL_NRP_REQ_FORWARD, BRISTOL_PARAMMASK & flags);
				break;
			case BRISTOL_REQ_DEBUG:
				//printf("SETUP DEBUG LEVELS: %i\n", BRISTOL_PARAMMASK & flags);
				audiomain->debuglevel = BRISTOL_PARAMMASK & flags;
				if (audiomain->debuglevel >= 2)
					bristolMidiOption(0, BRISTOL_NRP_DEBUG,
						audiomain->debuglevel - 2);
				break;
			default:
				break;
		}

		if (audiomain->debuglevel > 2)
			printf("cont 2: %i: res %i\n", msg->params.bristol.from, result);

		/*
		 * return a message, with the controlid assumably at the head
		 * of the playlist - we have just created it.
		msg->params.bristol.valueLSB = result;
		bristolMidiPrint(msg);
		 */
		bristolMidiSendMsg(msg->params.bristol.from, 0, 127, 0, result);

/*			audiomain->audiolist->controlid); */
		if (hclose > 0)
		{
			usleep(100000);
			bristolMidiClose(hclose);
		}
	}

	return(0);
}

