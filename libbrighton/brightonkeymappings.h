
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
/* #define BRISTOL_FIXEDWIDTH */

/*
 * This needs to be changed to allow it to have a couple of different fonts,
 * at least to include another one for the menus - a little larger. This would
 * give access to 3 fonts, this one totally compact, the new one, and the new
 * one rendered in double res
 */

#define KEYTAB_SIZE 256
#define KEY_HEIGHT 5

#define BIT_ON 1
#define BIT_OFF 2

typedef struct Bkey {
	int map;
	int width;
	unsigned char bitmap[KEY_HEIGHT];
} bkey;

bkey key[KEYTAB_SIZE];

void
brightonsetkey(int index, int width, unsigned char ch0, unsigned char ch1,
unsigned char ch2, unsigned char ch3, unsigned char ch4)
{
	if (index >= KEYTAB_SIZE)
		return;

	key[index].map = index;

#ifdef BRISTOL_FIXEDWIDTH
	key[index].width = 5;
#else
	key[index].width = width;
#endif

	key[index].bitmap[0] = ch0;
	key[index].bitmap[1] = ch1;
	key[index].bitmap[2] = ch2;
	key[index].bitmap[3] = ch3;
	key[index].bitmap[4] = ch4;
}

void
initkeys()
{
	int i;

	for (i = 0; i < KEYTAB_SIZE; i++)
	{
		key[i].map = -1;
		key[i].width = -1;
	}

	key[0].width = 4;
	key[0].bitmap[0] = 0xf0;
	key[0].bitmap[1] = 0xf0;
	key[0].bitmap[2] = 0xf0;
	key[0].bitmap[3] = 0xf0;
	key[0].bitmap[4] = 0xf0;

	brightonsetkey(' ', 3, 0x00, 0x00, 0x00, 0x00, 0x00);

	brightonsetkey('A', 5, 0x20, 0x50, 0x50, 0xa8, 0x88);
	brightonsetkey('B', 4, 0xe0, 0x90, 0xe0, 0x90, 0xe0);
	brightonsetkey('C', 4, 0x60, 0x90, 0x80, 0x90, 0x60);
	brightonsetkey('D', 4, 0xe0, 0x90, 0x90, 0x90, 0xe0);
	brightonsetkey('E', 3, 0xf0, 0x80, 0xd0, 0x80, 0xf0);
	brightonsetkey('F', 3, 0xf0, 0x80, 0xd0, 0x80, 0x80);

	brightonsetkey('G', 4, 0x60, 0x90, 0x80, 0xb0, 0x60);
	brightonsetkey('H', 4, 0x90, 0x90, 0xf0, 0x90, 0x90);
#ifdef BRISTOL_FIXEDWIDTH
	brightonsetkey('I', 1, 0x70, 0x20, 0x20, 0x20, 0x70);
#else
	brightonsetkey('I', 1, 0x80, 0x80, 0x80, 0x80, 0x80);
#endif
	brightonsetkey('J', 4, 0x70, 0x20, 0x20, 0xa0, 0x40);
	brightonsetkey('K', 4, 0x90, 0xa0, 0xc0, 0xa0, 0x90);
	brightonsetkey('L', 4, 0x80, 0x80, 0x80, 0x80, 0xf0);

	brightonsetkey('M', 5, 0x88, 0xd8, 0xa8, 0x88, 0x88);
	brightonsetkey('N', 4, 0x90, 0xd0, 0xb0, 0x90, 0x90);
	brightonsetkey('O', 4, 0x60, 0x90, 0x90, 0x90, 0x60);
	brightonsetkey('P', 4, 0xe0, 0x90, 0xe0, 0x80, 0x80);
	brightonsetkey('Q', 4, 0x60, 0x90, 0x90, 0xb0, 0x70);
	brightonsetkey('R', 4, 0xe0, 0x90, 0xe0, 0xa0, 0x90);
	brightonsetkey('S', 4, 0x70, 0x80, 0x60, 0x10, 0xe0);
	brightonsetkey('T', 5, 0xf8, 0x20, 0x20, 0x20, 0x20);

	brightonsetkey('U', 4, 0x90, 0x90, 0x90, 0x90, 0x60);
	brightonsetkey('V', 5, 0x88, 0x88, 0x50, 0x50, 0x20);
	brightonsetkey('W', 5, 0x88, 0x88, 0xa8, 0xd8, 0x88);
	brightonsetkey('X', 4, 0x90, 0x90, 0x60, 0x60, 0x90);
	brightonsetkey('Y', 5, 0x88, 0x50, 0x20, 0x20, 0x20);
	brightonsetkey('Z', 4, 0xf0, 0x10, 0x20, 0x40, 0xf0);

	brightonsetkey('0', 4, 0x60, 0x90, 0x90, 0x90, 0x60);
#ifdef BRISTOL_FIXEDWIDTH
	brightonsetkey('1', 5, 0x60, 0x20, 0x20, 0x20, 0x70);
#else
	brightonsetkey('1', 3, 0x40, 0xc0, 0x40, 0x40, 0xe0);
#endif
	brightonsetkey('2', 4, 0x60, 0x90, 0x20, 0x40, 0xf0);
	brightonsetkey('3', 4, 0x60, 0x90, 0x20, 0x90, 0x60);
	brightonsetkey('4', 4, 0x10, 0x30, 0x50, 0xf0, 0x10);
	brightonsetkey('5', 4, 0xf0, 0x80, 0xe0, 0x10, 0xe0);

	brightonsetkey('6', 4, 0x60, 0x80, 0xe0, 0x90, 0x60);
	brightonsetkey('7', 4, 0xf0, 0x10, 0x20, 0x40, 0x40);
	brightonsetkey('8', 4, 0x60, 0x90, 0x60, 0x90, 0x60);
	brightonsetkey('9', 4, 0x60, 0x90, 0x70, 0x10, 0x60);

	brightonsetkey('-', 3, 0x00, 0x00, 0xe0, 0x00, 0x00);
	brightonsetkey('_', 3, 0x00, 0x00, 0x00, 0x00, 0x70);
	brightonsetkey(':', 1, 0x00, 0x80, 0x00, 0x80, 0x00);
	brightonsetkey(';', 1, 0x00, 0x80, 0x00, 0x80, 0x00);
	brightonsetkey('?', 4, 0x60, 0x90, 0x20, 0x00, 0x20);
	brightonsetkey('/', 5, 0x08, 0x10, 0x20, 0x40, 0x80);
	brightonsetkey('.', 1, 0x00, 0x00, 0x00, 0x00, 0x80);
	brightonsetkey('+', 3, 0x00, 0x40, 0xe0, 0x40, 0x00);

	key['a'].map = 'A';
	key['b'].map = 'B';
	key['c'].map = 'C';
	key['d'].map = 'D';
	key['e'].map = 'E';
	key['f'].map = 'F';
	key['g'].map = 'G';
	key['h'].map = 'H';
	key['i'].map = 'I';
	key['j'].map = 'J';
	key['k'].map = 'K';
	key['l'].map = 'L';
	key['m'].map = 'M';

	key['n'].map = 'N';
	key['o'].map = 'O';
	key['p'].map = 'P';
	key['q'].map = 'Q';
	key['r'].map = 'R';
	key['s'].map = 'S';
	key['t'].map = 'T';
	key['u'].map = 'U';
	key['v'].map = 'V';
	key['w'].map = 'W';
	key['x'].map = 'X';
	key['y'].map = 'Y';
	key['z'].map = 'Z';

}

