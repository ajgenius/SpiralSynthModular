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
#include "ComplexEnvelopePlugin.h"
#include "ComplexEnvelopePluginGUI.h"
#include <Fl/Fl_Button.H>
#include "SpiralIcon.xpm"

extern "C" {
SpiralPlugin* SpiralPlugin_CreateInstance()
{
	return new ComplexEnvelopePlugin;
}

char** SpiralPlugin_GetIcon()
{
	return SpiralIcon_xpm;
}

int SpiralPlugin_GetID()
{
	return 33;
}

string SpiralPlugin_GetGroupName()
{
	return "SpiralSound";
}
}

///////////////////////////////////////////////////////

ComplexEnvelopePlugin::ComplexEnvelopePlugin() :
m_Triggered(0),
m_Position(-1),
m_SampleTime(2.0f)
{
	m_PluginInfo.Name="ComplexEnv";
	m_PluginInfo.Width=320;
	m_PluginInfo.Height=200;
	m_PluginInfo.NumInputs=2;
	m_PluginInfo.NumOutputs=2;
	m_PluginInfo.PortTips.push_back("Trigger CV");	
	m_PluginInfo.PortTips.push_back("Input");	
	m_PluginInfo.PortTips.push_back("CV");	
	m_PluginInfo.PortTips.push_back("Output");
}

ComplexEnvelopePlugin::~ComplexEnvelopePlugin()
{
}

PluginInfo &ComplexEnvelopePlugin::Initialise(const HostInfo *Host)
{	
	PluginInfo& Info = SpiralPlugin::Initialise(Host);		
	CVListToEnvelope(m_CVVec);
	return Info;
}

SpiralGUIType *ComplexEnvelopePlugin::CreateGUI()
{
	m_GUI = new ComplexEnvelopePluginGUI(m_PluginInfo.Width,
								  	    m_PluginInfo.Height,
										this,m_HostInfo);
	m_GUI->hide();		
	return m_GUI;
}

void ComplexEnvelopePlugin::CVListToEnvelope(const vector<Vec2> &CVVec)
{
	m_EnvSample.Zero();

	m_CVVec=CVVec;

	// check length
	if (m_EnvSample.GetLength()/m_HostInfo->SAMPLERATE!=m_SampleTime)
	{
		m_EnvSample.Clear();
		m_EnvSample.Allocate((int)m_SampleTime*m_HostInfo->SAMPLERATE);
	}
	
	vector<Vec2> Lines;
	
	if (m_Bezier)
	{
		// calculate the bezier spline that forms the line segments
		for (vector<Vec2>::const_iterator i=CVVec.begin();
			 i!=CVVec.end(); i++)
		{
			AddToSpline(*i);
		}					
		CalculateBezierSpline(&Lines,10);		
	}	
	else
	{
		// use the CV's as line endpoints for straight line interpolation
		Lines=CVVec;
	}
	
	vector<Vec2>::const_iterator i=Lines.begin();
	if (i==Lines.end()) return; 	// no lines
	vector<Vec2>::const_iterator ni=i+1;
	if (ni==Lines.end()) return; 	// no lines
			
	for (int n=0; n<m_EnvSample.GetLength(); n++)
	{
		float gt=n/(float)m_EnvSample.GetLength();
		float t=(gt-i->x)/(ni->x-i->x);				
		if (t<0) t=0;
	
		// see if we need to find the next line
		if (gt>=ni->x)
		{
			i++;
			ni++;		
			// env has ended before the sample length
			if (ni==Lines.end())
			{
				// todo: fill the rest with last value
				return;
			} 						
			t=Linear(i->x,ni->x,gt,0.0f,1.0f);
			if (t<0) t=0;
		}		
		float aa=Linear(0.0f,1.0f,t,ni->y,i->y);		
		m_EnvSample.Set(n,aa);		
	}	
}

void ComplexEnvelopePlugin::Execute()
{
	float temp=0;
	bool Freeze=false;
	
	if (m_EnvSample.GetLength()==0) return;
	
	if (GetInput(0,0)>0)
	{
		if(!m_Triggered)
		{
			m_Triggered=1.0f;		
			m_Position=0;		
		}
	}
	else
	{
		if(m_Triggered)
		{
			m_Triggered=0;
		}
	}
	
	if (m_Position==-1) 
	{
		GetOutputBuf(0)->Zero();
		return;
	}
	
	
	if (!InputExists(1))
	{
		for (int n=0; n<m_HostInfo->BUFSIZE; n++)
		{	
			// see if we've fallen off the end
			if (m_Position>m_EnvSample.GetLength())
			{
				m_Position=-1;
				break;
			}
			
			SetOutput(0,n,m_EnvSample[m_Position]);
			m_Position++;
		} 
	}
	else
	{
		for (int n=0; n<m_HostInfo->BUFSIZE; n++)
		{	
			// see if we've fallen off the end
			if (m_Position>m_EnvSample.GetLength())
			{
				m_Position=-1;
				break;
			}
			
			SetOutput(0,n,m_EnvSample[m_Position]);
			SetOutput(1,n,GetInput(1,n)*m_EnvSample[m_Position]);
			m_Position++;			
		} 
	}
}

void ComplexEnvelopePlugin::StreamOut(ostream &s)
{
	s<<m_Version<<" ";
	s<<m_Position<<" "<<m_SampleTime<<" "<<m_Bezier<<" ";
	
	s<<m_CVVec.size()<<endl;
	for (vector<Vec2>::iterator i=m_CVVec.begin();
		 i!=m_CVVec.end(); i++)
	{
		s<<i->x<<" "<<i->y<<endl;
	}
}

void ComplexEnvelopePlugin::StreamIn(istream &s)
{
	int version;
	s>>version;	
	s>>m_Position>>m_SampleTime>>m_Bezier;	
	
	m_CVVec.clear();
	int size;
	s>>size;
	for(int n=0; n<size; n++)
	{
		Vec2 t;
		s>>t.x>>t.y;
		m_CVVec.push_back(t);
	}
	
	((ComplexEnvelopePluginGUI*)m_GUI)->SetCVList(m_CVVec);
	CVListToEnvelope(m_CVVec);
}
