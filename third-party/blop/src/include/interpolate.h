#ifndef blop_interpolate_h
#define blop_interpolate_h

#include <ladspa.h>
#include <config.h>
#include "math_func.h"

/*****************************************************************************
 * Description: Interpolates between p0 and n0 taking the previous (p1)
 *              and next (n1) points into account, using a 3rd order
 *              polynomial (aka cubic spline)
 *
 *   Arguments: interval    Normalised time interval between inteprolated
 *                           sample and p0
 *              p1, p0      Samples prior to interpolated one
 *              n0, n1      Samples following interpolated one
 *
 *     Returns: interpolated sample
 *
 *        Code: Adapted from Steve Harris' plugin code
 *              swh-plugins-0.2.7/ladspa-util.h::cube_interp
 *              http://plugin.org.uk/releases/0.2.7/
 *
 *****************************************************************************/
static inline LADSPA_Data
interpolate_cubic (LADSPA_Data interval,
                   LADSPA_Data p1,
                   LADSPA_Data p0,
                   LADSPA_Data n0,
                   LADSPA_Data n1)
{
	return p0 + 0.5f * interval * (n0 - p1 +
	                   interval * (4.0f * n0 + 2.0f * p1 - 5.0f * p0 - n1 +
	                   interval * (3.0f * (p0 - n0) - p1 + n1)));
}

/*****************************************************************************
 * Description: Interpolates between p0 and n0 taking the previous (p1)
 *              and next (n1) points into account, using a 5th order
 *              polynomial.
 *
 *   Arguments: interval    Normalised time interval between inteprolated
 *                           sample and p0
 *              p1, p0      Samples prior to interpolated one
 *              n0, n1      Samples following interpolated one
 *
 *     Returns: interpolated sample
 *
 *        Code: http://www.musicdsp.org/archive.php?classid=5#62
 *
 *****************************************************************************/
static inline LADSPA_Data
interpolate_quintic (LADSPA_Data interval,
                     LADSPA_Data p1,
                     LADSPA_Data p0,
                     LADSPA_Data n0,
                     LADSPA_Data n1)
{
	return p0 + 0.5f * interval * (n0 - p1 +
	                   interval * (n0 - 2.0f * p0 + p1 +
	                   interval * ( 9.0f * (n0 - p0) + 3.0f * (p1 - n1) +
	                   interval * (15.0f * (p0 - n0) + 5.0f * (n1 - p1) +
	                   interval * ( 6.0f * (n0 - p0) + 2.0f * (p1 - n1))))));
}

/*
 * Linear interpolation
 */
static inline float
f_lerp (float value,
        float v1,
        float v2)
{
	value -= LRINTF (value - 0.5f);
	value *= (v2 - v1);
	value += v1;

	return value;
}

#endif /* blop_interpolate_h */
