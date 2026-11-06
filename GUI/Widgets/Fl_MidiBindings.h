// SPDX-License-Identifier: GPL-2.0-or-later
/*  SpiralSynthModular
 *  Copyright (C) 2026 ajgenius
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
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#ifndef SSM_FL_MIDIBINDINGS_H
#define SSM_FL_MIDIBINDINGS_H
#include <FL/Fl_Group.H>
#include <string>
#include <vector>

class Fl_Box;
class Fl_Button;
class Fl_Choice;
class Fl_Scroll;
class Fl_Spinner;
class Fl_Double_Window;

namespace Spiral
{
	namespace Core
	{
		class MidiMap;
	}
}

class Fl_MidiBindings: public Fl_Group
{
public:
	Fl_MidiBindings(int x, int y, int w, int h);
	~Fl_MidiBindings();

	void resize(int, int, int, int);
	void draw();

	void Bind(Spiral::Core::MidiMap *map);
	void Refresh();
	void EditBinding(const std::string &device, const std::string &control);
	bool Editing() const;
	void SetPicking(bool picking);

	std::string (*DeviceName)(const std::string &device, void *);
	void (*PickDevice)(void *);
	void (*PickControl)(const std::string &device, void *);
	void (*Learn)(void (*learned)(int channel, int controller, void *), void *learnData, void *);
	void (*SourcePicked)(const std::string &source, void *);
	void (*Changed)(void *);
	void *Data;

	size_t Rows() const { return m_Rows.size(); }
	std::string RowDevice(size_t row) const;
	std::string RowControl(size_t row) const;
	std::string RowText(size_t row) const;

private:
	struct Row
	{
		std::string Device, Control;
	};

	Spiral::Core::MidiMap *m_Map;
	Fl_Box *m_Status;
	std::string m_StatusText;
	Fl_Choice *m_Source;
	std::vector<std::string> m_Sources;
	Fl_Box *m_Heading;
	Fl_Scroll *m_List;
	Fl_Button *m_AddDevice;
	std::vector<Row> m_Rows;
	bool m_Picking;

	Fl_Double_Window *m_Window;
	Fl_Box *m_Title;
	Fl_Choice *m_Channel;
	Fl_Spinner *m_Controller;
	Fl_Button *m_LearnButton;
	Fl_Box *m_Heard;
	Fl_Button *m_Bind, *m_Unbind, *m_Cancel;
	std::string m_EditDevice, m_EditControl;
	bool m_Listening;

	void Relayout();
	void Repopulate();
	void ListSources();
	void ShowStatus();
	void BuildWindow();
	void Listen(bool on);
	void CloseWindow();
	std::string Label(const std::string &device) const;

	static void PollStatus(void *);
	static void AddDeviceClicked(Fl_Widget *, void *);
	static void SourceChosen(Fl_Widget *, void *);
	static void AddControlClicked(Fl_Widget *, void *);
	static void RemoveDeviceClicked(Fl_Widget *, void *);
	static void EditClicked(Fl_Widget *, void *);
	static void UnbindClicked(Fl_Widget *, void *);
	static void LearnClicked(Fl_Widget *, void *);
	static void Learned(int channel, int controller, void *);
	static void BindClicked(Fl_Widget *, void *);
	static void UnbindWindowClicked(Fl_Widget *, void *);
	static void CancelClicked(Fl_Widget *, void *);
};

#endif
