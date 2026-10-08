// SPDX-License-Identifier: GPL-2.0-or-later
// The contract reader consumes a positional patch into a Description whose
// replay is the file, and reports where each device's state sits in it.
#include "JSON.h"
#include "PositionalReader.h"
#include <cstdio>
#include <memory>
#include <sstream>
#include <string>

static int fails = 0;
#define CHECK(x) do { if (!(x)) { ++fails; printf("FAIL %s:%d %s\n", __FILE__, __LINE__, #x); } } while (0)

static std::string Replay(const spiralcore::Description &d)
{
	std::ostringstream s;
	d.Write(s);
	return s.str();
}

int main()
{
	std::string error;
	std::auto_ptr<Spumoni::JSON> contract(Spumoni::ParseJSON(SPIRALCORE_SCHEMA_DIR "/SpiralPositionalText-0.2.x.json", &error));
	std::auto_ptr<Spumoni::JSON> history(Spumoni::ParseJSON(SPIRALCORE_SCHEMA_DIR "/SpiralPositionalText.history.json", &error));
	CHECK(contract.get() && history.get());
	if (fails)
		return 1;

	spiralcore::PositionalReader reader(*contract, history.get());
	const std::string header = "SpiralSynthModular File Ver 4\n0 0 700 600 0 0 0 0\nSectionList\n";
	const std::string output = "Device 1 Plugin 0\n0 0 0  0 0 0\n";
	// Matrix: version, current, time, step, loop, notecut, 16 patterns of
	// (length speed octave, notes until x == -1), then the sequence.
	std::string matrix = "Device 2 Plugin 18\n10 20 6 Matrix 1 30 40\n4 0 0.5 3 1 0 ";
	for (int p = 0; p < 16; ++p)
		matrix += p ? "32 1 0 -1 " : "32 1 0 0 3  0.8 -1 ";
	for (int p = 0; p < 16; ++p)
		matrix += "0 ";
	matrix += "\n";
	const std::string footer = "-1 0 1\n2 0 0 0 1 0 0 0\n";

	spiralcore::Description d;
	CHECK(reader.Read(header + "2\n" + output + matrix + footer, d, error));
	CHECK(reader.Status() == "Decoded");
	CHECK(Replay(d) == header + "2\n" + output + matrix + footer);
	CHECK(d.Between().size() == d.Values().size() + 1);
	CHECK(reader.Devices().size() == 2);
	if (reader.Devices().size() == 2)
	{
		const spiralcore::PositionalReader::Device &m = reader.Devices()[1];
		CHECK(m.id == 2 && m.pluginID == 18);
		CHECK(d.Values()[m.record.begin] == "Device");
		CHECK(d.Values()[m.state.begin] == "4");           // the state starts at its version
		CHECK(d.Values()[m.state.end - 1] == "0");        // and ends with the sequence
		CHECK(m.state.end == m.record.end);
		CHECK(d.Between()[m.state.begin + 1] == " " && d.Between()[m.state.end] == " \n");
		// The name is a counted byte string: one value, the separator its gap.
		CHECK(d.Values()[m.record.begin + 7] == "Matrix" && d.Between()[m.record.begin + 7] == " ");
	}

	// A denormal is a valid lexeme here, kept as written; the host's istream
	// sets failbit on it and never reads another token.
	const std::string osc = "Device 3 Plugin 4\n0 0 0  0 0 0\n1 1 -3 0.36 0.5 0 1.4013e-45 1\n";
	CHECK(reader.Read(header + "3\n" + output + matrix + osc + footer, d, error));
	CHECK(reader.Status() == "Decoded" && reader.Devices().size() == 3);

	// A declared count that exceeds the records present is recovered, not fatal.
	CHECK(reader.Read(header + "5\n" + output + matrix + footer, d, error));
	CHECK(reader.Status() == "Recovered" && reader.Devices().size() == 2);
	CHECK(reader.Diagnostics().size() == 1 && reader.Diagnostics()[0].code == "CountMismatch");

	// Truncated inside a device: the devices before it survive, the broken
	// record's tokens are not in the description.
	std::string cut = header + "2\n" + output + matrix.substr(0, 40);
	CHECK(reader.Read(cut, d, error));
	CHECK(reader.Status() == "Partial" && reader.Devices().size() == 1);
	CHECK(Replay(d) == cut.substr(0, reader.Remainder()));

	// One device's state alone, from where a stream reader stands after the
	// device's header: the Matrix's values, the bytes they took, no more.
	{
		const std::string file = header + "2\n" + output + matrix + footer;
		size_t at = file.find("\n4 0 0.5");
		CHECK(at != std::string::npos);
		spiralcore::Description state;
		size_t consumed = 0;
		CHECK(reader.ReadState(18, file, at, state, consumed, error));
		CHECK(state.Values().size() > 20 && state.Values()[0] == "4" && state.Values().back() == "0");
		CHECK(Replay(state) == file.substr(at, consumed));
		CHECK(file.substr(at + consumed, 3) == " \n-");   // the stream carries on at the footer
		// A device with no state (Output) takes nothing; an unknown plugin id
		// is refused, not guessed.
		size_t header_end = file.find("\n0 0 0  0 0 0") + 13;
		CHECK(reader.ReadState(0, file, header_end, state, consumed, error) && consumed == 0 && state.Values().empty());
		CHECK(!reader.ReadState(9999, file, at, state, consumed, error) && error == "Unknown plugin layout");
	}

	if (!fails)
		printf("positional reader: replay, spans, lexemes and recovery passed\n");
	return fails ? 1 : 0;
}
