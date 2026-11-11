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

#include "Fl_WaveDisplay.H"
#include "Fl_DamageArea.H"
#include <FL/fl_draw.H>
#include <algorithm>
#include <climits>
#include <cstdlib>

using namespace std;

Fl_WaveDisplay::Fl_WaveDisplay(int x,int y,int w,int h, const char *Name) :
Fl_Widget(x,y,w,h,Name),
m_StartPos(1),
m_EndPos(10),
m_ViewStart(0),
m_ViewEnd(INT_MAX),
m_PlayPos(0),
m_PlayStart(0),
m_LoopStart(0),
m_LoopEnd(INT_MAX),
m_PosMarker(true),
m_Drew(false),
m_DrawnPos(0),
m_DrawnValue(0),
m_EnvStart(0),
m_EnvJump(0),
m_EnvValid(false)
{
}

Fl_WaveDisplay::~Fl_WaveDisplay()
{
}

void Fl_WaveDisplay::SetSample (const float *s, long len) {
     m_Sample.assign (s, s + len);
     m_EnvValid = false;
}

void Fl_WaveDisplay::ClearSample (void) {
     m_Sample.clear ();
     m_EnvValid = false;
}

// The play position moves every block: only the column it left and the
// one it reaches are drawn again, and only when it moved.
void Fl_WaveDisplay::SetPlayPos(long s)
{
	if (s == m_PlayPos) return;
	const int was = Column(m_PlayPos);
	m_PlayPos = s;
	if (!m_PosMarker || !visible_r()) return;
	DamageColumn(was);
	DamageColumn(Column(s));
}

// The column draw() puts a sample in, or -1 when it is out of view.
int Fl_WaveDisplay::Column(long sample) const
{
	const long length = (long)m_Sample.size();
	if (!length || w() <= 0) return -1;
	const long start = std::max(0L, m_ViewStart), end = std::min(m_ViewEnd, length - 1);
	long jump = (end - start) / w();
	if (jump == 0) jump = 1;
	if (sample < start || sample >= end - jump) return -1;
	return (int)((sample - start) / jump);
}

void Fl_WaveDisplay::DamageColumn(int column)
{
	if (column < 0) return;
	Fl_DamageArea(this, x() + column - 1, y(), 3, h());
}

void Fl_WaveDisplay::Envelope(long jump)
{
	if (m_EnvValid && m_EnvStart == m_ViewStart && m_EnvJump == jump) return;
	const long length = (long)m_Sample.size();
	m_Min.clear();
	m_Max.clear();
	for (long n = m_ViewStart; n < m_ViewEnd - jump; n += jump)
	{
		float max = m_Sample[n], min = m_Sample[n];
		for (long m = n; m < n + jump && m < length; m++)
		{
			if (max < m_Sample[m]) max = m_Sample[m];
			if (min > m_Sample[m]) min = m_Sample[m];
		}
		m_Min.push_back(min);
		m_Max.push_back(max);
	}
	m_EnvStart = m_ViewStart;
	m_EnvJump = jump;
	m_EnvValid = true;
}

void Fl_WaveDisplay::draw()
{
	int ho=h()/2;
	fl_color (m_BGColour);
	fl_rectf(x(), y(), w(), h());
	if (m_Sample.empty()) return;
	const long length=(long)m_Sample.size();

	if (m_ViewStart<0) m_ViewStart=0;
	if (m_ViewEnd>length-1) m_ViewEnd=length-1;

	if (m_PlayStart<0) m_PlayStart=0;
	if (m_PlayStart>length-1) m_PlayStart=length-1;
	if (m_LoopStart<0) m_LoopStart=0;
	if (m_LoopStart>length-1) m_LoopStart=length-1;
	if (m_LoopEnd<0) m_LoopEnd=0;
	if (m_LoopEnd>length-1) m_LoopEnd=length-1;

	int Jump=(m_ViewEnd-m_ViewStart)/w();
	if (Jump==0) Jump=1;
	Envelope(Jump);
	fl_font(fl_font(),10);

	// Only the columns in the clip, with room for a marker's label to their left.
	int cx, cy, cw, ch;
	fl_clip_box(x(), y(), w(), h(), cx, cy, cw, ch);
	const int first = std::max(0, cx - x() - 16), last = cx + cw - x() + 1;
	int pos=first;

	for(long n=m_ViewStart+(long)first*Jump; n<m_ViewEnd-Jump && pos<=last && pos<(int)m_Min.size(); n+=Jump)
	{
		if (m_PlayPos>=n && m_PlayPos<n+Jump)
		{
			fl_color (m_IndColour);
			fl_line(x()+pos, y(),
				    x()+pos, y()+h());
		}

		if (m_PlayStart>=n && m_PlayStart<n+Jump)
		{
			fl_color (m_MrkColour);
			fl_draw("S",x()+pos+2,y()+h());
			fl_line(x()+pos, y(),
				    x()+pos, y()+h());
		}

		if (m_LoopStart>=n && m_LoopStart<n+Jump)
		{
			fl_color (m_MrkColour);
			fl_draw("LS",x()+pos+2,y()+h());
			fl_line(x()+pos, y(),
				    x()+pos, y()+h());
		}

		if (m_LoopEnd>=n && m_LoopEnd<n+Jump)
		{
			fl_color (m_MrkColour);
			fl_draw("LE",x()+pos+2,y()+h());
			fl_line(x()+pos, y(),
				    x()+pos, y()+h());
		}

		if (n>m_StartPos && n<m_EndPos) fl_color (m_SelColour);
		else fl_color (m_FGColour);

		const float min=m_Min[pos]*ho, max=m_Max[pos]*ho;

		fl_line(x()+pos-1, y()+ho-(int)min,
				x()+pos-1, y()+ho-(int)max);
		pos++;
	}
}

int Fl_WaveDisplay::handle(int event)
{
	int xx=Fl::event_x();
	int yy=Fl::event_y();
	static int DragX,DragY;
	static int Mousebutton=0;
	static int Holding=0;
	static int GrabDist=10;

	if (m_Sample.empty()) return 1;
	const long length=(long)m_Sample.size();

	if (event==FL_PUSH)
	{
		GrabDist=(int)((m_ViewEnd-m_ViewStart)*0.03f);
		Mousebutton=Fl::event_button();
		DragX=xx;
		DragY=yy;

		if (Mousebutton==1)
		{
			int MousePos=(xx-x())*((m_ViewEnd-m_ViewStart)/w())+m_ViewStart;
			Holding=0;
			if (abs(MousePos-m_StartPos)<GrabDist)    Holding=1;
			else if (abs(MousePos-m_EndPos)<GrabDist) Holding=2;
			else if (abs(MousePos-m_PlayStart)<GrabDist) Holding=3;
			else if (abs(MousePos-m_LoopStart)<GrabDist) Holding=4;
			else if (abs(MousePos-m_LoopEnd)<GrabDist) Holding=5;
			else
			{
				m_StartPos=MousePos;
				m_EndPos=MousePos;
			}
		}
	}
	
	if (event==FL_DRAG)
	{
		if (Mousebutton==1)
		{	
			int MousePos=(xx-x())*((m_ViewEnd-m_ViewStart)/w())+m_ViewStart;				

			switch (Holding)
			{
				case 0:
				{
					if (MousePos>m_EndPos) m_EndPos=MousePos;
					else m_StartPos=MousePos;					
				} break;

				case 1:
				{
					m_StartPos=MousePos;
					if (m_StartPos>m_EndPos) Holding=2; // swap
				} break;
				

				case 2:
				{
					m_EndPos=MousePos;
					if (m_StartPos>m_EndPos) Holding=1; // swap
				} break;

				case 3: m_PlayStart=MousePos; break;
				case 4: m_LoopStart=MousePos; break;
				case 5: m_LoopEnd=MousePos; break;
			}
		}

		if (Mousebutton==2)
		{
			int Dist=(DragX-xx)*((m_ViewEnd-m_ViewStart)/w());
			if (m_ViewStart>0 && m_ViewEnd<length-1)
			{
				m_ViewStart+=Dist;
				m_ViewEnd+=Dist;
			}
			else // stop it sticking when at end/beginning
			{
				if ((Dist>0 && m_ViewStart<=0) ||
				    (Dist<0 && m_ViewEnd>=length-1))
				{
					m_ViewStart+=Dist;
					m_ViewEnd+=Dist;
				}
			}
			DragX=xx;
			DragY=yy;
		}

		if (Mousebutton==3)
		{
			// only draw wave at 1 pixel = 1 sample
			if ((m_ViewEnd-m_ViewStart)/w()==1)
			{
				int MousePos=(xx-x())*((m_ViewEnd-m_ViewStart)/w())+m_ViewStart;
				float Value=-(yy-y())/((float)h()/2.0f)+1.0f;
				if (Value>1.0f) Value=1.0f;
				if (Value<-1.0f) Value=-1.0f;
				if (MousePos>=0 && MousePos<length)
				{
					m_Sample[MousePos]=Value;
					// For whoever owns the sample: this copy is only what is shown.
					m_Drew=true;
					m_DrawnPos=MousePos;
					m_DrawnValue=Value;
				}
				m_EnvValid=false;
				redraw();
			}
		}

		do_callback();
		redraw();
	}

	if (m_EndPos>=length) m_EndPos=length-1;

	return 1;
}

void Fl_WaveDisplay::ZoomIn()
{
	int Zoom=(int)((m_ViewEnd-m_ViewStart)*0.03f);
	if ((m_ViewEnd-m_ViewStart)/w()>1)
	{
		m_ViewStart+=Zoom;
		m_ViewEnd-=Zoom;
	}

	redraw();
}

void Fl_WaveDisplay::ZoomOut()
{
	int Zoom=(int)((m_ViewEnd-m_ViewStart)*0.03f);
	m_ViewStart-=Zoom;
	m_ViewEnd+=Zoom;
	redraw();
}

////////////////////////////////////////////
