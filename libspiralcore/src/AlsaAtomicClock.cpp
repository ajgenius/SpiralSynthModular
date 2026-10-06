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

#include "AtomicClock.h"

AtomicClock::AtomicClock(float frequency) :
m_Time(0),
m_Frequency(frequency)
{
	int err;
	char timername[64];
	
	TimerDesc timer;
	timer.Class = SND_TIMER_CLASS_GLOBAL;
    timer.SClass = SND_TIMER_CLASS_NONE;
    timer.Card = 0;
    timer.Device = SND_TIMER_GLOBAL_SYSTEM;
    timer.Subdevice = 0;
    
	snd_timer_id_t *id;
    snd_timer_info_t *info;
    snd_timer_params_t *params;
	
	snd_timer_id_alloca(&id);
    snd_timer_info_alloca(&info);
    snd_timer_params_alloca(&params);
 		
    sprintf(timername, "hw:CLASS=%i,SCLASS=%i,CARD=%i,DEV=%i,SUBDEV=%i", timer.Class, timer.SClass, timer.Card, timer.Device, timer.Subdevice);
	
	if ((err = snd_timer_open(&m_Handle, timername, SND_TIMER_OPEN_NONBLOCK))<0) 
	{
    	fprintf(stderr, "timer open %i (%s)\n", err, snd_strerror(err));
        assert(0);
    }
		
	if ((err = snd_timer_info(m_Handle, info)) < 0) 
	{
		fprintf(stderr, "timer info %i (%s)\n", err, snd_strerror(err));
		assert(0);
    }
		 
	snd_timer_params_set_auto_start(params, 1);  
    snd_timer_params_set_ticks(params, (int)((1000000000 / snd_timer_info_get_resolution(info)) / frequency)); /* in Hz */
    //cerr<<snd_timer_info_is_slave(info)<<endl;
	if ((err = snd_timer_params(m_Handle, params)) < 0) 
	{
		fprintf(stderr, "timer params %i (%s)\n", err, snd_strerror(err));
 		exit(0);
    } 
		
	if ((err = snd_timer_start(m_Handle)) < 0) 
	{
		fprintf(stderr, "timer start %i (%s)\n", err, snd_strerror(err));
		exit(EXIT_FAILURE);
	}
}

double AtomicClock::Tick()
{
	struct pollfd *fds;
	int count = snd_timer_poll_descriptors_count(m_Handle);
    fds = (pollfd *) calloc(count, sizeof(struct pollfd));
	int err;
	
	if ((err = snd_timer_poll_descriptors(m_Handle, fds, count)) < 0) 
	{
        fprintf(stderr, "snd_timer_poll_descriptors error: %s\n", snd_strerror(err));
        return m_Time;
    }
	
    if ((err = poll(fds, count, 10000000)) < 0) 
	{
        fprintf(stderr, "poll error %i (%s)\n", err, strerror(err));
        return m_Time;
    }
    	
	if (err == 0) 
	{
    	fprintf(stderr, "timer time out!!\n");   
    }  
	
	snd_timer_read_t tr;
	while (snd_timer_read(m_Handle, &tr, sizeof(tr)) == sizeof(tr)) 
	{
    	//printf("TIMER: resolution = %uns, ticks = %u\n",tr.resolution, tr.ticks);
    }
	
	m_Time+=1/m_Frequency;
	
	return m_Time;
}
