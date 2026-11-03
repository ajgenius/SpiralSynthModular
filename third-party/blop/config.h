/* Minimal stand-in for BLOP's own autoconf config.h.

   src/wavedata.c and src/include/math_func.h include config.h, which
   upstream generates with its own configure. SSM builds these plugins with
   its own build system and passes what they read on the command line (see
   Makefile.am), so this file only has to exist. It must stay in this
   directory, ahead of SSM's own config.h on the include path, so that this
   is the one found.
*/
