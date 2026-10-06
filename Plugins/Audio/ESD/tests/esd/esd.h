#ifndef TEST_ESD_H
#define TEST_ESD_H
// Only the legacy API used by ESDClient; sockets below exercise real I/O.
typedef int esd_format_t;
enum { ESD_BITS16=1, ESD_STREAM=2, ESD_PLAY=4, ESD_RECORD=8, ESD_STEREO=16, ESD_MONO=32 };
extern "C" int esd_play_stream(esd_format_t, int, const char *, const char *);
extern "C" int esd_record_stream(esd_format_t, int, const char *, const char *);
#endif
