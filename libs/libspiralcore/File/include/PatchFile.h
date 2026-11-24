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
#ifndef SPIRAL_PATCH_FILE_H
#define SPIRAL_PATCH_FILE_H

#include "PatchDescription.h"
#include "Description.h"

#include <string>

namespace Spumoni { class JSON; }

namespace Spiral
{
	namespace File
	{
		class PatchFile
		{
		public:
			static bool Read(const Spumoni::JSON &document, Core::PatchDescription &out,
				std::string &error);
			static bool ReadText(const std::string &text, Core::PatchDescription &out,
				std::string &error);
			static bool ReadBody(const Spumoni::JSON &body, Core::PatchDescription &out,
				std::string &error);
			static bool Text(const Core::PatchDescription &definition, std::string &json,
				std::string &error);
			static bool WriteFile(const Core::PatchDescription &definition, const char *path,
				std::string &error);

			static std::string Schema(const Core::PatchDescription &definition);
			static void ApplySchema(const std::string &text, Core::PatchDescription &definition);

			static void Properties(const Core::Description &described, Core::DeviceDescription &out);
			static void Describe(const Core::DeviceDescription &device, Core::Description &out);
		private:
			PatchFile();
		};
	}
}

#endif
