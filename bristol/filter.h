
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

#ifndef FILTER_H
#define FILTER_H

#define VCF_FREQ_MAX  (0.825f)

typedef struct BristolFILTER {
	bristolOPSpec spec;
} bristolFILTER;

typedef struct BristolFILTERlocal {
	unsigned int flags;
	float Bout;
	float oSource;
	float output;
	float velocity;
	float history[1024];
	float cutoff;
	float a[8];
	float adash[8];
	float spc;

	float delay1, delay2, delay3, delay4, delay5;

	float out1, out2, out3, out4;

	float az1;
	float az2;
	float az3;
	float az4;
	float az5;
	float ay1;
	float ay2;
	float ay3;
	float ay4;
	float amf;
} bristolFILTERlocal;

#endif /* FILTER_H */

