// SPDX-License-Identifier: GPL-2.0-or-later
// A positional .ssm read under the SpiralPositionalText contract, into the
// same token form a Describe produces: every value with the gap before it.
#ifndef SPIRALCORE_POSITIONAL_READER_H
#define SPIRALCORE_POSITIONAL_READER_H

#include "Description.h"
#include <cstddef>
#include <string>
#include <vector>

namespace Spumoni
{
	class JSON;
}

namespace spiralcore
{
	// The contract (schemas/SpiralPositionalText-0.2.x.json) names the
	// fields each plugin wrote, in stream order, and the history catalogue
	// (SpiralPositionalText.history.json) adds the layouts later CVS commits
	// introduced. Walking a file under them consumes it token by token, so
	// the reader knows where every device's state ends -- the one thing the
	// plugins' StreamIn methods never could tell the host.
	//
	// The result is one Description for the whole file (replaying it is
	// byte-identical to what was decoded) and, per device, the index range
	// its record and its plugin state occupy in that description.
	class PositionalReader
	{
	public:
		struct Span
		{
			size_t begin, end;   // token indexes, end exclusive
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
			long index;          // wire index for UnusableWire, else -1
		};

		// Both trees stay owned by the caller for the reader's lifetime.
		PositionalReader(const Spumoni::JSON &contract, const Spumoni::JSON *history);

		// False only for an unusable contract; a damaged file still returns
		// true with Status() Recovered, Partial or Unreadable.
		bool Read(const std::string &text, Description &out, std::string &error);

		// One device's state alone: decoded under the contract from text at
		// `at` (the first byte after the device's header), out gets the
		// state's values and the gaps among them, consumed the bytes they
		// took. For a host that still reads the file as a stream: it hands
		// each device exactly its own state and carries on after it,
		// whether or not the device's module is there to read it.
		bool ReadState(long pluginID, const std::string &text, size_t at,
		               Description &out, size_t &consumed, std::string &error);

		const std::string &Status() const { return m_Status; }
		const std::vector<Device> &Devices() const { return m_Devices; }
		const std::vector<Diagnostic> &Diagnostics() const { return m_Diagnostics; }
		size_t Remainder() const { return m_Remainder; }   // first undecoded byte

	private:
		const Spumoni::JSON &m_Contract;
		const Spumoni::JSON *m_History;
		std::string m_Status;
		std::vector<Device> m_Devices;
		std::vector<Diagnostic> m_Diagnostics;
		size_t m_Remainder;
	};
}

#endif
