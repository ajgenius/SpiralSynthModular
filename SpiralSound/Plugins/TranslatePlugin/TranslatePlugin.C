/*  SpiralSound
 *  Copyleft (C) 2003 Andy Preston <andy@clublinux.co.uk>
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

#include "TranslatePlugin.h"
#include "TranslatePluginGUI.h"
#include "SpiralIcon.xpm"
#include "../../NoteTable.h"

using namespace std;

extern "C" {

SpiralPlugin* SpiralPlugin_CreateInstance () { return new TranslatePlugin; }

char** SpiralPlugin_GetIcon () { return SpiralIcon_xpm; }

int SpiralPlugin_GetID () { return 121; }

string SpiralPlugin_GetGroupName() { return "Control"; }

}

///////////////////////////////////////////////////////

float TranslatePass::Translate (float i) {
      return i;
}

float TranslateNoteToFreq::Translate (float i) {
      return (i + 1.0f) * (MAX_FREQ / 2);
}

float TranslateFreqToNote::Translate (float i) {
      return (i / MAX_FREQ * 2) - 1.0f;
}

float TranslateNoteToVolt::Translate (float i) {
      float Freq = (i + 1.0f) * (MAX_FREQ / 2);
      float Dif, MinDif = 30000;
      int Match=0;
      for (int c=0; c<131; c++) {
          Dif = fabs (NoteTable[c] - Freq);
          if (Dif > MinDif) break;
          MinDif = Dif;
          Match = c;
      }
      return (float)Match;
}

float TranslateVoltToNote::Translate (float i) {
      return (NoteTable[int (i)] / MAX_FREQ * 2) - 1.0f;
}

///////////////////////////////////////////////////////

TranslatePlugin::TranslatePlugin () :
m_Method (tr_Pass),
m_Translator (NULL)
{
    m_PluginInfo.Name = "Translate";
    m_PluginInfo.Width = 150;
    m_PluginInfo.Height = 60;
    m_PluginInfo.NumInputs = 1;
    m_PluginInfo.NumOutputs = 1;
    m_PluginInfo.PortTips.push_back ("Input");
    m_PluginInfo.PortTips.push_back ("Output");
    m_AudioCH->Register ("Method", &m_Method);
    SetUpTranslatorClass ();
}

TranslatePlugin::~TranslatePlugin () {
    if (m_Translator) delete m_Translator;
}

PluginInfo &TranslatePlugin::Initialise (const HostInfo *Host) {
    return SpiralPlugin::Initialise (Host);
}

SpiralGUIType *TranslatePlugin::CreateGUI() {
    return new TranslatePluginGUI (m_PluginInfo.Width, m_PluginInfo.Height, this, m_AudioCH, m_HostInfo);
}

void TranslatePlugin::SetUpTranslatorClass (void) {
     if (m_Translator) {
        delete m_Translator;
        m_Translator = NULL;
     }
     switch (m_Method) {
       case tr_Pass:
            m_Translator = new TranslatePass;
            break;
       case tr_NoteToFreq:
            m_Translator = new TranslateNoteToFreq;
            break;
       case tr_FreqToNote:
            m_Translator = new TranslateFreqToNote;
            break;
       case tr_NoteToVolt:
            m_Translator = new TranslateNoteToVolt;
            break;
       case tr_VoltToNote:
            m_Translator = new TranslateVoltToNote;
            break;
     }
}

void TranslatePlugin::ExecuteCommands () {
     if (m_AudioCH->IsCommandWaiting ()) {
        switch (m_AudioCH->GetCommand()) {
          case SETMETHOD:
               SetUpTranslatorClass ();
               break;
        }
     }
}

void TranslatePlugin::Execute () {
     for (int n=0; n<m_HostInfo->BUFSIZE; n++) {
         SetOutput (0, n, m_Translator->Translate (GetInput (0, n)));
     }
}

void TranslatePlugin::StreamOut (ostream &s) {
     s << m_Version << endl;
     s << m_Method;
}

void TranslatePlugin::StreamIn (istream &s) {
     int version;
     s >> version;
     s >> m_Method;
     SetUpTranslatorClass ();
}
