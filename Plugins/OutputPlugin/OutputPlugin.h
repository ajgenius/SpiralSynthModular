/*  SpiralSound
 *  Copyleft (C) 2001 David Griffiths <dave@pawfal.org>
 */

#ifndef OutputPLUGIN
#define OutputPLUGIN

#include "../SpiralPlugin.h"
#include "OutputAudioClient.h"

class OutputPlugin : public AudioDriver
{
public:
	enum Mode {NO_MODE,INPUT,OUTPUT,DUPLEX,CLOSED};

	OutputPlugin();
	virtual ~OutputPlugin();

	virtual PluginInfo& Initialise(const HostInfo *Host);
	virtual SpiralGUIType*  CreateGUI();

	virtual void Execute();
	virtual void ExecuteCommands();

	virtual bool Kill();
	virtual void Reset();

	virtual bool IsAudioDriver() { return true; }
	virtual AudioProcessType ProcessType() { return AudioDriver::ALWAYS; }
	virtual void ProcessAudio();

	enum GUICommands {NONE, OPENREAD, OPENWRITE, OPENDUPLEX, CLOSE, SET_VOLUME, CLEAR_NOTIFY};
	float m_Volume;

	Mode GetMode() { return m_Mode; }

	virtual void StreamOut(std::ostream &s) {}
	virtual void StreamIn(std::istream &s)  {}
private:
	static int m_RefCount;
	static int m_NoExecuted;
	static Mode m_Mode;
	bool m_NotifyOpenOut;
	bool m_CheckedAlready;
};

#endif
