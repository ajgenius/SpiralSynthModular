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

#include "SpiralPlugin.h"
#include "JackClient.h"
#include <pthread.h>
using spiralcore::JackClient;

using namespace std;

#ifndef JackPLUGIN
#define JackPLUGIN

const int MAX_PORTS = 64;
const int MIN_PORTS = 2;

class JackPlugin : public AudioDriver
{
public:
 	JackPlugin();
	virtual ~JackPlugin();

	virtual PluginInfo& Initialise(const HostInfo *Host);
	
	/* General Plugin Function */
	virtual void	Execute();
	virtual void	ExecuteCommands();

	virtual bool	Kill();
	virtual void	Reset();
	
	/* Audio Driver Specific Functions */
	virtual bool			IsAudioDriver() { return true; }
	virtual AudioProcessType	ProcessType() { return AudioDriver::ALWAYS; }		
	virtual void			ProcessAudio();
	virtual void ServiceAudio();
	virtual bool IsCallbackDriver() const { return true; }

	/* Jack Plugin Specific Functions */
	int GetInputCount() const { return m_InputCount; }

	int GetOutputCount() const { return m_OutputCount; }

	JackClient *GetJackClient()           { return m_JackClient; }

        void SetNumberPorts (int nInputs, int nOutputs);

	enum GUICommands{NONE,UPDATE_NAMES,SET_PORT_COUNT,CHECK_PORT_CHANGES,ATTACH,DETACH,CONNECT_INPUT,CONNECT_OUTPUT,DISCONNECT_INPUT,DISCONNECT_OUTPUT};

	struct GUIArgs
	{
		int PortIndex;

		int NumInputs;

		int NumOutputs;
		char Port[256];
	};

	void Attach();
	void Detach();

	/* Jack Plugin Streaming - soon to be obsolete and for backward compatibility only*/
	virtual void	StreamOut(std::ostream &s);
	virtual void	StreamIn(std::istream &s);			
private:
	GUIArgs m_GUIArgs;	
	
	int m_Version;
	
	// slightly clumsy, but we have to share this data with the gui
	int  m_NumInputPortNames;
	char m_InputPortNames[MAX_PORTS][256];
	int  m_NumOutputPortNames;
	char m_OutputPortNames[MAX_PORTS][256];

	void GetPortNames(std::vector<std::string> &InputNames,std::vector<std::string> &OutputNames) { m_JackClient->GetPortNames(InputNames,OutputNames); }
	void ConnectInput(int n, const std::string &JackPort)  { m_JackClient->ConnectInput(n,JackPort); }
	void ConnectOutput(int n, const std::string &JackPort) { m_JackClient->ConnectOutput(n,JackPort); }

        void CreatePorts (int nInputs, int nOutputs, bool );

	bool		m_UpdateNames;
	bool		m_Connected;	
	JackClient 	*m_JackClient;
	int		m_JackInstanceID;
	
	//clunky work-around for unique ID
	static int JackInstanceCount;
	// Exchange with JACK independently of the host graph/control gate.
	pthread_mutex_t m_TransferLock;
	std::vector<float> m_Capture, m_Playback;
	static void ProcessCallback(void *context, unsigned int frames);
	int m_InputCount;

	int m_OutputCount;
};

#endif
