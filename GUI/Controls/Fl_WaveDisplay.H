/*  SpiralPlugin
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

#ifndef FL_WAVEDISPLAY_H
#define FL_WAVEDISPLAY_H

#include <FL/Fl.H>
#include <FL/Fl_Widget.H>
#include <vector>

class Fl_WaveDisplay : public Fl_Widget {
   public:
      Fl_WaveDisplay (int x,int y,int w,int h, const char* Name);
      ~Fl_WaveDisplay();
      virtual void draw();
      virtual int handle (int event);
      void SetSample (const float* s, long len);
      void ClearSample (void);
      long GetRangeStart()      { return m_StartPos; }
      long GetRangeEnd()        { return m_EndPos; }
      long GetViewStart()       { return m_ViewStart; }
      void SetViewStart(long s) { m_ViewStart=s; }
      long GetViewEnd()         { return m_ViewEnd; }
      void SetViewEnd(long s)   { m_ViewEnd=s; }
      void SetPlayPos(long s);
      void SetPlayStart(long s) { m_PlayStart=s; }
      long GetPlayStart()       { return m_PlayStart; }
      void SetLoopStart(long s) { m_LoopStart=s; }
      long GetLoopStart()       { return m_LoopStart; }
      void SetLoopEnd(long s)   { m_LoopEnd=s; }
      long GetLoopEnd()         { return m_LoopEnd; }
      void SetPosMarker(bool s) { m_PosMarker=s; }
      // The sample a right drag last set, once: its place and new value.
      bool TakeDrawn(long &pos, float &value) {
           if (!m_Drew) return false;
           m_Drew=false; pos=m_DrawnPos; value=m_DrawnValue;
           return true;
      }
      void ZoomIn();
      void ZoomOut();
      void SetColours (unsigned b, unsigned f, unsigned s, unsigned i, unsigned m) {
           m_BGColour=(Fl_Color)b;  m_FGColour=(Fl_Color)f;  m_SelColour=(Fl_Color)s;
           m_IndColour=(Fl_Color)i; m_MrkColour=(Fl_Color)m;
      }
   private:
      Fl_Color m_BGColour, m_FGColour, m_SelColour, m_IndColour, m_MrkColour;
      std::vector<float> m_Sample;
      long m_StartPos, m_EndPos, m_ViewStart, m_ViewEnd;
      long m_PlayPos, m_PlayStart, m_LoopStart, m_LoopEnd;
      bool m_PosMarker;
      bool m_Drew;
      long m_DrawnPos;
      float m_DrawnValue;
      std::vector<float> m_Min, m_Max;
      long m_EnvStart, m_EnvJump;
      bool m_EnvValid;
      void Envelope(long jump);
      int Column(long sample) const;
      void DamageColumn(int column);
};

#endif
