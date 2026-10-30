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

#include "Device.h"

#ifndef NoteSnapPLUGIN
#define NoteSnapPLUGIN

class NoteSnapPlugin : public Spiral::Core::Device
{
public:
 	NoteSnapPlugin();
	virtual ~NoteSnapPlugin();
	
	virtual PluginInfo& Initialise(const HostInfo *Host);
	virtual void 		Execute();
	virtual void 		Reset();
	virtual void 		ExecuteCommands();
	virtual void	    Migrate(Spiral::Core::Description &d);

	bool GetFilter(int n) { return m_Filter[n].Value.AsBoolean; }
	void SetFilter(int n, bool v) { m_Filter[n].Value.AsBoolean = v; }

	enum GUICommands{NONE,NOTE_ON,NOTE_OFF};

private:

	int  m_Note;
	Spiral::Core::ToggleProperty m_Filter[12];
	float m_Out;
	float m_LastFreq;
	int   m_LastNote;
};

#endif
