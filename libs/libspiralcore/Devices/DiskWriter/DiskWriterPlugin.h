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

#ifndef DiskWriterPLUGIN
#define DiskWriterPLUGIN

#include "Device.h"
#include "DeviceClassRegistry.h"
#include "RiffWav.h"
#include "VoicePool.h"

// The recorder records what the output plays: the voices added together.
class DiskWriterPlugin : public Spiral::Core::Device, public VoiceSumInput
{
public:
	static const Spiral::Core::DeviceClass &Class();
 	DiskWriterPlugin();
	virtual ~DiskWriterPlugin();

	virtual PluginInfo& Initialise(const HostInfo *Host);
	virtual void 		Execute();
	virtual void 		ExecuteCommands();

	virtual void	    Describe(Spiral::Core::Description &d);
	virtual void	    Apply(Spiral::Core::Description::Reader &r);
	virtual void	    Migrate(Spiral::Core::Description &d);
	
	int GetBitsPerSample() { return AsSigned(m_BitsPerSample); }
	bool GetStereo() { return AsBoolean(m_Stereo); }

	enum GUICommands {NONE,OPENWAV,CLOSEWAV,RECORD,STOP};
	struct GUIArgs
	{
		char Name[256];
	};

private:
	GUIArgs m_GUIArgs;
	Spiral::Core::SetProperty m_BitsPerSample;
	Spiral::Core::ToggleProperty m_Stereo;
	Spiral::Core::ToggleProperty m_Open;
	Spiral::Core::ToggleProperty m_Recording;
	Spiral::Core::FloatProperty m_TimeRecorded;
	int m_Version;
	WavFile m_Wav;
};

#endif
