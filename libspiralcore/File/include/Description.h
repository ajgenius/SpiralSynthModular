// SPDX-License-Identifier: GPL-2.0-or-later
// The positional token form of a device's state: what a StreamOut used to
// insert, in order, with the writing left to whoever holds it.
#ifndef SPIRALCORE_DESCRIPTION_H
#define SPIRALCORE_DESCRIPTION_H

#include <iosfwd>
#include <sstream>
#include <string>
#include <vector>

namespace spiralcore
{
	// A description is the values a device wrote, as the text an ostream
	// with default flags writes for each operand, and the gaps between them.
	// With n values there are n + 1 gaps; Write() replays gap, value, gap...
	// and so reproduces the legacy stream byte for byte.
	//
	// The values are what a named description will carry. The gaps are the
	// legacy format's business only and go when the writer does.
	class Description
	{
	public:
		Description();

		// One operand of the old chain: s << v.
		template <class T> Description &Value(const T &v)
		{
			std::ostringstream text;
			text << v;
			AddValue(text.str());
			return *this;
		}

		// A literal between values: s << " ".
		Description &Separator(const char *text);

		// s << endl.
		Description &Line();

		const std::vector<std::string> &Values() const { return m_Values; }
		const std::vector<std::string> &Between() const { return m_Between; }

		// The stream the StreamOut would have written.
		void Write(std::ostream &s) const;

		// {"values": [...], "between": [...]}, one line.
		std::string JSON() const;
		// The same line read back: the JSON form feeds Apply the way the
		// positional reader does. Needs the JSON reader (HAVE_YAJL); false
		// with the reason in error, out untouched.
		static bool FromJSON(const std::string &text, Description &out, std::string &error);

	private:
		void AddValue(const std::string &text);

		std::vector<std::string> m_Values;
		std::vector<std::string> m_Between;   // ! always m_Values.size() + 1 entries
	};
}

#endif
