
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

/*
 * This file was taken out of the engine and moved to the audio library where
 * it correctly lies however it incorporates a few flags from bristol.h header
 * file that should really be removed to keep them independent. That is for
 * later study.
 */

/*#define DEBUG */

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _BRISTOL_JACK
#if (BRISTOL_HAS_ALSA == 1)
#include <alsa/iatomic.h>
#endif

/*
 * Drop this atomic stuff, it comes from the ALSA library and it not present on
 * a lot of systems. It is failing the Debian hurdles on some pretty wild host
 * architectures admittedly. Now the benefits of a real atomic access here are
 * small: there is one writer changing a value from 1 to zero. There is one
 * reader waiting for it to be zero, even as a non-atomic operation the worst
 * this can do is delay detection by a period.
 */
#ifdef DONT_USE_THIS //IATOMIC_DEFINED // Not very portable
atomic_t closedown = ATOMIC_INIT(1);
#else
int closedown = 1;
#endif

#include "bristol.h"
#include "bristoljack.h"

#ifdef _BRISTOL_JACK_SESSION
#include <jack/session.h>
char sessionfile[1024];
char commandline[1024];
#endif

extern int dupfd;
static char defreg[8] = "bristol";
static char *regname = defreg;

static jackDev jackdev;
static float *outbuf, *inbuf;

static int fsize = sizeof(jack_default_audio_sample_t);

#ifdef _BRISTOL_JACK_MIDI
extern void bristolJackSetMidiHandle(jack_client_t *);
extern int jackMidiRoutine(jack_nframes_t, void *);
#endif

static void
jack_shutdown(void *jackdev)
{
	((jackDev *) jackdev)->audiomain->atReq = BRISTOL_REQSTOP;
}

#ifdef _BRISTOL_JACK_SESSION
/*
 * This is polled by the GUI to see if any session events need handling. It is
 * called fromm the idle loop of the engine parent thread.
 */
int
bristolJackSessionCheck(audioMain *audiomain)
{
	int rval = 0;

	if ((jackdev.sEvent == NULL) || (jack_set_session_callback == NULL))
		return(0);

	snprintf(sessionfile, 1024, "%s%s", jackdev.sEvent->session_dir, regname);
	audiomain->sessionfile = sessionfile;

	snprintf(commandline, 1024, "%s -jsmfile \"${SESSION_DIR}%s\" -jsmuuid %s",
		audiomain->cmdline, regname, jackdev.sEvent->client_uuid);

	jackdev.sEvent->command_line = bristolmalloc(strlen(commandline) + 1);
	sprintf(jackdev.sEvent->command_line, "%s", commandline);

	/*
	 * Need to flag the event for outside handling
	 */
	if (audiomain->debuglevel > 1)
	{
		if (jackdev.audiomain->jackUUID[0] != '\0')
			printf("jack session callback: %s %s\n",
				jackdev.sEvent->client_uuid,
				jackdev.audiomain->jackUUID);
		else
			printf("jack session callback: %s\n", jackdev.sEvent->client_uuid);

		printf("session file is %s\n", audiomain->sessionfile);

		printf("command line is %s\n", jackdev.sEvent->command_line);
	}

	rval = jackdev.sEvent->type;

	jack_session_reply(jackdev.handle, jackdev.sEvent);

	/* This was not malloc'ed do does not belong to jack session manager */
	jack_session_event_free(jackdev.sEvent);
	jackdev.sEvent = NULL;

	return(rval);
}

void
jack_session_callback(jack_session_event_t *event, void *arg)
{
	if (jack_set_session_callback == NULL)
		return;

	if (jackdev.sEvent != NULL)
		jack_session_event_free(jackdev.sEvent);

	/* This is not a very good signaling method, it has race conditions */
	jackdev.sEvent = event;
}
#endif

static void
bufstats(float *buf, int nframes, int stage)
{
	float max = 0.0f, min = 1.0f;

	if (buf == NULL)
		return;

	for (; nframes > 0; nframes-= 8)
	{
		max = *buf > max? *buf:max; min = *buf < min? *buf:min; buf++;
		max = *buf > max? *buf:max; min = *buf < min? *buf:min; buf++;
		max = *buf > max? *buf:max; min = *buf < min? *buf:min; buf++;
		max = *buf > max? *buf:max; min = *buf < min? *buf:min; buf++;
		max = *buf > max? *buf:max; min = *buf < min? *buf:min; buf++;
		max = *buf > max? *buf:max; min = *buf < min? *buf:min; buf++;
		max = *buf > max? *buf:max; min = *buf < min? *buf:min; buf++;
		max = *buf > max? *buf:max; min = *buf < min? *buf:min; buf++;
	}
	printf("%i %p: min %f, max %f\n", stage, buf, min, max);
}

static int
audioShim(jack_nframes_t nframes, void *jd)
{
	jackDev *jackdev = (jackDev *) jd;
	register float *outL, *outR, *toutbuf = outbuf, gain;
	register int i, nint;

#ifdef _BRISTOL_JACK_MIDI
	if (~jackdev->audiomain->flags & BRISTOL_JACK_DUAL)
		jackMidiRoutine(nframes, NULL);
#endif

	/*
	 * We may need to consider jack changing its nframes on the fly. Whilst
	 * decreasing frames is not an issue, increasing them could be painful.
	 *
	 * I think I would prefer to reap all synths rather than code such an
	 * operation, or alternatively add in a shim to do some reframing.
	 * #warning need to consider jackd changes to nframes?
	 */
	if (jackdev->jack_in[BRISTOL_JACK_STDINL])
		jackdev->inbuf[BRISTOL_JACK_STDINL]
			= jack_port_get_buffer(jackdev->jack_in[BRISTOL_JACK_STDINL],
				nframes);
	if (jackdev->jack_in[BRISTOL_JACK_STDINR])
		jackdev->inbuf[BRISTOL_JACK_STDINR]
			= jack_port_get_buffer(jackdev->jack_in[BRISTOL_JACK_STDINR],
				nframes);

	if (jackdev->iocount > 0)
	{
		int j;
		float *buf;

		if ((gain = jackdev->audiomain->m_io_igc) <= 0)
			gain = 1.0;

		for (i = 0; i < jackdev->iocount; i++)
		{
			jackdev->inbuf[i] = buf =
				jackdev->audiomain->io_i[i] = 
				jack_port_get_buffer(jackdev->jack_in[i], nframes);

			for (j = 0; j < nframes; j+=8)
			{
				*buf++ *= gain;
				*buf++ *= gain;
				*buf++ *= gain;
				*buf++ *= gain;
				*buf++ *= gain;
				*buf++ *= gain;
				*buf++ *= gain;
				*buf++ *= gain;
			}

			jackdev->outbuf[i] =
				jackdev->audiomain->io_o[i] = 
				jack_port_get_buffer(jackdev->jack_out[i], nframes);

			memset(jackdev->outbuf[i], 0.0f, nframes * sizeof(float));
		}
	}

	toutbuf = inbuf;
	gain = 12.0f;

	for (i = 0; i < nframes; i+=8)
	{
		*toutbuf++ = jackdev->inbuf[BRISTOL_JACK_STDINL][i] * gain;
		*toutbuf++ = jackdev->inbuf[BRISTOL_JACK_STDINL][i] * gain;
		*toutbuf++ = jackdev->inbuf[BRISTOL_JACK_STDINL][i] * gain;
		*toutbuf++ = jackdev->inbuf[BRISTOL_JACK_STDINL][i] * gain;
		*toutbuf++ = jackdev->inbuf[BRISTOL_JACK_STDINL][i] * gain;
		*toutbuf++ = jackdev->inbuf[BRISTOL_JACK_STDINL][i] * gain;
		*toutbuf++ = jackdev->inbuf[BRISTOL_JACK_STDINL][i] * gain;
		*toutbuf++ = jackdev->inbuf[BRISTOL_JACK_STDINL][i] * gain;
	}

	/*
	 * For now we are not going to be too picky about inbuf.
	 *
	 * The buffers are however mono, which is a bit of a bummer as now I have
	 * to do the interleaving and deinterleaving. This could be changed but
	 * it would have to apply to the native audio drivers as well.
	 *
	 * This doAudioOps is the same one used for the last few years by bristol
	 * as its own dispatch routine. However, there are additional features
	 * possible with Jack such that each synth could requisition its own 
	 * ports dynamically, one or two depending on the synth stereo status.
	 *
	 * That should be an option, probably, but either way, if it were done
	 * then we would need to rework this dispatcher. That is not a bad thing
	 * since we could have the audioEngine write directly to the jack buffer
	 * rather than use the stereo interleaved outbuf.
	 * 
	 * The reworked dispatcher should be placed in here, it is currently in
	 * audioEngine.c
	 */
	doAudioOps(jackdev->audiomain, outbuf, inbuf);

	/*
	 * The bristol audio does this with zero suppression. Hm. Saved data is
	 * the output from the algo, it does not have outgain applied.
	 */
	if (dupfd > 0)
		nint = write(dupfd, outbuf, jackdev->audiomain->segmentsize * 2);

	/*
	 * We have an issue with sample formats. These are normalised to 0/1 
	 * whilst we have been working on full range values. No problem.
	 */
	if ((gain = jackdev->audiomain->outgain) < 1)
		gain = 1.0f;
	gain /= 32768.0;

	outL = (float *)
		jack_port_get_buffer(jackdev->jack_out[BRISTOL_JACK_STDOUTL], nframes);
	outR = (float *)
		jack_port_get_buffer(jackdev->jack_out[BRISTOL_JACK_STDOUTR], nframes);

	/*
	 * Deinterleave our output through to the jack buffers.
	 */
	toutbuf = outbuf;
	for (i = nframes; i > 0; i--)
	{
		*outL++ = *toutbuf++ * gain;
		*outR++ = *toutbuf++ * gain;
	}

	if (jackdev->iocount > 0)
	{
		register int j;
		register float *buf, gain;

		if ((gain = jackdev->audiomain->m_io_ogc) <= 0)
			gain = 1.0;

		for (i = 0; i < jackdev->iocount; i++)
		{
			buf = jackdev->outbuf[i];

			for (j = 0; j < nframes; j+=8)
			{
				*buf++ *= gain;
				*buf++ *= gain;
				*buf++ *= gain;
				*buf++ *= gain;
				*buf++ *= gain;
				*buf++ *= gain;
				*buf++ *= gain;
				*buf++ *= gain;
			}

			memset(jackdev->inbuf[i], 0.0f, nframes * sizeof(float));
		}
	}

	if (jackdev->audiomain->debuglevel > 9)
	{
		bufstats(jackdev->outbuf[0], nframes, 1);
		bufstats(jackdev->outbuf[2], nframes, 2);
	}

	return(0);
}

static void
bristolIntJackClose()
{
	int i;

	--closedown;

	printf("closedown on interrupt\n");

	jack_deactivate(jackdev.handle);

	jack_port_unregister(jackdev.handle,
		jackdev.jack_out[BRISTOL_JACK_STDOUTL]);
	jack_port_unregister(jackdev.handle,
		jackdev.jack_out[BRISTOL_JACK_STDOUTR]);
	jack_port_unregister(jackdev.handle,
		jackdev.jack_in[BRISTOL_JACK_STDINL]);
	jack_port_unregister(jackdev.handle,
		jackdev.jack_in[BRISTOL_JACK_STDINR]);

	for (i = 0; i < jackdev.iocount; i++)
	{
		jack_port_unregister(jackdev.handle, jackdev.jack_in[i]);
		jack_port_unregister(jackdev.handle, jackdev.jack_out[i]);
	}

	jack_client_close(jackdev.handle);

	_exit(0);
}

static int
bristolJackClose(jackDev *jackdev)
{
	int i;

	if ((jackdev->handle == NULL) || (jackdev->jack_out[BRISTOL_JACK_STDOUTL] == NULL))
		return(-1);

	/*
	 * So, if we don't have the ALSA atomic headers then just run it on ints,
	 * this bit of code is far from critical but collisions cause some gory
	 * shutdown sequences. The code used to call this shutdown from two 
	 * different threads, removed on of them instead to prevent the possibility
	 * of them battering each other.
	 */
#ifdef DONT_USE_THIS //IATOMIC_DEFINED // Not very portable
	if (atomic_dec_and_test(&closedown))
#else
	if (--closedown == 0)
#endif
		printf("unregistering jack interface: %p->%p\n",
			jackdev, jackdev->handle);
	else {
		printf("interface unregistered\n");
		return(0);
	}

	jack_deactivate(jackdev->handle);

	usleep(100000);

	jack_port_unregister(jackdev->handle,
		jackdev->jack_out[BRISTOL_JACK_STDOUTL]);
	jack_port_unregister(jackdev->handle,
		jackdev->jack_out[BRISTOL_JACK_STDOUTR]);
	jack_port_unregister(jackdev->handle,
		jackdev->jack_in[BRISTOL_JACK_STDINL]);
	jack_port_unregister(jackdev->handle,
		jackdev->jack_in[BRISTOL_JACK_STDINR]);

	for (i = 0; i < jackdev->iocount; i++)
	{
		jack_port_unregister(jackdev->handle, jackdev->jack_in[i]);
		jack_port_unregister(jackdev->handle, jackdev->jack_out[i]);
	}

	jackdev->jack_out[BRISTOL_JACK_STDOUTL] = NULL;

	jack_client_close(jackdev->handle);

	/*
	 * The API does not seem to be clear on who owns this however since we
	 * are exiting.....
	jackdev->handle = NULL;
	 */

	jackdev->audiomain->atReq |= BRISTOL_REQSTOP;
	jackdev->audiomain->mtReq |= BRISTOL_REQSTOP;

	return(0);
}

/*
 * This builds us our port list for connections. Per default we will connect
 * to the first capture port and the first two playback ports, but that 
 * should also be optional. We should have some fixed starting ports though,
 * even if we are going to pass parameters, since when each synth starts 
 * registering its own set of ports then they have to be linked somewhere 
 * by default.
 */
static void
jackPortStats(jackDev *jackdev, int dir)
{
	int i;

	bristolfree(jackdev->ports);

	if ((jackdev->ports = jack_get_ports(jackdev->handle, NULL, NULL,
		JackPortIsPhysical|dir)) == NULL)
	{
		printf("Empty jack_get_ports()\n");
		return;
	}

	for (i = 0; jackdev->ports[i] != NULL; i++)
		printf("Found port %s\n", jackdev->ports[i]);
}

static int
bristolJackOpen(jackDev *jackdev, audioMain *audiomain,
JackProcessCallback shim)
{
	int waitc = 10, sr;

	if (audiomain->audiodev != NULL)
		regname = audiomain->audiodev;

#ifdef _BRISTOL_JACK_SESSION
	if (audiomain->jackUUID[0] == '\0')
	{
		printf("registering jack interface: %s\n", regname);
		jackdev->handle = jack_client_open(regname, JackNullOption, NULL);
	} else {
		printf("reregistering jack interface: %s, UUDI %s\n",
			regname, &audiomain->jackUUID[0]);
		jackdev->handle = jack_client_open(regname, JackSessionID, NULL,
			&audiomain->jackUUID[0]);
	}
#else
	printf("registering jack interface: %s\n", regname);
	jackdev->handle = jack_client_open(regname, JackNullOption, NULL);
#endif

	if (jackdev->handle == 0)
	{
		char process_indexed[32];

		/*
		 * This had to be changed as the process ID is variable between 
		 * invocations. The port number is not, it has to be provided to build
		 * a connection and is unique.
		 */
		snprintf(process_indexed, 32, "%s_%i", regname, audiomain->port);

#ifdef _BRISTOL_JACK_SESSION
		if (audiomain->jackUUID[0] == '\0')
			jackdev->handle = jack_client_open(regname, JackNullOption, NULL);
		else
			jackdev->handle = jack_client_open(regname, JackSessionID, NULL,
				&audiomain->jackUUID[0]);
#else
		jackdev->handle = jack_client_open(regname, JackNullOption, NULL);
#endif

		if (jackdev->handle == 0)
		{
			printf("Cannot connect to jack\n");
			audiomain->atStatus = BRISTOL_REQSTOP;
			return(-1);
		}
	}

#ifdef _BRISTOL_JACK_MIDI
	if (~audiomain->flags & BRISTOL_JACK_DUAL)
		bristolJackSetMidiHandle(jackdev->handle);
#endif

	signal(SIGINT, bristolIntJackClose);
	signal(SIGTERM, bristolIntJackClose);
	signal(SIGHUP, bristolIntJackClose);
	signal(SIGQUIT, bristolIntJackClose);
	/*
	 * This is (was) perhaps not wise....
	 * Thanks to Hagen.
	signal(SIGSEGV, bristolIntJackClose);
	 */

	/* Samplerate mismatches should be reported however they are not critical */
	if (audiomain->samplerate != (sr = jack_get_sample_rate(jackdev->handle)))
		printf("\nJack SAMPLERATE MISMATCH: startBristol -jack -rate %i\n", sr);

	/*
	 * This value can change, and we should register a callback for such an
	 * event. The values are used internally and are already configured for
	 * some parts of the system, hence, for now, if this does not match we
	 * are going to exit as this is critical.
	 */
	if (audiomain->samplecount != (sr = jack_get_buffer_size(jackdev->handle)))
	{
		printf("\nJack PERIOD COUNT MISMATCH: `startBristol -jack -count %i`\n",
			sr);
		printf("\nYou need to ensure that bristol uses the same period size\n");

		/*
		 * This did call bristolJackClose(jackdev), however as we have not
		 * yet done much of the registration then the following is correct:
		 */
		jack_client_close(jackdev->handle);

		audiomain->atStatus = BRISTOL_REQSTOP;
		return(-1);
	}

	audiomain->samplecount = sr;
	audiomain->segmentsize = audiomain->samplecount * sizeof(float);

	/*
	 * We double this value since I am going to be using interleaved samples
	 * for some buffering. I should change this, just let the audio library
	 * do interleave if it is writing to physical devices.
	 */
	outbuf = (float *) bristolmalloc(audiomain->samplecount * fsize * 2);
	inbuf = (float *) bristolmalloc(audiomain->samplecount * fsize * 2);

	audiomain->atStatus = BRISTOL_OK;
	audiomain->flags |= BRISTOL_AUDIOWAIT;

	initAudioThread(audiomain);

	jackdev->audiomain = audiomain;

	jack_set_process_callback(jackdev->handle, shim, (void *) jackdev);

	jack_on_shutdown(jackdev->handle, jack_shutdown, (void *) jackdev);

#ifdef _BRISTOL_JACK_SESSION
	if (jack_set_session_callback)
		jack_set_session_callback(jackdev->handle, jack_session_callback,
			(void *) jackdev);
#endif

	if ((jackdev->jack_out[BRISTOL_JACK_STDOUTL] = jack_port_register(jackdev->handle,
		"out_left", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0)) == NULL)
	{
		printf("Cannot register jack port\n");
		audiomain->atStatus = BRISTOL_REQSTOP;
		return(-1);
	}
	if ((jackdev->jack_out[BRISTOL_JACK_STDOUTR] = jack_port_register(jackdev->handle,
		"out_right", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0)) == NULL)
	{
		printf("Cannot register jack port\n");
		audiomain->atStatus = BRISTOL_REQSTOP;
		return(-1);
	}

	/*
	 * We only use the input for a couple of the synths (Mini/Explorer) and
	 * even that had limited use. It was also employed historically for sync
	 * - read/modify/write sequences. We could drop this registration, but it
	 * can stay in for now, it will be used in the future.
	 */
	if ((jackdev->jack_in[BRISTOL_JACK_STDINL] = jack_port_register(jackdev->handle,
		"in_left", JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, 0)) == NULL)
	{
		printf("Cannot register jack port\n");
		audiomain->atStatus = BRISTOL_REQSTOP;
		return(-1);
	}
	if ((jackdev->jack_in[BRISTOL_JACK_STDINR] = jack_port_register(jackdev->handle,
		"in_right", JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, 0)) == NULL)
	{
		printf("Cannot register jack port\n");
		audiomain->atStatus = BRISTOL_REQSTOP;
		return(-1);
	}

	/*
	 * Our engine is going have to wait for the GUI to inform us what synth
	 * we are going to be running. It is wiser not to request callbacks until
	 * at least the first synth is active. If the priorities are correct this
	 * should not be an issue, but 'dropping' a synth costs a few cycles in 
	 * the midi thread.
	 */
	while (audiomain->flags & BRISTOL_AUDIOWAIT)
	{
		sleep(1);
		if (--waitc <= 0)
		{
			printf("Did not receive request from GUI, exiting.\n");
			/*
			 * We should really unregister here.....
			 */
			return(-1);
		}
	}

	if (jack_activate(jackdev->handle) != 0)
	{
		printf("Cannot activate jack\n");
		audiomain->atStatus = BRISTOL_REQSTOP;
		return(-1);
	}

	jackPortStats(jackdev, JackPortIsInput);

	if ((audiomain->flags & BRISTOL_AUTO_CONN) && (jackdev->ports != NULL))
	{
		int jport = 0;
		char *lport, *rport;

		lport = getenv("BRISTOL_AUTO_LEFT");
		rport = getenv("BRISTOL_AUTO_RIGHT");

		if ((lport != NULL) && (rport != NULL))
		{
			/*
			 * We should scan through the list looking for our ports
			 */
			for (jport = 0; jackdev->ports[jport] != NULL; jport++) {
				if (strcmp(lport, jackdev->ports[jport]) == 0)
				{
					if (jack_connect(jackdev->handle,
						jack_port_name(jackdev->jack_out[BRISTOL_JACK_STDOUTL]),
							jackdev->ports[jport]) != 0)
					{
						printf("Bristol Failed Conn: %s to %s failed\n",
							jack_port_name(jackdev->jack_out[BRISTOL_JACK_STDOUTL]),
							jackdev->ports[jport]);
					} else
						printf("Bristol Env AutoConn: %s to %s\n",
							jack_port_name(jackdev->jack_out[BRISTOL_JACK_STDOUTL]),
							jackdev->ports[jport]);
				}
				if (strcmp(rport, jackdev->ports[jport]) == 0)
				{
					if (jack_connect(jackdev->handle,
						jack_port_name(jackdev->jack_out[BRISTOL_JACK_STDOUTR]),
							jackdev->ports[jport]) != 0)
					{
						printf("Bristol Failed Conn: %s to %s failed\n",
							jack_port_name(jackdev->jack_out[BRISTOL_JACK_STDOUTR]),
							jackdev->ports[jport]);
					} else
						printf("Bristol Env AutoConn: %s to %s\n",
							jack_port_name(jackdev->jack_out[BRISTOL_JACK_STDOUTL]),
							jackdev->ports[jport]);
				}
			}
		} else for (jport = 0; jackdev->ports[jport] != NULL; jport++) {
			/*
			 * I don't like this code but I cannot find a way to ask jack if
			 * this port is audio or midi. Without this scan then it is possible
			 * that the engine will default a connection from audio to midi.
			 *
			 * It is okay here to attempt the connection but continue through
			 * the list in the event of failure but that is sloppy. All we
			 * actually want is to find the first two available ports to return
			 * audio to and one to receive from the daemon.
			 */
			if (strstr(jackdev->ports[jport], "midi") != 0)
			{
				printf("Skipping Jack Conn: %s\n", jackdev->ports[jport]);
				continue;
			}

			if (jack_connect(jackdev->handle,
				jack_port_name(jackdev->jack_out[BRISTOL_JACK_STDOUTL]),
					jackdev->ports[jport]) != 0)
			{
				printf("Bristol Defaulted Conn: %s to %s failed\n",
					jack_port_name(jackdev->jack_out[BRISTOL_JACK_STDOUTL]),
					jackdev->ports[jport]);
				continue;
			} else
				printf("Bristol Defaulted Conn: %s to %s\n",
					jack_port_name(jackdev->jack_out[BRISTOL_JACK_STDOUTL]),
					jackdev->ports[jport]);

			for (jport++; jackdev->ports[jport] != 0; jport++)
			{
				if (strstr(jackdev->ports[jport], "midi") != 0)
				{
					printf("Skipping Jack Conn: %s\n", jackdev->ports[jport]);
					continue;
				}

				if (jack_connect(jackdev->handle,
					jack_port_name(jackdev->jack_out[BRISTOL_JACK_STDOUTR]),
						jackdev->ports[jport]) != 0)
				{
					printf("Bristol Defaulted Conn: %s to %s failed\n",
						jack_port_name(jackdev->jack_out[BRISTOL_JACK_STDOUTR]),
						jackdev->ports[jport]);
					continue;
				} else
					printf("Bristol Defaulted Conn: %s to %s\n",
						jack_port_name(jackdev->jack_out[BRISTOL_JACK_STDOUTR]),
						jackdev->ports[jport]);
				break;
			}

			break;
		}
	}

	/*
	 * We now need to connect the ports together. For now this will be fixed
	 * but actually needs to be a set of options.
	 */
	jackPortStats(jackdev, JackPortIsOutput);

	if ((audiomain->flags & BRISTOL_AUTO_CONN) && (jackdev->ports != NULL))
	{
		int jport = 0;
		char *iport;

		iport = getenv("BRISTOL_AUTO_IN");

		if (iport != NULL)
		{
			/*
			 * We should scan through the list looking for our ports
			 */
			for (jport = 0; jackdev->ports[jport] != NULL; jport++)
			{
				if (strcmp(iport, jackdev->ports[jport]) == 0)
				{
					if (jack_connect(jackdev->handle, jackdev->ports[jport],
						jack_port_name(jackdev->jack_in[BRISTOL_JACK_STDINL])) != 0)
					{
						/*
						 * We don't actually need to fail any of these, it just
						 * means we have no defaults.
						 */
						printf("Bristol Env AutoConn: %s to %s failed\n",
							jackdev->ports[jport],
							jack_port_name(jackdev->jack_in[BRISTOL_JACK_STDINL]));
						continue;
					} else
						printf("Bristol Env AutoConn: %s to %s\n",
							jackdev->ports[jport],
							jack_port_name(jackdev->jack_in[BRISTOL_JACK_STDINL]));
				}
			}
		} else for (jport = 0; jackdev->ports[jport] != 0; jport++)
		{
			if (strstr(jackdev->ports[jport], "midi") != 0)
			{
				printf("Skipping Jack Conn: %s\n", jackdev->ports[jport]);
				continue;
			}

			if (jack_connect(jackdev->handle, jackdev->ports[jport],
				jack_port_name(jackdev->jack_in[BRISTOL_JACK_STDINL])) != 0)
			{
				/*
				 * We don't actually need to fail any of these, it just means
				 * we have no defaults.
				 */
				printf("Bristol Defaulted Conn: %s to %s failed\n",
					jackdev->ports[jport], jack_port_name(jackdev->jack_in[BRISTOL_JACK_STDINL]));
				continue;
			} else
				printf("Bristol Defaulted Conn: %s to %s\n",
					jackdev->ports[jport], jack_port_name(jackdev->jack_in[BRISTOL_JACK_STDINL]));

			break;
		}
	}

	/*
	 * So now see if we were requested to register more ports.
	 */
	if ((jackdev->iocount = audiomain->iocount) > 0)
	{
		int i;
		char pn[256];

		for (i = 0; i < jackdev->iocount; i++)
		{
			sprintf(pn, "out_%i", i + 1);
			if ((jackdev->jack_out[i] = jack_port_register(jackdev->handle,
				pn, JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0)) == NULL)
			{
				printf("Cannot register jack port\n");
				audiomain->atStatus = BRISTOL_REQSTOP;
				return(-1);
			}
			sprintf(pn, "in_%i", i + 1);
			if ((jackdev->jack_in[i] = jack_port_register(jackdev->handle,
				pn, JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, 0)) == NULL)
			{
				printf("Cannot register jack port\n");
				audiomain->atStatus = BRISTOL_REQSTOP;
				return(-1);
			}
		}
	}

	return(0);
}

/*
 * Test dynamic disconnect and reconnects. Eventually each synth will
 * potentially register its own set of ports. Want to do this with a
 * reasonably large number of iterations. Correctly speaking we should
 * probably take a copy of the jack_in port, null the jackdev entry
 * and then unregister it.
static void
jackPortBounce(jackDev *jackdev)
{
	jack_disconnect(jackdev->handle,
		jackdev->ports[0], jack_port_name(jackdev->jack_in[BRISTOL_JACK_STDINL]));

	if (jack_port_unregister(jackdev->handle, jackdev->jack_in[BRISTOL_JACK_STDINL]) == 0)
	{
		printf("Unregistered port\n");
		jackdev->jack_in[BRISTOL_JACK_STDINL] = 0;

		sleep(5);

		if ((jackdev->jack_in[BRISTOL_JACK_STDINL] = jack_port_register(jackdev->handle,
			"in", JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, 0)) != NULL)
			printf("Registered port\n");
	} else
		sleep(5);

	jack_connect(jackdev->handle,
		jackdev->ports[0], jack_port_name(jackdev->jack_in[BRISTOL_JACK_STDINL]));
}

jack_client_t *
bristolJackGetAudioHandle()
{
	if (jackdev.audiomain == NULL)
		sleep(1);
	if (jackdev.audiomain->flags & BRISTOL_JACK_DUAL)
		return(NULL);
	sleep(1);
	return(jackdev.handle);
}
 */

int
bristolJackInterface(audioMain *audiomain)
{
	/*
	 * This was added into 0.40.3 to prevent subgraph timeouts on exit. The
	 * call is made from the midi thread once the audio thread has done the 
	 * necessary cleanup work.
	 */
	if ((audiomain == NULL) || (audiomain->audiolist == 0))
		/*
		 * If the audiolist is empty (or we are called with a null pointer)
		 * then assume we are being requested to close down the interface.
		 */
		return(bristolJackClose(&jackdev));

	if (bristolJackOpen(&jackdev, audiomain, audioShim) != 0)
		return(-1);

	while (audiomain->atReq != BRISTOL_REQSTOP)
		sleep(1);

	return(0);
}
#endif /* _BRISTOL_JACK */

