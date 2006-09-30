/*  SpiralLoops
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

#ifndef GUIBASE
#define GUIBASE

class GUIBase
{
	public:
	
	GUIBase();
	virtual ~GUIBase();	
	
	// todo: these should be pure virtual funcs, to force 
	// GUI's to instantiate them
	virtual void CreateGUI(int xoff, int yoff, char *name) {} // =0;
	virtual void UpdateValues() {} // =0;

	protected:
	
	private:

};

#endif
