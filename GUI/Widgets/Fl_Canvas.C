/*  Canvas Widget
 *  Copyleft (C) 2002 David Griffiths <dave@pawfal.org>
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

#include <FL/fl_draw.H>
#include <FL/Fl_Scroll.H>
#include "Fl_Canvas.h"
#include <map>
#include <cstdlib>
#include <cctype>
#include <cstdio>
#include "Fl_DeviceGUI.h"
#include "SpiralPluginGUI.h"
#include <algorithm>
#include <iostream>
#include "SpiralInfo.h"
#include <math.h>

// no of calls to handle when dragged, before the widget is redrawn
// to allow the wire (connection currently being made) to be redrawn
static const int UPDATE_TICKS = 5;

static Fl_DeviceGUI *AsDevice(Fl_Widget *w)
{
	return dynamic_cast<Fl_DeviceGUI*>(w);
}

static int Numbers[512];

////////////////////////////////////////////////////////////////////////

Fl_Canvas::Fl_Canvas(int x, int y, int w, int h, const char *name) :
Fl_Group(x,y,w,h,name),
m_Menu(NULL),
m_BG(NULL),
m_BGData(NULL),
cb_Connection(NULL),
cb_Unconnect(NULL),
cb_AddDevice(NULL),
cb_Rename(NULL),
cb_CutDeviceGroup(NULL),
cb_CopyDeviceGroup(NULL),
cb_PasteDeviceGroup(NULL),
cb_PasteAsSubPatch(NULL),
cb_MergePatch(NULL),
cb_AboutPatch(NULL),
cb_GroupDevices(NULL),
cb_UngroupDevices(NULL),
cb_PageChanged(NULL),
cb_DropFile(NULL),
m_Dropping(false),
m_PickAt(NULL),
m_Picked(NULL),
m_PickData(NULL),
m_PickHot(false),
m_Isolated(-1),
m_Page(-1),
m_Owned(false),
m_Order(0),
m_Topology(0),
m_GraphLock(NULL),
m_StripInput(false),
m_StripPort(-1),
m_CanPaste(false),
m_Selecting(false),
m_UpdateTimer(0)
{
	m_IncompleteWire.OutputID=-1;
	m_IncompleteWire.OutputPort=-1;
	m_IncompleteWire.OutputTerminal=false;
	m_IncompleteWire.InputID=-1;
	m_IncompleteWire.InputPort=-1;
	m_IncompleteWire.InputTerminal=false;
        for (int n=0; n<512; n++) Numbers[n]=n;
}

////////////////////////////////////////////////////////////////////////

Fl_Canvas::~Fl_Canvas()
{
	if (m_Menu) delete m_Menu;
}

////////////////////////////////////////////////////////////////////////

void Fl_Canvas::draw()
{
	Fl_Widget*const* a = array();

  	if (damage() & ~FL_DAMAGE_CHILD) // redraw the entire thing:
	{
		if (m_BG)
		{
			int X=0,Y=0;
			while (Y<w())
			{
				while (X<h())
				{
					m_BG->draw(parent()->x()+X,parent()->y()+Y);
#if FL_MAJOR_VERSION == 1 && FL_MINOR_VERSION == 0
					X+=m_BG->w;
#else
					X+=m_BG->w();
#endif
				}
#if FL_MAJOR_VERSION == 1 && FL_MINOR_VERSION == 0
 				Y+=m_BG->h;
#else
				Y+=m_BG->h();
#endif
				X=0;
			}
		}
		else
		{
			draw_box();
		}

		// draw minimised modules first
   	 	for (int i=children(); i--;)
		{
   			Fl_Widget& o = **a++;
			Fl_DeviceGUI *device = AsDevice(&o);
			if (!device)
				continue;
			if (m_Isolated >= 0)
			{
				if (device->GetID() == m_Isolated)
					draw_child(o);
				continue;
			}
			if (device->IsMinimised())
			{
   	 			draw_child(o);
   				draw_outside_label(o);

				std::vector<int>::iterator sel = std::find( m_Selection.m_DeviceIds.begin(), m_Selection.m_DeviceIds.end(), device->GetID() );

				if (sel != m_Selection.m_DeviceIds.end())
				{
					fl_color(FL_YELLOW);
					fl_rect(o.x(), o.y(), o.w(), o.h());
				}
			}
		}

		if (m_Isolated < 0)
			DrawWires();

   	 	Fl_Widget*const* a = array();
		for (int i=m_Isolated < 0 ? children() : 0; i--;)
		{
   			Fl_Widget& o = **a++;
			Fl_DeviceGUI *device = AsDevice(&o);
			if (!device)
				continue;
			if (!device->IsMinimised())
			{
   	 			draw_child(o);
   				draw_outside_label(o);

				std::vector<int>::iterator sel = std::find( m_Selection.m_DeviceIds.begin(), m_Selection.m_DeviceIds.end(), device->GetID() );

				if (sel != m_Selection.m_DeviceIds.end())
				{
					fl_color(FL_YELLOW);
					fl_rect(o.x(), o.y(), o.w(), o.h());
				}
			}
		}
	}
	else // only redraw the children that need it:
	{
		for (int i = 0; i < children(); ++i)
		{
			Fl_DeviceGUI *below = AsDevice(child(i));
			if (!below || !below->damage() || !below->visible())
				continue;
			for (int j = i + 1; j < children(); ++j)
			{
				Fl_DeviceGUI *above = AsDevice(child(j));
				if (above && above->visible() && (below->IsMinimised() || !above->IsMinimised()) &&
				    above->x() < below->x() + below->w() && below->x() < above->x() + above->w() &&
				    above->y() < below->y() + below->h() && below->y() < above->y() + above->h() &&
				    fl_not_clipped(above->x(), above->y(), above->w(), above->h()))
					above->clear_damage(above->damage() | FL_DAMAGE_ALL);
			}
		}

		for (int i=children(); i--;)
		{
			Fl_Widget &o = **a++;
			Fl_DeviceGUI *device = AsDevice(&o);
			if (m_Isolated < 0 || (device && device->GetID() == m_Isolated))
			{
				const bool drawn = o.damage() && o.visible();
				update_child(o);
				if (drawn && device && std::find(m_Selection.m_DeviceIds.begin(), m_Selection.m_DeviceIds.end(), device->GetID()) != m_Selection.m_DeviceIds.end())
				{
					fl_color(FL_YELLOW);
					fl_rect(o.x(), o.y(), o.w(), o.h());
				}
			}
		}
  	}
	if (m_Isolated < 0)
	{
		DrawStrips();
		DrawSelection();
	}
	DrawPick();
}

void Fl_Canvas::DrawPick()
{
	if (!m_PickAt)
		return;

	if (m_PickHot)
	{
		fl_color(FL_CYAN);
		fl_line_style(FL_SOLID, 2);
		fl_rect(m_Hot.X - 2, m_Hot.Y - 2, m_Hot.W + 4, m_Hot.H + 4);
		fl_line_style(0);
	}

	Fl_Widget *view = parent() ? parent() : this;
	fl_font(FL_HELVETICA, 11);
	const int W = (int)fl_width(m_PickHint.c_str()) + 16, H = 20;
	fl_draw_box(FL_ROUNDED_BOX, view->x() + 8, view->y() + 8, W, H, FL_CYAN);
	fl_color(FL_BLACK);
	fl_draw(m_PickHint.c_str(), view->x() + 8, view->y() + 8, W, H, FL_ALIGN_CENTER);
}

void Fl_Canvas::StartPick(PickAt at, Picked picked, void *data, const string &hint)
{
	m_PickAt = at;
	m_Picked = picked;
	m_PickData = data;
	m_PickHint = hint;
	m_PickHot = false;
	ClearIncompleteWire();
	redraw();
}

void Fl_Canvas::StopPick()
{
	m_PickAt = NULL;
	m_Picked = NULL;
	m_PickData = NULL;
	m_PickHot = false;
	redraw();
}

void Fl_Canvas::Isolate(int id)
{
	m_Isolated = id;
	redraw();
}

int Fl_Canvas::PickEvent(int event)
{
	switch (event)
	{
		case FL_ENTER:
		case FL_MOVE:
		case FL_DRAG:
		{
			Pick pick;
			const bool hot = m_PickAt(this, Fl::event_x(), Fl::event_y(), pick, m_PickData);
			if (hot != m_PickHot || (hot && (pick.X != m_Hot.X || pick.Y != m_Hot.Y
			    || pick.W != m_Hot.W || pick.H != m_Hot.H)))
			{
				m_PickHot = hot;
				m_Hot = pick;
				redraw();
			}
			return 1;
		}
		case FL_LEAVE:
			if (m_PickHot)
			{
				m_PickHot = false;
				redraw();
			}
			return 1;
		case FL_PUSH:
		{
			Pick pick;
			if (!m_PickAt(this, Fl::event_x(), Fl::event_y(), pick, m_PickData))
				pick = Pick();
			Picked picked = m_Picked;
			void *data = m_PickData;
			StopPick();
			if (picked)
				picked(this, pick, data);
			return 1;
		}
		case FL_RELEASE:
			return 1;
		case FL_KEYBOARD:
		case FL_SHORTCUT:
			if (Fl::event_key() != FL_Escape)
				return 0;
			{
				Picked picked = m_Picked;
				void *data = m_PickData;
				StopPick();
				if (picked)
					picked(this, Pick(), data);
			}
			return 1;
	}
	return 0;
}

////////////////////////////////////////////////////////////////////////

void Fl_Canvas::Poll()
{
	// bit of a workaround...
	if (UserMakingConnection()) m_UpdateTimer++;

	if (m_UpdateTimer>UPDATE_TICKS)
	{
		m_UpdateTimer=0;
		redraw();
	}
}

////////////////////////////////////////////////////////////////////////

void Fl_Canvas::DrawSelection()
{
	if (m_Selecting)
	{
		int X, Y, W, H;

		fl_color(FL_YELLOW);

		X = min(m_StartSelectX, m_EndSelectX);
		Y = min(m_StartSelectY, m_EndSelectY);
		W = max(m_StartSelectX, m_EndSelectX) - X;
		H = max(m_StartSelectY, m_EndSelectY) - Y;

		fl_rect(X-1, Y-1, W+2, H+2);
	}
}

bool widget_intersects_rectangle (Fl_Widget* o, int X, int Y, int W, int H)
{
	int src1_x1=o->x(), src1_y1=o->y();
	int src1_x2=o->x()+o->w(), src1_y2=o->y()+o->h();

	int src2_x1=X, src2_y1=Y;
	int src2_x2=X+W, src2_y2=Y+H;

	int width=0, height=0;
	
	if (X < o->x())
	{
		src1_x1=X;
		src1_y1=Y;
		src1_x2=X+W;
		src1_y2=Y+H;

		src2_x1=o->x();
		src2_y1=o->y();
		src2_x2=o->w();
		src2_y2=o->h();
	}

	if (src2_x1 < src1_x2)
	{
		if (src1_x2 < src2_x2)
			width = src1_x2 - src2_x1;
		else
			width = src2_x2 - src2_x1;

		if (width == 0)
			return false;
		
		if (src2_y1 < src1_y1)
		{
			int tmp;

			tmp = src2_x1;
			src2_x1=src1_x1;
			src1_x1=tmp;

			tmp = src2_y1;
			src2_y1=src1_y1;
			src1_y1=tmp;

			tmp = src2_x2;
			src2_x2=src1_x2;
			src1_x2=tmp;
			
			tmp = src2_y2;
			src2_y2=src1_y2;
			src1_y2=tmp;

		}

		if (src2_y1 < src1_y2)
		{
			if (src1_y2 < src2_y2)
				height = src1_y2 - src2_y1;
			else
				height = src2_y2 - src2_y1;

			if ((height == 0))
				return false;
			else
				return true;
		}
	}

	return false;
}

void Fl_Canvas::CalculateSelection()
{
	Fl_Widget*const* a = array();
	int X, Y, W, H;

	X = min(m_StartSelectX, m_EndSelectX);
	Y = min(m_StartSelectY, m_EndSelectY);
	W = max(m_StartSelectX, m_EndSelectX) - X;
	H = max(m_StartSelectY, m_EndSelectY) - Y;

	m_HaveSelection = false;
	m_Selection.Clear();

 	for (int i=0; i<children(); i++)
	{
		Fl_Widget& o = **a++;
		Fl_DeviceGUI *device = AsDevice(&o);
		if (!device)
			continue;
		if (widget_intersects_rectangle(device, X, Y, W, H))
		{
			m_HaveSelection = true;
			m_Selection.m_DeviceIds.push_back(device->GetID());
			device->SetOnDragCallback(cb_OnDrag_s, this);
		}
	}
}

void Fl_Canvas::DeleteSelection (void) {
     if (! m_HaveSelection) return;
     // show some warning here
     for (unsigned int i=0; i<m_Selection.m_DeviceIds.size(); i++) {
         int ID = m_Selection.m_DeviceIds[i];
         Fl_DeviceGUI* o = FindDevice(ID);
         if (o) Fl_DeviceGUI::Kill(o);
     }
     m_HaveSelection = false;
     m_Selection.Clear();
     redraw();
}

void Fl_Canvas::PopupEditMenu (Fl_Group *group) {
     m_Menu->value(0);
     group->add(m_Menu);
     Fl_Menu_Item *cut=(Fl_Menu_Item*)&(m_Menu->menu()[1]);
     Fl_Menu_Item *copy=(Fl_Menu_Item*)&(m_Menu->menu()[2]);
     Fl_Menu_Item *paste=(Fl_Menu_Item*)&(m_Menu->menu()[3]);
     Fl_Menu_Item *pasteGrouped=(Fl_Menu_Item*)&(m_Menu->menu()[4]);
     Fl_Menu_Item *merge=(Fl_Menu_Item*)&(m_Menu->menu()[5]);
     Fl_Menu_Item *deleteitems=(Fl_Menu_Item*)&(m_Menu->menu()[6]);
     if ((cb_CopyDeviceGroup) && (m_HaveSelection)) copy->activate(); else copy->deactivate();
     if ((cb_CutDeviceGroup) && (m_HaveSelection)) cut->activate(); else cut->deactivate();
     if ((cb_PasteDeviceGroup) && (m_CanPaste)) paste->activate(); else paste->deactivate();
     if ((cb_PasteAsSubPatch) && (m_CanPaste)) pasteGrouped->activate(); else pasteGrouped->deactivate();
     if (m_HaveSelection) deleteitems->activate(); else deleteitems->deactivate();
     Fl_Menu_Item *groupItem=(Fl_Menu_Item*)&(m_Menu->menu()[7]);
     Fl_Menu_Item *ungroupItem=(Fl_Menu_Item*)&(m_Menu->menu()[8]);
     Fl_Menu_Item *backItem=(Fl_Menu_Item*)&(m_Menu->menu()[9]);
     const bool box = m_Selection.m_DeviceIds.size() == 1 && IsBox(m_Selection.m_DeviceIds[0]);
     if (cb_GroupDevices && m_HaveSelection) groupItem->activate(); else groupItem->deactivate();
     if (cb_UngroupDevices && box) ungroupItem->activate(); else ungroupItem->deactivate();
     if (m_Page != -1) backItem->activate(); else backItem->deactivate();
     m_Menu->popup();
     group->remove(m_Menu);
}

void Fl_Canvas::AddPluginName (const string &s, int ID) {
     // There's a bug here, that there's no menu at all if no plugins are found
     // This isn't IMMEDIATELY important, as if you've no plugins - there's nothing to copy/paste/etc anyway.
     if (! (m_Menu)) {
        m_Menu = new Fl_Menu_Button (0, 0, 4, 4, NULL);
        if (m_Menu->parent()) m_Menu->parent()->remove(m_Menu);
        m_Menu->type (Fl_Menu_Button::POPUP123);
        m_Menu->textsize (10);
        m_Menu->add ("Edit/Cut Currently Selected Devices", 0, (Fl_Callback*)cb_CutDeviceGroup, user_data());
        m_Menu->add ("Edit/Copy Currently Selected Devices", 0, (Fl_Callback*)cb_CopyDeviceGroup, user_data());
        m_Menu->add ("Edit/Paste Previously Copied Devices", 0, (Fl_Callback*)cb_PasteDeviceGroup, user_data());
        m_Menu->add ("Edit/Paste Them as a SubPatch", 0, (Fl_Callback*)cb_PasteAsSubPatch, user_data(), FL_MENU_DIVIDER);
        m_Menu->add ("Edit/Merge Existing Patch", 0, (Fl_Callback*)cb_MergePatch, user_data(), FL_MENU_DIVIDER);
        m_Menu->add ("Edit/Delete Currently Selected Devices", 0, (Fl_Callback*)cb_DeleteDeviceGroup, this, FL_MENU_DIVIDER);
        m_Menu->add ("Edit/Group Selected Devices into a SubPatch", 0, (Fl_Callback*)cb_GroupDevices, user_data());
        m_Menu->add ("Edit/Ungroup Selected SubPatch", 0, (Fl_Callback*)cb_UngroupDevices, user_data());
        m_Menu->add ("Edit/Back to Parent Patch", 0, (Fl_Callback*)cb_ParentPage, this);
     }
     m_Menu->add (s.c_str(), 0, (Fl_Callback*)cb_AddDeviceFromMenu, &Numbers[ID]);
}

void Fl_Canvas::AddAboutPatch () {
     if (!m_Menu || !cb_AboutPatch) return;
     int last = -1;
     int depth = 0;
     for (int item = 0; item < m_Menu->size(); item++) {
         const Fl_Menu_Item *entry = &(m_Menu->menu()[item]);
         if (!entry->text) { depth--; continue; }
         if (depth == 0) last = item;
         if (entry->flags & FL_SUBMENU) depth++;
     }
     if (last >= 0) ((Fl_Menu_Item*)&(m_Menu->menu()[last]))->flags |= FL_MENU_DIVIDER;
     m_Menu->add ("About Patch", 0, (Fl_Callback*)cb_AboutPatch, user_data());
}

void Fl_Canvas::StreamSelectionWiresIn(istream &s, std::map<int,int> NewDeviceIds, bool merge)
{
	MapNewDeviceIds = NewDeviceIds;
	StreamWiresIn(s, merge);
}

////////////////////////////////////////////////////////////////////////

void Fl_Canvas::DrawWires()
{
        fl_line_style( FL_SOLID, 2 );

	for(vector<CanvasWire>::iterator i=m_WireVec.begin();
		i!=m_WireVec.end(); i++)
	{
		Fl_DeviceGUI* SourceDevice = FindDevice(i->OutputID);
		Fl_DeviceGUI* DestDevice   = FindDevice(i->InputID);

		if (!SourceDevice || !DestDevice)
		{
			SpiralInfo::Alert("Cant find source or dest device while drawing wires");
			return;
		}

		const int SourceHolder = Holder(i->OutputID, m_Page);
		const int DestHolder = Holder(i->InputID, m_Page);
		if (SourceHolder == -1 || DestHolder == -1 || (SourceHolder == DestHolder && SourceHolder != i->OutputID))
			continue;
		int sx, sy, dx, dy;
		{
			const int k = PortThrough(SourceHolder, false, i->OutputID, i->OutputPort);
			Fl_DeviceGUI *Shown = FindDevice(SourceHolder);
			if (k < 0 || !Shown) continue;
			sx = Shown->GetPortX(k + Shown->GetInfo()->NumInputs);
			sy = Shown->GetPortY(k + Shown->GetInfo()->NumInputs);
		}
		{
			const int k = PortThrough(DestHolder, true, i->InputID, i->InputPort);
			Fl_DeviceGUI *Shown = FindDevice(DestHolder);
			if (k < 0 || !Shown) continue;
			dx = Shown->GetPortX(k);
			dy = Shown->GetPortY(k);
		}

         Fl_Color col = (Fl_Color) WIRE_COL0;
         switch (SourceDevice->GetPortType(i->OutputPort+SourceDevice->GetInfo()->NumInputs)) {
             case 0:     col = (Fl_Color) WIRE_COL0; break;
             case 1:     col = (Fl_Color) WIRE_COL1; break;
             case 2:     col = (Fl_Color) WIRE_COL2; break;
             case 3:     col = (Fl_Color) WIRE_COL3; break;
             case 4:     col = (Fl_Color) WIRE_COL4; break;
             default:    col = (Fl_Color) WIRE_COL0;
         }
         fl_color(col);

#ifdef NTK_MAJOR_VERSION
         fl_color( fl_color_add_alpha( fl_color(), 127 ) );
#endif

                double ep1_x, ep1_y, ep2_x, ep2_y,
                    ep1_mix_x,
                    ep1_new_x,
                    ep1_mid_x,
                    ep2_mid_x,
                    ep2_new_x,
                    ep1_mid_y,
                    ep2_mid_y;
                
                ep1_x = sx;
                ep1_y = sy;
                ep1_mid_y = ep1_y + 7.5;

                ep2_x = dx;
                ep2_y = dy;
                ep2_mid_y = 7.5 + ep2_y;
                
                ep1_mid_x = fabs(ep1_x-ep2_x)/2;
                ep1_new_x = ep1_x+ep1_mid_x;
                
                ep2_mid_x = fabs(ep1_x-ep2_x)/2;
                ep2_new_x = ep2_x-ep2_mid_x;

                if (SpiralInfo::CURVEDWIRES)
                {
                        fl_begin_line();

                        fl_curve( ep1_x, ep1_y, ep1_new_x, ep1_mid_y, ep2_new_x, ep2_mid_y, ep2_x, ep2_y );

                        fl_end_line();
                }
                else fl_line((int)ep1_x, (int)ep1_y, (int)ep2_x, (int)ep2_y);
	}

	DrawIncompleteWire();
        
        fl_line_style( FL_SOLID, 0 );
}

////////////////////////////////////////////////////////////////////////

bool Fl_Canvas::UserMakingConnection()
{
	return 	(m_IncompleteWire.InputID!=-1 || m_IncompleteWire.OutputID!=-1 || m_StripPort>=0);
}

////////////////////////////////////////////////////////////////////////

void Fl_Canvas::DrawIncompleteWire()
{
	if (m_StripPort >= 0)
	{
		int px, py;
		StripPoint(m_StripInput, m_StripPort, px, py);
		fl_color(FL_DARK_YELLOW);
		fl_line(px, py, Fl::event_x(), Fl::event_y());
	}

	// draw the wire we are currently connecting
	if(m_IncompleteWire.InputID!=-1)
	{
		Fl_DeviceGUI* Device = FindDevice(m_IncompleteWire.InputID);

		if (!Device)
		{
			SpiralInfo::Alert("Cant find source or dest device while drawing wires");
			return;
		}


        Fl_Color col = (Fl_Color) WIRE_COL0;
         switch (Device->GetPortType(m_IncompleteWire.InputPort)) {
             case 0:     col = (Fl_Color) WIRE_COL0; break;
             case 1:     col = (Fl_Color) WIRE_COL1; break;
             case 2:     col = (Fl_Color) WIRE_COL2; break;
             case 3:     col = (Fl_Color) WIRE_COL3; break;
             case 4:     col = (Fl_Color) WIRE_COL4; break;
             default:    col = (Fl_Color) WIRE_COL0;
         }
 		fl_color(col);

		fl_line(Device->GetPortX(m_IncompleteWire.InputPort),
				Device->GetPortY(m_IncompleteWire.InputPort),
				Fl::event_x(),
				Fl::event_y());
	}

	if(m_IncompleteWire.OutputID!=-1)
	{
		Fl_DeviceGUI* Device = FindDevice(m_IncompleteWire.OutputID);

		if (!Device)
		{
			SpiralInfo::Alert("Cant find source or dest device while drawing wires");
			return;
		}


        Fl_Color col = (Fl_Color) WIRE_COL0;
         switch (Device->GetPortType(m_IncompleteWire.OutputPort+Device->GetInfo()->NumInputs)) {
             case 0:     col = (Fl_Color) WIRE_COL0; break;
             case 1:     col = (Fl_Color) WIRE_COL1; break;
             case 2:     col = (Fl_Color) WIRE_COL2; break;
             case 3:     col = (Fl_Color) WIRE_COL3; break;
             case 4:     col = (Fl_Color) WIRE_COL4; break;
             default:    col = (Fl_Color) WIRE_COL0;
         }
 		fl_color(col);

		fl_line(Device->GetPortX(m_IncompleteWire.OutputPort+Device->GetInfo()->NumInputs),
				Device->GetPortY(m_IncompleteWire.OutputPort+Device->GetInfo()->NumInputs),
				Fl::event_x(),
				Fl::event_y());
	}
}

////////////////////////////////////////////////////////////////////////

void Fl_Canvas::ClearIncompleteWire()
{
	m_StripPort = -1;
	// Turn off both ports
	if (m_IncompleteWire.OutputID!=-1)
	{
		FindDevice(m_IncompleteWire.OutputID)->RemoveConnection(m_IncompleteWire.OutputPort+FindDevice(m_IncompleteWire.OutputID)->GetInfo()->NumInputs);
	}

	if (m_IncompleteWire.InputID!=-1)
	{
		FindDevice(m_IncompleteWire.InputID)->RemoveConnection(m_IncompleteWire.InputPort);
	}
	m_IncompleteWire.Clear();
}

////////////////////////////////////////////////////////////////////////
//  Event handlers for the canvas and the deviceGUIs

static string DroppedPath(const char *text)
{
	string line;
	for (const char *c = text; c && *c; ++c)
	{
		if (*c == '\n' || *c == '\r')
		{
			if (!line.empty())
				break;
			continue;
		}
		line += *c;
	}
	if (line.compare(0, 7, "file://") != 0)
		return line;
	const size_t slash = line.find('/', 7);
	if (slash == string::npos)
		return string();
	string path;
	for (size_t i = slash; i < line.size(); ++i)
	{
		if (line[i] == '%' && i + 2 < line.size() && isxdigit((unsigned char)line[i + 1])
		    && isxdigit((unsigned char)line[i + 2]))
		{
			path += (char)strtol(line.substr(i + 1, 2).c_str(), NULL, 16);
			i += 2;
		}
		else
			path += line[i];
	}
	return path;
}

void Fl_Canvas::cb_Dropped(void *data)
{
	Fl_Canvas *canvas = (Fl_Canvas *)data;
	if (canvas->cb_DropFile && !canvas->m_DroppedFile.empty())
		canvas->cb_DropFile(canvas, canvas->user_data());
}

int Fl_Canvas::handle (int event) {
    if (m_PickAt) return PickEvent(event);
    switch (event) {
       case FL_DND_ENTER:
       case FL_DND_DRAG:
       case FL_DND_LEAVE:
          return cb_DropFile ? 1 : 0;
       case FL_DND_RELEASE:
          m_Dropping = cb_DropFile != NULL;
          return m_Dropping ? 1 : 0;
       case FL_PASTE:
          if (!m_Dropping)
             break;
          m_Dropping = false;
          m_DroppedFile = DroppedPath(Fl::event_text());
          Fl::add_timeout(0.0, cb_Dropped, this);
          return 1;
    }
    bool stripInput;
    int stripPort;
    if (event==FL_PUSH && StripAt(Fl::event_x(), Fl::event_y(), stripInput, stripPort)) {
       StripClicked(stripInput, stripPort, Fl::event_button() == 3);
       return 1;
    }
    if (Fl_Group::handle (event)) return 1;
    if (event==FL_PUSH) {
       ClearIncompleteWire();
       redraw();
       m_DragX=Fl::event_x();
       m_DragY=Fl::event_y();
    }
    if ((Fl::event_button() == 1) && ((Fl::event_state() & FL_SHIFT) != 0) && ((Fl::event_state() & FL_CTRL) == 0)) {
       // Shift-left-drag on empty canvas selects a group of devices.
       if (event==FL_PUSH) {
          if (m_HaveSelection) {
             m_Selection.Clear();
             m_HaveSelection = false;
          }
          m_Selecting = true;
          m_StartSelectX=Fl::event_x();
          m_StartSelectY=Fl::event_y();
          m_EndSelectX=Fl::event_x();
          m_EndSelectY=Fl::event_y();
          ClearIncompleteWire();
          redraw();
          m_DragX=Fl::event_x();
          m_DragY=Fl::event_y();
       }
       if ((event==FL_DRAG) && m_Selecting) {
          m_EndSelectX = Fl::event_x();
          m_EndSelectY = Fl::event_y();
          Fl_Scroll* scroll = (Fl_Scroll *)parent();
          int newx = 0, xp = scroll->xposition();
          int newy = 0, yp = scroll->yposition();
          if ((m_EndSelectX < m_StartSelectX) && ((m_EndSelectX - x() - xp) <= 15))
             newx = 10;
          if ((m_EndSelectY < m_StartSelectY) && ((m_EndSelectY - y() - yp) <= 15))
             newy = 10;
          if ((m_EndSelectX > m_StartSelectX) && ((scroll->x() + scroll->w() - m_EndSelectX - 15) <= 15))
             newx = -10;
          if ((m_EndSelectY > m_StartSelectY) && ((scroll->y() + scroll->h() - m_EndSelectY - 15) <= 5))
             newy = -10;
          if ((newx!=0) || (newy!=0)) {
             position(x()+newx,y()+newy);
             m_StartSelectX += newx;
             m_StartSelectY += newy;
          }
          m_DragX=Fl::event_x();
          m_DragY=Fl::event_y();
          redraw();
       }
       if ((event==FL_RELEASE) && m_Selecting) {
          m_Selecting = false;
          if ((m_EndSelectX != m_StartSelectX) && (m_EndSelectY != m_StartSelectY))
             CalculateSelection();
          redraw();
       }
    }
    if ((Fl::event_button() == 2) || ((Fl::event_button() == 1) && ((Fl::event_state() & (FL_SHIFT | FL_CTRL)) == 0))) {
       // Plain left-drag or middle-drag on empty canvas pans the canvas.
       if (event==FL_PUSH) {
          ClearIncompleteWire();
          redraw();
          m_DragX=Fl::event_x();
          m_DragY=Fl::event_y();
       }
       if (event==FL_DRAG) {
          position (x() + (Fl::event_x() - m_DragX), y() + (Fl::event_y() - m_DragY));
          m_DragX=Fl::event_x();
          m_DragY=Fl::event_y();
          redraw();
       }
    }
    if ((Fl::event_button() == 3) || ((Fl::event_button() == 1) && ((Fl::event_state() & FL_CTRL) != 0))) {
       // Right-Click (or Ctrl-click for the M.B.C.)
       // Pop-up Edit/Plugins menu
       if (event==FL_PUSH) {
          m_x=Fl::event_x();
          m_y=Fl::event_y();
          PopupEditMenu (this);
       }
    }
    return 1;
}

void Fl_Canvas::cb_OnDrag_s (Fl_Widget* widget, int x, int y, void* data) {
     ((Fl_Canvas *)data)->cb_OnDrag_i (widget, x, y);
}

inline void Fl_Canvas::cb_OnDrag_i (Fl_Widget* widget, int xoffset, int yoffset) {
       if ((widget) && (widget->parent())) {
          int moved_device_id = ((Fl_DeviceGUI*)(widget->parent()))->GetID();
          if (m_HaveSelection &&
              std::find(m_Selection.m_DeviceIds.begin(), m_Selection.m_DeviceIds.end(),
                        moved_device_id) != m_Selection.m_DeviceIds.end()) {
             for (unsigned int i=0; i<m_Selection.m_DeviceIds.size(); i++) {
                 int ID = Selection().m_DeviceIds[i];
                 Fl_Widget *o = FindDevice(ID);
                 if ((o) && (m_Selection.m_DeviceIds[i] != moved_device_id)) {
                    o->position (o->x() + xoffset, o->y() + yoffset);
                 }
             }
          }
       }
       return;
}

void Fl_Canvas::cb_OnDragClick_s (Fl_Widget* widget, int event, int button, int shift_state, void* data) {
     ((Fl_Canvas *)data)->cb_OnDragClick_i (widget, event, button, shift_state);
}

inline void Fl_Canvas::cb_OnDragClick_i(Fl_Widget* widget, int event, int button,int shift_state) {
       if (!widget || !widget->parent() || button != 1) return;
       if (event == FL_RELEASE) {
          // A plain gesture keeps the group until movement has finished.
          if ((shift_state & (FL_SHIFT | FL_CTRL)) == 0) {
             m_Selection.Clear();
             m_HaveSelection = false;
             redraw();
          }
          return;
       }
       if (event != FL_PUSH) return;


       int ID = ((Fl_DeviceGUI*)widget->parent())->GetID();
       if (Fl::event_clicks() > 0 && IsBox(ID)) {
          ShowPage(ID);
          return;
       }
       std::vector<int>::iterator device_iter = std::find(
           m_Selection.m_DeviceIds.begin(), m_Selection.m_DeviceIds.end(), ID);
       if ((shift_state & FL_SHIFT) != 0) {
          // Shift adds before dragging, including when this item is already selected.
          if (device_iter == m_Selection.m_DeviceIds.end())
             m_Selection.m_DeviceIds.push_back(ID);
       }
       else if ((shift_state & FL_CTRL) != 0) {
          // Retain the existing Ctrl-click selection toggle.
          if (device_iter == m_Selection.m_DeviceIds.end())
             m_Selection.m_DeviceIds.push_back(ID);
          else
             m_Selection.m_DeviceIds.erase(device_iter);
       }
       // Plain press neither selects an item nor clears the existing group.
       m_HaveSelection = !m_Selection.m_DeviceIds.empty();
       redraw();
}

////////////////////////////////////////////////////////////////////////

void Fl_Canvas::PortClicked(Fl_DeviceGUI* Device, int Type, int Port, bool Value)
{
	if (Value && m_StripPort >= 0)
	{
		const bool input = m_StripInput;
		const int port = m_StripPort;
		ClearIncompleteWire();
		if (input == (Type==Fl_DeviceGUI::INPUT))
		{
			BindBoxPort(m_Page, input, port, Device->GetID(), Port);
			return;
		}
	}

	if(Value) // Turned on the port
	{
		if(m_IncompleteWire.InputID==-1 || m_IncompleteWire.OutputID==-1)
		{
			if (Type==Fl_DeviceGUI::OUTPUT)
			{
				// make sure we don't make a output->output connection
				if (m_IncompleteWire.OutputID==-1)
				{
					m_IncompleteWire.OutputPort=Port;
					m_IncompleteWire.OutputID=Device->GetID();
					m_IncompleteWire.OutputTerminal=Device->IsTerminal();
				}
				else
				{
					ClearIncompleteWire();
				}
			}
			else
			{
				// make sure we don't make a input->input connection
				if (m_IncompleteWire.InputID==-1)
				{
					m_IncompleteWire.InputPort=Port;
					m_IncompleteWire.InputID=Device->GetID();
					m_IncompleteWire.InputTerminal=Device->IsTerminal();
				}
				else
				{
					ClearIncompleteWire();
				}
			}

			// if both have now been set...
			if (m_IncompleteWire.InputID!=-1 && m_IncompleteWire.OutputID!=-1)
			{
				if (!ResolveWire(m_IncompleteWire))
				{
					ClearIncompleteWire();
					return;
				}
				m_WireVec.push_back(m_IncompleteWire);

				// send the connect callback
				cb_Connection(this,(void*)&m_IncompleteWire);
				LockGraph();
				m_Graph.AddConnection(m_IncompleteWire.OutputID,m_IncompleteWire.OutputTerminal,
									  m_IncompleteWire.InputID,m_IncompleteWire.InputTerminal);
				UnlockGraph();
				Reschedule();

				// Turn on both ports
				Fl_DeviceGUI* ODGUI = FindDevice(m_IncompleteWire.OutputID);
				ODGUI->AddConnection(m_IncompleteWire.OutputPort+ODGUI->GetInfo()->NumInputs);

				Fl_DeviceGUI* IDGUI = FindDevice(m_IncompleteWire.InputID);
				IDGUI->AddConnection(m_IncompleteWire.InputPort);

				m_IncompleteWire.Clear();

				redraw();
			}
		}
	}
	else // Turned off the port
	{
		// Find connections using this port
		int ID = Device->GetID();
		if (IsBox(ID) && !Resolve(ID, Type==Fl_DeviceGUI::INPUT, Port, ID, Port))
			return;
		bool Found=true;

		while (Found)
		{
			Found=false;

			for(vector<CanvasWire>::iterator i=m_WireVec.begin();
				i!=m_WireVec.end(); i++)
			{
				if ((Type==Fl_DeviceGUI::OUTPUT && i->OutputID==ID && i->OutputPort==Port) ||
					(Type==Fl_DeviceGUI::INPUT && i->InputID==ID && i->InputPort==Port))
				{
					// Turn off both ports
					Fl_DeviceGUI* ODGUI = FindDevice(i->OutputID);
					ODGUI->RemoveConnection(i->OutputPort+ODGUI->GetInfo()->NumInputs);

					Fl_DeviceGUI* IDGUI = FindDevice(i->InputID);
					IDGUI->RemoveConnection(i->InputPort);

					// send the unconnect callback
					cb_Unconnect(this,(void*)&(*i));
					LockGraph();
					m_Graph.RemoveConnection(i->OutputID,i->InputID);
					UnlockGraph();

					// Remove the wire
					m_WireVec.erase(i);

					Found=true;
					break;
				}
			}
		}

		Reschedule();
		redraw();

		// Clear the current m_Selection
		m_IncompleteWire.Clear();
	}
}

////////////////////////////////////////////////////////////////////////

void Fl_Canvas::ClearConnections(Fl_DeviceGUI* Device)
{
	PruneConnections(Device, 0, 0);
}

void Fl_Canvas::PruneConnections(Fl_DeviceGUI* Device, int inputs, int outputs)
{
	bool removedall=false;

	//make sure we don't leave a dangling incomplete wire this will cause errors/seg-faults
	if (UserMakingConnection() && Device && ((Device->GetID() == m_IncompleteWire.OutputID) ||
		(Device->GetID() == m_IncompleteWire.InputID)))
	{
		ClearIncompleteWire();
	}

	while (!removedall)
	{
		removedall=true;

		for (vector<CanvasWire>::iterator i=m_WireVec.begin();
			 i!=m_WireVec.end(); i++)
		{
			if ((i->OutputID==Device->GetID() && i->OutputPort>=outputs) ||
			    (i->InputID==Device->GetID() && i->InputPort>=inputs))
			{
				RemoveWire(i);
				removedall=false;
				break;
			}
		}
	}
	Reschedule();
}

void Fl_Canvas::RemoveWire(vector<CanvasWire>::iterator i)
{
	// Turn off both ports. A missing end must not crash the delete.
	Fl_DeviceGUI *source = FindDevice(i->OutputID);
	Fl_DeviceGUI *dest = FindDevice(i->InputID);
	if (source)
		source->RemoveConnection(i->OutputPort+source->GetInfo()->NumInputs);
	if (dest)
		dest->RemoveConnection(i->InputPort);

	// send the unconnect callback
	cb_Unconnect(this,(void*)&(*i));
	LockGraph();
	m_Graph.RemoveConnection(i->OutputID,i->InputID);
	UnlockGraph();

	m_WireVec.erase(i);
}

void Fl_Canvas::MoveInputs(Fl_DeviceGUI* Device, const vector<int> &to)
{
	for (vector<CanvasWire>::iterator i=m_WireVec.begin(); i!=m_WireVec.end(); )
	{
		if (i->InputID==Device->GetID() && i->InputPort>=0 && i->InputPort<(int)to.size() && to[i->InputPort]<0)
		{
			RemoveWire(i);
			i=m_WireVec.begin();
		}
		else ++i;
	}
	for (vector<CanvasWire>::iterator i=m_WireVec.begin(); i!=m_WireVec.end(); ++i)
	{
		if (i->InputID==Device->GetID() && i->InputPort>=0 && i->InputPort<(int)to.size())
			i->InputPort=to[i->InputPort];
	}
	RenumberBinds(Device->GetID(), true, to);
}

////////////////////////////////////////////////////////////////////////

void Fl_Canvas::RestorePortConnections(Fl_DeviceGUI* Device)
{
	for (vector<CanvasWire>::const_iterator i=m_WireVec.begin(); i!=m_WireVec.end(); ++i)
	{
		if (i->InputID==Device->GetID()) Device->AddConnection(i->InputPort);
		if (i->OutputID==Device->GetID())
			Device->AddConnection(Device->GetInfo()->NumInputs+i->OutputPort);
	}
}

void Fl_Canvas::RemoveDevice(Fl_DeviceGUI* Device)
{
	ClearConnections(Device);
	const int ID = Device->GetID();
	for (map<int,SubPatchDevice*>::iterator b = m_Boxes.begin(); b != m_Boxes.end(); ++b)
	{
		for (size_t k = 0; k < b->second->Inputs.size(); ++k)
			if (b->second->Inputs[k].InnerID == ID) b->second->Inputs[k].InnerID = -1;
		for (size_t k = 0; k < b->second->Outputs.size(); ++k)
			if (b->second->Outputs[k].InnerID == ID) b->second->Outputs[k].InnerID = -1;
	}
	m_PageOf.erase(ID);
	m_Boxes.erase(ID);
	Reschedule();
	if (m_Page == ID) ShowPage(-1);
	remove(Device);
	redraw();
}

////////////////////////////////////////////////////////////////////////

void Fl_Canvas::Clear()
{
	LockGraph();
	m_Graph.Clear();
	UnlockGraph();

	int kids=children();

	for(int n=0; n<kids; n++)
	{
		remove(child(0));
	}

	m_WireVec.clear();
	m_PageOf.clear();
	m_Boxes.clear();
	m_Page = -1;
	Reschedule();
	redraw();
}

void Fl_Canvas::ShowPage(int box)
{
	const int shown = m_Page;
	m_Page = (box == -1 || IsBox(box)) ? box : -1;
	for (int n = 0; n < children(); n++)
	{
		Fl_DeviceGUI *device = AsDevice(child(n));
		if (!device)
			continue;
		if (PageOf(device->GetID()) == m_Page)
			device->show();
		else
			device->hide();
	}
	ClearIncompleteWire();
	ClearSelection(this);
	if (m_Page != shown) CenterPatch();
	redraw();
	if (cb_PageChanged) cb_PageChanged(this, user_data());
}

void Fl_Canvas::DeviceMenu(Fl_DeviceGUI *device)
{
	const int id = device->GetID();
	if (std::find(m_Selection.m_DeviceIds.begin(), m_Selection.m_DeviceIds.end(), id) == m_Selection.m_DeviceIds.end())
	{
		m_Selection.Clear();
		m_Selection.m_DeviceIds.push_back(id);
		m_HaveSelection = true;
		redraw();
	}
	const size_t selected = m_Selection.m_DeviceIds.size();
	SubPatchDevice *box = selected == 1 ? Box(id) : NULL;
	SpiralPluginGUI *gui = dynamic_cast<SpiralPluginGUI *>(device->GetPluginWindow());

	enum { Front = 1, Back, RenameIt, DeleteIt, GroupIt, Open, UngroupIt, Info };
	Fl_Group *current = Fl_Group::current();
	Fl_Group::current(NULL);
	Fl_Menu_Button menu(0, 0, 0, 0);
	Fl_Group::current(current);
	menu.textsize(10);
	menu.add("Bring to Front", 0, NULL, (void *)Front);
	menu.add("Send to Back", 0, NULL, (void *)Back, FL_MENU_DIVIDER);
	menu.add("Rename Device", 0, NULL, (void *)RenameIt, FL_MENU_DIVIDER);
	menu.add("Delete", 0, NULL, (void *)DeleteIt, FL_MENU_DIVIDER);
	int last = -1;
	if (selected > 1 && cb_GroupDevices)
		last = menu.add("Group", 0, NULL, (void *)GroupIt);
	if (box)
	{
		last = menu.add("Open Subpatch", 0, NULL, (void *)Open);
		if (cb_UngroupDevices)
			last = menu.add("Ungroup", 0, NULL, (void *)UngroupIt);
	}
	if (last >= 0 && gui)
		menu.mode(last, menu.mode(last) | FL_MENU_DIVIDER);
	if (gui)
		menu.add("Info", 0, NULL, (void *)Info);

	const Fl_Menu_Item *picked = menu.menu()->popup(Fl::event_x(), Fl::event_y(), NULL, NULL, &menu);
	if (!picked)
		return;
	const long action = (long)picked->user_data();
	switch (action)
	{
		case Front:
			insert(*device, children());
			redraw();
			break;
		case Back:
			insert(*device, 0);
			redraw();
			break;
		case RenameIt:
			device->Rename();
			break;
		case DeleteIt:
			if (selected > 1)
				cb_DeleteDeviceGroup(this, this);
			else
				device->Delete();
			break;
		case GroupIt:
			cb_GroupDevices(this, user_data());
			break;
		case Open:
			ShowPage(id);
			break;
		case UngroupIt:
			cb_UngroupDevices(this, user_data());
			break;
		case Info:
			if (gui) gui->ShowHelp();
			break;
	}
}

string Fl_Canvas::BoxTitle(int id)
{
	Fl_DeviceGUI *device = FindDevice(id);
	return device ? device->GetName() : string();
}

vector<int> Fl_Canvas::BoxesOn(int page) const
{
	vector<int> boxes;
	for (map<int,SubPatchDevice*>::const_iterator b = m_Boxes.begin(); b != m_Boxes.end(); ++b)
		if (PageOf(b->first) == page)
			boxes.push_back(b->first);
	return boxes;
}

int Fl_Canvas::PageOf(int id) const
{
	map<int,int>::const_iterator i = m_PageOf.find(id);
	return i == m_PageOf.end() ? -1 : i->second;
}

void Fl_Canvas::SetPageOf(int id, int page)
{
	if (page == -1)
		m_PageOf.erase(id);
	else
		m_PageOf[id] = page;
	Reschedule();
}

SubPatchDevice *Fl_Canvas::Box(int id)
{
	map<int,SubPatchDevice*>::iterator i = m_Boxes.find(id);
	return i == m_Boxes.end() ? NULL : i->second;
}

void Fl_Canvas::RemoveBox(int id)
{
	m_Boxes.erase(id);
	Reschedule();
	if (m_Page == id)
		ShowPage(PageOf(id));
}

bool Fl_Canvas::Resolve(int id, bool input, int port, int &device, int &devicePort) const
{
	for (int depth = 0; depth < 64; ++depth)
	{
		map<int,SubPatchDevice*>::const_iterator box = m_Boxes.find(id);
		if (box == m_Boxes.end())
		{
			device = id;
			devicePort = port;
			return true;
		}
		const vector<BoundaryPort> &ports = input ? box->second->Inputs : box->second->Outputs;
		if (port < 0 || port >= (int)ports.size() || ports[port].InnerID < 0)
			return false;
		id = ports[port].InnerID;
		port = ports[port].InnerPort;
	}
	return false;
}

int Fl_Canvas::Holder(int id, int page) const
{
	for (int depth = 0; depth < 64; ++depth)
	{
		const int up = PageOf(id);
		if (up == page)
			return id;
		if (up == -1)
			return -1;
		id = up;
	}
	return -1;
}

int Fl_Canvas::PortThrough(int holder, bool input, int device, int port) const
{
	if (holder == device)
		return port;
	map<int,SubPatchDevice*>::const_iterator box = m_Boxes.find(holder);
	if (box == m_Boxes.end())
		return -1;
	const int count = (int)(input ? box->second->Inputs.size() : box->second->Outputs.size());
	for (int k = 0; k < count; ++k)
	{
		int d, p;
		if (Resolve(holder, input, k, d, p) && d == device && p == port)
			return k;
	}
	return -1;
}

void Fl_Canvas::WiresOnPage(int page, vector<CanvasWire> &out) const
{
	for (vector<CanvasWire>::const_iterator i = m_WireVec.begin(); i != m_WireVec.end(); ++i)
	{
		const int source = Holder(i->OutputID, page);
		const int dest = Holder(i->InputID, page);
		if (source == -1 || dest == -1 || (source == dest && source != i->OutputID))
			continue;
		CanvasWire wire = *i;
		wire.OutputID = source;
		wire.OutputPort = PortThrough(source, false, i->OutputID, i->OutputPort);
		wire.InputID = dest;
		wire.InputPort = PortThrough(dest, true, i->InputID, i->InputPort);
		if (wire.OutputPort >= 0 && wire.InputPort >= 0)
			out.push_back(wire);
	}
}

bool Fl_Canvas::ResolveWire(CanvasWire &wire) const
{
	if (!Resolve(wire.OutputID, false, wire.OutputPort, wire.OutputID, wire.OutputPort)
	    || !Resolve(wire.InputID, true, wire.InputPort, wire.InputID, wire.InputPort))
		return false;
	Fl_DeviceGUI *source = const_cast<Fl_Canvas *>(this)->FindDevice(wire.OutputID);
	Fl_DeviceGUI *dest = const_cast<Fl_Canvas *>(this)->FindDevice(wire.InputID);
	if (!source || !dest)
		return false;
	if ((source->GetPortType(wire.OutputPort + source->GetInfo()->NumInputs) == Sample::EVENT)
	    != (dest->GetPortType(wire.InputPort) == Sample::EVENT))
		return false;
	wire.OutputTerminal = source->IsTerminal();
	wire.InputTerminal = dest->IsTerminal();
	return true;
}

void Fl_Canvas::StripPoint(bool input, int index, int &x, int &y) const
{
	const Fl_Widget *view = parent() ? parent() : this;
	x = input ? view->x() + 14 : view->x() + view->w() - 30;
	y = view->y() + 46 + index * 24;
}

void Fl_Canvas::DrawStrips()
{
	map<int,SubPatchDevice*>::const_iterator box = m_Boxes.find(m_Page);
	if (box == m_Boxes.end())
		return;
	Fl_DeviceGUI *shown = FindDevice(m_Page);
	const Fl_Widget *view = parent() ? parent() : this;
	const Fl_Color text = fl_contrast(FL_BLACK, color());
	fl_font(FL_HELVETICA, 10);
	fl_color(text);
	const string mode = box->second->Config().Mode == "Virtual" ? string() : " (" + box->second->Config().Mode + ")";
	const string title = "SubPatch: " + (shown ? shown->GetName() : string()) + mode + "    (Edit > Back to Parent Patch)";
	fl_draw(title.c_str(), view->x() + 10, view->y() + 20);
	for (int input = 1; input >= 0; --input)
	{
		const vector<BoundaryPort> &ports = input ? box->second->Inputs : box->second->Outputs;
		for (size_t k = 0; k < ports.size(); ++k)
		{
			int px, py;
			StripPoint(input != 0, (int)k, px, py);
			Fl_DeviceGUI *inner = ports[k].InnerID < 0 ? NULL : FindDevice(ports[k].InnerID);
			if (inner && inner->visible())
			{
				const int at = input ? ports[k].InnerPort : ports[k].InnerPort + inner->GetInfo()->NumInputs;
				fl_color(FL_DARK_YELLOW);
				fl_line_style(FL_SOLID, 2);
				fl_line(px, py, inner->GetPortX(at), inner->GetPortY(at));
				fl_line_style(FL_SOLID, 0);
			}
			fl_color(FL_DARK_YELLOW);
			fl_rectf(px - PortSize, py - PortSize, PortSize * 2, PortSize * 2);
			fl_color(text);
			if (input)
				fl_draw(ports[k].Name.c_str(), px + PortSize + 4, py + 4);
			else
				fl_draw(ports[k].Name.c_str(), px - PortSize - 4 - (int)fl_width(ports[k].Name.c_str()), py + 4);
		}
	}
}

bool Fl_Canvas::StripAt(int x, int y, bool &input, int &port)
{
	SubPatchDevice *box = Box(m_Page);
	for (int side = 1; box && side >= 0; --side)
	{
		const int count = (int)(side ? box->Inputs.size() : box->Outputs.size());
		for (int k = 0; k < count; ++k)
		{
			int px, py;
			StripPoint(side != 0, k, px, py);
			if (abs(x - px) <= PortSize + 2 && abs(y - py) <= PortSize + 2)
			{
				input = side != 0;
				port = k;
				return true;
			}
		}
	}
	return false;
}

void Fl_Canvas::StripClicked(bool input, int port, bool unbind)
{
	const CanvasWire pending = m_IncompleteWire;
	ClearIncompleteWire();
	if (unbind)
		UnbindBoxPort(m_Page, input, port);
	else if (input && pending.InputID != -1 && pending.OutputID == -1)
		BindBoxPort(m_Page, true, port, pending.InputID, pending.InputPort);
	else if (!input && pending.OutputID != -1 && pending.InputID == -1)
		BindBoxPort(m_Page, false, port, pending.OutputID, pending.OutputPort);
	else
	{
		m_StripInput = input;
		m_StripPort = port;
	}
	redraw();
}

string Fl_Canvas::Name(int id)
{
	Fl_DeviceGUI *device = FindDevice(id);
	return device ? device->GetName() : string();
}

bool Fl_Canvas::IsEventPort(int id, bool input, int port)
{
	if (const SubPatchDevice *box = Box(id))
	{
		const vector<BoundaryPort> &ports = input ? box->Inputs : box->Outputs;
		return port >= 0 && port < (int)ports.size() && ports[port].IsEvent();
	}
	Fl_DeviceGUI *device = FindDevice(id);
	return device && device->GetPortType(input ? port : port + device->GetInfo()->NumInputs) == Sample::EVENT;
}

string Fl_Canvas::PortName(int id, bool input, int port)
{
	if (const SubPatchDevice *box = Box(id))
	{
		const vector<BoundaryPort> &ports = input ? box->Inputs : box->Outputs;
		return port >= 0 && port < (int)ports.size() ? ports[port].Name : string();
	}
	Fl_DeviceGUI *device = FindDevice(id);
	if (!device)
		return string();
	const DeviceGUIInfo *info = device->GetInfo();
	const int at = input ? port : info->NumInputs + port;
	return at >= 0 && at < (int)info->PortTips.size() ? info->PortTips[at] : string();
}

void Fl_Canvas::WiresThrough(int box, bool input, int port, vector<CanvasWire> &out) const
{
	const int page = PageOf(box);
	for (vector<CanvasWire>::const_iterator i = m_WireVec.begin(); i != m_WireVec.end(); ++i)
	{
		const int innerID = input ? i->InputID : i->OutputID;
		const int innerPort = input ? i->InputPort : i->OutputPort;
		const int outerID = input ? i->OutputID : i->InputID;
		if (Holder(innerID, page) == box && Holder(outerID, page) != box
		    && PortThrough(box, input, innerID, innerPort) == port)
			out.push_back(*i);
	}
}

void Fl_Canvas::EraseWire(const CanvasWire &wire)
{
	for (vector<CanvasWire>::iterator i = m_WireVec.begin(); i != m_WireVec.end(); ++i)
	{
		if (i->OutputID != wire.OutputID || i->OutputPort != wire.OutputPort
		    || i->InputID != wire.InputID || i->InputPort != wire.InputPort)
			continue;
		if (Fl_DeviceGUI *source = FindDevice(i->OutputID))
			source->RemoveConnection(i->OutputPort + source->GetInfo()->NumInputs);
		if (Fl_DeviceGUI *dest = FindDevice(i->InputID))
			dest->RemoveConnection(i->InputPort);
		cb_Unconnect(this, (void*)&(*i));
		LockGraph();
		m_Graph.RemoveConnection(i->OutputID, i->InputID);
		UnlockGraph();
		m_WireVec.erase(i);
		Reschedule();
		return;
	}
}

void Fl_Canvas::AddWire(CanvasWire wire)
{
	Fl_DeviceGUI *source = FindDevice(wire.OutputID);
	Fl_DeviceGUI *dest = FindDevice(wire.InputID);
	if (!source || !dest)
		return;
	wire.OutputTerminal = source->IsTerminal();
	wire.InputTerminal = dest->IsTerminal();
	m_WireVec.push_back(wire);
	cb_Connection(this, (void*)&wire);
	LockGraph();
	m_Graph.AddConnection(wire.OutputID, wire.OutputTerminal, wire.InputID, wire.InputTerminal);
	UnlockGraph();
	Reschedule();
	source->AddConnection(wire.OutputPort + source->GetInfo()->NumInputs);
	dest->AddConnection(wire.InputPort);
}

bool Fl_Canvas::BindBoxPort(int box, bool input, int port, int id, int idPort)
{
	SubPatchDevice *edited = Box(box);
	if (!edited || port < 0 || port >= (int)(input ? edited->Inputs.size() : edited->Outputs.size()) || PageOf(id) != box)
		return false;
	BoundaryPort &bound = (input ? edited->Inputs : edited->Outputs)[port];
	if (bound.InnerID == id && bound.InnerPort == idPort)
		return true;
	int device, devicePort;
	const bool event = IsEventPort(id, input, idPort);
	if (!input && event)
		return false;
	const bool reaches = Resolve(id, input, idPort, device, devicePort) && event == bound.IsEvent();
	vector<CanvasWire> through;
	WiresThrough(box, input, port, through);
	if (input && reaches)
		for (vector<CanvasWire>::const_iterator i = m_WireVec.begin(); i != m_WireVec.end(); ++i)
			if (i->InputID == device && i->InputPort == devicePort)
				return false;
	for (size_t w = 0; w < through.size(); ++w)
		EraseWire(through[w]);
	bound.InnerID = id;
	bound.InnerPort = idPort;
	if (event)
		bound.Type = "MusicalEventPort";
	else if (bound.IsEvent())
		bound.Type = string();
	for (size_t w = 0; reaches && w < through.size(); ++w)
	{
		CanvasWire moved = through[w];
		(input ? moved.InputID : moved.OutputID) = device;
		(input ? moved.InputPort : moved.OutputPort) = devicePort;
		AddWire(moved);
	}
	RefreshBox(box);
	return true;
}

void Fl_Canvas::UnbindBoxPort(int box, bool input, int port)
{
	SubPatchDevice *edited = Box(box);
	if (!edited || port < 0 || port >= (int)(input ? edited->Inputs.size() : edited->Outputs.size()))
		return;
	vector<CanvasWire> through;
	WiresThrough(box, input, port, through);
	for (size_t w = 0; w < through.size(); ++w)
		EraseWire(through[w]);
	(input ? edited->Inputs : edited->Outputs)[port].InnerID = -1;
	redraw();
}

void Fl_Canvas::AddBoxPort(int box, bool input, const string &name)
{
	AddBoxPort(box, input, name, "AudioPort");
}

void Fl_Canvas::AddBoxPort(int box, bool input, const string &name, const string &type)
{
	SubPatchDevice *edited = Box(box);
	if (!edited)
		return;
	(input ? edited->Inputs : edited->Outputs).push_back(BoundaryPort(-1, -1, name, type));
	RefreshBox(box);
}

void Fl_Canvas::RemoveBoxPort(int box, bool input, int port)
{
	SubPatchDevice *edited = Box(box);
	if (!edited || port < 0 || port >= (int)(input ? edited->Inputs.size() : edited->Outputs.size()))
		return;
	UnbindBoxPort(box, input, port);
	vector<BoundaryPort> &ports = input ? edited->Inputs : edited->Outputs;
	vector<int> moved;
	for (int k = 0; k < (int)ports.size(); ++k)
		moved.push_back(k < port ? k : k == port ? -1 : k - 1);
	ports.erase(ports.begin() + port);
	RenumberBinds(box, input, moved);
	RefreshBox(box);
}

bool Fl_Canvas::MoveBoxPort(int box, bool input, int from, int to)
{
	SubPatchDevice *edited = Box(box);
	if (!edited)
		return false;
	vector<BoundaryPort> &ports = input ? edited->Inputs : edited->Outputs;
	if (from < 0 || to < 0 || from >= (int)ports.size() || to >= (int)ports.size())
		return false;
	const BoundaryPort port = ports[from];
	ports.erase(ports.begin() + from);
	ports.insert(ports.begin() + to, port);
	vector<int> moved;
	for (int k = 0; k < (int)ports.size(); ++k)
		moved.push_back(k == from ? to : from < to && k > from && k <= to ? k - 1
			: to < from && k >= to && k < from ? k + 1 : k);
	RenumberBinds(box, input, moved);
	RefreshBox(box);
	return true;
}

void Fl_Canvas::RenameBoxPort(int box, bool input, int port, const string &name)
{
	SubPatchDevice *edited = Box(box);
	if (!edited || port < 0 || port >= (int)(input ? edited->Inputs.size() : edited->Outputs.size()))
		return;
	(input ? edited->Inputs : edited->Outputs)[port].Name = name;
	RefreshBox(box);
}

void Fl_Canvas::RenumberBinds(int box, bool input, const vector<int> &moved)
{
	for (map<int,SubPatchDevice*>::iterator outer = m_Boxes.begin(); outer != m_Boxes.end(); ++outer)
	{
		vector<BoundaryPort> &ports = input ? outer->second->Inputs : outer->second->Outputs;
		for (size_t k = 0; k < ports.size(); ++k)
			if (ports[k].InnerID == box && ports[k].InnerPort >= 0 && ports[k].InnerPort < (int)moved.size())
			{
				ports[k].InnerPort = moved[ports[k].InnerPort];
				if (ports[k].InnerPort < 0)
					ports[k].InnerID = -1;
			}
	}
}

void Fl_Canvas::RefreshBox(int box)
{
	SubPatchDevice *shown = Box(box);
	Fl_DeviceGUI *device = FindDevice(box);
	if (!shown || !device)
		return;
	shown->PortsChanged();
	const PluginInfo &ports = shown->GetPluginInfo();
	DeviceGUIInfo info = *device->GetInfo();
	info.NumInputs = ports.NumInputs;
	info.NumOutputs = ports.NumOutputs;
	info.PortTips = ports.PortTips;
	info.PortTypes = ports.PortTypes;
	info.Height = 20 + 12 * (info.NumInputs > info.NumOutputs ? info.NumInputs : info.NumOutputs);
	device->SetupPorts(info, false, true);
	redraw();
}

void Fl_Canvas::SetBoxMode(int box, const string &mode)
{
	SubPatchDevice *edited = Box(box);
	if (!edited)
		return;
	Spiral::Core::SubPatchConfig config = edited->Config();
	config.Mode = mode;
	edited->SetConfig(config);
	Reschedule();
	redraw();
}

void Fl_Canvas::SetBoxVoices(int box, long voices)
{
	SubPatchDevice *edited = Box(box);
	if (!edited)
		return;
	Spiral::Core::SubPatchConfig config = edited->Config();
	config.Voices = voices;
	edited->SetConfig(config);
	Reschedule();
}

bool Fl_Canvas::Owns(int box) const
{
	map<int,SubPatchDevice*>::const_iterator i = m_Boxes.find(box);
	return i != m_Boxes.end() && i->second->Config().Mode != "Virtual";
}

int Fl_Canvas::Scheduled(int id, int scope) const
{
	int result = id;
	for (int depth = 0; depth < 64; ++depth)
	{
		const int up = PageOf(id);
		if (up == scope)
			return result;
		if (up == -1)
			return -1;
		if (Owns(up))
			result = up;
		id = up;
	}
	return -1;
}

void Fl_Canvas::LockGraph()
{
	if (m_GraphLock)
		pthread_mutex_lock(m_GraphLock);
}

void Fl_Canvas::UnlockGraph()
{
	if (m_GraphLock)
		pthread_mutex_unlock(m_GraphLock);
}

void Fl_Canvas::Reschedule()
{
	LockGraph();
	++m_Topology;
	m_Owned = false;
	for (map<int,SubPatchDevice*>::const_iterator b = m_Boxes.begin(); b != m_Boxes.end(); ++b)
	{
		b->second->ClearGraph();
		m_Owned = m_Owned || Owns(b->first);
	}
	if (!m_Owned)
	{
		UnlockGraph();
		return;
	}

	GraphSort root;
	map<int,GraphSort*> graphs;
	graphs[-1] = &root;
	for (map<int,SubPatchDevice*>::const_iterator b = m_Boxes.begin(); b != m_Boxes.end(); ++b)
		if (Owns(b->first))
			graphs[b->first] = &b->second->Graph();

	map<int,bool> terminal;
	for (int pass = 0; pass < 2; ++pass)
		for (vector<CanvasWire>::const_iterator w = m_WireVec.begin(); w != m_WireVec.end(); ++w)
			for (map<int,GraphSort*>::iterator g = graphs.begin(); g != graphs.end(); ++g)
			{
				const int source = Scheduled(w->OutputID, g->first);
				const int dest = Scheduled(w->InputID, g->first);
				if (pass == 0)
				{
					if (source != -1) terminal[source] = terminal[source] || w->OutputTerminal;
					if (dest != -1) terminal[dest] = terminal[dest] || w->InputTerminal;
				}
				else if (source == -1 && dest == -1)
					continue;
				else if (source == -1 || dest == -1)
					g->second->AddNode(source == -1 ? dest : source, terminal[source == -1 ? dest : source]);
				else if (source != dest || source == w->OutputID)
					g->second->AddConnection(source, terminal[source], dest, terminal[dest]);
				else
					g->second->AddNode(source, terminal[source]);
			}

	const int next = 1 - m_Order;
	m_Orders[next] = root.GetSortedList();
	m_Order = next;
	UnlockGraph();
}

void Fl_Canvas::cb_ParentPage(Fl_Widget *, void *data)
{
	Fl_Canvas *canvas = (Fl_Canvas *)data;
	if (canvas->m_Page != -1)
		canvas->ShowPage(canvas->PageOf(canvas->m_Page));
}

void Fl_Canvas::ResetView()
{
	Fl_Scroll *scroll = dynamic_cast<Fl_Scroll*>(parent());

	if (!scroll) return;

	const int DesiredX = scroll->x() - w() / 2;
	const int DesiredY = scroll->y() - h() / 2;

	scroll->scroll_to(scroll->xposition() + x() - DesiredX,
	                  scroll->yposition() + y() - DesiredY);
}

void Fl_Canvas::CenterPatch()
{
	Fl_Scroll *scroll = dynamic_cast<Fl_Scroll*>(parent());

	if (!scroll) return;

	int left = 0, top = 0, right = 0, bottom = 0;
	bool any = false;

	for (int n=0; n<children(); n++)
	{
		Fl_Widget *o = child(n);
		if (!o->visible()) continue;
		const int ox = o->x() - x();
		const int oy = o->y() - y();

		left = any ? std::min(left, ox) : ox;
		top = any ? std::min(top, oy) : oy;
		right = any ? std::max(right, ox + o->w()) : ox + o->w();
		bottom = any ? std::max(bottom, oy + o->h()) : oy + o->h();
		any = true;
	}

	if (!any) return;

	CenterView(left, top, right, bottom);
}

void Fl_Canvas::CenterOn(int id)
{
	Fl_DeviceGUI *o = FindDevice(id);

	if (!o) return;

	CenterView(o->x() - x(), o->y() - y(), o->x() - x() + o->w(), o->y() - y() + o->h());
}

void Fl_Canvas::CenterView(int left, int top, int right, int bottom)
{
	Fl_Scroll *scroll = dynamic_cast<Fl_Scroll*>(parent());

	if (!scroll) return;

	const int PageW = scroll->w() - scroll->scrollbar.w();
	const int PageH = scroll->h() - scroll->hscrollbar.h();

	int TargetX = (left + right) / 2 - PageW / 2;
	int TargetY = (top + bottom) / 2 - PageH / 2;

	TargetX = std::max(0, std::min(TargetX, w() - PageW));
	TargetY = std::max(0, std::min(TargetY, h() - PageH));

	const int DesiredX = scroll->x() - TargetX;
	const int DesiredY = scroll->y() - TargetY;

	scroll->scroll_to(scroll->xposition() + x() - DesiredX,
	                  scroll->yposition() + y() - DesiredY);
}


////////////////////////////////////////////////////////////////////////

void Fl_Canvas::Rename(Fl_DeviceGUI* Device)
{
	if (cb_Rename) cb_Rename(this,Device);
}

////////////////////////////////////////////////////////////////////////

Fl_DeviceGUI *Fl_Canvas::FindDevice(int ID)
{
	for(int n=0; n<children(); n++)
	{
		if(((Fl_DeviceGUI*)child(n))->GetID()==ID)
		{
			return (Fl_DeviceGUI*)child(n);
		}
	}
	return NULL;
}

/////////////////////////////////////////////////////////////////////////

void Fl_Canvas::ToTop(Fl_DeviceGUI *o)
{
	if (children()<2) return; //no need to do anything

	// cast away the const :P
	Fl_Widget** a=(Fl_Widget**)array();
	int p=find(o);

	if (p<0)
	{
		cerr<<"ToTop couldn't find widget!"<<endl;
		return;
	}

	Fl_Widget *temp=a[0];
	Fl_Widget *last=a[0];

	for(int n=1; n<children(); n++)
	{
		if (n>p) // after the widget in the list
		{
			// move the widgets up
			a[n-1]=a[n];
		}
	}

	a[children()-1]=o; // put the raised one at the top of the list
}

void Fl_Canvas::ToBot(Fl_DeviceGUI *o)
{
	if (children()<2) return; //no need to do anything

	// cast away the const :P
	Fl_Widget** a=(Fl_Widget**)array();
	int p=find(o);

	if (p<0)
	{
		cerr<<"ToBot couldn't find widget!"<<endl;
		return;
	}

	Fl_Widget *temp=a[0];
	Fl_Widget *last=a[0];

	for(int n=1; n<children(); n++)
	{
		if (n<=p) // before the widget in the list
		{
			// move the widgets down
			temp=a[n];
			a[n]=last;
			last=temp;
		}
	}

	a[0]=o; // put the lowered one at the top of the list
}

/////////////////////////////////////////////////////////////////////////

void Fl_Canvas::StreamWiresIn(istream &s, bool merge)
{
	int NumWires;

	s>>NumWires;

	// my bad, didn't version this stream - remove one day...
	if (NumWires==-1)
	{
		int version;

		s>>version;
		s>>NumWires;

		for(int n=0; n<NumWires; n++)
		{
			CanvasWire NewWire;
			int dummy;

			s>>NewWire.OutputID;
			s>>dummy;
			s>>NewWire.OutputPort;
			s>>NewWire.OutputTerminal;
			s>>NewWire.InputID;
			s>>dummy;
			s>>NewWire.InputPort;
			s>>NewWire.InputTerminal;

			if (merge)
			{
				std::map<int, int>::iterator inputID = MapNewDeviceIds.find( NewWire.InputID);
				std::map<int, int>::iterator outputID = MapNewDeviceIds.find(NewWire.OutputID);

				if ((inputID != MapNewDeviceIds.end()) && (outputID != MapNewDeviceIds.end()))
				{
					NewWire.InputID = inputID->second;
					NewWire.OutputID = outputID->second;
				}
			}
			// A missing end, or a port the stand-in does not have, must not
			// drop the rest of the wires.
			Fl_DeviceGUI *source = FindDevice(NewWire.OutputID);
			Fl_DeviceGUI *destination = FindDevice(NewWire.InputID);
			if (!source || !destination
				|| NewWire.OutputPort < 0 || NewWire.OutputPort >= source->GetInfo()->NumOutputs
				|| NewWire.InputPort < 0 || NewWire.InputPort >= destination->GetInfo()->NumInputs)
			{
				std::cerr << "SSM: Skipping unavailable wire "
					<< NewWire.OutputID << ":" << NewWire.OutputPort << " -> "
					<< NewWire.InputID << ":" << NewWire.InputPort << ".\n";
			}
			else if (source->AddConnection(NewWire.OutputPort + source->GetInfo()->NumInputs) &&
				destination->AddConnection(NewWire.InputPort))
			{
				m_WireVec.push_back(NewWire);
					// Notify connection by callback
				cb_Connection(this,(void*)&NewWire);
				LockGraph();
				m_Graph.AddConnection(NewWire.OutputID,NewWire.OutputTerminal,NewWire.InputID,NewWire.InputTerminal);
				UnlockGraph();
				Reschedule();
			}
		}
	}
	else
	{
		for(int n=0; n<NumWires; n++)
		{
			CanvasWire NewWire;
			int dummy;

			s>>NewWire.OutputID;
			s>>dummy;
			s>>NewWire.OutputPort;
			s>>NewWire.InputID;
			s>>dummy;
			s>>NewWire.InputPort;

			if (merge)
			{
				std::map<int, int>::iterator inputID = MapNewDeviceIds.find( NewWire.InputID);
				std::map<int, int>::iterator outputID = MapNewDeviceIds.find(NewWire.OutputID);

				if ((inputID != MapNewDeviceIds.end()) && (outputID != MapNewDeviceIds.end()))
				{
					NewWire.InputID = inputID->second;
					NewWire.OutputID = outputID->second;
				}
			}

			Fl_DeviceGUI *source = FindDevice(NewWire.OutputID);
			Fl_DeviceGUI *destination = FindDevice(NewWire.InputID);
			if (!source || !destination
				|| NewWire.OutputPort < 0 || NewWire.OutputPort >= source->GetInfo()->NumOutputs
				|| NewWire.InputPort < 0 || NewWire.InputPort >= destination->GetInfo()->NumInputs)
			{
				std::cerr << "SSM: Skipping unavailable wire "
					<< NewWire.OutputID << ":" << NewWire.OutputPort << " -> "
					<< NewWire.InputID << ":" << NewWire.InputPort << ".\n";
			}
			else if (source->AddConnection(NewWire.OutputPort + source->GetInfo()->NumInputs) &&
				destination->AddConnection(NewWire.InputPort))
			{
				m_WireVec.push_back(NewWire);

				// Notify connection by callback
				cb_Connection(this,(void*)&NewWire);
				LockGraph();
				m_Graph.AddConnection(NewWire.OutputID,false,NewWire.InputID,false);
				UnlockGraph();
				Reschedule();
			}
		}
	}
}


void Fl_Canvas::cb_DeleteDeviceGroup (Fl_Widget* widget, void* data) {
     ((Fl_Canvas *)data)->cb_DeleteDeviceGroup_i();
}

inline void Fl_Canvas::cb_DeleteDeviceGroup_i() {
       DeleteSelection ();
}

void Fl_Canvas::cb_AddDeviceFromMenu (Fl_Widget* widget, void* data) {
     ((Fl_Canvas *)widget->parent())->cb_AddDeviceFromMenu_i (widget, data);
}

inline void Fl_Canvas::cb_AddDeviceFromMenu_i (Fl_Widget* widget, void* data) {
       if (cb_AddDevice) {
          int args[3];
          args[0]=*(int*)data;
          args[1]=m_x;
          args[2]=m_y;
          cb_AddDevice (this, args);
       }
}

////////////////////////////////////////////////////////////////////////

istream &operator>>(istream &s, Fl_Canvas &o)
{
	o.StreamWiresIn(s, false);
	return s;
}

Spiral::Core::Description &Describe(Spiral::Core::Description &d, Fl_Canvas &o)
{
	int version=0;
	d.Value(-1).Separator(" ").Value(version).Separator(" ");

	d.Value(o.m_WireVec.size()).Line();

	for(vector<CanvasWire>::iterator i=o.m_WireVec.begin();
		i!=o.m_WireVec.end(); i++)
	{
		d.Value(i->OutputID).Separator(" ");
		d.Value(0).Separator(" ");
		d.Value(i->OutputPort).Separator(" ");
		d.Value(i->OutputTerminal).Separator(" ");
		d.Value(i->InputID).Separator(" ");
		d.Value(0).Separator(" ");
		d.Value(i->InputPort).Separator(" ");
		d.Value(i->InputTerminal).Line();
	}

	return d;
}

void Describe(Spiral::Core::PatchDescription &patch, Fl_Canvas &o, int page)
{
	char text[32];
	vector<CanvasWire> wires;
	o.WiresOnPage(page, wires);

	for (vector<CanvasWire>::iterator i = wires.begin(); i != wires.end(); i++)
	{
		Spiral::Core::WireDescription &wire = patch.AddWire();
		snprintf(text, sizeof text, "%d", i->OutputID);   wire.FromDevice = text;
		snprintf(text, sizeof text, "%d", i->OutputPort); wire.FromPort = text;
		snprintf(text, sizeof text, "%d", i->InputID);    wire.ToDevice = text;
		snprintf(text, sizeof text, "%d", i->InputPort);  wire.ToPort = text;
	}
}

void Apply(const Spiral::Core::PatchDescription &patch, Fl_Canvas &o, const std::map<std::string, int> &ids)
{
	for (size_t n = 0; n < patch.Wires.size(); n++)
	{
		const Spiral::Core::WireDescription &wire = patch.Wires[n];
		std::map<std::string, int>::const_iterator from = ids.find(wire.FromDevice), to = ids.find(wire.ToDevice);
		CanvasWire NewWire;
		NewWire.OutputID = from == ids.end() ? -1 : from->second;
		NewWire.OutputPort = atoi(wire.FromPort.c_str());
		NewWire.InputID = to == ids.end() ? -1 : to->second;
		NewWire.InputPort = atoi(wire.ToPort.c_str());
		if ((o.IsBox(NewWire.OutputID) || o.IsBox(NewWire.InputID)) && !o.ResolveWire(NewWire))
		{
			std::cerr << "SSM: Skipping a wire whose subpatch port leads nowhere.\n";
			continue;
		}

		Fl_DeviceGUI *source = o.FindDevice(NewWire.OutputID);
		Fl_DeviceGUI *destination = o.FindDevice(NewWire.InputID);
		if (!source || !destination
			|| NewWire.OutputPort < 0 || NewWire.OutputPort >= source->GetInfo()->NumOutputs
			|| NewWire.InputPort < 0 || NewWire.InputPort >= destination->GetInfo()->NumInputs)
		{
			std::cerr << "SSM: Skipping unavailable wire "
				<< NewWire.OutputID << ":" << NewWire.OutputPort << " -> "
				<< NewWire.InputID << ":" << NewWire.InputPort << ".\n";
			continue;
		}
		NewWire.OutputTerminal = source->IsTerminal();
		NewWire.InputTerminal = destination->IsTerminal();
		if (source->AddConnection(NewWire.OutputPort + source->GetInfo()->NumInputs) &&
			destination->AddConnection(NewWire.InputPort))
		{
			o.m_WireVec.push_back(NewWire);
			o.cb_Connection(&o, (void*)&NewWire);
			o.LockGraph();
			o.m_Graph.AddConnection(NewWire.OutputID, NewWire.OutputTerminal, NewWire.InputID, NewWire.InputTerminal);
			o.UnlockGraph();
			o.Reschedule();
		}
	}
}

ostream &operator<<(ostream &s, Fl_Canvas &o)
{
	Spiral::Core::Description d;
	Describe(d, o);
	d.Write(s);
	return s;
}
