/*
 * Provide double fallbacks for environments lacking sinf and
 * friends (e.g. Solaris)
 */

#ifndef math_func_h
#define math_func_h

#include "float_cast.h"
#include <math.h> /* Must be after float_cast.h */
#include "config.h"

#ifdef HAVE_SINF
/* Use float functions */
#define SINF(x)        sinf(x)
#define COSF(x)        cosf(x)
#define FABSF(x)       fabsf(x)
#define FLOORF(x)      floorf(x)
#define EXPF(x)        expf(x)
#define POWF(x,p)      powf(x,p)
#define COPYSIGNF(s,d) copysignf(s,d)
#define LRINTF(x)      lrintf(x)

#else
/* Use double functions */
#define SINF(x)        sin(x)
#define COSF(x)        cos(x)
#define FABSF(x)       fabs(x)
#define FLOORF(x)      floor(x)
#define EXPF(x)        exp(x)
#define POWF(x,p)      pow(x)
#define COPYSIGNF(s,d) copysign(s,d)
#define LRINTF(x)      lrint(x)

#endif

#endif
