
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

/*#define DEBUG */

#ifdef BRISTOL_SEMAPHORE
#ifdef BRISTOL_SEM_OPEN
#include <fcntl.h>
#include <sys/stat.h>
static char sem_long_name[32];
static char sem_short_name[32];
#endif
#endif /* BRISTOL_SEMAPHORE */
#include <pthread.h>
#include "bristolmidi.h"
#include "bristol.h"

extern int midiExitReq;

extern int midiCheck();
extern void initMidiRoutines();
extern void checkcallbacks(bristolMidiMsg *);

extern audioMain audiomain;

bristolMidiHandler bristolMidiRoutines;
void printMidiMsg(bristolMidiMsg *);

void
midiThreadLoadReq(audioMain *audiomain)
{
	bristolMidiMsg midiMsg;

	if (audiomain->debuglevel)
		printf("load was requested\n");

	/*
	 * Take the midiMsg, construct what we want to send and forward it. We use
	 * the checkcallbacks for redistribution.
	 */
	midiMsg.command = MIDI_SYSEX;
	midiMsg.params.bristol.SysID = (audiomain->SysID >> 24) & 0x0ff;
	midiMsg.params.bristol.L = (audiomain->SysID >> 16) & 0x0ff;
	midiMsg.params.bristol.a = (audiomain->SysID >> 8) & 0x0ff;
	midiMsg.params.bristol.b = audiomain->SysID & 0x0ff;

	midiMsg.params.bristol.channel = 0;
	/* This is going to be awkward - need to find physical device */
	if (audiomain->midiHandle > 0)
		midiMsg.params.bristol.from = audiomain->midiHandle;
	else
		midiMsg.params.bristol.from = 0;

	if (audiomain->sessionfile != NULL)
	{
		midiMsg.params.bristol.msgType = MSG_TYPE_SESSION;
		midiMsg.params.bristol.msgLen = sizeof(bristolMsg)
			+ strlen(audiomain->sessionfile);

		midiMsg.params.bristolt2.operation = BRISTOL_MT2_READ;
		midiMsg.params.bristolt2.data = audiomain->sessionfile;
	} else {
		midiMsg.params.bristol.msgLen = sizeof(bristolMsg);
		midiMsg.params.bristol.msgType = MSG_TYPE_PARAM;

		midiMsg.params.bristol.operator = BRISTOL_LADI;
		midiMsg.params.bristol.controller = BRISTOL_LADI_LOAD_REQ;

		midiMsg.params.bristol.msgLen = sizeof(bristolMsg);
		midiMsg.params.bristol.valueMSB = midiMsg.params.bristol.valueLSB = 0;
	}

	//printf("loadReq from %i\n", midiMsg.params.bristol.from);

	checkcallbacks(&midiMsg);
	audiomain->sessionfile = 0;
}

void
midiThreadSaveReq(audioMain *audiomain)
{
	bristolMidiMsg midiMsg;

	if (audiomain->debuglevel)
		printf("save was requested\n");

	/*
	 * Take the midiMsg, construct what we want to send and forward it. We use
	 * the checkcallbacks for redistribution.
	 */
	midiMsg.command = MIDI_SYSEX;
	midiMsg.params.bristol.SysID = (audiomain->SysID >> 24) & 0x0ff;
	midiMsg.params.bristol.L = (audiomain->SysID >> 16) & 0x0ff;
	midiMsg.params.bristol.a = (audiomain->SysID >> 8) & 0x0ff;
	midiMsg.params.bristol.b = audiomain->SysID & 0x0ff;

	midiMsg.params.bristol.channel = 0;
	/* This is going to be awkward - need to find physical device */
	if (audiomain->midiHandle > 0)
		midiMsg.params.bristol.from = audiomain->midiHandle;
	else
		midiMsg.params.bristol.from = 0;

	if (audiomain->sessionfile != NULL)
	{
		midiMsg.params.bristol.msgType = MSG_TYPE_SESSION;
		midiMsg.params.bristol.msgLen = sizeof(bristolMsg)
			+ strlen(audiomain->sessionfile);

		midiMsg.params.bristolt2.operation = BRISTOL_MT2_WRITE;
		midiMsg.params.bristolt2.data = audiomain->sessionfile;
	} else {
		midiMsg.params.bristol.msgLen = sizeof(bristolMsg);
		midiMsg.params.bristol.msgType = MSG_TYPE_PARAM;

		midiMsg.params.bristol.operator = BRISTOL_LADI;
		midiMsg.params.bristol.controller = BRISTOL_LADI_SAVE_REQ;

		midiMsg.params.bristol.msgLen = sizeof(bristolMsg);

		midiMsg.params.bristol.valueMSB = midiMsg.params.bristol.valueLSB = 0;
	}

	//printf("saveReq from %i\n", midiMsg.params.bristol.from);

	checkcallbacks(&midiMsg);
	audiomain->sessionfile = 0;
}

#ifndef BRISTOL_SEMAPHORE
int
midiMsgForwarder(bristolMidiMsg *msg)
{
	/*
	 * Take bristol midi messages, write them into the forwarding path ring
	 * buffer.
	 */
	if (jack_ringbuffer_write_space(audiomain.rbfp) >= sizeof(bristolMidiMsg))
		jack_ringbuffer_write(audiomain.rbfp,
			(char *) msg,
			sizeof(bristolMidiMsg));
	
	return(0);
}
#endif

int
midiMsgHandler(bristolMidiMsg *msg, audioMain *audiomain)
{
	int event = (msg->command & ~MIDI_STATUS_MASK) >> 4;

#ifdef DEBUG
	printf("midiMsgHandler(%x, %x)\n", msg, audiomain);
	printMidiMsg(msg);
#endif

	/*
	 * Depending on the message type, handle the message. If this is a note
	 * event, channel or poly pressure then we will have to apply the value
	 * mapping here, as early as possible.
	 */
	switch(event) {
		case 0:
		case 1:
			/* 
			 * This does key mapping as well, but note on and off events use
			 * the same table, otherwise we would have issues. The key mapping
			 * table defaults to linear (1 to 1) and takes the System map as
			 * we will not be remapping system events.
			 */
			msg->params.key.key
				= bristolMidiRoutines.bmr[7].map[msg->params.key.key];
			msg->params.key.velocity
				= bristolMidiRoutines.bmr[event].map[msg->params.key.velocity];
			break;
		case 2:
			msg->params.pressure.pressure
				= bristolMidiRoutines.bmr[event].map[
					msg->params.pressure.pressure];
			break;
		case 5:
			msg->params.channelpress.pressure
				= bristolMidiRoutines.bmr[event].map[
					msg->params.channelpress.pressure];
			break;
	}

	bristolMidiRoutines.bmr[event].callback(audiomain, msg);

	return(0);
}

extern int exitReq;
static char *bSMD = "128.1";

void *
midiThread(audioMain *audiomain)
{
	int flags = BRISTOL_CONN_MIDI, exitstatus = -1, i;
#if (BRISTOL_HAS_ALSA == 1)
	char *device = bAMD;
#else
	char *device = bOMD;
#endif

#ifdef DEBUG
	printf("starting midi thread\n");
#endif

	audiomain->mtStatus = BRISTOL_WAIT;

	initMidiRoutines(audiomain, bristolMidiRoutines.bmr);
	/*
	 * bmr[7].floatmap may now contain a microtonal map, we should converge this
	 * onto the bmr.freq table.
	 */
	for (i = 0; i < 128; i++)
	{
		if (bristolMidiRoutines.bmr[7].floatmap[i] == 0)
			continue;

		bristolMidiRoutines.freq[i].freq
			= bristolMidiRoutines.bmr[7].floatmap[i];
		bristolMidiRoutines.freq[i].step = ((float) BRISTOL_BUFSIZE) /
			(bristolMidiRoutines.bmr[7].floatmap[i] * audiomain->samplerate);
	}

#ifdef BRISTOL_SEMAPHORE
	/*
	 * Get a couple of semaphores to separate the midi from the audio 
	 * processing. These are destroyed in the inthandler() in bristol.c as
	 * the midi signalling is closed down. The semaphores are shared in the
	 * note on/off code in midinote.c and the audioEngine.c which need to 
	 * track the same voicelists.
	 *
	 * The semaphores are only for note events. There are two lockout sections
	 * to minimise delays for most key events. That is not possible for all
	 * events, see midinote.c for details, it regards voice starvation.
	 */
	if (audiomain->sem_long == NULL)
		audiomain->sem_long = (sem_t *) bristolmalloc(sizeof(sem_t));
	if (audiomain->sem_short == NULL)
		audiomain->sem_short = (sem_t *) bristolmalloc(sizeof(sem_t));

#ifdef BRISTOL_SEM_OPEN
	sprintf(sem_long_name, "sem_long_%i", getpid());
	sprintf(sem_short_name, "sem_short_%i", getpid());
	audiomain->sem_long = sem_open(sem_long_name,O_CREAT,0,1);

	if (audiomain->sem_long == SEM_FAILED)
#else
	if (sem_init(audiomain->sem_long, 0, 1) < 0)
#endif
	{
		perror("sem_open");
		printf("could not get the long semaphore\n");
		audiomain->atReq = BRISTOL_REQSTOP;
		exitReq = 1;

		pthread_exit(&exitstatus);
	}

#ifdef BRISTOL_SEM_OPEN
	audiomain->sem_short = sem_open(sem_short_name,O_CREAT,0,1);
	if (audiomain->sem_short == SEM_FAILED)
#else
	if (sem_init(audiomain->sem_short, 0, 1) < 0)
#endif
	{
		perror("sem_open");   
		printf("could not get the short semaphore\n");
		audiomain->atReq = BRISTOL_REQSTOP;
		exitReq = 1;
		pthread_exit(&exitstatus);
	}
#else /* BRISTOL_SEMAPHORE */
	/*
	 * The alternative to semaphores is the ringbuffer, it is used for note
	 * events: the midithread stuffs them in the buffer, the audio thread 
	 * will pick them out and do the required work.
	 */
	audiomain->rb = jack_ringbuffer_create(8192);
	jack_ringbuffer_mlock(audiomain->rb);
	jack_ringbuffer_reset(audiomain->rb);
	audiomain->rbfp = jack_ringbuffer_create(8192);
	jack_ringbuffer_mlock(audiomain->rbfp);
	jack_ringbuffer_reset(audiomain->rbfp);

	/*
	 * Start the forward path ringbuffer. The direct path will only get started
	 * later when baudio structures are initialised.
	 */
	jack_ringbuffer_go(audiomain->rbfp);
#endif /* BRISTOL_SEMAPHORE */

	if ((audiomain->flags & BRISTOL_MIDIMASK) == BRISTOL_MIDI_OSS)
	{
		printf("midi oss\n");
		flags = BRISTOL_CONN_OSSMIDI;
	}

	if ((audiomain->flags & BRISTOL_MIDIMASK) == BRISTOL_MIDI_SEQ)
	{
		flags = BRISTOL_CONN_SEQ;
		if ((audiomain->mididev == (char *) NULL)
			|| (audiomain->mididev[0] == '\0'))
			device = "bristol";
		else
			device = audiomain->mididev;
		printf("midi interface: %s\n", device);
	} else if (audiomain->mididev != (char *) NULL)
		device = audiomain->mididev;

	if (audiomain->flags & BRISTOL_MIDI_OSC)
	{
		/*
		 * We need to init the OSC interface here.
		 */
		printf("midi osc not currently implemented\n");
	}

	if ((audiomain->flags & BRISTOL_MIDIMASK) == BRISTOL_MIDI_JACK)
	{
		device = bSMD;
		printf("midi jack: %s\n",device);
		flags = BRISTOL_CONN_SEQ;
	}

	/*
	 * Get a control port. It will only want SYSEX messages UNLESS it is the
	 * only link.
	 */
	if ((audiomain->controlHandle
		= bristolMidiOpen(audiomain->controldev,
			BRISTOL_DUPLEX|BRISTOL_CONN_TCP|BRISTOL_CONN_PASSIVE|
			(audiomain->flags & BRISTOL_MIDI_WAIT),
			audiomain->port,
			audiomain->flags
				& (BRISTOL_MIDI_SEQ|BRISTOL_MIDI_ALSA|BRISTOL_MIDI_OSS)?
				BRISTOL_REQ_SYSEX:BRISTOL_REQ_ALL,
			midiMsgHandler, audiomain)) < 0)
	{
		printf("Error opening control device, exiting midi thread\n");
		audiomain->atReq = BRISTOL_REQSTOP;
		exitReq = 1;
		pthread_exit(&exitstatus);
	} else
		printf("opened control socket\n");

	printf("midiOpen: %i(%x)\n", audiomain->port, flags);

	if ((audiomain->flags &
			(BRISTOL_MIDI_SEQ|BRISTOL_MIDI_ALSA|BRISTOL_MIDI_OSS))
		&& ((audiomain->midiHandle
			= bristolMidiOpen(device, flags|BRISTOL_RDONLY, -1, BRISTOL_REQ_NSX,
				midiMsgHandler, audiomain)) < 0))
	{
		printf("Error opening midi device %s/%i, exiting midi thread\n",
			device, audiomain->midiHandle);
		printf("Bristol cannot operate without a midi interface. Terminating\n");
		audiomain->atReq = BRISTOL_REQSTOP;
		exitReq = 1;
		bristolMidiClose(audiomain->controlHandle);
		pthread_exit(&exitstatus);
	} else
		printf("opened midi device %s\n", device);

#ifdef DEBUG
	printf("opened midi device: %i/%i\n",
		audiomain->controlHandle, audiomain->midiHandle);
#endif

	audiomain->mtStatus = BRISTOL_OK;

#ifndef BRISTOL_SEMAPHORE
	if (audiomain->flags & BRISTOL_JACK_DUAL)
		bristolMidiRegisterForwarder(NULL);
	else
		bristolMidiRegisterForwarder(midiMsgForwarder);
#endif

	/*
	 * This will become blocking midi code, now that we are threaded. In short,
	 * midiCheck should not return. This can be an issue to ensure that the
	 * midi device is closed.
	 */
	midiCheck();

	bristolMidiClose(audiomain->midiHandle);
	bristolMidiClose(audiomain->controlHandle);

	printf("midiThread exiting\n");

#ifdef BRISTOL_SEMAPHORE
#ifdef BRISTOL_SEM_OPEN
	sem_close(audiomain->sem_long);
	sem_close(audiomain->sem_short);
	sem_unlink(sem_long_name);
	sem_unlink(sem_short_name);
#else
	sem_destroy(audiomain->sem_long);
	sem_destroy(audiomain->sem_short);
	bristolfree(audiomain->sem_long);
	bristolfree(audiomain->sem_short);
#endif
#else
	jack_ringbuffer_free(audiomain->rb);
	jack_ringbuffer_free(audiomain->rbfp);
#endif /* SEMAPHORE */

	exitstatus = 0;
	pthread_exit(&exitstatus);
}

void
printMidiMsg(bristolMidiMsg *msg)
{
	if (msg->command == 0x00f0) {
		printf("\n	handle:  %i\n", msg->midiHandle);
		printf("	command: SYSEX\n");
	} else {
		printf("\n	handle:  %i\n", msg->midiHandle);
		printf("	chan:	%i\n", msg->channel);
		printf("	command: %x\n", msg->command);
		printf("	p1:	%i, p2:	%i\n",
			msg->params.key.key, msg->params.key.velocity);
	}
}

