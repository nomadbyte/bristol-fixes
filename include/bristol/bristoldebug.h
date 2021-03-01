
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

#ifndef BRISTOL_DEBUG_H
#define BRISTOL_DEBUG_H

#define BRISTOL_DEBUG_MASK 0x0000000f
#define BRISTOL_DEBUG1 0x1
#define BRISTOL_DEBUG2 0x2
#define BRISTOL_DEBUG3 0x3
#define BRISTOL_DEBUG4 0x4
#define BRISTOL_DEBUG5 0x5
#define BRISTOL_DEBUG6 0x6
#define BRISTOL_DEBUG7 0x7

#define MIDI_DEBUG_MASK 0x000000f0
#define MIDI_DEBUG1 (0x1 << 4)
#define MIDI_DEBUG2 (0x2 << 4)
#define MIDI_DEBUG3 (0x3 << 4)
#define MIDI_DEBUG4 (0x4 << 4)
#define MIDI_DEBUG5 (0x5 << 4)
#define MIDI_DEBUG6 (0x6 << 4)
#define MIDI_DEBUG7 (0x7 << 4)

#define MIDILIB_DEBUG_MASK 0x00000f00
#define MIDILIB_DEBUG1 (0x1 << 8)
#define MIDILIB_DEBUG2 (0x2 << 8)
#define MIDILIB_DEBUG3 (0x3 << 8)
#define MIDILIB_DEBUG4 (0x4 << 8)
#define MIDILIB_DEBUG5 (0x5 << 8)
#define MIDILIB_DEBUG6 (0x6 << 8)
#define MIDILIB_DEBUG7 (0x7 << 8)

#define OP_DEBUG_MASK 0x0000f000
#define OP_DEBUG1 (0x1 << 12)
#define OP_DEBUG2 (0x2 << 12)
#define OP_DEBUG3 (0x3 << 12)
#define OP_DEBUG4 (0x4 << 12)
#define OP_DEBUG5 (0x5 << 12)
#define OP_DEBUG6 (0x6 << 12)
#define OP_DEBUG7 (0x7 << 12)

#define LIB_DEBUG_MASK 0x000f0000
#define LIB_DEBUG1 (0x1 << 16)
#define LIB_DEBUG2 (0x2 << 16)
#define LIB_DEBUG3 (0x3 << 16)
#define LIB_DEBUG4 (0x4 << 16)
#define LIB_DEBUG5 (0x5 << 16)
#define LIB_DEBUG6 (0x6 << 16)
#define LIB_DEBUG7 (0x7 << 16)

#endif /* BRISTOL_DEBUG_H */

