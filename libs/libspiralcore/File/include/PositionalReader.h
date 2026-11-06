// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef SPIRALCORE_POSITIONAL_READER_H
#define SPIRALCORE_POSITIONAL_READER_H

#include "Description.h"
#include <cstddef>
#include <string>
#include <vector>

#ifdef Status
#pragma push_macro("Status")
#undef Status
#define SPIRALCORE_RESTORE_POSITIONAL_STATUS
#endif

namespace Spumoni
{
	class JSON;
}

namespace Spiral
{
namespace Core
{
	class PositionalReader
	{
	public:
		struct Span
		{
			size_t begin, end;
		};

		struct Device
		{
			long id, pluginID;
			Span record, state;
		};

		struct Diagnostic
		{
			std::string code, field, message;
			size_t offset;
			long index;
		};

		PositionalReader(const Spumoni::JSON &contract, const Spumoni::JSON *history);

		bool Read(const std::string &text, Description &out, std::string &error);

		bool ReadState(long pluginID, const std::string &text, size_t at,
		               Description &out, size_t &consumed, std::string &error);

		long LoadAs(long pluginID) const;

		const std::string &Status() const { return m_Status; }
		const std::vector<Device> &Devices() const { return m_Devices; }
		const std::vector<Diagnostic> &Diagnostics() const { return m_Diagnostics; }
		size_t Remainder() const { return m_Remainder; }

	private:
		const Spumoni::JSON &m_Contract;
		const Spumoni::JSON *m_History;
		std::string m_Status;
		std::vector<Device> m_Devices;
		std::vector<Diagnostic> m_Diagnostics;
		size_t m_Remainder;
	};
}
}

#ifdef SPIRALCORE_RESTORE_POSITIONAL_STATUS
#pragma pop_macro("Status")
#undef SPIRALCORE_RESTORE_POSITIONAL_STATUS
#endif

#endif
