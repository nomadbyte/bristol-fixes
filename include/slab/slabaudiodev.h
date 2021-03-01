
/*
 *  Diverse SLab audio routines.
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


#ifndef SLAB_DEVS
#define SLAB_DEVS

#include "slabrevisions.h"
#include "slabdefinitions.h"
#include "slabtrack.h"
#include "slabmixer.h"

#if (BRISTOL_HAS_ALSA == 1)
#include "slabalsadev.h"
#endif

/* ALSA has the value 0x04 */
#ifdef BRISTOL_PA
#ifndef AUDIO_PULSE
#define AUDIO_PULSE 0x10
#endif

/*
 * These make up the threaded interface, the Interface call never returns, it
 * calls the audio shim with each buffer of data.
 */
extern int bristolPulseClose();
extern int bristolPulseInterface();

/*
 * And these are the simple interface, this sits underneath libbristolaudio so
 * has the same interface as ALSA and OSS. The devRead() call is actually null,
 * it just zeros the buffer and continues.
 */
extern int pulseDevOpen();
extern int pulseDevClose();
extern int pulseDevRead();
extern int pulseDevWrite();
#endif

/*
 * I want these out of the compilation
#include <sys/types.h>

#ifdef SLAB_MIDI_SYNC
#include <sys/time.h>
#endif
 */

#include <stdio.h>

#define SYNC_SAMPLES 21

#define DEV_NAME_LEN	128
#define ALGO_COUNT		16

#define	SLAB_RDONLY			11 /* 0xB */
#define SLAB_WRONLY			12 /* 0xC */
#define SLAB_FULL_DUPLEX	13 /* 0xD */
#define SLAB_HALF_DUPLEX	14 /* 0xE */
#define SLAB_HOLD			15 /* 0xF */
#define SLAB_FLAG_MASK		0x000f

#define SLAB_NO_CONTROLS	0x0010 /* prevent /dev/audio or /dev/audioctl ioctl */
/* SB_DUPLEX has taken the 0x20 flag position */
#define SLAB_8_BIT_IN		0x0040 /* For general convergence options */
#define SLAB_8_BIT_OUT		0x0080 /* For general convergence options */
#ifdef SUBFRAGMENT
#define SLAB_SUBFRAGMENT	0x0100 /* decouple blockSampleSize from fragemnts */
#define SLAB_SUBF_IOCTL		0x2000 /* Allow subfragment to use old ioctl()s */
#endif
#ifdef IOCTL_DBG
#define SLAB_AUDIODBG	0x0200 /* For ioctl() debug info */
#endif
#define SLAB_FDUP		0x0400
#define SLAB_AUDIODBG2	0x80000000 /* For verbose debug info */

#ifdef SB_ONR
#define SLAB_NO_ONR	1
#define SLAB_1_BIT_ONR	2
#define SLAB_2_BIT_ONR	3
#define SLAB_3_BIT_ONR	4
#define SLAB_MAX_ONR	4
#endif

#ifdef METRONOME
#define INPUT_ADJUST	/* Retiming of input signal for synchronisation */
#define METRO_RECORD 0x40
#define METRO_MIN 100
#endif

/*
 * These should be MIX_C_GO_???
 */
#define ADIOD_INPUT			11
#define ADIOD_OUTPUT		12
#define ADIOD_DUPLEX		13

/*
 * structure definition made here, this is arrayed into the controlBuffer, one
 * entry per device supported, currently 8.
 */
typedef struct DuplexDev {
	int PID;			/* Analogue/Digital Input/Output Daemon */
	int devID;			/* This device identifier index */
	int BufferKey;		/* SHMEM Key */
	int samplecount;
	char *IBuffer;		/* analogue input buffer */
	int IBufferSize;	/* analogue output buffer size */
	int IBufferID;
	int ISegmentSize;	/* size of analogue output segments */
	char *OBuffer;		/* analogue output buffer */
	int OBufferSize;	/* analogue output buffer size */
	int OBufferID;
	int OSegmentSize;	/* size of analogue output segments */
	int OWorkSpaceSize;	/* For output fragmentation */
	/*
	 * PWrite is for mixengined to adiod. CRead is for adiod from mixengined
	 * to audio device.
	 */
	int PWriteBytes;	/* how much data written to buffer by parent */
	int CWriteBytes;	/* how much data written to device by adiod */
	char *PWrite;		/* where the engine is currently mixed to */
	char *CRead;		/* from where the adiod process is audio outing */
#ifdef FLOAT_SYNC_FIX
	char *mainPWrite;	/* Where the main mix is, regarding bus return mix */
	int mainPWriteBytes;	/* Counter of where the main mix is */
#endif
	int UnderRunCount;	/* Parent cannot provide data in time for write */
	/*
	 * CWrite is adiod into diskBuffer. PRead is mixiod into disk data file.
	 * The window mechanism of Bytes count is used to allow mixiod to determine
	 * how much data from this adiod can be synchronised to disk.
	 */
	int CReadBytes;		/* how much data read from device by adiod */
	int PReadBytes;		/* how much data read from buffer by mixiod */
	int realReadBytes;	/* how much data really read from device by adiod */
	char *PRead;		/* where the data is currently saved from */
	char *CWrite;		/* where the adiod process is audio inning */
	int OverRunCount;	/* Parent cannot clear out read audio data */
	int recordOffset1;
	int recordOffset2;
	int track1;
	int track2;
	int fd;				/* output process file descriptor */
	int fd2;			/* input process file descriptor */
	int mixerFD;		/* mixer device file descriptor */
	mixAlgo inputAlgorithm[ALGO_COUNT];
	mixAlgo outputAlgorithm[ALGO_COUNT];
	char devName[DEV_NAME_LEN];	/* output process file name */
	char mixerName[DEV_NAME_LEN];	/* mixer file name */
	int stereoCaps;
	int monoCaps;
	int recordCaps;
	int genCaps;
	int	cflags; 		/* configured RDONLY, WRONLY, HALF/FULL_DUPLEX */
	int	flags; 			/* active RDONLY, WRONLY, HALF_DUPLEX, FULL_DUPLEX */
	int readSampleRate;	/* For the physical device */
	int writeSampleRate;/* For the physical device */
	int channels;
	/*
	 * for altering analogue device parameters (4Front OSS stuff).
	 */
	int paramID;
	short paramValueLeft;
	short paramValueRight;
	int Command;		/* output operation to be executed */
	int OpState;	/* output operation to be executed */
	int status;			/* adiod operational status */
	unsigned long SleepPeriod;
	int inputLMax;
	int inputLAve;
	int inputRMax;
	int inputRAve;
	int outputLMax;
	int outputLAve;
	int outputRMax;
	int outputRAve;
	short d1;
	short d2;
#ifdef METRONOME
#ifdef INPUT_ADJUST
	int inIndex;	/* Will be used to determine current read slippage */
	int outIndex;	/* Will be used to determine current read index */
	int inAdjust;	/* Defines the number of samples to slip on input */
#endif
#endif
#ifdef SUBFRAGMENT
	int fragSize;
	int fragIndexSave;
	int fragOutIndexSave;
	char *fragBuf;
#endif
#ifdef ADIOD_MULTI
	int skipCount;
	int skipFlag;
#endif
#ifdef I_NR
	/*
	 * Input noise reduction params
	 */
	trackparams inRp[2];
#endif
	int preLoad;
#ifdef FLOAT_PROC
	int masterFloatAlgoLeft[MAX_DEVICES]; /* MAXDEVS is incorrent, there is */
	int masterFloatAlgoRight[MAX_DEVICES];/* relationship, it is just a count */
#endif
#ifdef DELTA_ACC /* For delta accumulation with microAdjust code */
	int lDeltaOut;
	int rDeltaOut;
	int lOldOut;
	int rOldOut;
	int lDeltaIn;
	int rDeltaIn;
	int lOldIn;
	int rOldIn;
#endif
#ifdef SLAB_MIDI_SYNC
	/*
	 * Note that we are going to rely on read rates. Also, these who structures
	 * should be moved into a separate header file in the libslabaudio space to
	 * remove dependencies on the diverse system header files.
	 */
	struct {long int tv_sec, tv_usec;} startTime;
	struct {long int tv_sec, tv_usec;} timeLastRead;
	int deltaReadSamples;
	float estReadRate;
	int syncPreLoad; /* 10 */
	int syncBias; /* 1500 */
	int syncHoldDown; /* 55 */
	int syncAdjust; /* 100 */
	int syncStability; /* internal factor */
#endif
#ifdef SLAB_NET_TAP
	/*
	 * Use device name length since it is already defined.
	 */
	char hostname[DEV_NAME_LEN];
	int port;
	int direction;
	int netfd;
#endif
#ifdef SOFT_START
	int crossFade;
#endif
#ifdef MICROADJUST
	/*
	 * These are used for per device microAdjustments, which will be used to
	 * sync devices.
	 */
	int microAdjust; /* This will be a sum of microAdjust and sync */
	int microSyncAdjust; /* This is just the sync factor */
	int microCurrentOut;
	int microCurrentIn;
#endif
#ifdef SECONDARY_FLAGS
	/*
	 * Intrusive flags, if changed, need an engine restart.
	 */
	int	sflags; /* secondary flags, some used (3.20) */
	int	siflags; /* secondary intrusive flags, some used (3.20) */
	int	tflags; /* tertiary flags (not used at the moment (3.20) */
	int	tiflags; /* tertiary  intrusiveflags (not used at the moment (3.20) */
#endif
	/*
	 * Keep a number of sample times.
	 */
	float sduration;
	struct {long int tv_sec, tv_usec;} realTimes[SYNC_SAMPLES];
	int syncIndex;
	float estimate;
} duplexDev;

#ifndef MONO_CAPABILITIES
#define MONO_CAPABILITIES(x) audioDev->monoCaps
#define STEREO_CAPABILITIES(x) audioDev->stereoCaps
#define ALL_CAPABILITIES(x) (audioDev->stereoCaps | audioDev->monoCaps)
#define ALL_DEV2_CAPABILITIES (audioDev->stereoCaps | audioDev->monoCaps)
#endif

#endif /* SLAB_DEVS */
