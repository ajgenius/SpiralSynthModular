// The boundary drawer: what a patch says about itself, and -- when the
// subpatch work arrives here -- what a subpatch exposes. GPL-2.0-or-later.
#ifndef SSM_FL_BOUNDARYDRAWER_H
#define SSM_FL_BOUNDARYDRAWER_H
#include <FL/Fl_Group.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Menu_Button.H>
#include <FL/Fl_Text_Editor.H>
#include <FL/Fl_Check_Button.H>
#include <FL/Fl_Choice.H>
#include <FL/Fl_Spinner.H>
#include "PatchProject.h"
#include <string>
#include <vector>

class Fl_Canvas;
class SubPatchDevice;

class Fl_BoundaryDrawer: public Fl_Group
{
public:
	enum Field
	{
		FieldTitle,
		FieldDescription,
		FieldCopyright,
		FieldLicense
	};

	Fl_BoundaryDrawer(int, int, int, int);
	~Fl_BoundaryDrawer();

	void resize(int, int, int, int);
	void draw();
	int handle(int);

	void Bind(Spiral::File::DocumentSection *document);
	bool Bound() const;
	bool ShowsDocument() const;

	void BindPorts(Fl_Canvas *canvas);
	bool ShowsPorts() const;

	void BindMidi(Fl_Group *page);
	bool ShowsMidi() const;
	void ShowMidi(bool on);

	bool Collapsed() const;
	void SetCollapsed(bool collapsed);
	bool PanelShown() const;
	int RailWidth() const;
	int ExpandedWidth() const;
	int CurrentWidth() const;

	void (*CollapseChanged)(void *);
	void *CollapseChangedData;
	void (*AddEvents)(void *);
	void *AddEventsData;
	void (*DocumentChanged)(void *);
	void *DocumentChangedData;

	void SetPatchVoices(unsigned voices, unsigned plays);
	void (*PatchVoicesChanged)(unsigned voices, void *);
	void *PatchVoicesChangedData;

	std::string FieldText(Field field) const;

	bool CanBundleLicense() const;
	bool BundleLicenseText() const;
	bool SetBundleLicenseText(bool on);
	void ShowLicenseText();

	bool Apply(Field field, const std::string &value);

	size_t CreditRows() const;
	bool SetCredit(size_t row, const std::string &name, const std::string &role);
	bool AddCredit();
	bool RemoveCredit(size_t row);

	size_t PortRows() const;
	std::string PortName(size_t row) const;
	bool PortIsInput(size_t row) const;
	bool RenamePort(size_t row, const std::string &name, std::string &error, bool refresh = true);
	bool MatchConnectedName(size_t row, std::string &error);
	bool RemovePort(size_t row, std::string &error);
	bool AddPort(bool input, std::string &error);
	bool MovePort(size_t fromRow, size_t toRow, std::string &error);
	void DropPortRow(size_t fromRow, int y);

	void Refresh();

private:
	Spiral::File::DocumentSection *m_Document;
	Fl_Canvas *m_Canvas;

	bool m_Collapsed;
	bool m_SplitDrag;
	int m_SplitX;
	int m_SplitW;
	Fl_Group *m_DocumentSection;
	Fl_Group *m_CreditSection;
	std::vector<Fl_Input *> m_Fields;
	std::vector<Field> m_FieldKinds;
	Fl_Text_Editor *m_About;
	Fl_Text_Buffer *m_AboutBuffer;
	Fl_Input *m_License;
	Fl_Menu_Button *m_LicenseMenu;
	Fl_Check_Button *m_Bundle;
	Fl_Button *m_ViewLicense;
	Fl_Box *m_Dates;
	Fl_Spinner *m_PatchVoices;
	Fl_Box *m_PatchPlays;
	Fl_Box *m_CreditHeader;
	std::vector<Fl_Input *> m_CreditNames;
	std::vector<Fl_Input *> m_CreditRoles;
	std::vector<Fl_Button *> m_CreditRemove;
	Fl_Button *m_AddCredit;
	Fl_Group *m_PortSection;
	Fl_Choice *m_Mode;
	Fl_Spinner *m_Voices;
	std::vector<Fl_Box *> m_PortHeaders;
	std::vector<Fl_Button *> m_PortHandles;
	std::vector<Fl_Input *> m_PortNames;
	std::vector<Fl_Button *> m_PortMatch;
	std::vector<Fl_Button *> m_PortRemove;
	Fl_Button *m_AddInput;
	Fl_Button *m_AddEvents;
	Fl_Button *m_AddOutput;
	Fl_Group *m_MidiPage;
	Fl_Button *m_PatchTab;
	Fl_Button *m_MidiTab;
	bool m_MidiShown;

	static void FieldEdited(Fl_Widget *, void *);
	static void AboutEdited(Fl_Widget *, void *);
	static void LicenseEdited(Fl_Widget *, void *);
	static void LicensePicked(Fl_Widget *, void *);
	static void BundleToggled(Fl_Widget *, void *);
	static void ViewLicenseClicked(Fl_Widget *, void *);
	static void CloseLicense(Fl_Widget *, void *);
	static void CreditEdited(Fl_Widget *, void *);
	static void AddClicked(Fl_Widget *, void *);
	static void RemoveClicked(Fl_Widget *, void *);
	static void RemoveCommitted(void *);
	static void ModePicked(Fl_Widget *, void *);
	static void VoicesPicked(Fl_Widget *, void *);
	static void PatchVoicesPicked(Fl_Widget *, void *);
	static void NameEdited(Fl_Widget *, void *);
	static void MatchClicked(Fl_Widget *, void *);
	static void AddPortClicked(Fl_Widget *, void *);
	static void AddPortCommitted(void *);
	static void AddEventsClicked(Fl_Widget *, void *);
	static void AddEventsCommitted(void *);
	static void RemovePortClicked(Fl_Widget *, void *);
	static void RemovePortCommitted(void *);
	static void DragCommitted(void *);
	static void TabClicked(Fl_Widget *, void *);
	int ContentTop() const;

	SubPatchDevice *SubPatch() const;
	void ShowSections();
	void PlacePorts();
	int RowNear(size_t fromRow, int y) const;

	void Changed();
	void Relayout();
	void SetPanelWidth(int panel);
	void Repopulate();
	void PlaceCredits();
	void NoteLayout();

	size_t m_RemoveRow;
	size_t m_RemovePortRow;
	bool m_AddPortInput;
	size_t m_DragFrom;
	int m_DragY;
};

#endif
