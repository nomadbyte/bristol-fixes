
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
 *
 * Define a structure that will represent any given IO of an operator.
 * Will require a name, buffer memory, flags (IO, Active, next, etc),
 *
 * These definitions declare the control structures for operator an IO control.
 * We need a separate set of structures that define the IO required by each
 * operator, which will be used by the GUI to understand what parameters need
	(*operator)->destroy = destroy;
 * to be represented to apply parameter changes to the operator.
 */

#ifndef _BRISTOL_H
#define _BRISTOL_H

/*#define BRISTOL_DBG */

#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#ifdef BRISTOL_SEMAPHORE
#include <semaphore.h>
#endif
#include <time.h>
#include <pthread.h>

#include <config.h>

/* #include "bristolvers.h" */
#include "bristoldebug.h"
#include "ringbuffer.h"

#include "bristolaudio.h"
#include "bristolmidiapi.h"
#include "bristolarpeggiation.h"
#include "bristolactivesense.h"

#include <sys/types.h>

#if (BRISTOL_HAS_ALSA == 1)
#define bAMD "hw:0,0"
#define bAAD "plughw:0,0"
#endif /* BRISTOL_HAS_ALSA */

#define bOMD "/dev/midi"
#define bOAD "/dev/audio"

#define BUFSZE 1024

#define BRISTOL_IO_COUNT 16
#define BRISTOL_PARAM_COUNT 16

#define BRISTOL_BUFSIZE BUFSZE

#define BRISTOL_ENGINE "bristolengine"

#define BRISTOL_JACK_MULTI		16 /* 2 legacy plus 16 multiIO */
#define BRISTOL_JACK_UUID_SIZE	32

/*
 * The following are channels and flags. They are applied to bristol system
 * messages, and to the baudio.midiflags
 *
 * We should define a controller map for bristol:
 *	0..63	Emulator operators (osc/env/filt, etc)
 *	64-95	Engine processes:
 *		64	Arpeggiator
 *		65	LADI
 *	96-126	Emulator effects except:
 *	127		SYSTEM commands: start/stop/init, etc.
 */
#define BRISTOL_LADI			65
#define BRISTOL_SYSTEM			127 /* 127/0 */
#define BRISTOL_HELLO			0x00002000
#define BRISTOL_COMMASK			0x00001f00
#define BRISTOL_PARAMMASK		0x000000ff
#define BRISTOL_REQSTART		0x00000100
#define BRISTOL_REQSTOP			0x00000200
#define BRISTOL_DEBUGON			0x00000300
#define BRISTOL_DEBUGOF			0x00000400
#define BRISTOL_VOICES			0x00000500
#define BRISTOL_INIT_ALGO		0x00000600
#define BRISTOL_MIDICHANNEL		0x00000700
#define BRISTOL_ALL_NOTES_OFF	0x00000800
#define BRISTOL_LOWKEY			0x00000900
#define BRISTOL_HIGHKEY			0x00000a00
#define BRISTOL_TRANSPOSE		0x00000b18 /* This include 2 octave up. */
#define BRISTOL_HOLD			0x00000c00
#define BRISTOL_EVENT_KEYON		0x00000d00 /* NOT SENT AS BRISTOL_SYSTEM */
#define BRISTOL_EVENT_KEYOFF	0x00000e00 /* NOT SENT AS BRISTOL_SYSTEM */
#define BRISTOL_EVENT_PITCH		0x00000f00 /* NOT SENT AS BRISTOL_SYSTEM */
/*#define BRISTOL_FREE			0x00000f00 */
#define BRISTOL_EXIT_ALGO		0x00001000
#define BRISTOL_UNISON			0x00001100
#define BRISTOL_REQ_FORWARD		0x00001200 /* Disable event forwarding on chan*/
#define BRISTOL_REQ_DEBUG		0x00001300 /* Debug level */
#define BRISTOL_MIDI_DEBUG1		0x00008000 /* Just messages */
#define BRISTOL_MIDI_DEBUG2		0x00010000 /* And internal functions() */
#define BRISTOL_MIDI_NRP_ENABLE	0x00020000

/* LADI are BRISTOL_SYSTEM 1278/1 */
#define BRISTOL_LADI_SAVE_REQ	0x00000001
#define BRISTOL_LADI_LOAD_REQ	0x00000002

#define BRISTOL_CC_HOLD1		64

#define SYSTEM_CHANNEL			0x07f /* used for initialisation requests. */

#define BRISTOL_CHAN_OMNI		127 /* Denotes any midi channel. */

//#define BRISTOL_VOICECOUNT		32 /* Was 16, now increased for GM-2 24/32 */
#define BRISTOL_MAXVOICECOUNT	128
#define BRISTOL_SYNTHCOUNT		64

/* Not sure why these are in hex, they should be decimal, minor issue */
#define BRISTOL_MINI			0x0000
#define BRISTOL_HAMMOND			0x0001
#define BRISTOL_PROPHET			0x0002
#define BRISTOL_DX				0x0003
#define BRISTOL_JUNO			0x0004
#define BRISTOL_EXPLORER		0x0005
#define BRISTOL_HAMMONDB3		0x0006
#define BRISTOL_VOX				0x0007
#define BRISTOL_RHODES			0x0008
#define BRISTOL_PROPHET10		0x0009
#define BRISTOL_MIXER			0x000a /* THIS SHOULD BE MOVED TO END!!! */
#define BRISTOL_SAMPLER			0x000b
#define BRISTOL_BRISTOL			0x000c
#define BRISTOL_DDD				0x000d
#define BRISTOL_PRO52			0x000e
#define BRISTOL_OBX				0x000f
#define BRISTOL_OBXA			0x0010
#define BRISTOL_RHODES_BASS		0x0011 /* Actually a DX or Rhodes */
#define BRISTOL_POLY			0x0012 /* Korg MonoPoly */
#define BRISTOL_POLY6			0x0013 /* Korg Poly-6 */
#define BRISTOL_AXXE			0x0014 /* ARP Axxe */
#define BRISTOL_ODYSSEY			0x0015 /* ARP Odyssey */
#define BRISTOL_MEMMOOG			0x0016 /* Memory Moog */
#define BRISTOL_2600			0x0017 /* Arp 2600 */
#define BRISTOL_SAKS			0x0018 /* EMS S Aks */
#define BRISTOL_MS20			0x0019 /* Korg MS20 */
#define BRISTOL_SOLINA			0x001a /* ARP Solina String Machine */
#define BRISTOL_ROADRUNNER		0x001b /* Crumar electric piano */
#define BRISTOL_GRANULAR		0x001c /* Granular sound generator */
#define BRISTOL_REALISTIC		0x001d /* Mood MG-1 sound generator */
#define BRISTOL_VOXM2			0x001e
#define BRISTOL_JUPITER8		0x001f /* Time to extend the table size to 64 */
#define BRISTOL_BIT_ONE			0x0020 /* Crumar Bit One, Bit99. */
#define BRISTOL_MASTER			0x0021 /* MIDI Master Controller */
#define BRISTOL_CS80			0x0022
#define BRISTOL_PRO1			0x0023
#define BRISTOL_VOYAGER			0x0024
#define BRISTOL_SONIC6			0x0025
#define BRISTOL_TRILOGY			0x0026 /* This is the synth circuit */
#define BRISTOL_TRILOGY_ODC		0x0027 /* This is the organ divider circuit */
#define BRISTOL_STRATUS			0x0028 /* Holder for the smaller brother */
#define BRISTOL_POLY800			0x0029
#define BRISTOL_BME700			0x002a
#define BRISTOL_BASSMAKER		0x002b
#define BRISTOL_SID_M1			0x002c /* 2 SID, audio and mod - sidney */
#define BRISTOL_SID_M2			0x002d /* 6 SID, 5 audio, 1 mod - canberra */

/*
 * Operators
 */
#define B_DCO			0
#define B_ENV			1
#define B_DCA			2
#define B_FILTER		3
#define B_NOISE			4
#define B_HAMMOND		5
#define B_RESONATOR		6
#define B_LESLIE		7
#define B_PRODCO		8
#define B_FMOSC			9
#define B_HPF			10
#define B_JUNODCO		11
#define B_CHORUS		12
#define B_VIBRACRHORUS	13
#define B_FILTER2		14
#define B_EXPDCO		15
#define B_LFO			16
#define B_VOXDCO		17
#define B_SDCO			18
#define B_ARPDCO		19
#define B_RINGMOD		20
#define B_ELECTRO_SW	21
#define B_REVERB		22
#define B_FOLLOWER		23
#define B_AKSDCO		24
#define B_AKSENV		25
#define B_AKSREVERB		26
#define B_AKSFILTER		27
#define B_HCHORUS		28
#define B_QUANTUM		29
#define B_B1OSC			30
#define B_CS80OSC		31
#define B_CS80ENV		32
#define B_TRILOGYOSC	33
#define B_ENV5S			34
#define B_NRO			35

/*
 * Audio interface types. The loest bytes is resevered.
 *
 * These are for audiomain.flags, there are 8 drivers available of which are
 * defined, one is deprecated leaving six. The Pulse has two interfaces one of
 * which may eventually be discarded. They are pulse simple and pulse, the 
 * latter is a full callback interface, multithreaded and rather heavy on CPU.
 */
#define BRISTOL_AUDIOMASK		0xff000000
#define BRISTOL_ALSA			0x01000000
#define BRISTOL_OSS				0x02000000
#define BRISTOL_PULSE			0x04000000
#define BRISTOL_JACK			0x08000000
#define BRISTOL_PULSE_T			0x10000000 /* Threaded */
#define BRISTOL_PULSE_S			0x20000000 /* Simple - lots of latency */
#define BRISTOL_DUMMY			0x40000000
/* This appears to collide */
#define BRISTOL_AUDIOWAIT		0x00002000

/*
 * MIDI interface types, may interact with audio drivers.
 */
#define BRISTOL_MIDIMASK		0x007f0000
#define BRISTOL_MIDI_ALSA		0x00010000
#define BRISTOL_MIDI_OSS		0x00020000
#define BRISTOL_MIDI_SLAB		0x00040000 /* this should be deprecated */
#define BRISTOL_MIDI_JACK		0x00080000
#define BRISTOL_MIDI_LADSP		0x00100000
#define BRISTOL_MIDI_SEQ		0x00200000
#define BRISTOL_MIDI_OSC		0x00400000

#define BRISTOL_MIDI_WAIT		0x00800000

/* Audio connection to Jack ports */
#define BRISTOL_AUTO_CONN		0x00008000
/* Separate registration for audio and midi */
#define BRISTOL_JACK_DUAL		0x00004000

#define BRISTOL_TERM -3
#define BRISTOL_FAIL -2
#define BRISTOL_EXIT -1
#define BRISTOL_OK 0
#define BRISTOL_WAIT 1
#define BRISTOL_INIT 2

/*
 * Global size management.
 */
#define BRISTOL_VPO 12.0
#define BRISTOL_VPO_DIV 10.5833333 /* 127 / this gives us our 12 per octave */

/*
 * Flags for IO management
 */
#define BRISTOL_DONE 4 /* Is this channel ready for use */
#define BRISTOL_GAIN 8 /* Do we have an active gain control on this channel */

/*
 * Parameter description structures.
 */
typedef struct BristolParamSpec {
	int int_val;
	float float_val;
	void *mem;
	float config_val;
	int midi_adj_algo;
	int midi_param;
	float config_amount; 
	float midi_amount; 
} bristolParamSpec;

/*
 * Modifier description structures.
 *
 * Define some modifier identifier flags:
 */
#define BRISTOL_PMOD_KEY 0 /* Apply a key (poly) index modifier */
#define BRISTOL_PMOD_PRESS 1 /* Apply a polypressure (poly) modifier */
/*
 * The rest are channel mods, and apply to all voices on bristolSound
 */
#define BRISTOL_CMOD_CONT_0 0
#define BRISTOL_CMOD_CONT_1 1
#define BRISTOL_CMOD_CONT_2 2
/* ... up to 32 controllers */
#define BRISTOL_CMOD_CONT_30 30
#define BRISTOL_CMOD_CONT_31 31
#define BRISTOL_CMOD_PITCHWHEEL 32
#define BRISTOL_CMOD_CHANPRESS 33
/*
 * And some control flags to define what is applied where.
 */
#define BRISTOL_CHANMOD 1 /* Apply a channel mod: chanpress, controller, etc */
#define BRISTOL_POLYMOD 2 /* Apply a polyphonic modifier: key or polypress */
#define BRISTOL_CHANMOD_INV 4 /* Invert value (ie, max - value) */
#define BRISTOL_POLYMOD_INV 8
#define BRISTOL_CHANMOD_CORRECT 0x10 /* Apply max / 2 - value */
#define BRISTOL_POLYMOD_CORRECT x020
#define BRISTOL_CHANMOD_UNIQUE 0x40 /* Apply uniquely, or adjust configured */
#define BRISTOL_POLYMOD_UNIQUE 0x80
typedef union BristolModSpec {
	unsigned int flags; /* type of modifiers to apply */
	int channelMod; /* index into CHANMOD table */
	int polyMod; /* index into PolyMod table */
	float cmGain;
	float pmGain;
} bristolModSpec;

typedef struct BristolOPParams {
	bristolParamSpec param[BRISTOL_PARAM_COUNT];
} bristolOPParams;

/*
 * Parameter management structures.
 */
typedef struct BristolParam {
	int type;
	int index;
	bristolParamSpec value;
	bristolModSpec modifiers;
} bristolParam;

/*
 * Voice structures
 */
#define BRISTOL_KEYON		0x0001
#define BRISTOL_KEYOFF		0x0002
#define BRISTOL_KEYDONE		0x0004
#define BRISTOL_KEYNEW		0x0008
#define BRISTOL_KEYREON		0x0010
#define BRISTOL_KEYOFFING	0x0020
#define BRISTOL_KEYSUSTAIN	0x0040
#define BRISTOL_KEYREOFF	0x0080
/*
 * There are Korg Mono/Poly specifics for VCO assignment.
 */
#define BRISTOL_K_VCO4	0x10000000
#define BRISTOL_K_VCO3	0x20000000
#define BRISTOL_K_VCO2	0x40000000
#define BRISTOL_K_VCO1	0x80000000
#define BRISTOL_VCO_MASK 0xf0000000
/*
 * Actually configured into baudio->mixflags
 */
#define BRISTOL_MULTITRIG		0x00800000
typedef struct BristolVoice {
	struct BristolVoice *next;
	struct BristolVoice *last;
	struct BAudio *baudio;
	int index;
	unsigned int flags;
	int offset;
	char ***locals;
	/*
	 * We need an event structure for each possible poly event. These will be
	 * fead to the voice operators for mods, etc. This should be views as the
	 * PolyMod table, ie, there are only two polyphonic modifiers, the key id
	 * and any polypressure generated.
	 */
	keyMsg key;
	keyMsg keyoff;
	int keyid;
	float velocity;
	pressureMsg pressure;
	float press;
	/*
	 * For polyponic portamento
	 */
	unsigned char lastkey; /* This should not be voice, but Audio parameter. */
	/* These are for the tendency generators */
	float dfreq; /* Desired frequency index */
	float cfreq; /* Current frequency index */
	float cfreqmult; /* rate of change from c to d */
	/* These are for the wavetable stepping rates */
	float dFreq; /* Desired step frequency index */
	float cFreq; /* Current step frequency index */
	float cFreqstep; /* rate of change from C to D */
	float cFreqmult; /* rate of change from C to D */
	float oFreq; /* Used to return pitchbend to original value */
	float chanpressure; /* Need a copy here */
	int transpose;
	float detune;
} bristolVoice;

/*
 * IO Channel structures
 */
#define BRISTOL_AC 1
#define BRISTOL_DC 2
#define BRISTOL_INPUT 4
#define BRISTOL_OUTPUT 8
typedef struct BristolIO {
	char *IOname;     /* Name of this IO function */
	struct BristolOP *owner; /* Pointer to the operator that owns this IO */
	int index;        /* Index of this IO */
	unsigned int flags;        /* diverse bits */
	struct BristolIO *last; /* Previous operator on list */
	struct BristolIO *next; /* Next operator on list */
	float *bufmem;    /* buffer memory for this IO */
	int samplecnt;    /* number of samples in buffer */
	int samplerate;   /* on this IO */
	bristolModSpec modifiers; /* Midi modifiers to streamed input */
} bristolIO;

typedef int (*bristolAlgo)();

/*
 * These are used for templating, not for operational control. We could change
 * the naming, since it is rather close to bristolOPParams, which pass the run-
 * time parameters to the operating code.
 */
#define BRISTOL_INT 1
#define BRISTOL_FLOAT 2
#define BRISTOL_ENUM 3
#define BRISTOL_TOGGLE 4

#define BRISTOL_ROTARY 1
#define BRISTOL_SLIDER 2
#define BRISTOL_BUTTON 4
#define BRISTOL_HIDE 8
typedef struct BristolOPParam {
	char *pname;
	char *description;
	int type; /* need method of defining enumerations "Square, sine" etc, FFS */
	int low;
	int high;
	unsigned int flags;
} bristolOPParam;

typedef struct BristolOPIO {
	char *ioname;
	char *description;
	int samplerate;
	int samplecount;
	unsigned int flags; /* AC/DC, Hide op, others */
	float *buf;
} bristolOPIO;

typedef struct BristolOPSpec {
	char *opname;
	char *description;
	int pcount;
	bristolOPParam param[BRISTOL_PARAM_COUNT];
	int iocount;
	bristolOPIO io[BRISTOL_IO_COUNT];
	int localsize; /* size of table require for runtime locals */
} bristolOPSpec;

#define BRISTOL_SOUND_START 1
#define BRISTOL_SOUND_END 2
#define BRISTOL_FX_STEREO 4
typedef struct BristolSound {
	char *name;
	int index; /* into operator list */
	int (*operate)(struct BristolOP *, bristolVoice *,
		bristolOPParams *, void *);
	/*unsigned char *param; Param structure for that op. */
	bristolOPParams *param; /* Param structure for that op. */
	int next;
	unsigned int flags;
} bristolSound;

/*
 * Bristol Operator control structure.
 */
typedef struct BristolOP {
	int	index;    /* Index of this OP */
	unsigned int flags;    /* diverse bits */
	struct BristolOP *next; /* Next operator on list */
	struct BristolOP *last; /* Previous operator on list */
	/*
	 * These are used for any information that is managed internally
	 */
	bristolOPSpec *specs; /* any parameter specs for this OP */
	int size; /* Size of specs memory */
	/*
	 * A number of routines called to manage the operator
	 */
	bristolAlgo init;
	bristolAlgo destroy;
	bristolAlgo reset;
	int (*param)(struct BristolOP *, bristolOPParams *, unsigned char, float);
	int (*operate)(struct BristolOP *, bristolVoice *, bristolOPParams *,
		void *);
} bristolOP;

extern bristolOP *bristolOPinit();
extern bristolIO *bristolIOinit();

/*
 * This are mixflags: the system reserves the space 0xffff0000.00000000, and
 * a poly algorithm can use the rest.
#define BRISTOL_SUSTAIN		0x0040000000000000ULL
 */
#define bfiltertype(x)	((x&(BRISTOL_LW_FILTER|BRISTOL_LW_FILTER2)) >> 48)
#define BRISTOL_LW_FILTER	0x0001000000000000ULL
#define BRISTOL_LW_FILTER2	0x0002000000000000ULL
#define BRISTOL_LW_FILTERS	(BRISTOL_LW_FILTER|BRISTOL_LW_FILTER2)
#define BRISTOL_GLIDE_AUTO	0x0004000000000000ULL
#define BRISTOL_SENSE		0x0008000000000000ULL
#define BRISTOL_SEQUENCE	0x0010000000000000ULL
#define BRISTOL_CHORD		0x0020000000000000ULL
#define BRISTOL_ARPEGGIATE	0x0040000000000000ULL
#define BRISTOL_V_UNISON	0x0080000000000000ULL
#define BRISTOL_MUST_PRE	0x0100000000000000ULL
#define BRISTOL_HAVE_OPED	0x0200000000000000ULL
#define BRISTOL_HOLDDOWN	0x0400000000000000ULL
#define BRISTOL_REMOVE		0x0800000000000000ULL
#define A_440				0x1000000000000000ULL
#define MASTER_ONOFF		0x2000000000000000ULL
#define BRISTOL_STEREO		0x4000000000000000ULL
#define BRISTOL_KEYHOLD		0x8000000000000000ULL

/* Monophonic note logic */
#define BRISTOL_MNL_LNP		0x0001
#define BRISTOL_MNL_HNP		0x0002
#define BRISTOL_MNL_TRIG	0x0010
#define BRISTOL_MNL_VELOC	0x0020

/*
 * Audio globals structure.
 */
typedef struct BAudio {
	struct BAudio *next;
	struct BAudio *last;
	int soundCount;
	bristolSound **sound; /* operator instance sequences */
	bristolSound **effect;
	int (*param)(struct BAudio *, u_char, u_char, float); /* param change */
	bristolAlgo preops; /* Pre polyphonic (ie, monophonic) voicing routine */
	bristolAlgo operate; /* Polyphonic voice mixing routine */
	bristolAlgo postops; /* Post polyphonic voicing routine: FX, etc. */
	bristolAlgo destroy; /* Voice destruction routine */
	bristolVoice *firstVoice;
	int debuglevel;
	/*
	 * This should become a generic midi modifier table.
	 */
	float contcontroller[MIDI_CONTROLLER_COUNT];
	int GM2values[MIDI_CONTROLLER_COUNT];
	/* We should put in a callback function here. */
	int (*midi)(struct BAudio *, int, float); /* param change */
	chanPressMsg chanPress;
	float chanpressure;
	int sid;
	int controlid;
	int midichannel;
	char ***locals; /* Unique locals per voice, a basic necessity. */
	void ***params; /* Unique params per voice, for midi poly support, etc. */
	char ***FXlocals;
	u_int64_t mixflags;
	unsigned int midiflags;
	/*
	 * These need to remain to the baudio structure
	 */
	float extgain;
	float glide;
	float glidemax;
	float gtune;
	float note_diff;
	float gain_diff;
	int transpose;
	arpeggiator arpeggio;
	float pitchwheel;
	int midi_pitch;
	float midi_pitchfreq;
	int cvoices;
	int lvoices;
	float *leftbuf;
	float *rightbuf;
	fTab ctab[DEF_TAB_SIZE];
	float *mixlocals;
	int voicecount;
	char lowkey;
	char highkey;
	unsigned char lastkey;
	float detune;
	float finetune;
	float coarsetune;
	float gain;
	fTab microtonalmap[128];
	float velocitymap[128];
	int midimap[128];
	u_char valuemap[128][128];
	int sensecount;
	int samplerate;
	int samplecount;
	int oversampling;
	/* For the corrected monophonic note logic */
	struct {
		int key[128];
		int velocity[128];
		int count;
		int flags;
		int low;
		int high;
		int extreme;
	} notemap;
} Baudio;

typedef struct AudioMain {
	Baudio *audiolist;
	bristolVoice *playlist;
	bristolVoice *playlast;
	bristolVoice *freelist;
	bristolVoice *freelast;
	bristolVoice *newlist;
	bristolVoice *newlast;
	bristolOP **palette; /* operator templates */
	bristolOP **effects; /* operator templates */
#ifdef BRISTOL_SEMAPHORE
	sem_t *sem_long;
	sem_t *sem_short;
#else
	void *unused1;
	void *unused2;
#endif
	struct timespec abstime;
	int voiceCount;
	int opCount;
	int debuglevel;
	unsigned int flags;
	int iosize;
	int preload;
	int samplecount;
	int segmentsize;
	int samplerate;
	int priority;
	struct sched_param asp;
	struct sched_param msp;
	int atReq;
	int atStatus;
	int mtReq;
	int mtStatus;
	int controlHandle; /* Sysex MIDI handle */
	int midiHandle; /* MIDI handle */
	unsigned int midiflags;
	int s440holder; /* not used? */
	char *audiodev; /* both of these really need to become multiheaded? */
	char *mididev;
	int port; /* for tcp connecctions */
	float ingain;
	float outgain;
	char *microTonalMappingFile;
	unsigned int SysID;
	jack_ringbuffer_t *rb; /* MIDI thread to audio threadd */
	jack_ringbuffer_t *rbfp; /* MIDI/Audio thread to forwarding thread */
	char jackUUID[BRISTOL_JACK_UUID_SIZE];
	int iocount;
	float *io_i[BRISTOL_JACK_MULTI];
	float *io_o[BRISTOL_JACK_MULTI];
	float m_io_ogc;
	float m_io_igc;
	char *cmdline;
	char *sessionfile;
	char *controldev;
} audioMain;

extern int cleanup();

extern Baudio *findBristolAudio(Baudio *, int, int);
extern Baudio *findBristolAudioByChan(Baudio *, int);
extern int bufmerge(float *, float, float *, float, int);
extern int bufadd(float *, float, int);
extern int bufset(float *, float, int);

extern void * bristolmalloc();
extern void * bristolmalloc0();
extern void bristolfree();
extern void bristolbzero();

extern void alterAllNotes();
extern int fillFreqTable();
extern int fillFreqBuf();

extern void initSoundAlgo();

#ifndef NULL
#define NULL 0
#endif

int bristolArpegReVoice(Baudio *, bristolVoice *, float);
int bristolArpegReAudio(audioMain *, Baudio *);
void bristolArpeggiatorInit(Baudio *);
void bristolArpeggiator(audioMain *, bristolMidiMsg *);
void bristolArpeggiatorNoteEvent(Baudio *, bristolMidiMsg *);

#endif /* _BRISTOL_H */

