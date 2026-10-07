// SPDX-License-Identifier: GPL-2.0-or-later
#include "Description.h"
#include <cstdio>
#include <cstdlib>
#include <ostream>

namespace spiralcore
{
	Description::Description()
	{
		m_Between.push_back("");
	}

	void Description::AddValue(const std::string &text)
	{
		m_Values.push_back(text);
		m_Between.push_back("");
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
