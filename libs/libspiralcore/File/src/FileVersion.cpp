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
#include "FileVersion.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace Spiral
{
	namespace File
	{
		namespace Version
		{
			const char ProjectKind[] = "";
			const char PatchKind[] = "Patch";
			const char SchemaKind[] = "Schema";

			std::string Header(const char *kind, long version)
			{
				std::string out = "SpiralSynthModular ";
				if (kind && *kind)
				{
					out += kind;
					out += ' ';
				}

				char number[32];
				std::sprintf(number, "%ld", version);
				return out + "File Ver " + number;
			}

			std::string ProjectHeader()
			{
				return Header(ProjectKind, Project);
			}

			std::string PatchHeader()
			{
				return Header(PatchKind, Patch);
			}

			std::string SchemaHeader()
			{
				return Header(SchemaKind, Schema);
			}

			Status Check(const std::string &header, const char *kind, long current, long &found)
			{
				found = 0;
				std::string prefix = "SpiralSynthModular ";
				if (kind && *kind)
					prefix += std::string(kind) + " ";
				prefix += "File Ver ";

				if (header.compare(0, prefix.size(), prefix) != 0)
					return Foreign;

				const char *digits = header.c_str() + prefix.size();
				char *end = NULL;
				long n = std::strtol(digits, &end, 10);
				if (end == digits || n <= 0)
					return Foreign;

				while (*end == ' ')
					++end;
				if (*end)
					return Foreign;

				found = n;
				if (n == current)
					return Current;
				return n > current ? Newer : Older;
			}

			std::string Reason(Status status, const char *kind, long found, long current)
			{
				std::string what = kind && *kind ? std::string(kind) + " file" : std::string("project file");
				char buffer[200];
				switch (status)
				{
					case Current:
						return std::string();
					case Older:
						std::sprintf(buffer, "%s version %ld is older than %ld", what.c_str(), found, current);
						return buffer;
					case Newer:
						std::sprintf(buffer, "%s version %ld was written by a newer SpiralSynthModular (this build reads version %ld)",
							what.c_str(), found, current);
						return buffer;
					case Foreign:
						break;
				}

				return "not a SpiralSynthModular " + what;
			}
		}
	}
}
