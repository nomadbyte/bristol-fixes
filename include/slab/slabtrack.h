
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

#ifndef SLAB_TRACK
#define SLAB_TRACK

/*****************************************************************
 *
 * The next set of definitions are for a track array. This is assigned
 * dynamically in shmem in the main init code. This is used to control the
 * relative volumes of each track, filter definitions, algorithm indeces.
 *
 * Its big, and we need MAX_TRACK_COUNT of them in the controlBuffer,
 * but it governs all of the internal digital sound processing algorithms.
 *
 *****************************************************************/

typedef struct SendParams {
	short Bus;
	short Vol;
} sendparams;

typedef struct TrackParams {
	short Bass;	/* A stack of filter parameters */
	short BassLim;
	short BassGain;
	short BassQ;
	short BassSweep;
	short bLSZeroCount;
	int BassOut;
	int bLastSample;
	short Treb;
	short TrebLim;
	short TrebGain;
	short TrebQ;
	short TrebSweep;
	short tLSZeroCount;
	int TrebOut;
	int tLastSample;
	short Para;			/* reved CB to 1.1.2 */
	short ParaLim;
	short ParaGain;
	short ParaQ;
	short ParaSweep;
	short pLSZeroCount;
	int ParaOut;
	int pLastSample;
	int LastSample;
	int maxSamplePre;	/* reved CB to 1.1.3 */
	int aveSamplePre;
	int maxSample;
	int aveSample;
	int flags;
	short volume;		/* The actual track vol */
	short leftVolume;	/* Two panning parameters */
	short rightVolume;
	float leftVhold;	/* Two de-rits parameters */
	float rightVhold;
	int dynLim;			/* parameters for the dynamics algorithm. CB - 1.1.0 */
	int dynRamp;
	int dynRampDown;	/* For decaying slope, if eventually used */
	int dynGain;
	int dynRelease;
	int dynTargetGain;
	int dynCurGain;
	int dynState;		/* reved CB to 1.1.1 */
	int dynCount;
#ifdef SLAB_PIPO
	int nrGain;			/* For PIPO operations */
	int nrCurGain;			/* For PIPO operations */
#endif
	/*
	 * Bussing parameters, with algos as indeces into the mixAlgo arrays.
	 * Send algos - implement 2.
	 * Filter Algo.
	 * Pan algo.
	 * The code will look to see if we have a window in the diskBuffer, and if
	 * so, will scan the trackparams arrays, calling the configured algos.
	 */
	int compressionIndex;
	int decompressionMethod;
	sendparams send[4];
	short audioOutputL;
	short audioOutputR;
	short recordAlgo;
	short dynamicsAlgo;
	short filterAlgo;
	short panAlgo;
	short stereoBus; /* This is the destination for the panned data */
	short midiDev;
#ifdef SLAB_SEPARATE
	int diskTrack; /* Separates mixing desk track from diskfile track */
#endif
#ifdef SOFT_START
	int softStart;
	int softStop;
#endif
} trackparams;

/*
 * This is for the front end
 */
typedef struct LocalRecord {
	short dev;
	short source;
	short gain;
	short treble;
	short mid;
	short bass;
	short special;
	short boost;
	short pan;
	int flags; /* Mute, Boost(?), Solo options */
	short volume;
	short leftVolume;
	short rightVolume;
	/*
	 * Put into array
	short send1;
	short send2;
	 */
	short sends[4];
	short returnVol;
	short returnLeft;
	short returnRight;
	short aux1;
	short aux2;
	short sBus;
	short mBus[4];
	short midiDevIndex;
} localrecord;

#endif /* SLAB_TRACK */

