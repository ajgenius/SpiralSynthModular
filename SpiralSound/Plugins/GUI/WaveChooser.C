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
#include <FL/fl_file_chooser.h>
#include "../../../config.h"

#ifdef USE_LIBSNDFILE
#include <sndfile.h>
#endif

char *WaveFileName (void) {
     string AvailFmt;
#ifdef USE_LIBSNDFILE
     string FmtName;
     SF_FORMAT_INFO  info;
     int major_count, m, p;
     sf_command (NULL, SFC_GET_FORMAT_MAJOR_COUNT, &major_count, sizeof (int));
     for (m = 0 ; m < major_count ; m++) {
         info.format = m;
         sf_command (NULL, SFC_GET_FORMAT_MAJOR, &info, sizeof (info));
         FmtName = info.name;
         while ((p=FmtName.find ('(')) >= 0 ) FmtName.replace (p, 1, '[');
         while ((p=FmtName.find (')')) >= 0 ) FmtName.replace (p, 1, ']');
         if (!AvailFmt.empty()) AvailFmt += '\t';
         AvailFmt += (string)FmtName + (string)" (*." + (string)info.extension + ')';
     }
#else
      AvailFmt = "{*.wav,*.WAV}";
#endif
      char *fn=fl_file_chooser("Load a sample", AvailFmt.c_str(), NULL);
      return fn;
}
