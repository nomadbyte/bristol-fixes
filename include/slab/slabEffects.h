
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


#ifndef SLAB_EFFECTS_H
#define SLAB_EFFECTS_H true

#ifndef NULL
#define NULL 0
#endif

/*
 * Maximum number of controllers supported by the MixSLab Effects GUI.
 */
#define CONTROLLER_COUNT	8
#define SLAB_NAME_LENGTH	128

#define SLAB_EFFECTS_MONO	0x1
#define SLAB_EFFECTS_STEREO	0x2
#define SLAB_EFFECTS_FLOAT	0x4

/*
 * For calls to seGetOpts()
 */
#define SLAB_RESOLUTION		1
#define SLAB_SAMPLE_RATE	2

/*
 * The API only spports SLab_Potmeter as of the 1.0 release. Later
 * implementations will use the rest, and drop the enumerated type in 
 * preference for #define's.
 */
enum controllerTypes {
	SLab_unassigned = -1,
	SLab_Potmeter,
	SLab_Scaler,
	SLab_Button
};

extern int *seAttach();
extern int seInit();
extern int seDetach();
extern int seAddCtrl();
extern int seGetCtrlVal();
extern int *seRead();
extern int seWrite();
extern int seGetOpts();
extern char *seGetFileName();
extern int seSetCtrlDisp();

#endif /* SLAB_EFFECTS_H */

