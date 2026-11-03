/*
    common.h - Blah. Some stuff

    Copyright (C) 2002  Mike Rawes

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#ifndef blop_common_h
#define blop_common_h

#include "math_func.h"

/* Handy constants and macros */

/*
 * Smallest generated non-zero float
 * Used for pre-empting denormal numbers
 */
#ifndef SMALLEST_FLOAT
#define SMALLEST_FLOAT (1.0 / (float)0xFFFFFFFF)
#endif

/*
 * Clip without branch (from http://musicdsp.org)
 */

static inline float
f_min (float x, float a)
{
	return a - (a - x + FABSF (a - x)) * 0.5f;
}
static inline float
f_max (float x, float b)
{
	return (x - b + FABSF (x - b)) * 0.5f + b;
}
static inline float
f_clip (float x, float a, float b)
{
	return 0.5f * (FABSF (x - a) + a + b - FABSF (x - b));
}

#endif /* blop_common_h */
