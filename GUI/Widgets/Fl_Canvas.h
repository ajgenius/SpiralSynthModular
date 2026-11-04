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

#include <FL/Fl_Group.H>
#include <pthread.h>
#include <FL/Fl_Output.H>
#include <FL/Fl_Image.H>
#include <FL/Fl_Menu_Button.H>
#include <vector>
#include <string>
#include "GraphSort.h"
#include "Fl_DeviceGUI.h"
#include "Description.h"
#include "PatchDescription.h"
#include "SubPatchDevice.h"

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
	Fl_Canvas(int x, int y, int w, int h, const char *name);
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
	void SetPasteAsSubPatchCallback(Fl_Callback* s) { cb_PasteAsSubPatch=s; }
	void SetMergePatchCallback (Fl_Callback* s) { cb_MergePatch=s; }
	void SetAboutPatchCallback (Fl_Callback* s) { cb_AboutPatch=s; }
	void SetGroupCallback (Fl_Callback* s) { cb_GroupDevices=s; }
	void SetUngroupCallback (Fl_Callback* s) { cb_UngroupDevices=s; }
	void SetPageCallback (Fl_Callback* s) { cb_PageChanged=s; }
	// A file dropped on the patch: the callback runs once the drop is
	// over, and DroppedFile() is the first file dropped.
	void SetDropFileCallback (Fl_Callback* s) { cb_DropFile=s; }
	const string &DroppedFile() const { return m_DroppedFile; }

	int Page() const { return m_Page; }
	void ShowPage(int box);
	// The device menu, as the private tree's editor has it.
	void DeviceMenu(Fl_DeviceGUI *device);
	// A subpatch box's name, and the boxes on a page, for the breadcrumb.
	string BoxTitle(int id);
	vector<int> BoxesOn(int page) const;
	int PageOf(int id) const;
	void SetPageOf(int id, int page);
	bool IsBox(int id) const { return m_Boxes.count(id) != 0; }
	// The device a subpatch is hosted in holds its ports and settings; the
	// canvas looks it up by ID, and forgets it when it goes.
	SubPatchDevice *Box(int id);
	void SetBox(int id, SubPatchDevice *box) { m_Boxes[id] = box; Reschedule(); }
	void RemoveBox(int id);
	bool Resolve(int id, bool input, int port, int &device, int &devicePort) const;
	int Holder(int id, int page) const;
	int PortThrough(int holder, bool input, int device, int port) const;
	void WiresOnPage(int page, vector<CanvasWire> &out) const;
	const vector<CanvasWire> &Wires() const { return m_WireVec; }
	string Name(int id);
	string PortName(int id, bool input, int port);
	bool IsEventPort(int id, bool input, int port);
	void EraseWire(const CanvasWire &wire);
	void AddBoxPort(int box, bool input, const string &name, const string &type);
	bool BindBoxPort(int box, bool input, int port, int id, int idPort);
	void UnbindBoxPort(int box, bool input, int port);
	void AddBoxPort(int box, bool input, const string &name);
	void RemoveBoxPort(int box, bool input, int port);
	bool MoveBoxPort(int box, bool input, int from, int to);
	void RenameBoxPort(int box, bool input, int port, const string &name);
	void SetBoxMode(int box, const string &mode);
	void SetBoxVoices(int box, long voices);
	bool Owns(int box) const;
	unsigned Topology() const { return m_Topology; }
	// The last item on the popup, after a divider: the patch's own entry
	// below the device groups. Added once the plugins are in, so it
	// lands at the end rather than ahead of them.
	void AddAboutPatch ();

        void DeleteSelection (void);
	void ClearConnections(Fl_DeviceGUI* Device);
	void PruneConnections(Fl_DeviceGUI* Device, int inputs, int outputs);
	void MoveInputs(Fl_DeviceGUI* Device, const vector<int> &to);
	void RestorePortConnections(Fl_DeviceGUI* Device);
	void RemoveDevice(Fl_DeviceGUI* Device);
	void Clear();
	// Scroll the view back to where a fresh window starts.
	void ResetView();
	// Scroll the view so the patch sits in the middle of it.
	void CenterPatch();
	// Scroll the view so one device sits in the middle of it.
	void CenterOn(int id);

	/* Picking, as a page inspector picks: until it stops, the canvas takes
	   the pointer itself and a click names what is under it instead of
	   acting on it. The host says what that is -- a device, a control on
	   one -- and its bounds, which are outlined as the pointer passes.
	   Escape, or a click on nothing, picks Device -1, and either way the
	   pick has stopped before Picked runs, so Picked may start another. */
	struct Pick
	{
		Pick(): Device(-1), X(0), Y(0), W(0), H(0) {}
		int Device;
		string Control;
		int X, Y, W, H;
	};
	typedef bool (*PickAt)(Fl_Canvas *canvas, int x, int y, Pick &out, void *data);
	typedef void (*Picked)(Fl_Canvas *canvas, const Pick &pick, void *data);
	void StartPick(PickAt at, Picked picked, void *data, const string &hint);
	void StopPick();
	bool Picking() const { return m_PickAt != NULL; }
	// Only this device is drawn, the patch around it set aside; -1 draws all.
	void Isolate(int id);
	int Isolated() const { return m_Isolated; }
        void AddPluginName(const string &s, int ID);

        GraphSort* GetGraph() { return &m_Graph; }
	const list<int> &ExecutionOrder() { return m_Owned ? m_Orders[m_Order] : m_Graph.GetSortedList(); }

	void Poll();

	void ToTop(Fl_DeviceGUI *o);
	void ToBot(Fl_DeviceGUI *o);

	void StreamSelectionWiresIn(istream &s, map<int,int> NewDeviceIds, bool merge);

	void StreamWiresIn(istream &s, bool merge);

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
	bool CanPaste() {return m_CanPaste; }
	CanvasGroup Selection() { return m_Selection; }

	void SetGraphLock(pthread_mutex_t *lock) { m_GraphLock = lock; }

	static void SetDeviceCallbacks(Fl_DeviceGUI *device, Fl_Canvas *data)
	{
		device->SetOnDragCallback(Fl_Canvas::cb_OnDrag_s, data);
		device->SetOnClickCallback(Fl_Canvas::cb_OnDragClick_s, data);
	}

private:
	void PopupEditMenu (Fl_Group *group);
	void DrawSelection();
	void DrawPick();
	int PickEvent(int event);
	void CenterView(int left, int top, int right, int bottom);
	void CalculateSelection();
	void DrawWires();
	void ClearIncompleteWire();
	void RemoveWire(vector<CanvasWire>::iterator i);
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
	void (*cb_PasteAsSubPatch)(Fl_Widget*, void*);
	void (*cb_MergePatch)(Fl_Widget*, void*);
	void (*cb_AboutPatch)(Fl_Widget*, void*);
	void (*cb_GroupDevices)(Fl_Widget*, void*);
	void (*cb_UngroupDevices)(Fl_Widget*, void*);
	void (*cb_PageChanged)(Fl_Widget*, void*);
	void (*cb_DropFile)(Fl_Widget*, void*);
	static void cb_Dropped(void *data);
	bool m_Dropping;
	string m_DroppedFile;
	void DrawStrips();
	void StripPoint(bool input, int index, int &x, int &y) const;
	bool StripAt(int x, int y, bool &input, int &port);
	void StripClicked(bool input, int port, bool unbind);
	bool ResolveWire(CanvasWire &wire) const;
	void WiresThrough(int box, bool input, int port, vector<CanvasWire> &out) const;
	void AddWire(CanvasWire wire);
	void RefreshBox(int box);
	void RenumberBinds(int box, bool input, const vector<int> &moved);
	int Scheduled(int id, int scope) const;
	void Reschedule();
	PickAt m_PickAt;
	Picked m_Picked;
	void *m_PickData;
	string m_PickHint;
	bool m_PickHot;
	Pick m_Hot;
	int m_Isolated;
	int m_Page;
	bool m_Owned;
	list<int> m_Orders[2];
	int m_Order;
	volatile unsigned m_Topology;
	pthread_mutex_t *m_GraphLock;
	void LockGraph();
	void UnlockGraph();
	bool m_StripInput;
	int m_StripPort;
	map<int,int> m_PageOf;
	map<int,SubPatchDevice*> m_Boxes;
        map<int,int> MapNewDeviceIds;
	vector<CanvasWire> m_WireVec;
	CanvasWire m_IncompleteWire;
	GraphSort m_Graph;
	CanvasGroup m_Selection;
	bool m_CanPaste, m_HaveSelection, m_Selecting;
	int m_x, m_y, m_UpdateTimer, m_DragX, m_DragY;
	int m_StartSelectX, m_StartSelectY, m_EndSelectX,m_EndSelectY;
	friend istream &operator>>(istream &s, Fl_Canvas &o);
	friend Spiral::Core::Description &Describe(Spiral::Core::Description &d, Fl_Canvas &o);
	friend void Describe(Spiral::Core::PatchDescription &patch, Fl_Canvas &o, int page);
	friend void Apply(const Spiral::Core::PatchDescription &patch, Fl_Canvas &o, const std::map<std::string, int> &ids);
	// Callbacks
        static void cb_OnDrag_s (Fl_Widget* widget, int x, int y, void* data);
        inline void cb_OnDrag_i (Fl_Widget* widget, int x,int y);
        static void cb_OnDragClick_s (Fl_Widget* widget, int event, int button, int shift_state, void* data);
        inline void cb_OnDragClick_i(Fl_Widget* widget, int event, int button,int shift_state);
        static void cb_DeleteDeviceGroup (Fl_Widget* widget, void* data);
        inline void cb_DeleteDeviceGroup_i();
        static void cb_ParentPage (Fl_Widget* widget, void* data);
        static void cb_AddDeviceFromMenu (Fl_Widget* widget, void* data);
        inline void cb_AddDeviceFromMenu_i (Fl_Widget* widget, void* data);
};

istream &operator>>(istream &s, Fl_Canvas &o);
Spiral::Core::Description &Describe(Spiral::Core::Description &d, Fl_Canvas &o);
void Describe(Spiral::Core::PatchDescription &patch, Fl_Canvas &o, int page = -1);
void Apply(const Spiral::Core::PatchDescription &patch, Fl_Canvas &o, const std::map<std::string, int> &ids);
ostream &operator<<(ostream &s, Fl_Canvas &o);   // the description, written

#endif
