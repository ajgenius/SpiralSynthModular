/* Runs each BLOP module's set-up and tear-down.

   Upstream's modules fill in their descriptors in _init() and free them in
   _fini(), and upstream links them with -nostartfiles so those replace the C
   runtime's own. A libtool module keeps the runtime's (two _init symbols),
   and macOS never calls _init at all, so the plugins would load with no
   descriptors. The build renames them instead (-D_init=blop_init
   -D_fini=blop_fini, in Makefile.am), and this file, compiled into every
   module with the same flags, runs them when the module is loaded and
   unloaded.
*/

void _init(void);
void _fini(void);

__attribute__((constructor)) static void blop_load(void) { _init(); }
__attribute__((destructor)) static void blop_unload(void) { _fini(); }
