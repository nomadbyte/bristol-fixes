
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
#ifndef BRIGHTONMIXERMEMORY_H
#define BRIGHTONMIXERMEMORY_H

#include "brightonMini.h"

typedef float cOpaque[18];
typedef struct cClear {
	float gain;
	float preSend;
	float dynSens;
	float dynAtt;
	float dynDec;
	float dynSus;
	float dynRel;
	float treble;
	float tFreq;
	float mid;
	float bass;
	float bFreq;
	float postSend;
	float fxVol;
	float fxSpd;
	float fxDpt;
	float pan;
	float vol;
} cClear;
typedef union CParams {
	cOpaque opaque;
	cClear clear;
} cparams;

typedef struct ChanMem {
	char scratch[20];
	int inputSelect;
	int preSend[4];
	int dynamics;
	int filter;
	int postSend[4];
	int fxAlgo;
	cparams p;
	int mute;
	int solo;
	int boost;
	int outputSelect[16];
	int stereoBus;
	float data[30];
} chanMem;

typedef float bOpaque[FXP_COUNT];
typedef struct BClear {
	float igain;
	float speed;
	float depth;
	float m;
	float pan;
	float gain;
	int algorithm;
	float outputSelect[16];
	float pad[FXP_COUNT - 7 - 16];
} bClear;
typedef union BParams {
	bClear clear;
	bOpaque opaque;
} bparams;
typedef struct BusMem {
	char name[32]; // For eventual FX attachments.
	bparams b;
} busMem;

typedef float vbOpaque[3];
typedef struct VBClear {
	float vol;
	float left;
	float right;
} vbClear;
typedef union VBus {
	vbClear clear;
	vbOpaque opaque;
} vBus;

typedef struct MixerMemV1 {
	char name[20];
	int version;
	int chancount;
	busMem bus[16];
	vBus vbus[8];
	chanMem chan[MAX_CHAN_COUNT];
} mixerMem;

extern int saveMixerMemory(guiSynth *synth, char *name);
extern int loadMixerMemory(guiSynth *synth, char *name, int param);
extern void *initMixerMemory(int count);
extern int setMixerMemory(mixerMem *, int, int, float *, char *);
extern char* getMixerMemory(mixerMem *m, int op, int param);


#endif
