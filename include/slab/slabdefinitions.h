
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

#ifndef SLAB_DEFS
#define SLAB_DEFS

#ifdef SECONDARY_FLAGS
#define SAMPLE_RATE_SYNC 1
#define DEV_DISABLE 2

#if (BRISTOL_HAS_ALSA == 1)
#define AUDIO_ALSA 4
#endif
#endif

#define AUDIO_DUMMY 0x8000

#ifdef CD_WRITE
#define ISO_OPTS_COUNT 7
#endif

#ifdef CD_RECORD
#define ISO_OPTS_COUNT 6
#endif

/* 
 * This is to link into the libaudio routines. libslabaudio is provided in
 * source code to improve compatibility with different audio drivers.
 */
#ifdef AUDIO_API
#define SL_SETAUDIODEVPARAM SL_setAudioDevParam2(&controlBuffer->adiod[0],
#else
#define SL_SETAUDIODEVPARAM SL_setAudioDevParam(
#endif

#ifdef MASTERING
#define MASTERING_MASK	0x00ff00
#define MASTER_CDR		0x100/* output daemon support for cdr master files */
#ifdef MASTERING_WAV
#define MASTER_WAV		0x200/* output daemon support for wav master files */
#endif
#ifdef MASTERING_MP3
#define MASTER_MP3		0x400/* output daemon support for mp3 master files */
#endif
#define MASTER_RES3		0x1000/* output daemon support for some master files */
#define MASTER_RES4		0x2000/* output daemon support for some master files */
#define MASTER_RES5		0x4000/* output daemon support for some master files */
#define MASTER_RES6		0x8000/* output daemon support for some master files */
#endif
#define CD_TRACK_COUNT 32

#ifndef NULL
#define NULL 0
#endif

/*
 * The following are for the audio net tap
 */
#define SLAB_INPUT_TAP 1
#define SLAB_OUTPUT_TAP 2

typedef struct cBuff *ControlBuffer;

typedef struct CAlgo {
	mixAlgo algo;
	short factor;
	short pad1;
} cAlgo;

/*
 * This is not used. The intention is error checking, since a PC can only 
 * reasonably support 4 cards at the moment.
 */
#define MAX_USED_DEVICES 4
/*
 * I want to remove all references to PRIMARY nd SECONDARY devs, and use code
 * that does for (i=0;s<MAX_DEVICES;i++)
 */
#define PRIMARY_DEV		0
#define SECONDARY_DEV	1
#define TERTIARY_DEV	2

#define ALL_DEVICES(x) for (x = 0; x < MAX_DEVICES; x++)
#ifdef ADIOD_MULTI
#define IF_ACTIVE(y, x) if (y->adiod[x].PID > 1)
#define ALL_ACTIVE_DEVICES(y, x) for (x = 0; x < MAX_DEVICES; x++) \
	if (y->adiod[x].PID > 1)
#endif
#define ALL_SLAVE_DEVICES(x) for (x = 1; x < MAX_DEVICES; x++)

#define MAX_TRACK_COUNT	64

#ifdef FX_CHAINS
#ifdef FLOAT_PROC
/*
 * Float proc uses a busTrack for internal processing as well
 */
#define BUS_TRACK_COUNT 512 /* up to 512 active operations, 8 per track */
#else
#define BUS_TRACK_COUNT 64 /* up to 64 active effects */
#endif
#else
#define BUS_TRACK_COUNT 16
#endif
#define BUS_REAL_COUNT 16 /* These are the actual external FX busses */

#define SEND_TRACKS		8 /* 8 mono send busses */
#define AUX_TRACKS		8 /* 8 mono aux busses */
#define BUS_TRACKS		4 /* 4 stereo mixdown busses */
#define DEV_TRACKS		MAX_DEVICES * 2 /* 2 stereo devices */
#define ALL_TRACKS		MAX_TRACK_COUNT + BUS_TRACKS + SEND_TRACKS + \
	AUX_TRACKS + DEV_TRACKS + 3

#define ALL_TRACK_INDECES(x) for (x = 0; x < MAX_TRACK_COUNT; x++)

/*
 * These are for array indices
 */
#define MASTER			MAX_TRACK_COUNT
#define MASTER_LEFT		MASTER + 1
#define MASTER_RIGHT	MASTER + 2
#define MASTER_LEFT2	MASTER + 3
#define MASTER_RIGHT2	MASTER + 4
#define BUS_0			MASTER_RIGHT2 + 1
#define BUS_1			BUS_0 + 1
#define BUS_2			BUS_1 + 1
#define BUS_3			BUS_2 + 1
#define BUS_4			BUS_3 + 1

#define SEND_0_BUS		BUS_4 + 1 /* Send zero is "direct" */
#define SEND_1_BUS		SEND_0_BUS + 1
#define SEND_2_BUS		SEND_1_BUS + 1
#define SEND_3_BUS		SEND_2_BUS + 1
#define SEND_4_BUS		SEND_3_BUS + 1
#define SEND_5_BUS		SEND_4_BUS + 1
#define SEND_6_BUS		SEND_5_BUS + 1
#define SEND_7_BUS		SEND_6_BUS + 1
#define SEND_8_BUS		SEND_7_BUS + 1

#define AUX_0_BUS		SEND_8_BUS + 1
#define AUX_1_BUS		AUX_0_BUS + 1
#define AUX_2_BUS		AUX_1_BUS + 1
#define AUX_3_BUS		AUX_2_BUS + 1
#define AUX_4_BUS		AUX_3_BUS + 1
#define AUX_5_BUS		AUX_4_BUS + 1
#define AUX_6_BUS		AUX_5_BUS + 1
#define AUX_7_BUS		AUX_6_BUS + 1

#define INTERPOLATE_8K	0x01
#define INTERPOLATE_22K	0x02
#define INTERP_MONO		0x04
#define INTERPOLATE		0x07 /* Any of the above */
#define RE_RECORD		0x10 /* Re-record the main out signal */

#define TAPE_MEM_COUNT 8

/*
 * Introduced enumerations to try and prevent assigning command to the status
 * variables, and vice versa. GCC does not check for this condition,
 * however, so the only gain now is that we do not need to specify the 
 * commands or statii values explicitly.
 */
#define MIX_C_RESET 	0	/* return to start of disk, lose any state info */
#define MIX_C_GO		1	/* start pre-load from current locn */
#define MIX_C_RECORD	2	/* start pre-load from current locn for recording */
#define MIX_C_GO_INPUT	3	/* start pre-load from current location */
#define MIX_C_GO_OUTPUT	4	/* start pre-load from current location */
#define MIX_C_GO_DUPLEX	5	/* start pre-load from current location */
#define MIX_C_GOTO		6	/* start-preload from current startOffest */
#define MIX_C_STOP		7	/* stop current operations */
#define MIX_C_CLOSE		8	/* close the current data file */
#define MIX_C_OPEN		9	/* open new file as specified in trackName[] */
#define MIX_C_SLEEP		10	/* request daemon go sleep. Will signal to awaken */
#define MIX_C_IDLE		11	/* request daemon go idle. */
#define MIX_C_SYN_REQ	12
#define MIX_C_PEND		13
#define MIX_C_EXIT 		-1	/* cleanup and clear out */

/*
 * As with the commands I would prefer to make this into a set of #define's
 * as well.
 */
enum mixIOstatus {
	MIX_S_WAITING = 0,
	MIX_S_RUNNING,
	MIX_S_LOADING,
	MIX_S_WRITING,
	MIX_S_EOF,
	MIX_S_CLOSED_WAIT, /* no data file currently opened */
	MIX_S_SLEEPING,
	MIX_S_IDLING,
	MIX_S_SUCCEEDED,
	MIX_S_SUCCEED_SLEEP,
	MIX_S_FAILED,
	MIX_S_FAILED_SLEEP,
	MIX_S_SKIP,
	MIX_S_EXIT = -1
};

/*
 * Compression definitions, should be in seperate file?
 * 8 classes available, at the moment, only 2 are implemented
 */
#define COMP_CLASS_SIZE				3
#define COMP_INDEX_SIZE				4
#define DECOMP_METHOD_SIZE			3

#define COMP_CLASS_NONE				0
#define COMP_CLASS_LOSSY_2_TO_1		1
#define COMP_CLASS_LOSSY_4_TO_1		2
#define COMP_CLASS_NON_LOSSY_FIXED	3 /* Not implemented yet */
#define COMP_CLASS_NON_LOSSY_VAR	4 /* Not implemented yet */

#define COMP_INDEX_AMPLITUDE		0 /* This has been made default. */
#define COMP_INDEX_TIME_DOMAIN		1
#define COMP_INDEX_HYBRID			2 /* This _should_ be made default */
#define COMP_INDEX_SHIFT			3

/*
 * Some flag definitions for the busParams:
 */
#define BUS_TYPE_SYNC	0x1
#ifdef FLOAT_PROC
#define BUS_TYPE_FLOAT	0x2
#endif
#define BUS_TYPE_SYNC	0x1
#ifdef FLOAT_PROC
#define BUS_TYPE_INSERT	0x01000 /* to support insert busses */
#endif
#define BUS_STEREO		0x10000 /* not used */
#define BUS_ON			0x20000 /* to enable disable effect */
#ifdef FX_TRIMMING
#define BUS_TRIM_IN		0x40000
#define BUS_TRIM_OUT	0x80000
#endif

enum DiskStatus {
	MIX_TRACK_CLOSED = 0,
	MIX_TRACK_OPEN,
	MIX_TRACK_FAILED
};

/*
 * Textual status for IO daemon
 */
#define MIXIO_OKAY		"okay"
#define MIXIO_OKAY_WAIT	"waiting"
#define MIXIO_OKAY_LOAD	"loading"
#define MIXIO_SLEEPING	"sleeping"

#define REC1		1
#define REC2		2

#define AU_LEFT		0
#define AU_RIGHT	1
#define LEFT		1
#define RIGHT		2
#define LEFT2		4
#define RIGHT2		8

#define SET			1
#define GET 		2

#ifdef SLAB_PIPO
/*
 * Really want this to be a configurable parameter.
 */
#define X_RATE 256
#endif

/*
 * Scratch types
 */
#define MONO_SCRATCH	0
#define STEREO_SCRATCH	1
#define HYBRID_SCRATCH	2

/*
 * Pick a couple of arbitrary constants to identify our own shared 
 * memory segments.
 *
 * Should really make these contiguous:
 *	CONTROL_KEY 130465
 *	DISK_BUFFER_KEY CONTROL_KEY+1
 *	etc.
 * This would allow management of multiple engines, each with its own shared
 * memory and other segments. FFS.
 */
#define	CONTROL_KEY 130465
#define	AI_BUFFER_KEY 280364
#define	AO_BUFFER_KEY 251265
#define	AO2_BUFFER_KEY 251266
#define	DISK_BUFFER_KEY 251264
#define	BUS_BUFFER_KEY 251273

/*
 * This should really be calculated. The problem with Sun is the MAX SHMEM
 * segment appears to be 1MB. If the blockSampleSize is 8k, then we very soon
 * sun out of space for diskbuffering of large track counts. The calculation
 * should be something like 1MB/(trackCount*blockSampleSize)). FFS.
 */
#define BUS_BUFFER_WINDOW 4
#define DISK_BUFFER_WINDOW 8

#define OUTPUT_BUFFER_WINDOW 4

/*
 * Set of commands for the "virtual tape"
 */
#define TAPE_STOP		0
#define TAPE_START		1 /* This is just a continue operation */
#define TAPE_CONT		1

/*
 * Track commands
 */
#define SAMPLE_RATE		0
#define RESOLUTION		1
#define TRACK_COUNT		2
#define BLK_SMPL_CNT	4
#define WRITE_SAMPLE_RATE	8
#define OUT_BUF_WIN		0x10

#define SLAB_MUTE		1
#define SLAB_SOLO		2
#define SLAB_BOOST		4
#define SLAB_ALTMUTE	8

#define SLAB_MERGE_CODE		0x1
#define SLAB_NORMALISE_CODE	0x2
#define SLAB_REVERSE_CODE	0x4

#define SLAB_MERGE			SLAB_OPCODE_BIT | SLAB_MERGE_CODE
#define SLAB_NORMALISE		SLAB_OPCODE_BIT | SLAB_NORMALISE_CODE
#define SLAB_REVERSE		SLAB_OPCODE_BIT | SLAB_REVERSE_CODE

#define MONO_CAPABILITIES(x) controlBuffer->adiod[x].monoCaps
#define STEREO_CAPABILITIES(x) controlBuffer->adiod[x].stereoCaps
#define ALL_CAPABILITIES(x) \
	(controlBuffer->adiod[x].stereoCaps | controlBuffer->adiod[x].monoCaps)
#define ALL_DEV2_CAPABILITIES \
	(controlBuffer->adiod[x].stereoCaps | controlBuffer->adiod[x].monoCaps)

#define CLIP16(x)			x>32767? 32767:x < -32767? -32767:x
#define CLIP24(x)			x>8388607? 32767:x < -8388608? -32768:x>>8
#define CLIP32(x)			CLIP24(x)
#define CLIPFLOAT(x)		x>32767? 32767:x < -32768? -32768:x

/*
 * This is for 24->8 internal format to 8bits external
 *
 * Any truncation algorithm is going to be noisy. The different resolutions 
 * will improve the signal to noise ratio, but some will introduce clipping 
 * rather than quantisation errors.
 *
 * Also, they will significantly alter the volume of the output signal unless
 * shifted by 16 bits, which can be extremely annoying during recording. This
 * is to be compensated by an automatic output volume adjustment on record
 * start/stop?
 * [This would not be advisable since it would also adjust the input instrument
 * volume, we will have to go for a reduction in PCM level].
 */
#define CLIP24_8(x)			x>8388607? 127:x < -8388608? -128:x>>16
#define CLIP24_8_1(x)		x>4194303? 127:x < -4194304? -128:x>>15
#define CLIP24_8_2(x)		x>2097151? 127:x < -2097152? -128:x>>14
#define CLIP24_8_3(x)		x>1048575? 127:x < -1048576? -128:x>>13

#define CLIPFLOAT_8(x)		x>32767? 127:x < -32768? -128:x/128
#define CLIPFLOAT_8_1(x)	x>16383? 127:x < -16384? -128:x/64
#define CLIPFLOAT_8_2(x)	x>8191? 127:x < -8192? -128:x/32
#define CLIPFLOAT_8_3(x)	x>4095? 127:x < -4096? -128:x/16

#define CHECK_MAX16(x, y)	x = (y>=0)? (y>x? y:x): ((-y)>x)? -y:x;
#define CHECK_MAXFLOAT(x, y)	x = (y>=0)? (y>x? y:x): ((-y)>x)? -y:x;
#define CHECK_MAX CHECK_MAX16

/*
 * This looks ugly, but its just looks at the size of the delta. As delta (x)
 * exceeds the limit (y) but increasing amounts, it is reduced but a respective
 * Q factor.
 * First uses BLim, second uses delta to be reduced.
 */
#define applyQfactor(x, y, z) (x > (y << 3)? y >> (z + 4): \
	x > (y << 2)? y >> (z  + 3): x > (y << 1)? y >> (z + 1): x > y? y:x)
#define applyQfactor2(x, y, z) (x > (y << 3)? x >> (z + 4): \
	x > (y << 2)? x >> (z  + 3): x > (y << 1)? x >> (z + 1): x > y? y:x)

#ifndef TEST
/*
 * If doing TEST, this divide will error?
 */
#define MAKE_LOCN(x) x / (controlBuffer->trackDesc.trackCount * \
	(controlBuffer->trackDesc.resolution >> 3))
#define MAKE_INTERNAL(x) ((x & ~(controlBuffer->blockSampleSize - 1)) * \
	controlBuffer->trackDesc.trackCount * \
	controlBuffer->trackDesc.resolution >> 3)
#else
#define MAKE_LOCN(x) x
#endif

#define MAKE_SAMPLE(x) x * (controlBuffer->trackDesc.trackCount * \
	(controlBuffer->trackDesc.resolution >> 3))

#define E_FILTER_BIT		1
#define E_SEND1_BIT			2
#define E_SEND2_BIT			3
#define E_SEND3_BIT			4
#define E_SEND4_BIT			5
#define E_SEND5_BIT			6
#define E_SEND6_BIT			7
#define E_SEND7_BIT			8
#define E_SEND8_BIT			9
#define E_SEND9_BIT			10
#define E_SEND10_BIT		11
#define E_SEND11_BIT		12
#define E_SEND12_BIT		13
#define E_SEND13_BIT		14
#define E_SEND14_BIT		15
#define E_SEND15_BIT		16
#define E_SEND16_BIT		17
#define E_FIND_MAX_BIT		18
#define E_DYNAMICS_BIT		19
#define E_MIDI_BIT			20

#define E_FILTER			(1<<E_FILTER_BIT)
#define E_SEND1				(1<<E_SEND1_BIT)
#define E_SEND2				(1<<E_SEND2_BIT)
#define E_SEND3				(1<<E_SEND3_BIT)
#define E_SEND4				(1<<E_SEND4_BIT)
#define E_SEND5				(1<<E_SEND5_BIT)
#define E_SEND6				(1<<E_SEND6_BIT)
#define E_SEND7				(1<<E_SEND7_BIT)
#define E_SEND8				(1<<E_SEND8_BIT)
#define E_SEND9				(1<<E_SEND9_BIT)
#define E_SEND10			(1<<E_SEND10_BIT)
#define E_SEND11			(1<<E_SEND11_BIT)
#define E_SEND12			(1<<E_SEND12_BIT)
#define E_SEND13			(1<<E_SEND13_BIT)
#define E_SEND14			(1<<E_SEND14_BIT)
#define E_SEND15			(1<<E_SEND15_BIT)
#define E_SEND16			(1<<E_SEND16_BIT)
#define E_FIND_MAX			(1<<E_FIND_MAX_BIT)
#define E_DYNAMICS			(1<<E_DYNAMICS_BIT)
#define E_MIDI				(1<<E_MIDI_BIT)

#define MAKE_SENDMASK(x)	(2<<(x))

#define E_FILTER_MASK		~E_FILTER
#define E_SEND1_MASK		~E_SEND1_BIT
#define E_SEND2_MASK		~E_SEND2_BIT
#define E_SEND3_MASK		~E_SEND3_BIT
#define E_SEND4_MASK		~E_SEND4_BIT
#define E_SEND5_MASK		~E_SEND5_BIT
#define E_SEND6_MASK		~E_SEND6_BIT
#define E_SEND7_MASK		~E_SEND7_BIT
#define E_SEND8_MASK		~E_SEND8_BIT
#define E_SEND9_MASK		~E_SEND9_BIT
#define E_SEND10_MASK		~E_SEND10_BIT
#define E_SEND11_MASK		~E_SEND11_BIT
#define E_SEND12_MASK		~E_SEND12_BIT
#define E_SEND13_MASK		~E_SEND13_BIT
#define E_SEND14_MASK		~E_SEND14_BIT
#define E_SEND15_MASK		~E_SEND15_BIT
#define E_SEND16_MASK		~E_SEND16_BIT

/*
 * A few file locations
 */
#define SLAB_HOME "SLAB_HOME"

#define DEFAULT_LOCATION "/usr/local/slab"

#define DIR_NAME_LEN	256
#define DATA_BASE "../dataBase"
#define CD_FILE_DIR "/tapes"
#define EFFECTS_DIR "/effects"
#define ENGINE_DIR "/bin"
#define TMP_DIR "/tmp"

#define FORMAT_BARS 1001
#define FORMAT_BEATS 1002
#define FORMAT_MINSEC 1003

#endif /* SLAB_DEFS */

