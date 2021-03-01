
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

#ifndef BRIGHTON_MENU_H
#define BRIGHTON_MENU_H

typedef struct brightonMenuItem {
	int type;
	unsigned int flags;
	char *name;
	char *text;
	struct BrightonMenu *submenu;
	int *value; /* Pointer to value represented */
	int ar, ag, ab; /* default active color */
	int pr, pg, pb; /* default inactive color */
} brightonMenuItem;

typedef struct BrightonMenu {
	int type;
	unsigned int flags;
	char *name;
	brightonMenuItem *entry[];
	int x, y; /* location of drawing */
	int current; /* current selection */
	int panel, device; /* device relations */
	int r, g, b; /* default menu color */
	int br, bg, bb; /* default entry border color */
	int abr, abg, abb; /* default active BG color */
	int pbr, pbg, pbb; /* default inactive BG color */
	int size; /* font sizes, just one to start! */
} brightonMenu;

#endif /* BRIGHTON_MENU_H */

