// Copyright (C) 2003 David Griffiths <dave@pawfal.org>
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

#ifndef SPIRALCORE_CHANNEL_HANDLER_H
#define SPIRALCORE_CHANNEL_HANDLER_H

#include <pthread.h>
#include <string>
#include <map>
#include <vector>
#include <iostream>
#include "CommandRingBuffer.h"

namespace Spiral
{
namespace Core
{

class ChannelHandler
{
public:
    enum Type{INPUT,OUTPUT,OUTPUT_REQUEST,TABLE};

    ChannelHandler();
    ~ChannelHandler();

    // only call these from the audio thread

	// don't call Register*() when the audio thread is running
	// kind says what a typed Register() knows of the channel: 'i' a whole
	// number, 'f' a real one, 's' text. A command with no types of its own
	// has its arguments sent as its channels' kinds. A table of doubles is
	// registered 'd', so it is read back at their width.
    void     RegisterData(const std::string &ID, Type t, void *pData, int size, char kind=0);
    void     Register(const std::string &ID, bool* pData,  Type t=ChannelHandler::INPUT)    
		{ RegisterData(ID, t,(bool*)pData,sizeof(bool),'i'); }
    void     Register(const std::string &ID, char* pData,  Type t=ChannelHandler::INPUT)    
		{ RegisterData(ID, t,(char*)pData,sizeof(char),'i'); }
    void     Register(const std::string &ID, int* pData,   Type t=ChannelHandler::INPUT)
		{ RegisterData(ID, t,(int*)pData,sizeof(int),'i'); }
    void     Register(const std::string &ID, long* pData,   Type t=ChannelHandler::INPUT)    
		{ RegisterData(ID, t,(long*)pData,sizeof(long),'i'); }
    void     Register(const std::string &ID, short* pData, Type t=ChannelHandler::INPUT)    
		{ RegisterData(ID, t,(short*)pData,sizeof(short),'i'); }
    void     Register(const std::string &ID, float* pData, Type t=ChannelHandler::INPUT)    
		{ RegisterData(ID, t,(float*)pData,sizeof(float),'f'); }
    void     Register(const std::string &ID, double* pData, Type t=ChannelHandler::INPUT)    
		{ RegisterData(ID, t,(double*)pData,sizeof(double),'f'); }
    void     RegisterString(const std::string &ID, char *pData, Type t=ChannelHandler::INPUT)
		{ RegisterData(ID, t,(char*)pData,sizeof(char)*4096,'s'); }
    void     Register(const std::string &ID, void** pData, Type t=ChannelHandler::INPUT)    
		{ RegisterData(ID, t,(void**)pData,sizeof(void*)); }
	void     UpdateDataNow(unsigned long frame=0);
	// A published table changes only when the writer has a new set of rows.
	// UpdateDataNow swaps it into view under its usual nonblocking audio lock.
	void RegisterTable(const std::string &ID);
	void RegisterByteSnapshot(const std::string &ID);
	void PublishBytes(const std::string &ID, std::string &bytes);
	bool ReadByteSnapshot(const std::string &ID, std::vector<unsigned char> &bytes);
	void PublishTable(const std::string &ID, std::vector<double> &cells);
	bool ReadTable(const std::string &ID, unsigned long &revision, std::vector<double> &cells);
	void     SetUsed(const std::string &ID, int bytes);
	unsigned long GetFrame() const { return m_Frame; }
		
	bool     IsCommandWaiting() { return m_Command[0]; }
	char     GetCommand() { return m_Command[0]; }
	
	// use bulk transfers to copy variable and large sized data to the gui
	void	 SetupBulkTransfer(void *source) { m_BulkSrc=source; }
	
    // only call these from the gui thread
    // the ID comes from the order the channel is registered in
    void          GetData(const std::string &ID, void *data);
    const bool    GetBool(const std::string &ID)              { bool t; GetData(ID,&t); return t; }
    const char    GetChar(const std::string &ID)              { char t; GetData(ID,&t); return t; }
    const int     GetInt(const std::string &ID)               { int t; GetData(ID,&t); return t; }
    const long    GetLong(const std::string &ID)              { long t; GetData(ID,&t); return t; }
    const short   GetShort(const std::string &ID)             { short t; GetData(ID,&t); return t; }
    const float   GetFloat(const std::string &ID)             { float t; GetData(ID,&t); return t; }
    const double  GetDouble(const std::string &ID)            { double t; GetData(ID,&t); return t; }
    const void   *GetPtr(const std::string &ID)               { void *t; GetData(ID,&t); return t; }
    void   GetString(const std::string &ID,char* str)      { GetData(ID,str); }

    void          ReplaceData(const std::string &ID, void *pData, int size);

    void          SetData(const std::string &ID, void *s);
    void          Set(const std::string &ID, const bool& s)     { SetData(ID,(void*)&s); }
    void          Set(const std::string &ID, const char& s)     { SetData(ID,(void*)&s); }
    void          Set(const std::string &ID, const int& s)      { SetData(ID,(void*)&s); }
    void          Set(const std::string &ID, const long& s)     { SetData(ID,(void*)&s); }
    void          Set(const std::string &ID, const short& s)    { SetData(ID,(void*)&s); }
    void          Set(const std::string &ID, const float& s)    { SetData(ID,(void*)&s); }
    void          Set(const std::string &ID, const double& s)   { SetData(ID,(void*)&s); }
    void          SetString(const std::string &ID, const char* s);
    void          SetPtr(const std::string &ID, const void* s)   { SetData(ID,(void*)&s); }

	void          SetCommand(char command);

	// initialises the data from the audio side to the internal buffers
	void          FlushChannels();
	
	////////////////////////////////////////////
	// This is useful for getting data from the audio side immediately,
	// as it will block until complete, and for larger amounts of data, as
	// it won't be doing the copy every update.
		
	void 		  RequestChannelAndWait(const std::string &ID);
			
	///////////////////////////////////////////		
	// Use for copying large variable sized data very infrequently.
	// uses an OUTPUT_REQUEST channel as a transfer buffer.
	// Send the size needed to the gui down another channel.
	// This can be a slowww function, as it can take many audio updates
	// to complete, depending on the size of the buffer channel and the 
	// total size of the transfer.
	
	void 		  BulkTransfer(const std::string &ID, void *dest, int size);
	
	// delays for one or more updates. use for syncing commands
	void          Wait();

	void          RegisterCommand(const std::string &name, char command, const std::vector<std::string> &channels, const std::string &types = "");
	bool          CommandArguments(const std::string &name, std::vector<std::string> &channels, std::string *types = NULL) const;
	bool          SendCommand(const CommandRingBuffer::Command &command);
	bool          CommandsQueued() { return m_Commands && !m_Commands->Empty(); }

	int           ChannelSize(const std::string &ID, Type *t=NULL);
	// The bytes of a channel in use (SetUsed), and the kind it was registered as.
	int           ChannelUsed(const std::string &ID);
	char          ChannelKind(const std::string &ID);
	void          ChannelNames(std::vector<std::string> &names) const;
	bool          Peek(const std::string &ID, int offset, void *dest, int size);
	bool          SetDataAt(const std::string &ID, int offset, const void *src, int size);
	unsigned long Changes() const { return m_Changes; }
	
private:

    class Channel
    {
    public:
        Channel(Type t): type(t), tableRevision(1), tablePending(false) {}

        Type type;
		std::vector<double> pendingCells, cells;
		std::string pendingBytes, bytes;
		unsigned long tableRevision;
		bool tablePending;
		void FlushTable()
		{
			if (tablePending)
			{
				if (kind == 'B') bytes.swap(pendingBytes);
				else cells.swap(pendingCells);
				tablePending = false;
				++tableRevision;
			}
		}
        void *data_buf;
        int   size;
        int   used;
        void *data;
		char  kind;
		bool requested;
		bool updated;
    };

    std::map<std::string,Channel*> m_ChannelMap;
    std::vector<std::map<std::string,Channel*>::iterator> m_Order;
	char m_Command[2];

	struct NamedCommand
	{
		char Command;
		std::vector<std::string> Channels;
		std::string Types;
	};
	std::map<std::string,NamedCommand> m_NamedCommands;
	CommandRingBuffer *m_Commands;
	void TakeCommand();
	unsigned long m_Frame;
	
	bool m_UpdateIndicator;
	bool m_Pending;
	unsigned long m_Changes;
	
	void *m_BulkSrc;
	
	int   m_BulkSize;
	int   m_BulkPos;
	std::string m_BulkID;
	
    pthread_mutex_t* m_Mutex;	
};
}
}
#endif
