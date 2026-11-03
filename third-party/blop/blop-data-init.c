/* Runs a generated wavetable module's set-up.

   The same as blop-init.c, for the modules upstream's wavegen writes: their
   _init() fills in the table's lookup, and they have no _fini().
*/

void _init(void);

__attribute__((constructor)) static void blop_data_load(void) { _init(); }
