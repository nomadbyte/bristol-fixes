
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


#ifndef SLAB_BUS
#define SLAB_BUS

typedef struct effectControl {
	char controllerName[16];
	char controllerDisp[20];
	int controllerType;
	int controllerValue;
} effectcontrol;

typedef struct BusParams {
	int *pRead;	/* the memory indeces */
	int *pWrite;
	int *cRead;
	int *cWrite;
	int *childRead;
	int pReadBytes;	/* The byte counters */
	int pWriteBytes;
	int cReadBytes;
	int childReadBytes;
	int cWriteBytes;
	char *mainpWrite; /* For asynchronous returns, each bus needs pointer */
	int mainpWriteBytes;
	int *bufferPointer;
	int bufferOffset;
	int *childBufferPointer;
	int bufferSize;
	char busName[16];
	char busShortName[8];
	int busID;
	int busPID;
	int requestPID;
	int grantPID;
	int Poverrun;
	int Punderrun;
	int Coverrun;
	int Cunderrun;
	short leftVol;
	short rightVol;
	short volume;
	short outputDevID;
	effectcontrol effectControls[CONTROLLER_COUNT];
	int paramID;
	int flags;
	int (*callback)();
	int signal; /* Typically this will be "CONT", but may be USR1 or 2 */
#ifdef FX_CHAINS
	char chain_id; /* indicate which of the return busses we are chained to */
	char last_id; /* indicates next effect in chain. */
	char next_id; /* indicates next effect in chain. */
	char stereo_id; /* Used to indicate a stereo bus linkage. */
#endif
#ifdef FX_TRIMMING
	short trim_in;
	short trim_out;
#endif
#ifdef FLOAT_PROC
	fbalgo fbAlgo;
	fbcontrols fbControls[FB_CONTROLS];
#endif
} busparams;

#endif /* SLAB_BUS */

