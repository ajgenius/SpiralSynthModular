// SPDX-License-Identifier: GPL-2.0-or-later
#include "Description.h"
#include <cstdio>
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
