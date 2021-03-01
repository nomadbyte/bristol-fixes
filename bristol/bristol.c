
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

#include <signal.h>
#include <strings.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/types.h>
#include <wait.h>

#if defined(linux)
#include <sched.h>
#endif

#include "bristol.h"
#include "bristolhelp.h"
#include "bristolmidi.h"
#include "bristolblo.h"

static char defaultcdev[16];

extern int dupfd;
void inthandler();

int exitReq = 0;
static int jh = -1, execreq = 1;

extern void *audioThread();
extern void midiThreadSaveReq(audioMain *);
extern void midiThreadLoadReq(audioMain *);
extern void *midiThread();
extern void doNoteChanges();
extern int bristolMidiTerminate();

pthread_t spawnThread();

audioMain audiomain;
static char sessionfile[1024];

char *outputfile = NULL;

#ifdef BRISTOL_PA
extern int bristolPulseInterface();
#endif

#ifdef _BRISTOL_JACK
extern int bristolJackInterface();
#ifdef _BRISTOL_JACK_SESSION
extern int bristolJackSessionCheck();
#endif
#endif
static int jsmd = 200;

static char *bImport = NULL;

/*
 * LADI parameters. LADI requires a bit of work in the engine:
 * We have to send messages to all the attached GUI and request they save
 * state. This should be tacked into the MIDI thread however it is blocking
 * code that is waiting for events so for now we will just send the message.
 */
int sessmgrmode = 1;
volatile sig_atomic_t ladiRequest = 0;

static void
savehandler()
{
	if (sessmgrmode)
		ladiRequest = 1;
	else
		ladiRequest = 0;
}

/*
 * LADI does not support this, neither is it likely to
 */
static void
loadhandler()
{
	if (sessmgrmode)
		ladiRequest = 2;
	else
		ladiRequest = 0;
}

static void
buildCmdLine(audioMain *audiomain, int argc, char **argv)
{
	int size = 14, i, haveport = 0;

	for (i = 1; i < argc; i++)
		size += strlen(argv[i]) + 1;

	audiomain->cmdline = bristolmalloc0(size + 1);

	printf("%s", argv[0]);
	if (rindex(argv[0], '/') == NULL)
		sprintf(audiomain->cmdline, "startBristol");
	else
		sprintf(audiomain->cmdline, "%s", rindex(argv[0], '/') + 1);

	for (i = 1; i < argc; i++)
	{
		if (argv[i][0] == '-')
			printf("\n    %s", argv[i]);
		else
			printf(" %s", argv[i]);

		if (strcmp(argv[i], "-port") == 0)
		{
			if (haveport == 0)
			{
				haveport = 1;
				i++;
				printf(" %s", argv[i]);
	//			sprintf(audiomain->cmdline,
	//				"%s -port %s", audiomain->cmdline, argv[i]);
				continue;
			}
			haveport = 1;
		}

		/* Don't stuff these back onto the commandline */
		if ((strcmp(argv[i], "-jsmuuid") == 0)
			|| (strcmp(argv[i], "-rate") == 0)
			|| (strcmp(argv[i], "-count") == 0)
			|| (strcmp(argv[i], "-port") == 0)
			|| (strcmp(argv[i], "-jsmfile") == 0))
		{
			i++;
			printf(" %s", argv[i]);
			continue;
		}
		sprintf(audiomain->cmdline, "%s %s", audiomain->cmdline, argv[i]);
	}
	printf("\njsm will use '%s'\n", audiomain->cmdline);
}

/*
 * We are going to create, initially, two threads. One will field MIDI events,
 * and translate the (for now) raw data into bristol MIDI messages. The messages
 * are queued, and then fielded by the audio thread. The audio thread reacts to
 * the midi events and starts sound generation operations. The MIDI thread will
 * remain responsible for MIDI parameter management, but not real-time messages.
 *
 * It could make sense to move the threads requests into the midi library for
 * use with other programmes?
 */
int
main(int argc, char **argv)
{
	int argCount = 1, harmonics = 31, logtype = BRISTOL_LOG_BRISTOL;
	int pstatus=-129;
	int bloCutin = 84;
	float bloCutoff = 1.5, bloFraction = 0.8;
	int bloMin = 3;
	pid_t cpid;
	pthread_t audiothread = (pthread_t) NULL, midithread, logthread;
	int watchdog = 30000000;
	int exitdecr = 25000;

#ifndef BRISTOL_SEMAPHORE
	bristolMidiMsg msg;
#endif

	bzero(&audiomain, sizeof(audioMain));
	audiomain.samplecount = 256; /* default this - gets overriden later */
	audiomain.iosize = 0; /* This needs to be calculated, samplecount * float */
	audiomain.preload = 4; /* This should be less, preferably 4 (was 8). */
	audiomain.ingain = 0;
	audiomain.outgain = 4;
	audiomain.iocount = 0;
	audiomain.m_io_ogc = 1.0;
	audiomain.m_io_igc = 1.0;
	/* default to OSS until ALSA goes 1.0 */
/*	audiomain.flags = BRISTOL_OSS|BRISTOL_MIDI_OSS; */

#ifdef BRISTOL_JACK_DEFAULT
	audiomain.flags = BRISTOL_JACK|BRISTOL_MIDI_WAIT;
	/* Default the devicename, can be overridden later */
	audiomain.audiodev = argv[0];
#else
#if (BRISTOL_HAS_ALSA == 1)
	audiomain.flags = BRISTOL_ALSA|BRISTOL_MIDI_WAIT;
#else
	/* So default to OSS */
	audiomain.flags = BRISTOL_OSS;
#endif
#endif

#ifdef BRISTOL_JACK_DEFAULT_MIDI
	audiomain.flags |= BRISTOL_MIDI_JACK;
	/* Default the devicename, can be overridden later */
#else
#if (BRISTOL_HAS_ALSA == 1)
	audiomain.flags |= BRISTOL_MIDI_SEQ;
#else
	/* So default to OSS */
	audiomain.flags = BRISTOL_MIDI_OSS;
#endif
#endif

	audiomain.samplerate = 44100; /* This is a default, gets overwritten */
	audiomain.priority = 65;

	if ((argc == 2) && (strcmp(argv[1], "warranty") == 0))
	{
		printf("bristol version %s\n", VERSION);
		printf("%s", gplwarranty);
		exit(0);
	}
	if ((argc == 2) && (strcmp(argv[1], "conditions") == 0))
	{
		printf("bristol version %s\n", VERSION);
		printf("%s", gplconditions);
		exit(0);
	}

#ifndef MONOLITHIC
	if (argc <= 2) {
		printf("You probably prefer to use the startBristol script.\n");
		exit(1);
	}
#endif

	/*
	 * This could be made more intelligent such as a name for what to connect
	 * to for example. That is for later study.
	 */
	if ((getenv("BRISTOL_AUTOCONN") != NULL)
		&& (strcmp(getenv("BRISTOL_AUTOCONN"), "true") == 0))
		audiomain.flags |= BRISTOL_AUTO_CONN;
	if ((getenv("BRISTOL_LOG_CONSOLE") != NULL)
		&& (strcmp(getenv("BRISTOL_LOG_CONSOLE"), "true") == 0))
		logtype = BRISTOL_LOG_CONSOLE;

	logthread = bristolOpenStdio(logtype);

	bristolMidiOption(0, BRISTOL_NRP_SYSID_H, 0x534C);
	bristolMidiOption(0, BRISTOL_NRP_SYSID_L, 0x6162);
	audiomain.SysID = 0x534c6162;
	/* Localhost implies a TCP socket as it is a 'host' */
	snprintf(defaultcdev, 16, "localhost");
	audiomain.controldev = defaultcdev;

	while (argc > argCount)
	{
		if (strcmp(argv[argCount], "-daemon") == 0)
		{
			if (fork() == 0)
			{
				audiomain.flags &= ~BRISTOL_MIDI_WAIT;

				setsid();

				bristolOpenStdio(BRISTOL_LOG_DAEMON);

				argCount++;
				continue;
			} else
				exit(0);
		}

		if (strcmp(argv[argCount], "-syslog") == 0)
		{
			bristolOpenStdio(BRISTOL_LOG_DAEMON);
			bristolOpenStdio(BRISTOL_LOG_SYSLOG);
			argCount++;
			continue;
		}

		if ((strcmp(argv[argCount], "-log") == 0)
			|| (strcmp(argv[argCount], "-logengine") == 0))
		{
			bristolOpenStdio(BRISTOL_LOG_DAEMON);
			argCount++;
			continue;
		}

		if (strcmp(argv[argCount], "-glwf") == 0)
		{
			blo.flags |= BRISTOL_LWF;
			argCount++;
			continue;
		}

		if (strcmp(argv[argCount], "-exec") == 0)
			execreq = 0;
		if (strcmp(argv[argCount], "-gui") == 0)
		{
			execreq = 0;
			/* And print a GPL notice as we are probably going headless */
			printf("%s", gplnotice);
		}

		if (strcmp(argv[argCount], "-blo") == 0)
			harmonics = atoi(argv[++argCount]);
		if (strcmp(argv[argCount], "-bloci") == 0)
			bloCutin = atoi(argv[++argCount]);
		if (strcmp(argv[argCount], "-bloco") == 0)
			bloCutoff = atof(argv[++argCount]);
		if (strcmp(argv[argCount], "-blomin") == 0)
			bloMin = atoi(argv[++argCount]);
		if (strcmp(argv[argCount], "-blofraction") == 0)
			bloFraction = atof(argv[++argCount]);

		if (((strcmp(argv[argCount], "-o") == 0) ||
			(strcmp(argv[argCount], "-output") == 0))
			&& (strlen(argv[argCount]) == 2))
				outputfile = argv[++argCount];

		if ((strcmp(argv[argCount], "-T") == 0)
			|| (strcmp(argv[argCount], "-server") == 0))
			audiomain.flags &= ~BRISTOL_MIDI_WAIT;

		if (strcmp(argv[argCount], "-watchdog") == 0)
		{
			if (++argCount <= argc)
				watchdog = atoi(argv[argCount]) * 1000000;
		}

		if (strcmp(argv[argCount], "-mididev") == 0)
		{
			if (++argCount <= argc)
				audiomain.mididev = argv[argCount];
		} else if (strcmp(argv[argCount], "-midi") == 0) {
			if (++argCount <= argc)
			{
				/*
				 * Note that some of these types of interface to not actually
				 * apply to MIDI at the moment. OSS and ALSA are rawmidi specs.
				 * SEQ is ALSA sequencer support. The others may or may not
				 * provide midi messaging, however it is possible to have jack
				 * audio drivers with ALSA rawmidi interfacing, for example.
				 */
				if ((strcmp(argv[argCount], "none") == 0)
					|| (strcmp(argv[argCount], "dummy") == 0))
					audiomain.flags &= ~BRISTOL_MIDIMASK;
				else if (strcmp(argv[argCount], "oss") == 0)
					audiomain.flags = (audiomain.flags & ~BRISTOL_MIDIMASK)
						|BRISTOL_MIDI_OSS;
				else if ((strcmp(argv[argCount], "rawalsa") == 0)
						|| (strcmp(argv[argCount], "alsaraw") == 0))
					audiomain.flags = (audiomain.flags & ~BRISTOL_MIDIMASK)
						|BRISTOL_MIDI_ALSA;
				else if (strcmp(argv[argCount], "slab") == 0)
					audiomain.flags = (audiomain.flags & ~BRISTOL_MIDIMASK)
						|BRISTOL_MIDI_SLAB;
				else if ((strcmp(argv[argCount], "seq") == 0)
						|| (strcmp(argv[argCount], "alsa") == 0))
					audiomain.flags = (audiomain.flags & ~BRISTOL_MIDIMASK)
						|BRISTOL_MIDI_SEQ;
				else if (strcmp(argv[argCount], "jack") == 0)
					audiomain.flags = (audiomain.flags & ~BRISTOL_MIDIMASK)
						|BRISTOL_MIDI_JACK;
				else if (strcmp(argv[argCount], "osc") == 0)
					/* Osc is not exclusive */
					audiomain.flags |= BRISTOL_MIDI_OSC;
			}
		}

		if ((strcmp(argv[argCount], "-scala") == 0)
			|| (strcmp(argv[argCount], "-scl") == 0))
		{
			if (++argCount <= argc)
				audiomain.microTonalMappingFile =  argv[argCount];
		}

		if (strcmp(argv[argCount], "-audiodev") == 0)
		{
			if (++argCount <= argc)
				audiomain.audiodev = argv[argCount];
		}

		if ((strcmp(argv[argCount], "-emulate") == 0)
			|| (strcmp(argv[argCount], "-register") == 0))
		{
			if ((++argCount < argc) && (argv[argCount][0] != '-'))
			{
				if (audiomain.flags & BRISTOL_JACK)
					audiomain.audiodev = argv[argCount];
				if (audiomain.flags & (BRISTOL_MIDI_SEQ|BRISTOL_MIDI_JACK))
					audiomain.mididev = argv[argCount];
			}
		}

		if (strcmp(argv[argCount], "-audio") == 0) {
			if (++argCount <= argc)
			{
				if (strcmp(argv[argCount], "oss") == 0)
					audiomain.flags
						= (audiomain.flags & ~BRISTOL_AUDIOMASK)|BRISTOL_OSS;
				else if (strcmp(argv[argCount], "alsa") == 0) {
					audiomain.flags
						= (audiomain.flags & ~BRISTOL_AUDIOMASK)|BRISTOL_ALSA;
					audiomain.audiodev = NULL;
				} else if (strcmp(argv[argCount], "jack") == 0) {
					audiomain.flags
						= (audiomain.flags & ~BRISTOL_AUDIOMASK)|BRISTOL_JACK;
					/* Default the devicename, can be overridden later */
					audiomain.audiodev = argv[0];
				} else if (strcmp(argv[argCount], "pulsethread") == 0) {
					audiomain.flags
						= (audiomain.flags & ~BRISTOL_AUDIOMASK)|BRISTOL_PULSE_T;
					/* Default the devicename, can be overridden later */
					audiomain.audiodev = argv[0];
				} else if (strcmp(argv[argCount], "pulse") == 0) {
					audiomain.flags
						= (audiomain.flags &~BRISTOL_AUDIOMASK)|BRISTOL_PULSE_S;
					/* Default the devicename, can be overridden later */
					audiomain.audiodev = argv[0];
				} else if (strcmp(argv[argCount], "dummy") == 0)
					audiomain.flags = (audiomain.flags &
						~(BRISTOL_AUDIOMASK|BRISTOL_MIDIMASK|BRISTOL_MIDI_ALSA))
						|BRISTOL_DUMMY|BRISTOL_MIDI_SEQ;
			}
		}

		/*
		 * Overall output gain for weak signals.
		 *
		 * the 'gain' option is now per synth, negotiated over the datalink.
		if (strcmp(argv[argCount], "-gain") == 0)
		{
			if (++argCount <= argc)
				audiomain.outgain = atoi(argv[argCount]);
		}
		 */
		if (strcmp(argv[argCount], "-outgain") == 0)
		{
			if (++argCount <= argc)
				audiomain.outgain = atoi(argv[argCount]);
		}

		/*
		 * Overall input gain for weak signals.
		 */
		if (strcmp(argv[argCount], "-ingain") == 0)
		{
			if (++argCount <= argc)
				audiomain.ingain = atoi(argv[argCount]);
		}

		/*
		 * This is the TCP control port connection only.
		 */
		if ((strcmp(argv[argCount], "-port") == 0) && ((argCount + 1) < argc))
			audiomain.port = atoi(argv[argCount + 1]);

		if ((strcmp(argv[argCount], "-host") == 0) && ((argCount + 1) <= argc))
			snprintf(defaultcdev, 16, "%s", argv[argCount + 1]);

		if (strcmp(argv[argCount], "-oss") == 0)
			audiomain.flags = (audiomain.flags &
				~(BRISTOL_AUDIOMASK|BRISTOL_MIDIMASK))
				|BRISTOL_OSS|BRISTOL_MIDI_OSS;

		if (strcmp(argv[argCount], "-seq") == 0)
			audiomain.flags |= BRISTOL_MIDI_SEQ;

		if (strcmp(argv[argCount], "-alsa") == 0)
		{
			audiomain.flags = (audiomain.flags &
				~(BRISTOL_AUDIOMASK|BRISTOL_MIDIMASK))
				|BRISTOL_ALSA|BRISTOL_MIDI_SEQ;
			audiomain.audiodev = NULL;
		}

		if (strcmp(argv[argCount], "-osc") == 0)
			audiomain.flags |= BRISTOL_MIDI_OSC;

		if (strcmp(argv[argCount], "-jack") == 0)
		{
			audiomain.flags = (audiomain.flags &
				~(BRISTOL_AUDIOMASK|BRISTOL_MIDIMASK|BRISTOL_MIDI_ALSA))
				|BRISTOL_JACK|BRISTOL_MIDI_JACK;
			/* Default the devicename, can be overridden later */
			audiomain.audiodev = argv[0];
		}

		if ((strcmp(argv[argCount], "-jsmd") == 0) && (argc > argCount))
			jsmd = atoi(argv[++argCount]) / 25;

		if ((strcmp(argv[argCount], "-jsmuuid") == 0) && (argc > argCount))
		{
			snprintf(audiomain.jackUUID, BRISTOL_JACK_UUID_SIZE, "%s",
				argv[++argCount]);
		}

		if (((strcmp(argv[argCount], "-jsmfile") == 0) ||
			(strcmp(argv[argCount], "-import") == 0)) && (argc > argCount))
		{
			snprintf(sessionfile, 1024, "%s", argv[++argCount]);
			bImport = sessionfile;
			++argCount;
			continue;
		}

		if ((strcmp(argv[argCount], "-mogc") == 0) && (argc > argCount))
		{
			audiomain.m_io_ogc = atof(argv[++argCount]);
			argCount++;
		}

		if ((strcmp(argv[argCount], "-migc") == 0) && (argc > argCount))
		{
			audiomain.m_io_igc = atof(argv[++argCount]);
			argCount++;
		}

		if ((strcmp(argv[argCount], "-arp2600") == 0)
			&& (audiomain.flags & BRISTOL_JACK))
			/* This was '4' but turned out to be a bad default */
			audiomain.iocount = 0;

		if ((strcmp(argv[argCount], "-multi") == 0) && (argc > argCount)
			&& (audiomain.flags & BRISTOL_JACK))
		{
			if ((audiomain.iocount = atoi(argv[argCount + 1]))
				> BRISTOL_JACK_MULTI)
				audiomain.iocount = BRISTOL_JACK_MULTI;
				argCount++;
		}

		if (strcmp(argv[argCount], "-autoconn") == 0)
			audiomain.flags |= BRISTOL_AUTO_CONN;

		if (strcmp(argv[argCount], "-dummyaudio") == 0)
			audiomain.flags = (audiomain.flags &
				~(BRISTOL_AUDIOMASK|BRISTOL_MIDIMASK|BRISTOL_MIDI_ALSA))
				|BRISTOL_DUMMY|BRISTOL_MIDI_SEQ;

		if (strcmp(argv[argCount], "-jdo") == 0)
			audiomain.flags |= BRISTOL_JACK_DUAL;

		if ((strcmp(argv[argCount], "-rate") == 0) && (argc > argCount))
			audiomain.samplerate = atoi(argv[argCount++ + 1]);

		if ((strcmp(argv[argCount], "-count") == 0) && (argc > argCount))
			audiomain.samplecount = atoi(argv[argCount++ + 1]);

		if ((strcmp(argv[argCount], "-preload") == 0) && (argc > argCount))
			audiomain.preload = atoi(argv[argCount++ + 1]);

		if ((strcmp(argv[argCount], "-priority") == 0) && (argc > argCount))
		{
			if ((audiomain.priority = atoi(argv[argCount++ + 1])) < 0)
				audiomain.priority = 0;
		}

		/*
		 * Debug values in the engine will get overridden by the GUI when 
		 * distributed but this is needed for debug of the init operations.
		 */
		if (strcmp(argv[argCount], "-debug") == 0)
		{
			if ((argCount < argc) && (argv[argCount + 1][0] != '-'))
			{
				if ((audiomain.debuglevel = atoi(argv[argCount + 1])) > 15)
					audiomain.debuglevel = 15;
			} else
				/*
				 * If we only have -debug and no value then increment the debug 
				 * level.
				 */
				if (++audiomain.debuglevel > 15)
					audiomain.debuglevel = 15;
		}

		/* SYSEX System ID */
		if ((strcmp(argv[argCount], "-sysid") == 0) && (argCount < argc))
		{
			unsigned int sysid = 0x534C6162, o = argCount + 1;

			/*
			 * We will get 0x534C6162 for example
			 */
			if ((argv[o][0] == '0') || (argv[o][1] == 'x')
				|| (strlen(argv[o]) == 10))
			{
				sscanf(argv[o], "0x%x", &sysid);
				sysid &= 0x7f7f7f7f;
				bristolMidiOption(0, BRISTOL_NRP_SYSID_H, sysid >> 16);
				bristolMidiOption(0, BRISTOL_NRP_SYSID_L, sysid & 0x0000ffff);
				audiomain.SysID = sysid;
				if (audiomain.debuglevel)
					printf("fixing sysex system id at 0x%x\n", audiomain.SysID);
			}
		}

		/*
		 * LADI enable
		 */
		if ((strcmp(argv[argCount], "-ladi") == 0) && (argCount < argc))
		{
			if (strcmp(argv[argCount + 1], "bristol") == 0)
				sessmgrmode = 1;
			else if (strcmp(argv[argCount + 1], "brighton") == 0)
				sessmgrmode = 0;
			else if (strcmp(argv[argCount + 1], "both") == 0)
				sessmgrmode = 1;

			argCount+=2;
			continue;
		}

		if (strcmp(argv[argCount], "-session") == 0)
			sessmgrmode = 0;

		argCount++;
	}

	printf("bristol version %s\n", VERSION);

	if (strncmp("unix:", defaultcdev, 5) == 0)
	{
		if (strlen(defaultcdev) == 5)
			snprintf(defaultcdev, 16, "unix:%i", audiomain.port);
		else
			audiomain.port = atoi(&defaultcdev[5]);
	}

	/* Print the argument list, debug level does not apply here */
	buildCmdLine(&audiomain, argc, argv);

	generateBLOwaveforms(harmonics, BRISTOL_VPO, bloCutin, bloCutoff, bloMin,
		bloFraction, audiomain.samplerate);

	audiomain.atStatus = audiomain.mtStatus = BRISTOL_EXIT;
	audiomain.atReq = audiomain.mtReq = BRISTOL_OK;

	/*
	 * Create two threads, one for midi, then one for audio.
	 *
	 * We cannot create the audio thread yet, since we do not know how many 
	 * voices to create - this is a function of the midi start requests from
	 * some controller.
	 */
	if (audiomain.debuglevel)
		printf("spawning midi thread\n");
	midithread = spawnThread(midiThread,
		audiomain.priority == 0? 0:
			audiomain.priority > 25? audiomain.priority - 25:
				audiomain.priority - 1);

	signal(SIGINT, inthandler);
	signal(SIGPIPE, SIG_IGN);
/*	signal(SIGSEGV, inthandler); */
	signal(SIGUSR1, savehandler);
	signal(SIGUSR2, loadhandler);

	if (audiomain.debuglevel)
		printf("parent going into idle loop\n");

	/*
	 * To support jack we are going to use this parent thread. We will have it
	 * reschedule itself to something less than that requested for the audio
	 * thread and have it handle MIDI events from the Jack interface at some
	 * higher priority than than the midi thread. The actual MIDI thread will
	 * still accept events from the GUI but they are generally long events 
	 * for audio reconfiguration.
	 * 
	 * We should put some lockout on this code - make sure that the MIDI
	 * thread has initialised since we may need some of that init here.
	 */
	while (audiomain.mtStatus != BRISTOL_OK)
	{
		if (audiomain.atReq == BRISTOL_REQSTOP)
			/*
			 * Midi thread init failed, exit to prevent a lockout. Might
			 * look odd that it is in the atReq and not in some mt status
			 * however the midi thread is rquesting the audiothread to exit.
			 * Since we have not started it yet then we can kind of exit
			 * here.
			 */
			exit(0);

		if (audiomain.debuglevel)
			printf("Init waiting for midi thread OK status\n");
		usleep(100000);
	}

	if (audiomain.debuglevel)
		printf("Got midi thread OK status\n");

	while (!exitReq)
	{
		/*
		 * We should wait for children here, and restart them. We should also 
		 * monitor any bristol system sysex messages, since they will be used
		 * to request we start the audio thread.
		 */
		if (audiomain.atReq & BRISTOL_REQSTART)
		{
			/*
			 * See if the audio thread is active, if not, start it. May 
			 * eventually support multiple audio threads.
			 */
			if (audiomain.atStatus == BRISTOL_EXIT)
			{
				int i = 20;

				if (audiomain.debuglevel)
					printf("spawning audio thread\n");
				audiothread = spawnThread(audioThread, audiomain.priority);

				while (audiomain.atStatus != BRISTOL_OK)
				{
					if (audiomain.atStatus == BRISTOL_FAIL)
					{
						/* 
						 * If we don't have an audio thread then there is 
						 * probably an issue with the audio devices so reap the
						 * midi thread too.
						 */
						pthread_cancel(midithread);
						break;
					}

					if (audiomain.debuglevel)
						printf("init waiting for audio thread OK status\n");

					usleep(250000);

					if (--i < 0)
					{
						printf("failed to receive audio thread OK, exiting\n");
						audiomain.mtStatus = BRISTOL_EXIT;
						return(-1);
					}
				}

				/*
				 * Request jack linkup to its midi sequencer. If we did not
				 * compile with jack for whatever reason then the library will
				 * report it.
				 *
				 * This call will not reschedule the thread, we need to do that,
				 * also this is not a thread but the parent process.
				 */
				if (audiomain.flags & BRISTOL_MIDI_JACK)
				{
					char regname[64];

					sprintf(regname, "%smt", 
						audiomain.mididev == NULL?
							audiomain.audiodev:
							audiomain.mididev);

					if ((jh = bristolMidiOpen(regname,
						(audiomain.flags & BRISTOL_JACK_DUAL)|
						BRISTOL_CONN_JACK|BRISTOL_DUPLEX, -1,
						BRISTOL_REQ_NSX, midiMsgHandler, audiomain)) < 0)
						printf("requested jack midi did not link up\n");
					else {
						if (audiomain.debuglevel)
								printf("requested jack midi link: %i\n", jh);
						bristolMidiOption(0, BRISTOL_NRP_MIDI_GO, 1);
					}

					audiomain.midiHandle = jh;

#if defined(linux)
					if (audiomain.priority - 1 > 0)
					{
						struct sched_param schedparam;

						if (sched_getparam(0, &schedparam) != 0)
							printf("Scheduler getparam failed...\n");

						schedparam.__sched_priority = audiomain.priority - 1;
						if (sched_setscheduler(0, SCHED_FIFO, &schedparam) == 0)
						{
							if (audiomain.debuglevel)
								printf("set jmidi thread priority\n");
						} else
							printf("could not set jmidi thread priority\n");
					} else
						printf("no request to set jmidi thread priority\n");
#endif
				}
			}
			exitdecr = 0;
		} else if (audiomain.flags & BRISTOL_MIDI_WAIT) {
			/*
			 * If we do not get an audio thread start request within an amount
			 * of time (should be a parameter) and we have not been given the
			 * daemon flag then we should consider failing and stoping the
			 * process.
			 */
			if ((watchdog -= exitdecr) <= 0) 
			{
				printf("init watchdog exception\n");
				audiomain.atStatus = BRISTOL_FAIL;
				bristolMidiTerminate();
			} else if (audiomain.debuglevel > 8)
				printf("init watchdog %i\n", watchdog);
		}

		if ((audiomain.atStatus == BRISTOL_FAIL)
			|| (audiomain.atStatus == BRISTOL_TERM))
			break;
		else {
			/*
			 * We may have to reconsider this stage for RT safe audio threading:
			 * MIDI event redistribution may require writing to a TCP socket
			 * from the audio thread, that could theoretically block, or at
			 * least not a good thing for an RT scheduled thread the way Jack
			 * wants them to behave.
			 *
			 * The workaround is to specify '-forward' at runtime to prevent
			 * event redistribution. A better fix would be to open another 
			 * ringbuffer for events back to the GUI and have them sent from 
			 * here, changing this timer.
			 *
			 * The 0.50.2 fix uses this second ringbuffer from the audio/midi
			 * thread signalling back here to the parent thread so do the
			 * actual forwarding.
			 */
#ifndef BRISTOL_SEMAPHORE
			while (jack_ringbuffer_read_space(audiomain.rbfp)
				>= sizeof(bristolMidiMsg))
			{
				jack_ringbuffer_read(audiomain.rbfp, (char *) &msg,
					sizeof(bristolMidiMsg));

				bristolMidiRawWrite(msg.mychannel, &msg,
					msg.params.bristol.msgLen);
			}

			usleep(25000);
#else
			sleep(1);
#endif
		}

		if ((ladiRequest == 1) && sessmgrmode)
		{
			ladiRequest = 0;

			/* We will need to send a message to the GUI */
			midiThreadSaveReq(&audiomain);
		}
		if ((ladiRequest == 2) && sessmgrmode)
		{
			ladiRequest = 0;

			/* We will need to send a message to the GUI */
			midiThreadLoadReq(&audiomain);
		}

		if (bImport && (--jsmd < 0))
		{
			audiomain.sessionfile = bImport;
			midiThreadLoadReq(&audiomain);
			bImport = NULL;
			audiomain.sessionfile = NULL;
		}

#ifdef _BRISTOL_JACK_SESSION
		if (sessmgrmode) {
			switch (bristolJackSessionCheck(&audiomain)) {
				case 1: // Save
					midiThreadSaveReq(&audiomain);
					break;
				case 2: // Save and Quit
					midiThreadSaveReq(&audiomain);
					audiomain.atReq = BRISTOL_REQSTOP;
					break;
				case 3: // Save Template
					midiThreadSaveReq(&audiomain);
					break;
				case 4: // Presumptious load Template
					//if (audiomain.debuglevel)
						printf("potentially precocious jsm load request\n");
					midiThreadLoadReq(&audiomain);
					break;
			}
		}
#endif
	}

	if (audiomain.atStatus == BRISTOL_FAIL)
		printf("audio thread failed: exiting.\n");
	else
		printf("audio thread returned: exiting.\n");

	if (audiomain.flags & BRISTOL_JACK)
	{
		audiomain.atReq = BRISTOL_REQSTOP;
#ifdef _BRISTOL_JACK
		bristolJackInterface(NULL);
#endif
	}

	if (audiomain.flags & BRISTOL_PULSE_T)
	{
		audiomain.atReq = BRISTOL_REQSTOP;
#ifdef BRISTOL_PA
		bristolPulseInterface(NULL);
#endif
	}

	/* Unregister the jack interface */
	if ((jh >= 0) && (audiomain.flags & BRISTOL_MIDI_JACK)
		&& (audiomain.flags & BRISTOL_JACK_DUAL))
		bristolMidiClose(jh);

	/* If exec was executed then we should give the GUI a chance to cleanup */
	if ((execreq) && ((cpid = wait(&pstatus)) > 0))
		printf("GUI id %i returned %i\n", cpid, pstatus);

	if (audiothread)
	{
		if (pthread_join(audiothread, NULL) != 0)
			printf("audiothread exit error\n");
		else
			printf("audiothread exited\n");
	}

	if (pthread_join(midithread, NULL) != 0)
		printf("midithread exit error\n");
	else
		printf("midithread exited\n");

	printf("bristol parent exiting\n");
	printf("terminating logging thread\n");

	sleep(1);

	bristolOpenStdio(BRISTOL_LOG_TERMINATE);

	if ((logthread != 0) && (pthread_join(logthread, NULL) != 0))
		return(-1);

	return(0);
}

pthread_t spawnThread(void * (*threadcode)(void *), int priority)
{
	int retcode;
	pthread_t thread;
	int policy;

#if defined(linux)
	struct sched_param schedparam;
#endif

	/*
	 * Create thread, detach it.
	 */
	retcode = pthread_create(&thread, NULL, threadcode, &audiomain);
	if (retcode != 0)
		fprintf(stderr, "create a failed %d\n", retcode);

//	retcode = pthread_detach(thread);
//	if (retcode != 0)
//		fprintf(stderr, "detach failed %d\n", retcode);

#if defined(linux)
	if (pthread_getschedparam(thread, &policy, &schedparam) != 0)
	{
		printf("could not get thread schedule\n");

		return(thread);
	}
#endif

	/*
	 * This may have to go into a subroutine as the midi thread may need to 
	 * reuse it to ensure we do not get priority inversion. At the moment both
	 * threads are RT FIFO though.
	 */
#if defined(linux)
	if (priority != 0)
	{
		policy = SCHED_FIFO;
		schedparam.__sched_priority = priority;

		if (pthread_setschedparam(thread, policy, &schedparam) == 0)
			printf("rescheduled thread: %i\n", schedparam.__sched_priority);
		else
			printf("could not reschedule thread\n");
	} else
		printf("no requested to reschedule thread\n");
#else
	printf("will not reschedule thread\n");
#endif

	return(thread);
}

void
midithreadexit()
{
	printf("Null palette\n");
	audiomain.atReq = BRISTOL_REQSTOP;
	audiomain.mtStatus = BRISTOL_EXIT;

	bristolMidiTerminate();

	pthread_exit(0);
}

void
inthandler()
{
	int i = 50;

	if (audiomain.debuglevel)
		printf("inthandler\n");

	bristolMidiOption(0, BRISTOL_NRP_MIDI_GO, 0);

	audiomain.atReq = BRISTOL_REQSTOP;
	exitReq = 1;

	for (; i > 0; i--)
	{
		if (audiomain.atStatus != BRISTOL_EXIT)
			usleep(100000);
	}

	bristolMidiTerminate();
}

void
alterAllNotes(Baudio *baudio)
{
	bristolVoice *voice = audiomain.playlist;

	while (voice != NULL)
	{
		if (voice->baudio == baudio)
			doNoteChanges(voice);

		voice = voice->next;
	}
}

