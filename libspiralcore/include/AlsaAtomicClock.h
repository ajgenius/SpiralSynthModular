// Copyright (C) 2004 David Griffiths <dave@pawfal.org>
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

#include <iostream>
#include <alsa/asoundlib.h>

using namespace std;

class AtomicClock
{
public:
	AtomicClock(float frequency);
	~AtomicClock() {snd_timer_close(m_Handle);}
	double Tick();
	
private:

	snd_timer_t *m_Handle;
    snd_timer_info_t *m_Info;
    snd_timer_params_t *m_Params;	
	
	double m_Time;
	float m_Frequency;
	
	struct TimerDesc
	{
		int Class;
    	int SClass;
    	int Card;
    	int Device;
    	int Subdevice;
	};
};	
