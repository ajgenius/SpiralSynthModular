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
#ifndef SPIRAL_FILE_VERSION_H
#define SPIRAL_FILE_VERSION_H

#include <string>

namespace Spiral
{
	namespace File
	{
		namespace Version
		{
			const long Project = 10;
			const long Patch = 3;
			const long Schema = 1;

			extern const char ProjectKind[];
			extern const char PatchKind[];
			extern const char SchemaKind[];

			std::string Header(const char *kind, long version);
			std::string ProjectHeader();
			std::string PatchHeader();
			std::string SchemaHeader();

			enum Status
			{
				Current,
				Older,
				Newer,
				Foreign
			};

			Status Check(const std::string &header, const char *kind, long current, long &found);

			std::string Reason(Status status, const char *kind, long found, long current);
		}
	}
}

#endif
