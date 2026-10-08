/* Minimal stand-in for swh-plugins own autoconf config.h.

   The vendored sources include "config.h" (and util/ includes "../config.h"),
   which upstream generates with its own configure. SSM builds these plugins
   with its own build system, so this file supplies the two definitions the
   sources actually read. It must stay in this directory, and the plugins are
   compiled without SSM own build directory on the include path, so that
   this is the config.h they find.
*/
#define HAVE_LRINTF 1
