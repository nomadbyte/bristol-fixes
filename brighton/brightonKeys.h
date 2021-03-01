
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
 * These need to be redone. Each white key should be placed first, then the
 * black keys layered on top. That way we can get better graphics with shading
 * on the white keys for each black key.
 */

#ifndef BRIGHTONKEYS_H
#define BRIGHTONKEYS_H

#define KEY_COUNT 61
#define KEY_COUNT_2OCTAVE2 32
#define KEY_COUNT_2OCTAVE 29
#define KEY_COUNT_3OCTAVE 44
#define KEY_COUNT_3_OCTAVE 37
#define KEY_COUNT_4OCTAVE 49
#define KEY_COUNT_PEDAL 24
#define VKEY_COUNT 48
#define KEY_COUNT_5OCTAVE KEY_COUNT
#define KEY_COUNT_6_OCTAVE 73

extern guimain global;

extern brightonLocations keys[KEY_COUNT_5OCTAVE];
extern brightonLocations keysprofile[KEY_COUNT_5OCTAVE];
extern brightonLocations keysprofile2[KEY_COUNT_5OCTAVE];
extern brightonLocations keys6octave[KEY_COUNT_6_OCTAVE];
extern brightonLocations keys6hammond[KEY_COUNT_6_OCTAVE];
extern brightonLocations keys2octave[KEY_COUNT_2OCTAVE];
extern brightonLocations keys2octave2[KEY_COUNT_2OCTAVE2];
extern brightonLocations keys3_octave[KEY_COUNT_3_OCTAVE];
extern brightonLocations keys3octave[KEY_COUNT_3OCTAVE];
extern brightonLocations keys3octave2[KEY_COUNT_3OCTAVE];
extern brightonLocations keys4octave[KEY_COUNT_4OCTAVE];
extern brightonLocations keys4octave2[KEY_COUNT_4OCTAVE];
extern brightonLocations pedalBoard[KEY_COUNT_PEDAL];
extern brightonLocations vkeys[VKEY_COUNT];
extern brightonLocations mods[2];
extern int bristolMidiControl(int, int, int, int, int);

extern int modCallback(brightonWindow *, int, int, float);
extern int keyCallback(brightonWindow *, int, int, float);
#endif /* BRIGHTONKEYS_H */

