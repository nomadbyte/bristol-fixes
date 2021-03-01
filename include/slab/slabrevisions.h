
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


#ifndef SLAB_REVS
#define SLAB_REVS

/*****************************************************************
 *
 * The diskBuffer is a large area of shared memory. The child process,
 * or MIX IO Daemon, reads from the disk into this buffer. It has a 
 * trailing edge of where it needs to write data back to disk (only if
 * recording), and a leading edge into which it may load data.
 * The engine process or MIX Daemon also has a leading and trailing 
 * edge into this buffer. Preferably the mixiod can load one block extra 
 * into the buffer to keep ahead of the MIXD.
 *

              MIXD using this for new writen data - PWrite
                   |           MIXD currently mixing this data - PRead
                   |             |
                   V             V
    _______________________________________________________
    |              |             |            |           |
    |              |             |            |           |
    |              |             |            |           |
    -------------------------------------------------------
    ^              ^             ^            ^
    |              MIXIOD cannot use this     |
    |                                     MIXIOD loading this portion - CRead
    MIXIOD needs to synchronise this portion to disk - CWrite

 *
 * Management of the various window pointers is responsibility of the
 * engine process. Child process is dumb - it reads as far ahead as it
 * can, and synchronises back to disk all the data between CWrite and PWrite. 
 * Each of the 4 sections of the shared disk buffer consists of X kB from each
 * track. EG, if we have 16 tracks, at 8192 samples 16bit, this is 1MB of
 * circular buffer space. The daemon will only write data to disk for the
 * track data being recorded, not the whole buffer segment.
 *
 * The audio daemon buffers and bus buffers are similarly managed.
 *
 * Changed the PWrite management to be the adiod for duplex operation.
 * In this case the pointer is managed by adiod during recording, so
 * newly recorded data does not even hit the mixengine.
 *
 * Any changes to this file should result in alterations to the structure
 * revision. The controlBuffer revision should be checked whenever a
 * daemon attaches, reporting an error with different revisions. It is
 * possible to have different code releases use the same controlBuffer
 * revision, but not visa versa.
 *
 * Revised to allow building different releases simply* and easily.
 *
 *****************************************************************/

#define BUILD_LEVEL 410 /* For compiling different version from same codebase */

/*
 * Defines the revision of the controlbuffer. Should be reved if changes are
 * made to this file. Applications will check this structure MAJOR.MINOR.REV
 * when attaching. If they are different, will fail attachment.
 *
 * Reved to 1.1.0 for inclusion of per track dynamics.
 * Reved to 1.1.1 for additional dynamics parameters.
 * Reved to 1.1.2 for addition of parametric filter.
 * Reved to 1.1.3 for pre and post dyn/EQ VU meterage.
 * Reved to 1.1.4 for ADSR dynamics envelope
 * Reved to 1.1.5 for decompressionMethod
 * Reved to 1.1.7 for initial Midi Stuff
 * Reved to 1.2.0 for 2.0 beta1
 * Reved to 1.2.1 for removal of CSI file stuff
 * Reved to 1.3.0 for 2.0 release
 * Reved to 1.4.0 for post 2.0 release
 * Reved to 1.5.0 for 2.1 release (input noise reduction)
 *
 * Reved to 1.6.0 for 2.2 integrations (effects chains, trims, on/off)
 * Reved to 1.6.1 for 2.2 audio buffer window configurations.
 * Reved to 1.6.2 for 2.2 n-backout edits
 * Reved to 1.6.3 for 2.2 beta 1
 * Reved to 1.6.4 for DAEMON_VERS entries
 * Reved to 1.6.5 for SB_NR testing
 * Reved to 1.6.6 for GEN_CONVERGE testing
 * Reved to 1.6.7 for adiod debug options
 * Reved to 1.6.8 for SB_ONR additions
 * Reved to 1.6.9 for METRONOME additions
 * Reved to 1.6.10 for input adjustment additions
 * Reved to 1.6.11 for output mastering
 *
 * Reserved 1.7.0 for 2.20 release.
 *
 * Reved to 1.7.1 for new audio dev config routines.
 * Reved to 1.8.0 for 2.30 beta release.
 * Reved to 1.9.0 for 2.30 release.
 * Reved to 2.0.0 for 3.00 beta releases.
 * Reved to 2.1.0 for 3.00 releases.
 * Reved to 3.10 for 3.10 releases.
 * Reserved 4.0 for 4.0 releases.
 */

/*
 * Defines the revision of the software. Should be reved with each software
 * release.
 * Reved to 2.0.-1(b) for first round of beta testing.
 * Reved to 2.0.-2(b) for another half duplex fix.
 * Reved to 2.0.0(r) ready for distribution.
 * Reved to 2.1.0(r) ready for distribution.
 * Reved to 2.2.X(a) for alpha and beta phases.
 * Reved to 2.20.0(r) for release
 * Reved to 2.20.1(r) for release
 * Reved to 2.21.0(d) for audio dev issues. Will become 2.30?
 * Reved to 2.30-7(b)
 * Reved to 3.0-1(b) includes floating point fully bussed.
 * Reved to 3.0-2(b)
 * Reved to 3.0-3(b)
 * Reved to 3.10-1(f)
 * Reved to 4.0-8(b)
 * Reved to 4.09-11(b)
 */

#if BUILD_LEVEL == 230
#define SLAB_MAJOR_ID	1	/* 1.X.-Y(z) */
#ifdef MIDI
#define SLAB_MINOR_ID	A	/* X.A.-Y(z) */
#else
#define SLAB_MINOR_ID	8	/* X.8.-Y(z) */
#endif
#define SLAB_REV_ID		-7	/* X.Y.-7(z) */
#define SLAB_STATUS_ID	'b'	/* 'r'elease, 'a'lpha, 'b'eta, 'f'ix, 'd'evel */
#define APP_MAJOR_ID	2	/* 2.X.Y(Z) */
#ifdef MIDI
#define APP_MINOR_ID	A	/* X.A.Y(Z) */
#else
#define APP_MINOR_ID	30	/* X.30.Y(Z) */
#endif
#define APP_REV_ID		-7	/* X.Y.-7(Z) */
#define APP_STATUS_ID	'b'	/* 'r'elease, 'a'lpha, 'b'eta, 'f'ix, 'd'evel */
#endif /* 230 */

#if BUILD_LEVEL == 300
#define SLAB_MAJOR_ID	3	/* 3.X.-Y(z) */
#ifdef MIDI
#define SLAB_MINOR_ID	A	/* X.A.-Y(z) */
#else
#define SLAB_MINOR_ID	3	/* X.03.-Y(z) */
#endif
#define SLAB_REV_ID		0	/* X.Y.0(z) */
#define SLAB_STATUS_ID	'f'	/* 'r'elease, 'a'lpha, 'b'eta, 'f'ix, 'd'evel */
#define APP_MAJOR_ID	3	/* 3.X.Y(Z) */
#ifdef MIDI
#define APP_MINOR_ID	A	/* X.A.Y(Z) */
#else
#define APP_MINOR_ID	3	/* X.03.Y(Z) */
#endif
#define APP_REV_ID		0	/* X.Y.0(Z) */
#define APP_STATUS_ID	'f'	/* 'r'elease, 'a'lpha, 'b'eta, 'f'ix, 'd'evel */
#endif /* 300 */

#if BUILD_LEVEL == 310
#define SLAB_MAJOR_ID	3	/* 3.X.-Y(z) */
#ifdef MIDI
#define SLAB_MINOR_ID	A	/* X.A.-Y(z) */
#else
#define SLAB_MINOR_ID	11	/* X.11.-Y(z) */
#endif
#define SLAB_REV_ID		-3	/* X.Y.-3(z) */
#define SLAB_STATUS_ID	'b'	/* 'r'elease, 'a'lpha, 'b'eta, 'f'ix, 'd'evel */
#define APP_MAJOR_ID	3	/* 3.X.Y(Z) */
#ifdef MIDI
#define APP_MINOR_ID	A	/* X.A.Y(Z) */
#else
#define APP_MINOR_ID	11	/* X.11.Y(Z) */
#endif
#define APP_REV_ID		-3	/* X.Y.-3(Z) */
#define APP_STATUS_ID	'b'	/* 'r'elease, 'a'lpha, 'b'eta, 'f'ix, 'd'evel */
#endif /* 310 */

#if BUILD_LEVEL == 320
#define SLAB_MAJOR_ID	3	/* 2.X.-Y(z) */
#ifdef MIDI
#define SLAB_MINOR_ID	A	/* X.A.-Y(z) */
#else
#define SLAB_MINOR_ID	21	/* X.21.-Y(z) */
#endif
#define SLAB_REV_ID		-3	/* X.Y.-3(z) */
#define SLAB_STATUS_ID	'd'	/* 'r'elease, 'a'lpha, 'b'eta, 'f'ix, 'd'evel */
#define APP_MAJOR_ID	3	/* 3.X.Y(Z) */
#ifdef MIDI
#define APP_MINOR_ID	A	/* X.A.Y(Z) */
#else
#define APP_MINOR_ID	21	/* X.21.Y(Z) */
#endif
#define APP_REV_ID		-3	/* X.Y.-3(Z) */
#define APP_STATUS_ID	'd'	/* 'r'elease, 'a'lpha, 'b'eta, 'f'ix, 'd'evel */
#endif /* 320 */

#if BUILD_LEVEL == 400
#define SLAB_MAJOR_ID	4	/* 2.X.-Y(z) */
#ifdef MIDI
#define SLAB_MINOR_ID	A	/* X.A.-Y(z) */
#else
#define SLAB_MINOR_ID	0	/* X.0.-Y(z) */
#endif
#define SLAB_REV_ID		0	/* X.Y.-4(z) */
#define SLAB_STATUS_ID	'r'	/* 'r'elease, 'a'lpha, 'b'eta, 'f'ix, 'd'evel */
#define APP_MAJOR_ID	4	/* 4.X.Y(Z) */
#ifdef MIDI
#define APP_MINOR_ID	A	/* X.A.Y(Z) */
#else
#define APP_MINOR_ID	0	/* X.0.Y(Z) */
#endif
#define APP_REV_ID		0	/* X.Y.-4(Z) */
#define APP_STATUS_ID	'r'	/* 'r'elease, 'a'lpha, 'b'eta, 'f'ix, 'd'evel */
#endif /* 400 */

#if BUILD_LEVEL == 410
#define SLAB_MAJOR_ID	4	/* 2.X.-Y(z) */
#ifdef MIDI
#define SLAB_MINOR_ID	A	/* X.A.-Y(z) */
#else
#define SLAB_MINOR_ID	9	/* X.1.-Y(z) */
#endif
#define SLAB_REV_ID		-13	/* X.Y.-1(z) */
#define SLAB_STATUS_ID	'b'	/* 'r'elease, 'a'lpha, 'b'eta, 'f'ix, 'd'evel */
#define APP_MAJOR_ID	4	/* 4.X.Y(Z) */
#ifdef MIDI
#define APP_MINOR_ID	A	/* X.A.Y(Z) */
#else
#define APP_MINOR_ID	9	/* X.01.Y(Z) */
#endif
#define APP_REV_ID		-13	/* X.Y.-1(Z) */
#define APP_STATUS_ID	'b'	/* 'r'elease, 'a'lpha, 'b'eta, 'f'ix, 'd'evel */
#endif /* 410 */

#if BUILD_LEVEL == 500
#define SLAB_MAJOR_ID	5	/* 2.X.-Y(z) */
#ifdef MIDI
#define SLAB_MINOR_ID	A	/* X.A.-Y(z) */
#else
#define SLAB_MINOR_ID	0	/* X.0.-Y(z) */
#endif
#define SLAB_REV_ID		-1	/* X.Y.-6(z) */
#define SLAB_STATUS_ID	'd'	/* 'r'elease, 'a'lpha, 'b'eta, 'f'ix, 'd'evel */
#define APP_MAJOR_ID	5	/* 4.X.Y(Z) */
#ifdef MIDI
#define APP_MINOR_ID	A	/* X.A.Y(Z) */
#else
#define APP_MINOR_ID	0	/* X.0.Y(Z) */
#endif
#define APP_REV_ID		-1	/* X.Y.-1(Z) */
#define APP_STATUS_ID	'd'	/* 'r'elease, 'a'lpha, 'b'eta, 'f'ix, 'd'evel */
#endif /* 500 */

/*
 * The following are compilation options,
 * impression software, and expression software 
 *
 * Define some options. FX_ONOFF requires FX_CHAINS.....
 */
#define ADIOD_DBG		/* Duplex dev debug flags */
#define OUTPUT_VU		/* output VU meters done in adiod */
#define INPUT_TRACKING	/* VU meters will track the input source */
#define GEN_CONVERGE	/* For arbitrary 16<->8 audio IO */
#define I_NR true		/* filters and dynamics in record channel */
#define SB_DUPLEX	0x20/* Support for SoundBlaster 8 bit full duplex input */
#define SB_NR			/* Support for SoundBlaster interpolative noise red */
#define SB_ONR			/* Support for 8 bit output noise reduction */
#define SOX_FE			/* Support for SOX frontend */
#define NBE			1	/* n-backout edit facilities version */
#define LOOPS			/* Export data, then loop it to main audio out */
#define DAEMON_VERS		/* All Daemons will register their revisions */
/*
 * New device configuration was intended for 3.0, but due to issues with the
 * previous config methods it has been moved forward.
 */
#define NEW_DEV_CONFIG	/* All devices to be configured from dev file */
#define NEWER_DEV_CONFIG/* ALL devices options configured from dev file */
/*
 * Want to have the engine insert metronome beats at a configurable interval,
 * and GUI will allow beats such as 4/4, 3/4, perhaps 6/8 (others on demand)
 * and a tempo. The controlBuffer will only carry the sample insert location
 * This will be appended in WaveSLab with Beat and Bar highlights, plus
 * ability to select beats and bars for editing.
 * Note that input adjust really requires METRONOME support.
 */
#define METRONOME 0x20	/* Applied to conversionFlags, watch for RE_RECORD */
#define SMPTE_EDITS		/* WaveSLab support for frame marking/selection */
/*
 * Going to add a couple of features to a possible 2.30 release. These will be
 * additional libmixer ioctl() debug options, plus a subfragment feature to 
 * decouple the audio fragment size from the blockSampleSize, a capability to 
 * use a modified audio init sequence and finally a single adiod for all
 * devices for better sync.
 */
#if BUILD_LEVEL >= 230
#define FX_CHAINS		/* Ability to chain effects - now a requirement */
#define FX_TRIMMING		/* Ability to trim inbound and outbound effect signal */
#define FX_ONOFF		/* Ability to bypass effects */
#define IOCTL_DBG /* More output of ioctl routines */
#define SUBFRAGMENT /* New dev init, plus subfragment bss to output size */
#define MICROADJUST /* Ability to permille speedup/slowdown of IO speeds */
#define HELLO_DIS /* Ability to disable hello message */
#define MICRO_TAPE /* Insert small tape controller on desk */
#define NEW_DBG /* Ability to disable hello message */
#define SMALL_ICONS /* This is for GUI only */
#define SLAB_PIPO 0x1000 /* Punch in punch out support */
#define YASBHACK /* Yet another SoundBlaster hack, for 16/8 support */
#endif /* BUILD_LEVEL 230 */
/*
 * The above feature definitions are for 2.X software, and the features below
 * will be 3.0 and above.
 */
#if BUILD_LEVEL >= 300
/*
 * The next two are going into SLab 3.0 only. They will not be compiled into
 * the code that becomes 2.20 (they will hit the 2.2 beta for testing).
 */
#define FLOAT_PROC		0x800 /* Processing of floats as well as ints */
#define SLAB_SEPARATE	/* Separate disk track from mixer track */
#define MASTERING		0x80 /* output daemon support for master files */
#define PRIVATE_SCALE	/* Internal scaler with modified graphics */
#define FLOAT_SYNC_FIX
#define USE_SHADOW		/* Use the shadow device for auditioning */
#endif /* BUILD_LEVEL 300 */

#if BUILD_LEVEL >= 310
#define ADIOD_MULTI 0x4000 /* Single audio IO daemon */
#define SESSION_TIMERS /* milli second timers for session recording */
#define SESSION_POINTERS /* Offsets for session list from TapeSLab */
#define SMOOTH_MEMORY /* Check for alterations from baseline to current mix */
/*
 * NOTE: We do not need a SOFT_START flag, I would prefer to make this into
 * an enable/disable flag.
 */
#define SOFT_START 0x8000 /* Use crossfade when starting ALL record ops */
#define SLAB_MIDI_SYNC /* Engine is general sync (SMPTE), GUI is MIDI now */
#define GLIBC_21 /* This should be dropped */
#define DEV_SELECTOR	/* Requestor for parameters of some known cards */
#define DE_CLICK
#define AUDIO_API
#define SECONDARY_FLAGS /* devDisable, SLAB_MIDI_SYNC and others require this */
#endif /* BUILD_LEVEL 310 */

#if BUILD_LEVEL >= 320
/*
 * 3.20 is a build for Xoan Pedrokova(?), with the single main enhancement that
 * there is an API for linking to an external mix driver. This will be used by
 * the University to build a MIDI driver and a hardware mixer for SLab. The
 * API will be based on the sessionRecording facilities (if they agree), such
 * that track/master/operator events will be linked into the mix via session
 * recording which is the single common point for all events. There will be some
 * enhancements required to allow the MIDI interface to define some things like
 * the algorithms and file/device management.
 * To avoid the complexities of having to link to the controlBuffer, this will
 * require we have an internal socket interface to which session events are
 * sent.
 */
#define CONTROL_API /* */
#define GLIBC_21 /* This is a requirement for the control API */
/*
 * This is not going into this release? Maybe, maybe not, it would make sense
 * to let it hit the CONTROL_API release.
 */
#endif /* BUILD_LEVEL 320 */

#if BUILD_LEVEL >= 400
#define CD_MASTER		/* GUI Frontend to cdwrite */
#define CD_R_OUTPUT	2352/* CD-R redbook mastering format */
#define CD_RECORD		/* burner application */
#define DAEMON_AUTOTUNE 0x08 /* mixiod and mixengined will evaluate sleepP */
#define CONTROL_FLOAT
/*
 * To disable ALSA support, comment out this next line. FreeBSD does not have
 * any access to ALSA devices, hence they are not included in compilation.
 *
 * This now defaults to disabled, I don't think ALSA has either the stability
 * nor the penetration for general use. There are large changes between ALSA
 * releases, so if you have 0.5.X or later, be prepare for hacking my audio
 * libraries. If you enable ALSA driver support, you need to check out the
 * makefile in the "src/bin" directory as well.
 */
#define MASTERING_WAV
#define MASTERING_MP3
#endif /* BUILD_LEVEL 400 */

#if BUILD_LEVEL >= 410
#define INC_SOURCE /* Some alterations for distribution of source code */
#endif /* BUILD_LEVEL 410 */

#if BUILD_LEVEL >= 420
#define FULL_MIDI_SYNC
#endif /* BUILD_LEVEL 420 */

#if BUILD_LEVEL >= 500
/*
 * Not sure exactly where we will go after 4.0, but this will probably be the
 * first MIDI support, with MTC sync. Will also look towards SMTPE sync, and 
 * also into more general MIDI support. Realtime mixing is also a possibility.
 * Should also look to build some kind of event editor for session recording.
 *
 * Going for a hit on realtime mixing, 24bit/96kHz support. Will include 
 * multichannel cards, and require floating point support.
 *
 * Sync is slowly slipping, there have been very few request for sync, although
 * several parties have asked for multihead board, and multihead is decidedly
 * with realtime operations. Apart from that, I am very interested in putting
 * this together, and MIDI sync kind of depends on developments at the Uni
 * in Spain, who are writing a MIDI interface. On a related note, I should
 * put out some requests for somebody to write an EDL interface.....
 */
#define MULTI_CHANNEL
#define REAL_TIME
#endif /* BUILD_LEVEL 500 */

/*
 * There are a few definitions which are pretty global. In order to be able to
 * make a true slabaudiodev.h visible they are needed here.
 */
/*
 * This is going to be the max configuration - 64-16-8-4-2:
 * 64 tracks, plus master, master lefts and rights, bus LR, and .
 * 8 EACH of sends and auxs (these are more or less the same.....)
 * 8 mix busses (ie, 4 stereo busses).
 * 8 Stereo outs (for multihead support, or aux monitor support).
 */
#define MAX_DEVICES		8
typedef int (*mixAlgo)();

/*
 * These are now being taken right out of the sys/soundcard.h file. This is
 * perhaps not too clever, but I do not want to require this in the source GUI
 * source code. It should be turned into API calls?
 */
#ifndef INC_SOURCE
#define SLAB_NRDEVICES		SOUND_MIXER_NRDEVICES
#define SLAB_DEVICE_LABELS	SOUND_DEVICE_LABELS
#else
#define SLAB_NRDEVICES		64
#endif

#endif /* SLAB_REVS */

