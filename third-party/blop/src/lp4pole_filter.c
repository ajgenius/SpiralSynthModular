/*
    lp4pole_filter.c - lp4pole filter admin.

    Copyright (C) 2003  Mike Rawes

    See lp4pole_filter.h for history

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

#include <stdlib.h>
#include <ladspa.h>
#include "lp4pole_filter.h"

LP4PoleFilter *
lp4pole_new (unsigned long sample_rate)
{
	LP4PoleFilter * lpf;

	lpf = (LP4PoleFilter *) malloc (sizeof (LP4PoleFilter));

	if (lpf) {
		lpf->inv_nyquist = 2.0f / (LADSPA_Data) sample_rate;
		lp4pole_init (lpf);
	}

	return lpf;
}

void
lp4pole_cleanup (LP4PoleFilter * lpf)
{
	if (lpf)
		free (lpf);
}

void
lp4pole_init (LP4PoleFilter * lpf)
{
	lpf->in1  = lpf->in2  = lpf->in3  = lpf->in4  = 0.0f;
	lpf->out1 = lpf->out2 = lpf->out3 = lpf->out4 = 0.0f;
	lpf->max_abs_in = 0.0f;
}
