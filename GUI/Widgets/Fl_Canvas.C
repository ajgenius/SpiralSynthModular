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

#include "FL/fl_draw.H"
#include <FL/Fl_Scroll.H>
#include "Fl_Canvas.h"
#include "Fl_DeviceGUI.h"
#include <iostream>
#include "../../SpiralSound/SpiralInfo.h"

// no of calls to handle when dragged, before the widget is redrawn
// to allow the wire (connection currently being made) to be redrawn
static const int UPDATE_TICKS = 5;

static int Numbers[512];

////////////////////////////////////////////////////////////////////////

Fl_Canvas::Fl_Canvas(int x, int y, int w, int h, char *name) :
Fl_Group(x,y,w,h,name),
m_Menu(NULL),
m_BG(NULL),
m_BGData(NULL),
cb_Connection(NULL),
cb_Unconnect(NULL),
cb_AddDevice(NULL),
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
			if (((Fl_DeviceGUI*)&o)->IsMinimised())
			{
   	 			draw_child(o);
   				draw_outside_label(o);

				std::vector<int>::iterator sel = std::find( m_Selection.m_DeviceIds.begin(), m_Selection.m_DeviceIds.end(), ((Fl_DeviceGUI*)&o)->GetID() );

				if (sel != m_Selection.m_DeviceIds.end())
				{
					fl_color(FL_YELLOW);
					fl_rect(o.x(), o.y(), o.w(), o.h());
				}
			}
		}

		DrawWires();

		// draw maximised modules on top of everything else
   	 	Fl_Widget*const* a = array();
		for (int i=children(); i--;)
		{
   			Fl_Widget& o = **a++;
			if (!((Fl_DeviceGUI*)&o)->IsMinimised())
			{
   	 			draw_child(o);
   				draw_outside_label(o);

				std::vector<int>::iterator sel = std::find( m_Selection.m_DeviceIds.begin(), m_Selection.m_DeviceIds.end(), ((Fl_DeviceGUI*)&o)->GetID() );

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
		for (int i=children(); i--;) update_child(**a++);
  	}
	DrawSelection();
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
		if (widget_intersects_rectangle(&o, X, Y, W, H))
		{
			m_HaveSelection = true;
			m_Selection.m_DeviceIds.push_back(((Fl_DeviceGUI*)&o)->GetID());
			((Fl_DeviceGUI*)&o)->SetOnDragCallback(cb_OnDrag_s, this);
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
     Fl_Menu_Item *merge=(Fl_Menu_Item*)&(m_Menu->menu()[4]);
     Fl_Menu_Item *deleteitems=(Fl_Menu_Item*)&(m_Menu->menu()[5]);
     if ((cb_CopyDeviceGroup) && (m_HaveSelection)) copy->activate(); else copy->deactivate();
     if ((cb_CutDeviceGroup) && (m_HaveSelection)) cut->activate(); else cut->deactivate();
     if ((cb_PasteDeviceGroup) && (m_CanPaste)) paste->activate(); else paste->deactivate();
     if (m_HaveSelection) deleteitems->activate(); else deleteitems->deactivate();
     m_Menu->popup();
     group->remove(m_Menu);
}

void Fl_Canvas::AddPluginName (const string &s, int ID) {
     // There's a bug here, that there's no menu at all if no plugins are found
     // This isn't IMMEDIATELY important, as if you've no plugins - there's nothing to copy/paste/etc anyway.
     if (! (m_Menu)) {
        m_Menu = new Fl_Menu_Button (0, 0, 4, 4, NULL);
        m_Menu->type (Fl_Menu_Button::POPUP123);
        m_Menu->textsize (10);
        m_Menu->add ("Edit/Cut Currently Selected Devices", 0, (Fl_Callback*)cb_CutDeviceGroup, user_data());
        m_Menu->add ("Edit/Copy Currently Selected Devices", 0, (Fl_Callback*)cb_CopyDeviceGroup, user_data());
        m_Menu->add ("Edit/Paste Previously Copied Devices", 0, (Fl_Callback*)cb_PasteDeviceGroup, user_data(), FL_MENU_DIVIDER);
        m_Menu->add ("Edit/Merge Existing Patch", 0, (Fl_Callback*)cb_MergePatch, user_data(), FL_MENU_DIVIDER);
        m_Menu->add ("Edit/Delete Currently Selected Devices", 0, (Fl_Callback*)cb_DeleteDeviceGroup, this);
     }
     m_Menu->add (s.c_str(), 0, (Fl_Callback*)cb_AddDeviceFromMenu, &Numbers[ID]);
}

void Fl_Canvas::StreamSelectionWiresIn(istream &s, std::map<int,int> NewDeviceIds, bool merge, bool paste)
{
	MapNewDeviceIds = NewDeviceIds;
	StreamWiresIn(s, merge, paste);
}

void Fl_Canvas::StreamSelectionWiresOut(ostream &s)
{
	int total_wires = 0, curpos=0;

	curpos = s.tellp();

	s<<-1<<endl;

	if (m_WireVec.size()>0)
		for(vector<CanvasWire>::iterator i=m_WireVec.begin();
			i!=m_WireVec.end(); i++)
		{
			std::vector<int>::iterator output = std::find( m_Selection.m_DeviceIds.begin(), m_Selection.m_DeviceIds.end(), i->OutputID );
			std::vector<int>::iterator input = std::find( m_Selection.m_DeviceIds.begin(), m_Selection.m_DeviceIds.end(), i->InputID );

			if ((input != m_Selection.m_DeviceIds.end()) && (output != m_Selection.m_DeviceIds.end()))
			{
				s<<i->OutputID<<" ";
				s<<0<<" ";
				s<<i->OutputPort<<" ";
				s<<i->OutputTerminal<<" ";
				s<<i->InputID<<" ";
				s<<0<<" ";
				s<<i->InputPort<<" ";
					s<<i->InputTerminal<<endl;

				total_wires += 1;
			}
		}

	if (total_wires >= 1)
	{
		s.seekp(curpos, ios::beg);
		s<<total_wires<<endl;
		s.seekp(0, ios::end);
	}
}

////////////////////////////////////////////////////////////////////////

void Fl_Canvas::DrawWires()
{
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

		fl_line(SourceDevice->GetPortX(i->OutputPort+SourceDevice->GetInfo()->NumInputs),
				SourceDevice->GetPortY(i->OutputPort+SourceDevice->GetInfo()->NumInputs),
				DestDevice->GetPortX(i->InputPort),
				DestDevice->GetPortY(i->InputPort));
	}

	DrawIncompleteWire();
}

////////////////////////////////////////////////////////////////////////

bool Fl_Canvas::UserMakingConnection()
{
	return 	(m_IncompleteWire.InputID!=-1 || m_IncompleteWire.OutputID!=-1);
}

////////////////////////////////////////////////////////////////////////

void Fl_Canvas::DrawIncompleteWire()
{
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

int Fl_Canvas::handle (int event) {
    if (Fl_Group::handle (event)) return 1;
    if (event==FL_PUSH) {
       ClearIncompleteWire();
       redraw();
       m_DragX=Fl::event_x();
       m_DragY=Fl::event_y();
    }
    if ((Fl::event_button() == 1) && ((Fl::event_state() & FL_SHIFT) == 0) && ((Fl::event_state() & FL_CTRL) == 0)) {
       // Left-Click  (or plain click for those who are mouse-button challenged)
       // Select / Multi-select / move devices
       // Handled below - If on a non-selected plugin, deselect and move
       // Handled below - If on a selected plugin, move selection
       // Handled Here - If on canvas - multi select
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
    if ((Fl::event_button() == 2) || ((Fl::event_button() == 1) && ((Fl::event_state() & FL_SHIFT) != 0))) {
       // Middle-Click  (or shift-click for the mouse button challenged) - old left click
       // Handled Below - If on items allows selecting of individual items
       // Handled Here - If on canvas, drags canvas
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
          if (m_HaveSelection) {
             if (m_Selection.m_DeviceIds.size() <= 0)
                m_HaveSelection = false;
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

void Fl_Canvas::cb_OnDragClick_s (Fl_Widget* widget, int button, int shift_state, void* data) {
     ((Fl_Canvas *)data)->cb_OnDragClick_i (widget, button, shift_state);
}

inline void Fl_Canvas::cb_OnDragClick_i(Fl_Widget* widget, int button,int shift_state) {
       // this bit seems to be unnecessary - andy preston
       // if ((button==3) && ((shift_state & FL_CTRL) != 0)) {
       //   PopupEditMenu(widget->parent());
       // }
       if ((widget) && (button==1)) {
          int ID = ((Fl_DeviceGUI*)(widget->parent()))->GetID();
          std::vector<int>::iterator device_iter = std::find(m_Selection.m_DeviceIds.begin(), m_Selection.m_DeviceIds.end(), ID);
          if (((shift_state & FL_SHIFT) != 0) || ((shift_state & FL_CTRL) != 0)) {
             if (m_HaveSelection) {
                if (device_iter != m_Selection.m_DeviceIds.end())
                   m_Selection.m_DeviceIds.erase(device_iter);
                else
                   m_Selection.m_DeviceIds.push_back(ID);
             }
             else {
                m_Selection.Clear();
                m_HaveSelection = true;
                m_Selection.m_DeviceIds.push_back(ID);
             }
          }
          else {
             m_Selection.Clear();
             m_HaveSelection = true;
             m_Selection.m_DeviceIds.push_back(ID);
          }
          redraw();
       }
}

////////////////////////////////////////////////////////////////////////

void Fl_Canvas::PortClicked(Fl_DeviceGUI* Device, int Type, int Port, bool Value)
{
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
				m_WireVec.push_back(m_IncompleteWire);

				// send the connect callback
				cb_Connection(this,(void*)&m_IncompleteWire);
				m_Graph.AddConnection(m_IncompleteWire.OutputID,m_IncompleteWire.OutputTerminal,
									  m_IncompleteWire.InputID,m_IncompleteWire.InputTerminal);

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
		bool Found=true;

		while (Found)
		{
			Found=false;

			for(vector<CanvasWire>::iterator i=m_WireVec.begin();
				i!=m_WireVec.end(); i++)
			{
				if ((Type==Fl_DeviceGUI::OUTPUT && i->OutputID==Device->GetID() && i->OutputPort==Port) ||
					(Type==Fl_DeviceGUI::INPUT && i->InputID==Device->GetID() && i->InputPort==Port))
				{
					// Turn off both ports
					Fl_DeviceGUI* ODGUI = FindDevice(i->OutputID);
					ODGUI->RemoveConnection(i->OutputPort+ODGUI->GetInfo()->NumInputs);

					Fl_DeviceGUI* IDGUI = FindDevice(i->InputID);
					IDGUI->RemoveConnection(i->InputPort);

					// send the unconnect callback
					cb_Unconnect(this,(void*)&(*i));
					m_Graph.RemoveConnection(i->OutputID,i->InputID);

					// Remove the wire
					m_WireVec.erase(i);

					Found=true;
					break;
				}
			}
		}

		redraw();

		// Clear the current m_Selection
		m_IncompleteWire.Clear();
	}
}

////////////////////////////////////////////////////////////////////////

void Fl_Canvas::ClearConnections(Fl_DeviceGUI* Device)
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
			if (i->OutputID==Device->GetID() ||
			    i->InputID==Device->GetID())
			{
				// Turn off both ports
				FindDevice(i->OutputID)->RemoveConnection(i->OutputPort+FindDevice(i->OutputID)->GetInfo()->NumInputs);
				FindDevice(i->InputID)->RemoveConnection(i->InputPort);

				// send the unconnect callback
				cb_Unconnect(this,(void*)&(*i));
				m_Graph.RemoveConnection(i->OutputID,i->InputID);

				m_WireVec.erase(i);
				removedall=false;
				break;
			}
		}
	}
}

////////////////////////////////////////////////////////////////////////

void Fl_Canvas::RemoveDevice(Fl_DeviceGUI* Device)
{
	ClearConnections(Device);
	remove(Device);
	redraw();
}

////////////////////////////////////////////////////////////////////////

void Fl_Canvas::Clear()
{
	m_Graph.Clear();

	int kids=children();

	for(int n=0; n<kids; n++)
	{
		remove(child(0));
	}

	m_WireVec.clear();
	redraw();
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

void Fl_Canvas::StreamWiresIn(istream &s, bool merge, bool paste)
{
	int NumWires;

	s>>NumWires;

	// my bad, didn't version this stream - remove one day...
	if (paste || NumWires==-1)
	{
		int version;

		if (!paste)
		{
			s>>version;
			s>>NumWires;
		}

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

			if (paste || merge)
			{
				std::map<int, int>::iterator inputID = MapNewDeviceIds.find( NewWire.InputID);
				std::map<int, int>::iterator outputID = MapNewDeviceIds.find(NewWire.OutputID);

				if ((inputID != MapNewDeviceIds.end()) && (outputID != MapNewDeviceIds.end()))
				{
					NewWire.InputID = inputID->second;
					NewWire.OutputID = outputID->second;
				}
			}
			// if we can turn on both ports
			if (FindDevice(NewWire.OutputID)->AddConnection(NewWire.OutputPort+
					FindDevice(NewWire.OutputID)->GetInfo()->NumInputs) &&
				FindDevice(NewWire.InputID)->AddConnection(NewWire.InputPort))
			{
				m_WireVec.push_back(NewWire);
					// Notify connection by callback
				cb_Connection(this,(void*)&NewWire);
				m_Graph.AddConnection(NewWire.OutputID,NewWire.OutputTerminal,NewWire.InputID,NewWire.InputTerminal);
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

			if (paste || merge)
			{
				std::map<int, int>::iterator inputID = MapNewDeviceIds.find( NewWire.InputID);
				std::map<int, int>::iterator outputID = MapNewDeviceIds.find(NewWire.OutputID);

				if ((inputID != MapNewDeviceIds.end()) && (outputID != MapNewDeviceIds.end()))
				{
					NewWire.InputID = inputID->second;
					NewWire.OutputID = outputID->second;
				}
			}

			// if we can turn on both ports
			if (FindDevice(NewWire.OutputID)->AddConnection(NewWire.OutputPort+
					FindDevice(NewWire.OutputID)->GetInfo()->NumInputs) &&
				FindDevice(NewWire.InputID)->AddConnection(NewWire.InputPort))
			{
				m_WireVec.push_back(NewWire);

				// Notify connection by callback
				cb_Connection(this,(void*)&NewWire);
				m_Graph.AddConnection(NewWire.OutputID,false,NewWire.InputID,false);
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
	o.StreamWiresIn(s, false, false);
	return s;
}

ostream &operator<<(ostream &s, Fl_Canvas &o)
{
	int version=0;
	s<<-1<<" "<<version<<" ";

	s<<o.m_WireVec.size()<<endl;

	for(vector<CanvasWire>::iterator i=o.m_WireVec.begin();
		i!=o.m_WireVec.end(); i++)
	{
		s<<i->OutputID<<" ";
		s<<0<<" ";
		s<<i->OutputPort<<" ";
		s<<i->OutputTerminal<<" ";
		s<<i->InputID<<" ";
		s<<0<<" ";
		s<<i->InputPort<<" ";
		s<<i->InputTerminal<<endl;
	}

	return s;
}
