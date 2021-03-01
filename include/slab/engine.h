
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

#ifndef SLAB_ENGINE
#define SLAB_ENGINE

#include "slabrevisions.h"
#include "slabdefinitions.h"

#include "slabTimer.h"
#ifdef NEW_DBG
#include "slabDebugMasks.h"
#endif
#ifdef FLOAT_PROC
#include "fbDefs.h"
#endif
#include "slabEffects.h"
#ifdef MIDI
#include "slabMidiSpecs.h"
#endif

#include "slabtrack.h"
#include "slabbus.h"
#include "slabaudiodev.h"
#include "slabcbuf.h"

#include "slabcdefs.h"

#ifdef BRISTOL_PA
#include "bristolpulse.h"
#endif

#endif /* SLAB_ENGINE */

