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

#include "../SpiralPlugin.h"
#include <FL/Fl_Pixmap.H>

#ifndef TranslatePLUGIN
#define TranslatePLUGIN

class TranslatePlugin;

class TranslateClass {
   public:
      virtual float Translate (float i) = 0;
};

class TranslatePass : public TranslateClass {
   public:
      virtual float Translate (float i);
};

class TranslateNoteToFreq : public TranslateClass {
   public:
      virtual float Translate (float i);
};

class TranslateFreqToNote : public TranslateClass {
   public:
      virtual float Translate (float i);
};

class TranslateNoteToVolt : public TranslateClass {
   public:
      virtual float Translate (float i);
};

class TranslateVoltToNote : public TranslateClass {
   public:
      virtual float Translate (float i);
};

////////////////////////////////////////////////////////////////////////

class TranslatePlugin : public SpiralPlugin {
   public:
      TranslatePlugin();
      virtual ~TranslatePlugin ();
      virtual PluginInfo& Initialise (const HostInfo *Host);
      virtual SpiralGUIType* CreateGUI ();
      virtual void Execute ();
      enum GUICommands { NOCMD, SETMETHOD };
      virtual void ExecuteCommands ();
      virtual void StreamOut (std::ostream &s);
      virtual void StreamIn (std::istream &s);
      enum TranMethod { tr_Pass, tr_NoteToFreq, tr_FreqToNote, tr_NoteToVolt, tr_VoltToNote };
      int GetMethod (void) { return m_Method; }
   private:
      int m_Method;
      TranslateClass *m_Translator;
      void SetUpTranslatorClass (void);
};

#endif
