// SPDX-License-Identifier: GPL-2.0-or-later
#include "Description.h"
#include <cstdio>
#include <cstdlib>
#include <cctype>
#include <ostream>

namespace Spiral
{
namespace Core
{
	Description::Description()
		: m_Named(false), m_SavedAs(-1)
	{
		m_Between.push_back("");
	}

	void Description::AddValue(const std::string &text)
	{
		m_Values.push_back(text);
		m_Between.push_back("");
		m_Keys.push_back("");
		m_Classes.push_back("");
	}

	Description &Description::Key(const std::string &key, const std::string &propertyClass)
	{
		if (!m_Values.empty() && !key.empty())
		{
			m_Keys.back() = key;
			m_Classes.back() = propertyClass;
			m_Named = true;
		}
		return *this;
	}

	static std::string Folded(const std::string &text)
	{
		std::string out(text);
		for (size_t i = 0; i < out.size(); ++i)
			out[i] = (char)toupper((unsigned char)out[i]);
		return out;
	}

	int Description::Find(const std::string &key) const
	{
		const std::string wanted = Folded(key);
		for (size_t i = 0; i < m_Keys.size(); ++i)
			if (!m_Keys[i].empty() && Folded(m_Keys[i]) == wanted)
				return (int)i;
		return -1;
	}

	int Description::Version() const
	{
		if (m_Values.empty())
			return -1;
		return atoi(m_Values[0].c_str());
	}

	int Description::Field(size_t index, const std::string &key) const
	{
		if (m_Named)
			return Find(key);
		return index < m_Values.size() ? (int)index : -1;
	}

	Description &Description::Separator(const char *text)
	{
		m_Between.back() += text;
		return *this;
	}

	Description &Description::Line()
	{
		m_Between.back() += '\n';
		return *this;
	}

	void Description::Write(std::ostream &s) const
	{
		for (size_t i = 0; i < m_Values.size(); ++i)
		{
			s << m_Between[i] << m_Values[i];
		}
		s << m_Between.back();
	}

	Description::Reader::Reader(const Description &d)
	: m_Description(d), m_At(0), m_End(d.m_Values.size()), m_Failed(false)
	{
	}

	Description::Reader::Reader(const Description &d, size_t begin, size_t end)
	: m_Description(d), m_At(begin), m_End(end), m_Failed(false)
	{
		if (m_End > d.m_Values.size()) m_End = d.m_Values.size();
		if (m_At > m_End) m_At = m_End;
	}

	const std::string &Description::Reader::Next()
	{
		static const std::string none;
		if (m_Description.m_Named)
			while (m_At < m_End && !m_Description.m_Keys[m_At].empty()) ++m_At;
		if (!More())
		{
			m_Failed = true;
			return none;
		}
		return m_Description.m_Values[m_At++];
	}

	Description::Reader &Description::Reader::Value(std::string &x)
	{
		if (More())
			x = Next();
		else
			m_Failed = true;
		return *this;
	}

	// The whole value must be the number; a range error (a denormal, an
	// overflow) still is one, as the C library spells it.
	Description::Reader &Description::Reader::Value(double &x)
	{
		if (More())
		{
			const std::string &text = Next();
			char *end = NULL;
			double v = strtod(text.c_str(), &end);
			if (!text.empty() && end == text.c_str() + text.size())
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

	bool Description::Reader::At(size_t index, std::string &x)
	{
		if (index >= m_End) return false;
		x = m_Description.Values()[index];
		return true;
	}

	bool Description::Reader::At(size_t index, float &x)
	{
		if (index >= m_End) return false;
		const std::string &text = m_Description.Values()[index];
		char *end = NULL;
		double v = strtod(text.c_str(), &end);
		if (text.empty() || end != text.c_str() + text.size()) return false;
		x = static_cast<float>(v);
		return true;
	}

	void Description::Reader::Rest(Description &out)
	{
		out = Description();
		out.m_Named = m_Description.m_Named;
		out.m_Between[0] = m_Description.m_Between[m_At];
		for (; m_At < m_End; ++m_At)
		{
			out.m_Values.push_back(m_Description.m_Values[m_At]);
			out.m_Between.push_back(m_Description.m_Between[m_At + 1]);
			out.m_Keys.push_back(m_Description.m_Keys[m_At]);
			out.m_Classes.push_back(m_Description.m_Classes[m_At]);
		}
	}

	Description::Reader &Description::Reader::Value(float &x)
	{
		double v = x;
		Value(v);
		x = static_cast<float>(v);
		return *this;
	}

	// JSON string escaping. Values are what an ostream wrote, so anything
	// below 0x20 is a raw byte a plugin chose to emit; it is kept, escaped.
	static void Quote(std::string &out, const std::string &text)
	{
		out += '"';
		for (size_t i = 0; i < text.size(); ++i)
		{
			unsigned char c = (unsigned char)text[i];
			switch (c)
			{
				case '"': out += "\\\""; break;
				case '\\': out += "\\\\"; break;
				case '\n': out += "\\n"; break;
				case '\r': out += "\\r"; break;
				case '\t': out += "\\t"; break;
				default:
					if (c < 0x20)
					{
						char buf[8];
						std::sprintf(buf, "\\u%04x", c);
						out += buf;
					}
					else
					{
						out += (char)c;
					}
			}
		}
		out += '"';
	}

	static void QuoteList(std::string &out, const std::vector<std::string> &list)
	{
		out += '[';
		for (size_t i = 0; i < list.size(); ++i)
		{
			if (i) out += ", ";
			Quote(out, list[i]);
		}
		out += ']';
	}

	std::string Description::JSON() const
	{
		std::string out = "{\"values\": ";
		QuoteList(out, m_Values);
		out += ", \"between\": ";
		QuoteList(out, m_Between);
		out += '}';
		return out;
	}
}
}
