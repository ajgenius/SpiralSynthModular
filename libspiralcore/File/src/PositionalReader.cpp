// SPDX-License-Identifier: GPL-2.0-or-later
// The decode walk of SpiralTesting's PositionalPatchReader, kept to what a
// token stream needs: no item tree is built, the few integers a layout
// refers back to (counts, the plugin id, a sentinel) live in a scope map.
#include "PositionalReader.h"
#include "JSONParser.h"
#include <cerrno>
#include <cstdlib>
#include <map>
#include <memory>
#include <stdexcept>

namespace
{
	typedef SpiralJSON::JSONValue Value;
	typedef std::map<std::string, long> Scope;
	const size_t itemLimit = 262144;
	const size_t byteLimit = 16 * 1024 * 1024;

	bool Space(char c)
	{
		return c == ' ' || c == '\t' || c == '\r' || c == '\n' || c == '\f' || c == '\v';
	}

	const Value &Require(const Value &object, const char *key, Value::Type type)
	{
		const Value *value = object.Get(key);
		if (!value || value->GetType() != type)
			throw std::runtime_error(std::string("Invalid contract member: ") + key);

		return *value;
	}

	bool Integer(const std::string &text, long &value)
	{
		if (text.empty())
			return false;

		size_t start = text[0] == '-' || text[0] == '+' ? 1 : 0;
		if (start == text.size() || text.find_first_not_of("0123456789", start) != std::string::npos)
			return false;

		errno = 0;
		char *end = NULL;
		value = std::strtol(text.c_str(), &end, 10);
		return errno != ERANGE && end == text.c_str() + text.size();
	}

	long IntegerValue(const Value &value)
	{
		long number;
		if (value.GetType() != Value::Number || !Integer(value.Text(), number))
			throw std::runtime_error("Expected integer");

		return number;
	}

	// Numeric syntax is checked without converting through double, so the
	// source lexeme (precision, exponent, a denormal) survives untouched.
	bool Numeric(const std::string &text)
	{
		size_t at = 0;
		if (at < text.size() && (text[at] == '-' || text[at] == '+'))
			++at;

		size_t digits = 0;
		while (at < text.size() && text[at] >= '0' && text[at] <= '9')
		{
			++at;
			++digits;
		}

		if (at < text.size() && text[at] == '.')
		{
			++at;
			while (at < text.size() && text[at] >= '0' && text[at] <= '9')
			{
				++at;
				++digits;
			}
		}

		if (!digits)
			return false;

		if (at < text.size() && (text[at] == 'e' || text[at] == 'E'))
		{
			++at;
			if (at < text.size() && (text[at] == '+' || text[at] == '-'))
				++at;

			size_t start = at;
			while (at < text.size() && text[at] >= '0' && text[at] <= '9')
				++at;

			if (start == at)
				return false;
		}

		return at == text.size();
	}

	bool Same(const Value &a, const Value &b)
	{
		if (a.GetType() != b.GetType())
			return false;

		switch (a.GetType())
		{
			case Value::Array:
				if (a.Size() != b.Size())
					return false;
				for (size_t i = 0; i < a.Size(); ++i)
					if (!Same(*a.At(i), *b.At(i)))
						return false;
				return true;

			case Value::Object:
			{
				std::vector<std::string> keys = a.Keys();
				if (keys.size() != b.Keys().size())
					return false;
				for (size_t i = 0; i < keys.size(); ++i)
					if (!b.Get(keys[i]) || !Same(*a.Get(keys[i]), *b.Get(keys[i])))
						return false;
				return true;
			}

			case Value::Boolean:
				return a.AsBool() == b.AsBool();

			default:
				return a.Text() == b.Text();
		}
	}

	// Every layout a plugin id may carry: the contract's own entry plus the
	// history's inline Fields. Revisions that only point back at the
	// contract add nothing.
	struct Layouts
	{
		const Value &contract;
		std::map<long, std::vector<const Value *> > plugins;

		Layouts(const Value &definition, const Value *history):
		    contract(definition)
		{
			const Value &base = Require(contract, "Plugins", Value::Object);
			std::vector<std::string> names = base.Keys();
			for (size_t i = 0; i < names.size(); ++i)
			{
				const Value &plugin = *base.Get(names[i]);
				Add(IntegerValue(Require(plugin, "PluginID", Value::Number)), Require(plugin, "Fields", Value::Array));
			}

			if (!history)
				return;

			if (IntegerValue(Require(*history, "LegacyHistoryVersion", Value::Number)) != 1)
				throw std::runtime_error("Unsupported legacy history version");

			const Value &old = Require(*history, "Plugins", Value::Object);
			names = old.Keys();
			for (size_t i = 0; i < names.size(); ++i)
			{
				const Value &plugin = *old.Get(names[i]);
				long id = IntegerValue(Require(plugin, "PluginID", Value::Number));
				const Value &revisions = Require(plugin, "Revisions", Value::Object);
				std::vector<std::string> commits = revisions.Keys();
				for (size_t j = 0; j < commits.size(); ++j)
					if (const Value *fields = revisions.Get(commits[j])->Get("Fields"))
						Add(id, *fields);
			}
		}

		void Add(long id, const Value &fields)
		{
			std::vector<const Value *> &options = plugins[id];
			for (size_t i = 0; i < options.size(); ++i)
				if (Same(*options[i], fields))
					return;

			options.push_back(&fields);
		}
	};

	struct Decoder
	{
		typedef spiralcore::PositionalReader Reader;

		struct Wire
		{
			long inputID, outputID, inputPort, outputPort;
		};

		const Layouts &layouts;
		const std::string &source;
		size_t at, nodes, origin, mark;
		std::vector<Reader::Span> tokens;
		std::vector<Reader::Device> devices;
		std::vector<Wire> wires;
		std::vector<Reader::Diagnostic> &diagnostics;
		Reader::Span state;

		Decoder(const Layouts &l, const std::string &s, std::vector<Reader::Diagnostic> &d):
		    layouts(l),
		    source(s),
		    at(s.compare(0, 3, "\xef\xbb\xbf") == 0 ? 3 : 0),
		    nodes(0),
		    origin(at),
		    mark(at),
		    diagnostics(d)
		{
			state.begin = state.end = 0;
		}

		void Skip()
		{
			while (at < source.size() && Space(source[at]))
				++at;
		}

		// A value occupies [start, at); the gap before it is [mark, start).
		void Log(size_t start)
		{
			Reader::Span token = { start, at };
			tokens.push_back(token);
			mark = at;
		}

		// Decoding of a record is rolled back by restoring both cursors.
		void Rewind(size_t start, size_t logged)
		{
			at = start;
			mark = start;
			tokens.resize(logged);
		}

		std::string Scan()
		{
			Skip();
			size_t start = at;
			while (at < source.size() && !Space(source[at]))
				++at;

			if (start == at)
				throw std::runtime_error("Unexpected end of input");

			return source.substr(start, at - start);
		}

		std::string Token()
		{
			std::string text = Scan();
			Log(at - text.size());
			return text;
		}

		std::string Peek()
		{
			size_t saved = at;
			Skip();
			std::string text = at == source.size() ? "" : Scan();
			at = saved;
			return text;
		}

		void Warn(const std::string &code, const std::string &field, const std::string &message)
		{
			Reader::Diagnostic entry;
			entry.code = code;
			entry.field = field;
			entry.message = message;
			entry.offset = at;
			entry.index = -1;
			diagnostics.push_back(entry);
		}

		const Value &Plugin(const Value &definition, const Scope &scope)
		{
			const std::string &key = Require(definition, "PropertyName", Value::String).Text();
			Scope::const_iterator id = scope.find(key);
			if (id == scope.end())
				throw std::runtime_error("Missing plugin id property");

			std::map<long, std::vector<const Value *> >::const_iterator found = layouts.plugins.find(id->second);
			if (found == layouts.plugins.end())
				throw std::runtime_error("Unknown plugin layout");

			const Value *match = NULL;
			std::string token = Peek();
			long version = 0;
			bool numeric = Integer(token, version);
			for (size_t i = 0; i < found->second.size(); ++i)
			{
				const Value &fields = *found->second[i];
				bool selected = !fields.Size() && (token.empty() || token == "Device" || token == "-1");
				if (fields.Size())
				{
					const Value &first = *fields.At(0);
					const Value *expected = first.Get("Equals");
					selected = numeric && expected && Require(first, "Name", Value::String).Text() == "Version"
						&& IntegerValue(*expected) == version;
				}

				if (selected)
				{
					if (match)
						throw std::runtime_error("Ambiguous plugin layout");

					match = &fields;
				}
			}

			if (!match)
				throw std::runtime_error("Unknown plugin state version");

			return *match;
		}

		void Fields(const Value &fields, Scope &object, unsigned depth)
		{
			for (size_t i = 0; i < fields.Size(); ++i)
			{
				const Value &field = *fields.At(i);
				long integer;
				if (Read(field, object, depth + 1, integer))
					object[field.Get("Name")->Text()] = integer;
			}
		}

		// Consumes one definition. True when the value is an integer the
		// layout may refer back to, returned in integer.
		bool Read(const Value &definition, Scope &scope, unsigned depth, long &integer)
		{
			if (++nodes > itemLimit || depth > 64)
				throw std::runtime_error("Decoded item or nesting limit");

			const std::string type = definition.Get("Type")->Text();
			const Value *name = definition.Get("Name");
			const std::string field = name ? name->Text() : "";
			const Value *expected = definition.Get("Equals");
			const Value *structure = layouts.contract.Get("Structures")->Get(type);
			if (structure)
				return Read(*structure, scope, depth + 1, integer);

			if (type == "Object" || type == "PluginState")
			{
				Scope object;
				if (type == "PluginState")
					state.begin = tokens.size();
				Fields(type == "Object" ? *definition.Get("Fields") : Plugin(definition, scope), object, depth);
				if (type == "PluginState")
					state.end = tokens.size();
				return false;
			}

			if (type == "Array")
			{
				const Value &items = *definition.Get("Items");
				if (const Value *count = definition.Get("Count"))
				{
					long n;
					if (count->Get("Type")->Text() == "PropertyValue")
					{
						Scope::const_iterator length = scope.find(count->Get("PropertyName")->Text());
						if (length == scope.end())
							throw std::runtime_error("Missing count property");
						n = length->second;
					}
					else
						n = IntegerValue(*count->Get("Value"));

					if (n < 0 || static_cast<unsigned long>(n) > itemLimit)
						throw std::runtime_error("Array count out of range");

					for (long i = 0; i < n; ++i)
						Read(items, scope, depth + 1, integer);
				}
				else
				{
					const Value &fields = *items.Get("Fields");
					long sentinel = IntegerValue(*definition.Get("Until")->Get("Equals"));
					while (true)
					{
						Scope entry;
						long first;
						if (!Read(*fields.At(0), entry, depth + 1, first))
							throw std::runtime_error("Sentinel field is not an integer");
						if (first == sentinel)
							break;

						entry[fields.At(0)->Get("Name")->Text()] = first;
						for (size_t i = 1; i < fields.Size(); ++i)
						{
							long member;
							if (Read(*fields.At(i), entry, depth + 1, member))
								entry[fields.At(i)->Get("Name")->Text()] = member;
						}
					}
				}
				return false;
			}

			if (type == "ByteString")
			{
				long length;
				if (!Integer(Token(), length) || length < 0 || at == source.size() || !Space(source[at]))
					throw std::runtime_error("Invalid byte-string length or separator");

				++at;
				if (static_cast<unsigned long>(length) > source.size() - at)
					throw std::runtime_error("Truncated byte string");

				size_t start = at;
				at += length;
				Log(start);
				return false;
			}

			if (type == "Byte")
			{
				Skip();
				if (at == source.size())
					throw std::runtime_error("Missing byte");

				integer = static_cast<unsigned char>(source[at++]);
				Log(at - 1);
				return true;
			}

			std::string token = Token();
			if (type == "Integer" || type == "Boolean")
			{
				if (!Integer(token, integer))
					throw std::runtime_error("Invalid integer in " + field);

				if (type == "Boolean" && integer != 0 && integer != 1)
					Warn("InvalidBoolean", field, "Retained numeric value outside 0/1");

				if (expected && IntegerValue(*expected) != integer)
					throw std::runtime_error("Unexpected value in " + field);

				return true;
			}

			if (type == "Number")
			{
				// Non-JSON numeric spellings are kept as they are, with a diagnostic.
				std::auto_ptr<Value> parsed(SpiralJSON::ParseJSONText(token));
				if (!Numeric(token) || !parsed.get() || parsed->GetType() != Value::Number)
					Warn("InvalidNumber", field, "Retained raw numeric token");
			}

			if (expected && (expected->GetType() != Value::String || expected->Text() != token))
				throw std::runtime_error("Unexpected value in " + field);

			return false;
		}

		// The Devices and Wires arrays: complete records until the -1 marker
		// or the end, with the declared count treated as advisory.
		void Records(const Value &definition, const Scope &root)
		{
			const std::string &name = definition.Get("Name")->Text();
			Scope::const_iterator declared = root.find(definition.Get("Count")->Get("PropertyName")->Text());
			if (declared == root.end())
				throw std::runtime_error("Missing count property");

			const Value &items = *definition.Get("Items");
			const Value &structure = Require(Require(layouts.contract, "Structures", Value::Object),
			                                 items.Get("Type")->Text().c_str(), Value::Object);
			size_t count = 0;
			while (true)
			{
				Skip();
				if (at == source.size() || (name == "Devices" && Peek() == "-1"))
					break;

				size_t start = at, logged = tokens.size();
				try
				{
					Scope record;
					Fields(Require(structure, "Fields", Value::Array), record, 0);
					++count;
					if (name == "Devices")
					{
						Reader::Device device = { record["ID"], record["PluginID"], { logged, tokens.size() }, state };
						devices.push_back(device);
					}
					else
					{
						Wire wire = { record["InputID"], record["OutputID"], record["InputPort"], record["OutputPort"] };
						wires.push_back(wire);
					}
				}
				catch (const std::runtime_error &)
				{
					Rewind(start, logged);
					throw;
				}
			}

			if (declared->second < 0 || static_cast<unsigned long>(declared->second) != count)
				Warn("CountMismatch", name, "Declared count differs from complete available records");
		}

		void Wires()
		{
			std::map<long, unsigned> ids;
			for (size_t i = 0; i < devices.size(); ++i)
				++ids[devices[i].id];

			for (std::map<long, unsigned>::const_iterator i = ids.begin(); i != ids.end(); ++i)
				if (i->second > 1)
					Warn("DuplicateDeviceID", "Devices", "Device ID is ambiguous");

			for (size_t i = 0; i < wires.size(); ++i)
			{
				const Wire &wire = wires[i];
				if (ids[wire.inputID] != 1 || ids[wire.outputID] != 1 || wire.inputPort < 0 || wire.outputPort < 0)
				{
					Warn("UnusableWire", "Wires", "Missing/ambiguous device or negative port; wire retained");
					diagnostics.back().index = static_cast<long>(i);
				}
			}
		}

		// The gap-value log as a Description: between[0] value[0] ... between[n].
		void Finish(spiralcore::Description &out) const
		{
			size_t last = origin;
			for (size_t i = 0; i < tokens.size(); ++i)
			{
				out.Separator(source.substr(last, tokens[i].begin - last).c_str());
				out.Value(source.substr(tokens[i].begin, tokens[i].end - tokens[i].begin));
				last = tokens[i].end;
			}
			out.Separator(source.substr(last, at - last).c_str());
		}
	};
}

namespace spiralcore
{
	PositionalReader::PositionalReader(const Value &contract, const Value *history):
	    m_Contract(contract),
	    m_History(history),
	    m_Remainder(0)
	{
	}

	bool PositionalReader::Read(const std::string &text, Description &out, std::string &error)
	{
		out = Description();
		m_Status.clear();
		m_Devices.clear();
		m_Diagnostics.clear();
		m_Remainder = 0;
		try
		{
			if (text.size() > byteLimit)
				throw std::runtime_error("Source exceeds 16 MiB");

			Layouts layouts(m_Contract, m_History);
			Decoder decoder(layouts, text, m_Diagnostics);
			Scope root;
			std::string status = "Decoded";
			const Value &fields = Require(m_Contract, "Fields", Value::Array);
			try
			{
				for (size_t i = 0; i < fields.Size(); ++i)
				{
					const Value &field = *fields.At(i);
					const std::string &name = field.Get("Name")->Text();
					if (name == "Devices" || name == "Wires")
						decoder.Records(field, root);
					else
					{
						size_t start = decoder.at, logged = decoder.tokens.size();
						try
						{
							long integer;
							if (decoder.Read(field, root, 0, integer))
								root[name] = integer;
						}
						catch (const std::runtime_error &)
						{
							decoder.Rewind(start, logged);
							throw;
						}
					}
				}

				decoder.Skip();
				if (decoder.at != text.size())
					throw std::runtime_error("Undecodable trailing data");
			}
			catch (const std::runtime_error &failure)
			{
				decoder.Warn("UndecodableRemainder", "", failure.what());
				status = decoder.devices.empty() ? "Unreadable" : "Partial";
			}

			decoder.Wires();
			if (status == "Decoded" && !m_Diagnostics.empty())
				status = "Recovered";

			decoder.Finish(out);
			m_Devices = decoder.devices;
			m_Status = status;
			m_Remainder = decoder.at;
			error.clear();
			return true;
		}
		catch (const std::runtime_error &failure)
		{
			error = failure.what();
			return false;
		}
	}

	bool PositionalReader::ReadState(long pluginID, const std::string &text, size_t at,
	                                 Description &out, size_t &consumed, std::string &error)
	{
		try
		{
			if (text.size() > byteLimit)
				throw std::runtime_error("Source exceeds 16 MiB");
			if (at > text.size())
				throw std::runtime_error("State offset past the end");

			Layouts layouts(m_Contract, m_History);
			Decoder decoder(layouts, text, m_Diagnostics);
			decoder.at = decoder.origin = decoder.mark = at;

			// The Device structure's State field: a PluginState keyed by PluginID.
			const Value &device = Require(Require(m_Contract, "Structures", Value::Object), "Device", Value::Object);
			const Value &fields = Require(device, "Fields", Value::Array);
			const Value *state = NULL;
			for (size_t i = 0; i < fields.Size() && !state; ++i)
				if (fields.At(i)->Get("Type")->Text() == "PluginState")
					state = fields.At(i);
			if (!state)
				throw std::runtime_error("Contract has no PluginState field");

			Scope scope;
			scope[Require(*state, "PropertyName", Value::String).Text()] = pluginID;
			long integer;
			decoder.Read(*state, scope, 0, integer);

			out = Description();
			decoder.Finish(out);
			consumed = decoder.at - at;
			error.clear();
			return true;
		}
		catch (const std::runtime_error &failure)
		{
			error = failure.what();
			return false;
		}
	}
}
