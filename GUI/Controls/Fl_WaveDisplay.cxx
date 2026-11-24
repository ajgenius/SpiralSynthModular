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
#include <cmath>
#include <cstdlib>

using namespace std;

Fl_WaveDisplay::Fl_WaveDisplay(int x,int y,int w,int h, const char *Name) :
Fl_Widget(x,y,w,h,Name),
m_StartPos(0),
m_EndPos(0),
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
m_EnvEnd(0),
m_EnvWidth(0),
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
	const long end = std::min(m_ViewEnd, (long)m_Sample.size() - 1);
	if (sample < m_ViewStart || sample > end || w() <= 0) return -1;
	return (int)((sample - m_ViewStart) * (double)w() / (end - m_ViewStart + 1));
}

long Fl_WaveDisplay::SampleAt(int column) const
{
	return m_ViewStart + (long)(std::max(0, std::min(w(), column)) *
		(double)(m_ViewEnd - m_ViewStart + 1) / std::max(1, w()));
}

void Fl_WaveDisplay::DamageColumn(int column)
{
	if (column >= 0) Fl_DamageArea(this, x() + column - 1, y(), 3, h());
}

void Fl_WaveDisplay::Envelope()
{
	if (m_EnvValid && m_EnvStart == m_ViewStart && m_EnvEnd == m_ViewEnd && m_EnvWidth == w()) return;
	m_Min.resize(w());
	m_Max.resize(w());
	for (int col = 0; col < w(); ++col)
	{
		const long start = SampleAt(col), end = SampleAt(col + 1);
		float low = m_Sample[start], high = low;
		for (long i = start + 1; i < end; ++i)
		{
			low = std::min(low, m_Sample[i]);
			high = std::max(high, m_Sample[i]);
		}
		m_Min[col] = low;
		m_Max[col] = high;
	}
	m_EnvStart = m_ViewStart;
	m_EnvEnd = m_ViewEnd;
	m_EnvWidth = w();
	m_EnvValid = true;
}

static int WaveY(float value, int height)
{
	if (!std::isfinite(value)) value = 0;
	value = std::max(-1.f, std::min(1.f, value));
	return (int)((1.0 - value) * (height - 1) * .5);
}

void Fl_WaveDisplay::draw()
{
	if (w() <= 0 || h() <= 0) return;
	fl_push_clip(x(), y(), w(), h());
	fl_color(m_BGColour);
	fl_rectf(x(), y(), w(), h());
	const int mid = y() + h() / 2;
	if (!m_Sample.empty())
	{
		const long last = (long)m_Sample.size() - 1;
		m_ViewStart = std::max(0L, std::min(m_ViewStart, last));
		m_ViewEnd = std::max(m_ViewStart, std::min(m_ViewEnd, last));
		if (m_EndPos > m_StartPos && m_EndPos > m_ViewStart && m_StartPos <= m_ViewEnd)
		{
			const int left = Column(std::max(m_StartPos, m_ViewStart));
			const int right = m_EndPos > m_ViewEnd ? w() : Column(m_EndPos);
			fl_color(m_SelColour);
			fl_rectf(x() + left, y(), right - left, h());
		}
	}
	fl_color(m_MrkColour);
	fl_line(x(), mid, x() + w() - 1, mid);
	if (m_Sample.empty())
	{
		fl_font(FL_HELVETICA, 8);
		fl_draw("No sample", x(), y(), w(), h(), FL_ALIGN_CENTER);
		fl_pop_clip();
		return;
	}
	Envelope();
	int cx, cy, cw, ch;
	fl_clip_box(x(), y(), w(), h(), cx, cy, cw, ch);
	fl_color(m_FGColour);
	for (int col = std::max(0, cx - x()); col < std::min(w(), cx + cw - x()); ++col)
		fl_line(x() + col, y() + WaveY(m_Max[col], h()),
		        x() + col, y() + WaveY(m_Min[col], h()));
	const int play = Column(m_PlayPos);
	if (m_PosMarker && play >= 0)
	{
		fl_color(m_IndColour);
		fl_line(x() + play, y(), x() + play, y() + h() - 1);
	}
	const long markers[] = {m_PlayStart, m_LoopStart, m_LoopEnd};
	const char *labels[] = {"S", "LS", "LE"};
	fl_font(FL_HELVETICA, 8);
	fl_color(m_MrkColour);
	for (int i = 0; i < 3; ++i)
	{
		const int col = Column(markers[i]);
		if (col < 0) continue;
		fl_line(x() + col, y(), x() + col, y() + h() - 1);
		fl_draw(labels[i], x() + std::max(0, std::min(col + 2, w() - 16)), y() + h() - 10 * (i + 1), 16, 10, FL_ALIGN_CENTER);
	}
	fl_pop_clip();
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
			long MousePos=std::min(length - 1, SampleAt(xx - x()));
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
			long MousePos=std::min(length - 1, SampleAt(xx - x()));				

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
				long MousePos=std::min(length - 1, SampleAt(xx - x()));
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
