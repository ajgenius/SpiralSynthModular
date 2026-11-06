// SPDX-License-Identifier: GPL-2.0-or-later
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

	class Description
	{
	public:
		Description();

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
		struct Shape
		{
			std::string Key, Class, Name;
		};
		Description &Shaped(const std::string &key, const std::string &propertyClass, const std::string &name);
		const std::vector<Shape> &Shapes() const { return m_Shapes; }

		Description &Separator(const char *text);

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

		void Write(std::ostream &s) const;

		std::string JSON() const;
		static bool FromJSON(const std::string &text, Description &out, std::string &error);

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
			void Rest(Description &out);
			bool More() const { return m_At < m_End; }
			bool Named() const { return m_Description.Named(); }
			size_t Position() const { return m_At; }
			bool Failed() const { return m_Failed; }
		private:
			const std::string &Next();
			const Description &m_Description;
			size_t m_At, m_End;
			bool m_Failed;
		};

	private:
		void AddValue(const std::string &text);

		std::vector<std::string> m_Values;
		std::vector<Shape> m_Shapes;
		std::vector<std::string> m_Between;
		std::vector<std::string> m_Keys;
		std::vector<std::string> m_Classes;
		bool m_Named;
		long m_SavedAs;
	};
}
}

#endif
