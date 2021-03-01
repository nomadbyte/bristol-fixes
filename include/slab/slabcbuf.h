
/*
 *  Diverse SLab Include Files
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
 */

#ifndef SLAB_CBUF
#define SLAB_CBUF

typedef struct TrackDesk {
	int trackCount;
	int resolution;
	int sampleRate;
	int writeSampleRate;
	int trackLength; /* in samples */
	enum DiskStatus diskstatus;
} trackdesc;

/*
 * This is a rather large structure, but carries all information for 
 * daemon synchronisation.
 * MARK: put the mixiod data into structures, add arrays for multiple disk
 * support.
 * Want to add more "version" integers, one per daemon.
 */
typedef struct cBuff {
	int structVersion;
	int releaseVersion;
#ifdef DAEMON_VERS
	int engineVersion;
	int mixengineVersion;
	int mixiodVersion;
	int adiodVersion;
#ifdef MIDI
	int midiEngineVersion;
#endif
#endif
#ifdef NEW_DBG
	int debugLevel;
#endif
	int mixdPID;		/* PID of mother process - responsible for CLI */
	int mixiodPID;		/* Mixer disk IO daemon */
	int mixEnginePID;	/* the actual workhorse */

	char *diskBuffer;	/* disk buffer */
	int diskBufferSize;	/* disk buffer size */
	int diskBufferFD;

	int loadSegSize;	/* size of data to fetch per reqest */
	int preLoadSize;	/* amount of buffer to maintain filled */
	int preMixSize;		/* amount of output buffer to initialise before start */
	int EndOfFile;

	int mixAlgo;
	int subMixAlgo;
	int outputMixAlgo;
	int inputAlgo;
	int outputAlgo;
	int subRecordAlgo;
	int filterAlgo;
#ifdef SB_ONR
	int sbONrAlgo;
#endif
#ifdef SB_NR
	int sbNrAlgo;
#endif
	int compressionClass;
	int decompressionMethod;
	char *compressionBuffer;

	/*
	 * This is a performance tuning issue. It controls the number of windows
	 * we build into the output buffer. Default should be 4, or
	 * OUTPUT_BUFFER_WINDOW, but we should allow lower configurations.
	 */
	int audioWindowSize;

	int conversionFlags;

	mixAlgo mixAlgorithm[ALGO_COUNT];
	mixAlgo subMixAlgorithm[ALGO_COUNT];
	/* Added the next two for eventual speed/sample conversions
	 * although the function will probably be left to the IO daemons */
	mixAlgo subRecordAlgorithm[ALGO_COUNT]; /* Demultiplexes inputs */
	mixAlgo outputMixAlgorithm[ALGO_COUNT];
#ifdef FLOAT_PROC
	mixAlgo busFloatAlgo[FB_ALGO_COUNT];
	int fbDepth;
	float *floatBuffer;
#endif

	int	cindex;
	cAlgo compression[COMP_CLASS_SIZE][COMP_INDEX_SIZE];
	cAlgo decompression[COMP_CLASS_SIZE][COMP_INDEX_SIZE][DECOMP_METHOD_SIZE];

	mixAlgo dynamicsAlgorithm[ALGO_COUNT];
	mixAlgo filterAlgorithm[ALGO_COUNT];

	int startOffset;	/* begin reading from disk at this location in file */
	int currentROffset;	/* begin reading from disk at this location in file */
	int currentWOffset;	/* begin write to disk at this location in file */
	int endOffset;		/* stop reading from disk at this location in file */
	int lseeked;

	char * dioPRead;	/* where the engine process is mixing from */
	char * dioPWrite;	/* where the engine is currently reocrding to */
	char * dioCRead;	/* where the mixiod process is loading to = from disk */
	char * dioCWrite;	/* where the mixiod is currently read from = to disk */

	int dioPReadBytes;	/* amount of data read from buffer by parent */
	int dioPWriteBytes;	/* amount of data written to buffer by parent */
	int dioCReadBytes;	/* amount of data read to buffer by child */
	int dioCWriteBytes;	/* amount of data written from buffer by child */

	duplexDev adiod[MAX_DEVICES + 1];

	int *busBuffer;
	int busBufferSize;
	busparams busParams[BUS_TRACK_COUNT];
#ifdef FX_CHAINS
#ifdef FLOAT_PROC
	/* 
	 * If we are going to use FLOATs, then we need to have indeces for each 
	 * track, rather than for each bus.
	 */
	char trackStart[MAX_TRACK_COUNT]; /* index of first busParamPtr in chain */
	char trackNext[MAX_TRACK_COUNT]; /* index of next busParamPtr to chain */
	int trackCount[MAX_TRACK_COUNT]; /* Ensure termination of this loop */
#endif
	char busStart[BUS_REAL_COUNT]; /* index of first busParamPtr in chain */
	char busNext[BUS_REAL_COUNT]; /* index of next busParamPtr to chain */
	int busCount[BUS_REAL_COUNT]; /* Ensure termination of this loop */
	int *fxBuffer[BUS_REAL_COUNT]; /* Ensure termination of this loop */
#endif

	int mixEngineCommand;	/* mix operation to be executed */
	int mixiodCommand;		/* disk operation to be executed */

	/*
	 * Record offsets are used since we only expect to be able to record
	 * one or two track simultaniously. To save disk write operations
	 * we only write the data for the recording tracks, not all tracks.
	 * These are specified as two offsets of recordSegSize, 3 and 4 are not
	 * yet supported.
	 * DEPRECATED recordOffset moved to DuplexDev.
	 */
	int blockSampleSize;/* Number of samples in a given recorded segment */
	int recordSegSize;	/* size of segment to be moved per track, in bytes */

	/*
	 * COMPRESSION: Added for compression code.
	 */
	int dioRealReadBytes;	/* Before decompression */
	int dioRealWriteBytes;	/* After compression */
	int dioReadSegSize;		/* After comdec algorithm */
	int dioWriteSegSize;	/* size of segment to be synchronised to disk */
	int factor;				/* current expected compression ratio */

	int diskBufferWindow;	/* amount of buffer to maintain filled */
	enum mixIOstatus mixEngineStatus;	/* mixiod operational status */
	enum mixIOstatus mixiodStatus;	/* mixiod operational status */

	/*
	 * These take values of 100K (usleep).
	 * These MUST be calculated: if the BSS is 8192, we can reasonably sleep
	 * for about 100ms when sampling rate is 44100. However, if BSS is 4096,
	 * the delay should be half that. There are similar issues with sample
	 * rate. Calculation should be something like:
	 * (blockSampleSize * 1000000 / sampleRate) us. FFS.
	 * In addition, the sleep period should be self tuning - if we wake up,
	 * and there is a window, reduce the sleep period. If we wake up and there
	 * is still no window (whilst running) increase the sleep period. Need to
	 * be careful with idle situations. FFS.
	 */
	unsigned long mixiodSleepPeriod;
	unsigned long mixEngineSleepPeriod;

	/*
	 * These are saved in the controlBuffer, and should only be set once.
	 * diskBufferID will eventually move into arrays.
	 */
	int controlSegID;
	int diskBufferID;
	int busBufferID;
	/*
	 * This is for track descriptions.
	 * Used for track naming, sample frequency, track counts, bit
	 * resolution etc.etc.
	 *
	 * NOTES:MARK: Not sure where this is supposed to be saved yet.....
	 */
	trackdesc trackDesc;
#ifdef SLAB_SEPARATE
	int mixerSize;
#endif
	/*
	 * Assign 3 extra trackparams for the Master and master LR
	 */
	trackparams trackParams[ALL_TRACKS];
#ifdef I_NR
	/*
	 * For input gating and NR we need seperate algo pointers (handled in
	 * mixiod, rather than mixengined, and some record parameters.
	 * This stuff should be moved to the DuplexDev structure.
	 */
	mixAlgo inputDynamics[ALGO_COUNT];
	mixAlgo inputFilter[ALGO_COUNT];
#endif
	char trackName[DEV_NAME_LEN];
#ifdef MASTERING
	char masteringFileName[DEV_NAME_LEN];
	int masteringFD;
#endif

	int XTalk;
	int wowFlutter;
	int scratchLevel;	/* The volume of the scratchs */
	int scratchDensity;	/* Number of scratches applied per sampleBlockSize */
	int scratchAlgo;	/* Stereo, mono or hybrid scratch */
	int scratchInited;

	/*
	 * The following are used for merge, normalisation, reverse, operations etc.
	 * As per 1.0 and pre release, these edit operations are handled by the
	 * frontend.
	 */
	int srcTrack;
	int srcStart;
	int srcVolume;
	int dstTrack;
	int dstStart;
	int dstVolume;
	int count;
	int editStat;
	int editCommand;

	/*
	 * Fixed VU Meter counters. These should be moved into respective
	 * device structures.
	 * THESE HAVE BEEN MOVED TO THE DUPLEX DEV STRUCTURE.
	int inputLMax;
	int inputLAve;
	int inputRMax;
	int inputRAve;
	int inputLMax2;
	int inputLAve2;
	int inputRMax2;
	int inputRAve2;
	int outputLMax;
	int outputLAve;
	int outputRMax;
	int outputRAve;
	int outputLMax2;
	int outputLAve2;
	int outputRMax2;
	int outputRAve2;
	 */
#ifdef MIDI
	MidiMain midiMain;
	MidiTrack midiTrack[MIDI_TRACK_COUNT];
	MidiDev midiiod[MAX_DEVICES];
#endif
#ifdef METRONOME
	int metroInsertStrong;	/* Insert metronome bar pulse on this interval. */
	int metroInsertWeak;	/* Insert metronome beat pulse on this interval. */
	int metroInsertLevel;	/* Signal "blip" to insert */
	int metroMajor;			/* beats per bar */
	int metroMinor;			/* bar count */
	int metroBpm;			/* beats per minute */
	int metroAdjust;		/* beats per minute */
#endif
#ifdef MICROADJUST
	/*
	 * These are used for microTape speed adjustments, which are global.
	 */
	int microAdjust;
	int microCurrentOut;
	int microCurrentIn;
#endif
#ifdef CONTROL_API
	int remoteSocketDescriptor;
	char *interp; /* This should be Tcl_Interp, but this will hold it */
#endif
#ifdef CONTROL_FLOAT
	fbop fullBusAlgo[32];
#endif
} controlbuffer;

#endif /* SLAB_CBUF */
