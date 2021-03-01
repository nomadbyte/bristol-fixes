
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

#ifndef AKSFILTER_H
#define AKSFILTER_H

#define VCF_FREQ_MAX  (0.825f)
#define AKS_WAVE_SZE 1024
#define AKS_WAVE_SZE_H 512
#define AKS_WAVE_SZE_M 1023

typedef struct BristolAKSFILTER {
	bristolOPSpec spec;
	float *wave;
} bristolAKSFILTER;

typedef struct BristolAKSFILTERlocal {
	unsigned int flags;
	float step;
	float Bout;
	float Eout;
	float hp;
	float roc;
} bristolAKSFILTERlocal;

#endif /* AKSFILTER_H */

