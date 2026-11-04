// SPDX-License-Identifier: GPL-2.0-or-later
// The positional token form of a device's state: what a StreamOut used to
// insert, in order, with the writing left to whoever holds it.
#ifndef SPIRALCORE_DESCRIPTION_H
#define SPIRALCORE_DESCRIPTION_H

#include <iosfwd>
#include <sstream>
#include <string>
#include <vector>

namespace Spiral
{
namespace Core
{
	class FloatProperty;
	class SignedProperty;
	class UnsignedProperty;
	class ToggleProperty;
	class SetProperty;
	class StringProperty;
	class NoteProperty;
	class VectorProperty;

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

		Description &Value(const FloatProperty &p);
		Description &Value(const SignedProperty &p);
		Description &Value(const UnsignedProperty &p);
		Description &Value(const ToggleProperty &p);
		Description &Value(const SetProperty &p);
		Description &Value(const StringProperty &p);
		Description &Value(const NoteProperty &p);

		Description &Key(const std::string &key, const std::string &propertyClass);
		// What a key does not say about a node on its path: the class and
		// name of a property that holds others, a list or a pattern, or the
		// name of one held. The file writer gives them to the nodes it
		// builds from the keys, and a file read back keeps them here, so
		// the private tree's named lists come through as they went in.
		struct Shape
		{
			std::string Key, Class, Name;
		};
		Description &Shaped(const std::string &key, const std::string &propertyClass, const std::string &name);
		const std::vector<Shape> &Shapes() const { return m_Shapes; }

		// A literal between values: s << " ".
		Description &Separator(const char *text);

		// s << endl.
		Description &Line();

		const std::vector<std::string> &Values() const { return m_Values; }
		const std::vector<std::string> &Between() const { return m_Between; }
		const std::vector<std::string> &Keys() const { return m_Keys; }
		const std::vector<std::string> &Classes() const { return m_Classes; }
		bool Named() const { return m_Named; }
		int Find(const std::string &key) const;

		int Version() const;
		long SavedAs() const { return m_SavedAs; }
		void SavedAs(long pluginID) { m_SavedAs = pluginID; }
		int Field(size_t index, const std::string &key) const;
		template <class T> void SetValue(size_t index, const T &v)
		{
			std::ostringstream text;
			text << v;
			m_Values[index] = text.str();
		}

		// The stream the StreamOut would have written.
		void Write(std::ostream &s) const;

		// {"values": [...], "between": [...]}, one line.
		std::string JSON() const;
		// The same line read back: the JSON form feeds Apply the way the
		// positional reader does. False with the reason in error, out untouched.
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
			Reader &Value(FloatProperty &p);
			Reader &Value(SignedProperty &p);
			Reader &Value(UnsignedProperty &p);
			Reader &Value(ToggleProperty &p);
			Reader &Value(SetProperty &p);
			Reader &Value(StringProperty &p);
			Reader &Value(NoteProperty &p);
			Reader &Count(const VectorProperty &p, int &n);
			int Find(const std::string &key) const { return m_Description.Find(key); }
			template <class T> bool At(size_t index, T &x)
			{
				if (index >= m_End) return false;
				std::istringstream text(m_Description.Values()[index]);
				T v;
				if (!(text >> v)) return false;
				x = v;
				return true;
			}
			bool At(size_t index, std::string &x);
			bool At(size_t index, float &x);
			// What is left of the span, values and the gaps among them, as a
			// description of its own; the reader is at its end. For a device
			// that keeps a state it cannot read and gives it back as it was.
			void Rest(Description &out);
			// Values left in the span.
			bool More() const { return m_At < m_End; }
			bool Named() const { return m_Description.Named(); }
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
		std::vector<Shape> m_Shapes;
		std::vector<std::string> m_Between;   // ! always m_Values.size() + 1 entries
		std::vector<std::string> m_Keys;
		std::vector<std::string> m_Classes;
		bool m_Named;
		long m_SavedAs;
	};
}
}

#endif
