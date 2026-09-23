/*  MousePlugin
 *  Copyleft (C) 2002 Dan Bethell <dan@pawfal.org>
 *                    Dave Griffiths <dave@pawfal.org>
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
#ifndef SCRATCH_H
#define SCRATCH_H

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <termios.h>
#include <stdio.h>
#include <pthread.h>

#include <iostream>
#include <string>

class scratch {
   public:
      scratch (char *dev);
      ~scratch();
      void setData(char c);
      char getData();
      void stop();
   private:
      pthread_t scratch_thread;
      pthread_mutex_t mutex;
      char data;
};

void openSerialPort (char *dev);
void closeSerialPort();
void listen (scratch *scr);

#endif
