/*  SpiralSound
 *  Copyleft (C) 2001 David Griffiths <dave@pawfal.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "WaveChooser.h"
#include <string>
#include <FL/Fl_File_Chooser.H>
#include "../../../config.h"

using namespace std;

#ifdef USE_LIBSNDFILE
#include <sndfile.h>
#endif

// As this stands, we get one load dialog per plugin,
// I'd rather have just ONE dialog for all plugins

// large chunks of this are based on fl_file_chooser() from the FLTK source code

// strlcpy() and strlcat() are some really useful BSD string functions
// that work the way strncpy() and strncat() *should* have worked.

size_t our_strlcpy (char *dst, const char *src, size_t size) {
       size_t srclen;
       size--;
       srclen = strlen (src);
       if (srclen > size) srclen = size;
       memcpy (dst, src, srclen);
       dst[srclen] = '\0';
       return (srclen);
}

static Fl_File_Chooser *fc = (Fl_File_Chooser *)0;

static void (*current_callback)(const char*) = 0;

static void callback(Fl_File_Chooser *, void*) {
       if (current_callback && fc->value()) (*current_callback)(fc->value());
}

static char retname[1024];

char *WaveFileName (void) {
     string AvailFmt;
     #ifdef USE_LIBSNDFILE
            string FmtName;
            SF_FORMAT_INFO info;
            int major_count, m, p;
            sf_command (NULL, SFC_GET_FORMAT_MAJOR_COUNT, &major_count, sizeof (int));
            for (m = 0 ; m < major_count ; m++) {
                info.format = m;
                sf_command (NULL, SFC_GET_FORMAT_MAJOR, &info, sizeof (info));
                FmtName = info.name;
                while ((p=FmtName.find ('(')) >= 0 ) FmtName.replace (p, 1, "[");
                while ((p=FmtName.find (')')) >= 0 ) FmtName.replace (p, 1, "]");
                if (!AvailFmt.empty()) AvailFmt += '\t';
                AvailFmt += (string)FmtName + (string)" (*." + (string)info.extension + ')';
            }
     #else
            AvailFmt = "{*.wav,*.WAV}";
     #endif
     char *fname;
     char *title = "Load a wave";
     if (!fc) {
        fname = ".";
        // as ever, I'm a bit worried that this is never deallocated
        fc = new Fl_File_Chooser (fname, AvailFmt.c_str(), Fl_File_Chooser::CREATE, title);
        fc->callback (callback, 0);
     } else {
        // strip away the old filename, but keep the directory
        our_strlcpy (retname, fc->value(), sizeof(retname));
        char *p = strrchr(retname, '/');
        if (p) {
           if (p == retname) retname[1] = '\0';
           else *p = '\0';
        }
        fc->directory (retname);
     }
     fc->show();
     while (fc->shown()) Fl::wait();
     if (fc->value()) return (char *)fc->value();
     else return 0;
}
