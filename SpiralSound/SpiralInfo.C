/*  SpiralSound
 *  Copyleft (C) 2000 David Griffiths <dave@pawfal.org>
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

#include <iostream>
#include <fstream>
#include <stdlib.h>
#include "FL/fl_draw.h"
#include "FL/fl_message.h"
#include "SpiralInfo.h"
#include "../SpiralSynthPluginLocation.h"


//#define DEBUG_CONFIG

float RandFloat (float s, float e) {
      return s+((rand()%10000/10000.0)*(e-s));
}




string SpiralInfo::LOCALE      = "EN";
int    SpiralInfo::BUFSIZE     = 512;
int    SpiralInfo::FRAGSIZE    = 256;
int    SpiralInfo::FRAGCOUNT   = 8;
int    SpiralInfo::SAMPLERATE  = 44100;
long   SpiralInfo::MAXSAMPLE   = 32767;
float  SpiralInfo::VALUECONV   = 1.0f/MAXSAMPLE;
bool   SpiralInfo::WANTMIDI    = false;
int    SpiralInfo::FILTERGRAN  = 50;
string SpiralInfo::OUTPUTFILE  = "/dev/dsp";
string SpiralInfo::MIDIFILE    = "/dev/midi";
int    SpiralInfo::POLY        = 1;
bool   SpiralInfo::USEPLUGINLIST = false;
unsigned SpiralInfo::GUI_COLOUR = 179;
unsigned SpiralInfo::SCOPE_BG_COLOUR = fl_rgb_color (20, 60, 20);
unsigned SpiralInfo::SCOPE_FG_COLOUR = fl_rgb_color (100, 200, 100);
unsigned SpiralInfo::SCOPE_SEL_COLOUR = FL_WHITE;
unsigned SpiralInfo::SCOPE_IND_COLOUR = fl_rgb_color (203, 255, 0);
unsigned SpiralInfo::SCOPE_MRK_COLOUR = fl_rgb_color (155, 155, 50);

/*int SpiralInfo::GUICOL_Tool=179;
int SpiralInfo::GUICOL_Button=181;
int SpiralInfo::GUICOL_Canvas=181;
int SpiralInfo::GUICOL_Device=181;
int SpiralInfo::GUIDEVICE_Box=30;*/

int SpiralInfo::GUICOL_Tool=48;
int SpiralInfo::GUICOL_Button=42;
int SpiralInfo::GUICOL_Canvas=50;
int SpiralInfo::GUICOL_Device=52;
int SpiralInfo::GUIDEVICE_Box=30;

vector<string> SpiralInfo::PLUGINVEC;

string SpiralInfo::PLUGIN_PATH;

SpiralInfo* SpiralInfo::m_SpiralInfo=NULL;

SpiralInfo* SpiralInfo::Get() {
   if (!m_SpiralInfo) {
      m_SpiralInfo = new SpiralInfo;
   }
   return m_SpiralInfo;
}

SpiralInfo::SpiralInfo():
m_Version(2)
{
   m_ResFileName = getenv("HOME");
   m_ResFileName += "/.spiralmodular";
   PLUGIN_PATH = PLUGIN_PATH_LOCATION;
}

void SpiralInfo::LoadPrefs() {
     ifstream i (m_ResFileName.c_str());
     if (!i) {
        cerr << "Creating " << m_ResFileName << endl;
        SavePrefs ();
        return;
     }
     StreamInPrefs (i);
}

void SpiralInfo::SavePrefs() {
     ofstream o (m_ResFileName.c_str());
     StreamOutPrefs (o);
}

void SpiralInfo::StreamOutMainColourPrefs (ostream &s)
{
	s << "[MAIN COLOURS]" << endl;
	s << "ToolBoxColour     = " << GUICOL_Tool << endl;
	s << "ButtonColour      = " << GUICOL_Button << endl;
	s << "CanvasColour      = " << GUICOL_Canvas << endl;
	s << "DeviceColour      = " << GUICOL_Device << endl;
	s << "DeviceBoxType     = " << GUIDEVICE_Box << endl;

	s << endl; //section spacer
}

bool SpiralInfo::StreamInMainColourPrefs (istream &s, string &section, string &ident)
{
	char temp[256];
	bool result = false;
	
	/* just ignore if this isn't calling us */						
	if (section == "MAIN COLOURS")
	{
		if (ident == "ToolBoxColour")
			s >> GUICOL_Tool;
								
		else if (ident == "ButtonColour")
			s >> GUICOL_Button;

		else if (ident == "CanvasColour")
			s >> GUICOL_Canvas;

		else if (ident == "DeviceColour")
			s >> GUICOL_Device;

		else if (ident == "DeviceBoxType")
			s >> GUIDEVICE_Box;
		
		else
			/* ignore idents that don't belong here/we don't know about */
			s >> temp;	
			
		result = true;	
	}
	
	return result;
}

void SpiralInfo::StreamOutDeviceColourPrefs (ostream &s)
{
	s << "[DEVICE COLOURS]" << endl;
	s << "GUIColour         = " << GUI_COLOUR << endl;
	s << "ScopeBGColour     = " << SCOPE_BG_COLOUR << endl;
	s << "ScopeFGColour     = " << SCOPE_FG_COLOUR << endl;
	s << "ScopeSelColour    = " << SCOPE_SEL_COLOUR << endl;
	s << "ScopeIndColour    = " << SCOPE_IND_COLOUR << endl;
	s << "ScopeMrkColour    = " << SCOPE_MRK_COLOUR << endl;

	s << endl; //section spacer
}

bool SpiralInfo::StreamInDeviceColourPrefs (istream &s, string &section, string &ident)
{
	char temp[256];
	bool result = false;
	
	/* just ignore if this isn't calling us */						
	if (section == "DEVICE COLOURS")
	{
		if (ident == "GUIColour")
			s >> GUI_COLOUR;
						
		else if (ident == "ScopeBGColour")
			s >> SCOPE_BG_COLOUR;

		else if (ident == "ScopeFGColour")
			s >> SCOPE_FG_COLOUR;

		else if (ident == "ScopeSelColour")
			s >> SCOPE_SEL_COLOUR;

		else if (ident == "ScopeIndColour")
			s >> SCOPE_IND_COLOUR;

		else if (ident == "ScopeMrkColour")
			s >> SCOPE_MRK_COLOUR;
							
		else
			/* ignore idents that don't belong here/we don't know about */
			s >> temp; 
			
		result = true;	
	}
	
	return result;
}

void SpiralInfo::StreamOutAudioSettingsPrefs (ostream &s)
{
	s << "[AUDIO SETTINGS]" << endl;

	s << "BufferSize        = " << BUFSIZE << endl;
	s << "FragmentSize      = " << FRAGSIZE << endl;
	s << "FragmentCount     = " << FRAGCOUNT << endl;
	s << "Samplerate        = " << SAMPLERATE << endl;
	s << "WantMidi          = " << WANTMIDI << endl;
	s << "FilterGranularity = " << FILTERGRAN << endl;
	s << "Output            = " << OUTPUTFILE << endl;
	s << "Midi              = " << MIDIFILE << endl;
	s << "UsePluginList     = " << USEPLUGINLIST << endl;
	s << "Polyphony         = " << POLY << endl;

	s << endl; //section spacer
}

bool SpiralInfo::StreamInAudioSettingsPrefs (istream &s, string &section, string &ident)
{
	char temp[256];
	bool result = false;
	
	/* just ignore if this isn't calling us */						
	if (section == "AUDIO SETTINGS")
	{
		if (ident == "BufferSize")
			s >> BUFSIZE;
								
		else if (ident == "FragmentSize")
			s >> FRAGSIZE;

		else if (ident == "FragmentCount")
			s >> FRAGCOUNT;

		else if (ident == "Samplerate")
			s >> SAMPLERATE;
		
		else if (ident == "WantMidi")
			s >> WANTMIDI;

		else if (ident == "FilterGranularity")
			s >> FILTERGRAN;

		else if (ident == "Output")
			s >> OUTPUTFILE;

		else if (ident == "Midi")
			s >> MIDIFILE;

		else if (ident == "UsePluginList")
			s >> USEPLUGINLIST;

		else if (ident == "Polyphony")
			s >> POLY;

		else
			/* ignore idents that don't belong here/we don't know about */
			s >> temp;	
			
		result = true;	
	}
	
	return result;
}

void SpiralInfo::StreamInPrefs (istream &s) 
{
     // These lines are from SpiralInfo
     char temp[256];
     
     s >> temp >> temp >> temp;
     s >> temp >> temp >> m_Version;
     s >> temp >> temp >> LOCALE;

	switch (m_Version)
	{
		case 2:
		{
			char test;
			string section;

			 /* skip next line, since this is just a spacer between file header
			    and the rest of the file config
			 */   
			s.seekg (2, ios::cur );
			
			while (! s.eof ())
			{
				/* peek first char to find out if this is a delimiter line
				   a header line, or a regular section 
				*/
				test = s.peek();

				switch (test)
				{
					/* if the line starts with an open bracket just presume its a header */
					case '[':
					{
						/* skip open bracket */
						s.ignore(1);

						/* read header line up to close bracket or end of line */
						s.getline (temp, 256, ']');
						
						/* move to next line */
						s.seekg (1, ios::cur );  
						
						/* assign current section string for easy testing */
						section = temp;
						cout << "at section " + section << endl;
						
						/* the plugins format is a bit odd so we 
						   do it all here at once mostly the same 
						   as the old way
						*/  
						if (section == "PLUGINS")
						{
							s >> temp >> temp >> PLUGIN_PATH;
							s >> temp >> temp;
			
							string st;
							PLUGINVEC.clear();
							while (st!="end" && !s.eof()) 
							{
								s >> st;
								if (st!="end" && USEPLUGINLIST) PLUGINVEC.push_back (st);
							}	

							/* move to next line */
							s.seekg (1, ios::cur );  
						}						
					}
					break;

					case '\n':
					{
						 /* skip this line, its just a spacer between section */
						s.seekg (1, ios::cur );
					}					
					break;
					
					default:
					{
						string ident;
						s >> ident;//read in identifier name
						s >> temp;//skip "="

						/* Check to see if this a Audio Settings section */
						if (StreamInAudioSettingsPrefs(s, section, ident)) { 
#ifdef DEBUG_CONFIG
							cout << "successfully read " + section + ", " + ident << endl; 
#endif
						/* Check next to see if this a Main Colour section */
						} else if (StreamInMainColourPrefs(s, section, ident)) { 
#ifdef DEBUG_CONFIG
								cout << "successfully read " + section + ", " + ident << endl;  
#endif
						/* Check next to see if this a Device Colour section */
						} else if (StreamInDeviceColourPrefs(s, section, ident)) { 
#ifdef DEBUG_CONFIG
								cout << "successfully read " + section + ", " + ident << endl;  
#endif
						/* Ignore idents for sections that are invalid or we don't know about */
						} else {
							s >> temp;
#ifdef DEBUG_CONFIG
							cout << "ignoring. don't know how to handle, " + section + ", " + ident << endl;  
#endif
						}	

						s.seekg (1, ios::cur );  //move to next line
					}
				}	
			}		
		}
    		break;
     		
		default: //old file fallback
		{
			s >> temp >> temp >> BUFSIZE;
			s >> temp >> temp >> FRAGSIZE;
			s >> temp >> temp >> FRAGCOUNT;
			s >> temp >> temp >> SAMPLERATE;

			s >> temp >> temp >> WANTMIDI;
			s >> temp >> temp >> FILTERGRAN;

			s >> temp >> temp >> OUTPUTFILE;
			s >> temp >> temp >> MIDIFILE;

			s >> temp >> temp >> USEPLUGINLIST;

			s >> temp >> temp >> POLY;
			if (m_Version>0) {
				s >> temp >> temp >> GUI_COLOUR;
				s >> temp >> temp >> SCOPE_BG_COLOUR;
				s >> temp >> temp >> SCOPE_FG_COLOUR;
				s >> temp >> temp >> SCOPE_SEL_COLOUR;
				s >> temp >> temp >> SCOPE_IND_COLOUR;
				s >> temp >> temp >> SCOPE_MRK_COLOUR;
			}
			s >> temp >> temp;
			s >> temp >> temp >> GUICOL_Tool;
			s >> temp >> temp >> GUICOL_Button;
			s >> temp >> temp >> GUICOL_Canvas;
			s >> temp >> temp >> GUICOL_Device;
			s >> temp >> temp >> GUIDEVICE_Box;
			s >> temp >> temp >> PLUGIN_PATH;
			s >> temp >> temp;
			
			string st;
			PLUGINVEC.clear();
			if (USEPLUGINLIST) 
			{
				while (st!="end" && !s.eof()) 
				{
					s >> st;
					if (st!="end") PLUGINVEC.push_back (st);
				}	
			}
		}
	}
#if __APPLE__
     // ignore custom paths, plugins are encapsulated in the app anyway
     // this prevents the program to fail if the user move the application icon
     PLUGIN_PATH = PLUGIN_PATH_LOCATION;
#endif
}

void SpiralInfo::StreamOutPrefs (ostream &s) 
{
	s << "SpiralSound resource file" << endl << endl;
	s << "Version           = " << m_Version << endl;
	s << "Locale            = " << LOCALE << endl;

	s << endl; //section spacer
	
	/* Write Audio Settings section */
	StreamOutAudioSettingsPrefs(s);

	/* Write Main Colour section */
	StreamOutMainColourPrefs(s);
						
	/* Write Device Colour section */
	StreamOutDeviceColourPrefs(s);

	/* Write PLUGINS section
	
	   because of the plugin path output format it must always be appended with an "end" line,
	   and currently this MUST be the last thing in the file
	*/
	s << "[PLUGINS]" << endl;
	s << "PluginPath        = " << PLUGIN_PATH << endl;
	s << "PluginList        = " << endl;
	for (vector<string>::iterator i=PLUGINVEC.begin(); i!=PLUGINVEC.end(); i++) 
	{
		s << *i << endl;
	}
	s << "end" << endl;  
}

void SpiralInfo::Alert (string Text) {
     // fl_message (Text.c_str());
     cerr << "Spiral alert: " << Text << endl;
}

void SpiralInfo::Log (string Text) {

}
