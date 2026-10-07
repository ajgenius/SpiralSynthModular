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

		// Reading a description back, value by value, in the order it was
		// written: what a plugin's Apply does with what its Describe said.
		// Value(x) extracts the next value into x the way an istream with
		// default flags would, which is what the legacy StreamIn did; a
		// std::string takes the whole value; a float or double takes any
		// number the text spells, denormals included, where one C++ library's
		// istream refuses them. Past the end, or when the text
		// does not extract, x is left alone and Failed() stays set. A reader
		// can be limited to a span of the values: the host gives each device
		// the span the file reader found for its state, and no device can
		// read past its own.
		class Reader
		{
		public:
			explicit Reader(const Description &d);
			Reader(const Description &d, size_t begin, size_t end);
			template <class T> Reader &Value(T &x)
			{
				if (More())
				{
					std::istringstream text(Next());
					T v;
					if (text >> v)
						x = v;
					else
						m_Failed = true;
				}
				else
				{
					m_Failed = true;
				}
				return *this;
			}
			Reader &Value(std::string &x);
			Reader &Value(float &x);
			Reader &Value(double &x);
			// What is left of the span, values and the gaps among them, as a
			// description of its own; the reader is at its end. For a device
			// that keeps a state it cannot read and gives it back as it was.
			void Rest(Description &out);
			// Values left in the span.
			bool More() const { return m_At < m_End; }
			size_t Position() const { return m_At; }
			bool Failed() const { return m_Failed; }
		private:
			// The next value, consumed; past the end, Failed() and "".
			const std::string &Next();
			const Description &m_Description;
			size_t m_At, m_End;
			bool m_Failed;
		};

	private:
		void AddValue(const std::string &text);

		std::vector<std::string> m_Values;
		std::vector<std::string> m_Between;   // ! always m_Values.size() + 1 entries
	};
}

#endif
