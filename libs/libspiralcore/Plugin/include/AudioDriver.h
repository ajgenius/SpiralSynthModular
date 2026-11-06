/*  SpiralSound
 *  Copyleft (C) 2002 David Griffiths <dave@pawfal.org>
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

#include "Device.h"

#ifndef SPIRAL_AUDIODRIVER
#define SPIRAL_AUDIODRIVER

class AudioDriver : public Spiral::Core::Device
{
public:
	/* if this is an ALWAYS process then ProcessAudio must
	   always be called regardless whether it has something to 
	   process or not, so it is run along side GUI commands,
	   ala ExecuteCommands, and is run even if paused.
	   
	   If its MANUAL we are the ones doing the pushing of data
	   so we don't have to worry about it if we aren't hooked up,
	   and we do have to worry about synchronization with other 
	   plugins, so ProcessAudio should be run along side of
	   regular audio updates, ala Execute. This is eg. for
	   a File Output driver.
	   
	   NEVER means we never need to run ProcessAudio, eg,
	   a dummy audio driver.
	 */  
	enum AudioProcessType { ALWAYS, MANUAL, NEVER };
	
	AudioDriver() : ChangeBufferAndSampleRate(NULL) {}

	virtual bool IsAudioDriver() { return true; }

	virtual void ServiceAudio() {}

	virtual void ProcessAudio()=0;
	
	virtual AudioProcessType ProcessType() { return NEVER; }		

	// Callbacks to main engine. Should only called by plugin hosts.
	void SetChangeBufferAndSampleRateCallback(void(*s)(long unsigned int, long unsigned int, void *)) { ChangeBufferAndSampleRate = s; } ;

protected:
	void (*ChangeBufferAndSampleRate)(long unsigned int BufferSize, long unsigned int SampleRate, void *);	
};

#endif
