// The boundary drawer: what a patch says about itself, and -- when the
// subpatch work arrives here -- what a subpatch exposes. GPL-2.0-or-later.
//
// Chrome, like the menu strip and the toolbar: it sits OUTSIDE the canvas
// scroll, so it cannot ride the canvas scroll, join canvas hit-testing, or
// land in the canvas invalidation path. The host lays it out beside the
// scroll and gives the scroll what is left.
//
// This is the private tree's Fl_BoundaryDrawer with the parts this tree has
// no model for left out: one document for the project, no scopes to inherit
// from, no boundary ports. The shape is kept so the two meet without a
// translation step when those arrive.
//
// Colours come from the host: color() for the panel, selection_color() for
// the rail, which is drawn like the toolbar's plastic buttons. The widget
// knows nothing about where the host keeps its palette.
#ifndef SSM_FL_BOUNDARYDRAWER_H
#define SSM_FL_BOUNDARYDRAWER_H
#include <FL/Fl_Group.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Menu_Button.H>
#include <FL/Fl_Check_Button.H>
#include "PatchProject.h"
#include <string>
#include <vector>

class Fl_BoundaryDrawer: public Fl_Group
{
public:
	// The document fields the panel edits one per row. Credits are a list
	// and are not one of these: they get their own section.
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

	/* Point the panel at a document. The panel edits it in place and
	   fires DocumentChanged after each edit; the host decides when that
	   reaches a file. NULL unbinds: the panel goes blank. */
	void Bind(Spiral::File::DocumentSection *document);
	bool Bound() const;
	bool ShowsDocument() const;

	/* A vertical drag bar, like a GTK paned: drag the rail to set the
	   panel width. The width is remembered for this process only, not
	   written anywhere, so a new launch starts collapsed again.
	   PanelShown is the document section actually on screen, which a
	   collapsed drawer is not. */
	bool Collapsed() const;
	void SetCollapsed(bool collapsed);
	bool PanelShown() const;
	int RailWidth() const;
	int ExpandedWidth() const;
	int CurrentWidth() const;

	void (*CollapseChanged)(void *);
	void *CollapseChangedData;
	void (*DocumentChanged)(void *);
	void *DocumentChangedData;

	/* What the panel displays for a field. Empty when unbound. */
	std::string FieldText(Field field) const;

	/* Licence is a dropdown of known identifiers plus whatever is already
	   stored. Bundle asks a save to drop the full text into the SSMP.
	   False when this identifier has no text to inline. */
	bool CanBundleLicense() const;
	bool BundleLicenseText() const;
	bool SetBundleLicenseText(bool on);

	/* Write the field on the bound document. False when unbound. An
	   empty value clears the field. */
	bool Apply(Field field, const std::string &value);

	/* Credits: one row per entry, name and role. */
	size_t CreditRows() const;
	bool SetCredit(size_t row, const std::string &name, const std::string &role);
	bool AddCredit();
	bool RemoveCredit(size_t row);

	/* Re-read the bound document after it changed under the panel. */
	void Refresh();

private:
	Spiral::File::DocumentSection *m_Document;

	bool m_Collapsed;
	bool m_SplitDrag;
	int m_SplitX;
	int m_SplitW;
	Fl_Group *m_DocumentSection;
	Fl_Group *m_CreditSection;
	std::vector<Fl_Input *> m_Fields;
	/* Not an Fl_Input_Choice: FLTK 1.3's draws its own FL_UP_BOX and
	   FL_DOWN_BOX whatever it is told, so it cannot be dressed like the
	   rest of the panel. An input and a menu button side by side can. */
	Fl_Input *m_License;
	Fl_Menu_Button *m_LicenseMenu;
	Fl_Check_Button *m_Bundle;
	Fl_Box *m_Dates;
	Fl_Box *m_CreditHeader;
	std::vector<Fl_Input *> m_CreditNames;
	std::vector<Fl_Input *> m_CreditRoles;
	std::vector<Fl_Button *> m_CreditRemove;
	Fl_Button *m_AddCredit;

	static void FieldEdited(Fl_Widget *, void *);
	static void LicenseEdited(Fl_Widget *, void *);
	static void LicensePicked(Fl_Widget *, void *);
	static void BundleToggled(Fl_Widget *, void *);
	static void CreditEdited(Fl_Widget *, void *);
	static void AddClicked(Fl_Widget *, void *);
	static void RemoveClicked(Fl_Widget *, void *);
	static void RemoveCommitted(void *);

	void Changed();
	void Relayout();
	void SetPanelWidth(int panel);
	void Repopulate();
	void PlaceCredits();
	void NoteLayout();

	size_t m_RemoveRow;
};

#endif
