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

#include <FL/Fl_Group.h>
#include <FL/Fl_Output.h>
#include <FL/Fl_Image.h>
#include <FL/Fl_Menu_Button.h>
#include <vector>
#include <string>
#include "../../GraphSort.h"
#include "Fl_DeviceGUI.h"

#ifndef CANVAS_WIDGET
#define CANVAS_WIDGET

using namespace std;

class Fl_DeviceGUI;

class CanvasWire
{
public:
	CanvasWire() { Clear(); }

	void Clear()
	{
		OutputPort=-1;
		OutputID=-1;
		OutputTerminal=false;
		InputPort=-1;
		InputID=-1;
		InputTerminal=false;
		DelMe=false;
	}

	int OutputID;
	int OutputPort;
	bool OutputTerminal;
	int InputID;
	int InputPort;
	bool InputTerminal;
	bool DelMe;
};

class CanvasGroup
{
public:
	CanvasGroup() { Clear(); }

	void Clear()
	{
		m_DeviceIds.clear();
	}

	vector<int> m_DeviceIds;
};

class Fl_Canvas : public Fl_Group
{
public:
	Fl_Canvas(int x, int y, int w, int h, char *name);
	~Fl_Canvas();
	Fl_Menu_Button *m_Menu;

	virtual void draw();
	virtual int handle(int event);
	int globalhandle(int event);

	void PortClicked(Fl_DeviceGUI* Device, int Type, int Port, bool Value);
	void Rename(Fl_DeviceGUI* Device);

	void SetConnectionCallback(Fl_Callback* s) { cb_Connection=s; }
	void SetUnconnectCallback(Fl_Callback* s) { cb_Unconnect=s; }
	void SetAddDeviceCallback(Fl_Callback* s) { cb_AddDevice=s; }
	void SetRenameCallback(Fl_Callback* s) { cb_Rename=s; }
	void SetCutDeviceGroupCallback(Fl_Callback* s) { cb_CutDeviceGroup=s; }
	void SetCopyDeviceGroupCallback(Fl_Callback* s) { cb_CopyDeviceGroup=s; }
	void SetPasteDeviceGroupCallback(Fl_Callback* s) { cb_PasteDeviceGroup=s; }
	void SetMergePatchCallback (Fl_Callback* s) { cb_MergePatch=s; }

        void DeleteSelection (void);
	void ClearConnections(Fl_DeviceGUI* Device);
	void RemoveDevice(Fl_DeviceGUI* Device);
	void Clear();
        void AddPluginName(const string &s, int ID);

        GraphSort* GetGraph() { return &m_Graph; }

	void Poll();

	void ToTop(Fl_DeviceGUI *o);
	void ToBot(Fl_DeviceGUI *o);

	void StreamSelectionWiresIn(istream &s, map<int,int> NewDeviceIds, bool merge, bool paste);
	void StreamSelectionWiresOut(ostream &s);

	void StreamWiresIn(istream &s, bool merge, bool paste);

	static void AppendSelection(int DeviceId, Fl_Canvas *data)
	{
		Fl_DeviceGUI *o =  data->FindDevice(DeviceId);
		if (o)
		{
			data->m_HaveSelection = true;
			data->m_Selection.m_DeviceIds.push_back(DeviceId);
			o->SetOnDragCallback(Fl_Canvas::cb_OnDrag_s, data);
			data->redraw();
		}
	}


	static void ClearSelection(Fl_Canvas *data)
	{
		data->m_HaveSelection=false;
		data->m_Selection.Clear();
		data->redraw();
	}

	static void EnablePaste(Fl_Canvas *data) { data->m_CanPaste=true; }

	bool HaveSelection() {return m_HaveSelection; }
	CanvasGroup Selection() { return m_Selection; }

	static void SetDeviceCallbacks(Fl_DeviceGUI *device, Fl_Canvas *data)
	{
		device->SetOnDragCallback(Fl_Canvas::cb_OnDrag_s, data);
		device->SetOnClickCallback(Fl_Canvas::cb_OnDragClick_s, data);
	}

private:
	void PopupEditMenu (Fl_Group *group);
	void DrawSelection();
	void CalculateSelection();
	void DrawWires();
	void ClearIncompleteWire();
	void DrawIncompleteWire();
	bool UserMakingConnection();
	Fl_DeviceGUI *FindDevice(int ID);
	Fl_Image *m_BG;
	char *m_BGData;
	void (*cb_Connection)(Fl_Widget*, void*);
	void (*cb_Unconnect)(Fl_Widget*, void*);
	void (*cb_AddDevice)(Fl_Widget*, void*);
	void (*cb_Rename)(Fl_Widget*, void*);
	void (*cb_CutDeviceGroup)(Fl_Widget*, void*);
	void (*cb_CopyDeviceGroup)(Fl_Widget*, void*);
	void (*cb_PasteDeviceGroup)(Fl_Widget*, void*);
	void (*cb_MergePatch)(Fl_Widget*, void*);
        map<int,int> MapNewDeviceIds;
	vector<CanvasWire> m_WireVec;
	CanvasWire m_IncompleteWire;
	GraphSort m_Graph;
	CanvasGroup m_Selection;
	bool m_CanPaste, m_HaveSelection, m_Selecting;
	int m_x, m_y, m_UpdateTimer, m_DragX, m_DragY;
	int m_StartSelectX, m_StartSelectY, m_EndSelectX,m_EndSelectY;
	friend istream &operator>>(istream &s, Fl_Canvas &o);
	friend ostream &operator<<(ostream &s, Fl_Canvas &o);
	// Callbacks
        static void cb_OnDrag_s (Fl_Widget* widget, int x, int y, void* data);
        inline void cb_OnDrag_i (Fl_Widget* widget, int x,int y);
        static void cb_OnDragClick_s (Fl_Widget* widget, int button, int shift_state, void* data);
        inline void cb_OnDragClick_i(Fl_Widget* widget, int button,int shift_state);
        static void cb_DeleteDeviceGroup (Fl_Widget* widget, void* data);
        inline void cb_DeleteDeviceGroup_i();
        static void cb_AddDeviceFromMenu (Fl_Widget* widget, void* data);
        inline void cb_AddDeviceFromMenu_i (Fl_Widget* widget, void* data);
};

istream &operator>>(istream &s, Fl_Canvas &o);
ostream &operator<<(ostream &s, Fl_Canvas &o);

#endif
