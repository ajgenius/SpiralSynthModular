// SPDX-License-Identifier: GPL-2.0-or-later
#include "JSON.h"
#include "detail/JSONReader.h"

#include <algorithm>
#include <cerrno>
#include <climits>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <string>

using namespace std;

namespace Spumoni
{
	namespace
	{
		struct KeyValue
		{
			string Key;
			const JSON *Value;
		};

		bool KeyLess(const KeyValue &a, const KeyValue &b)
		{
			return a.Key < b.Key;
		}

		void AppendEscaped(string &out, const string &text)
		{
			out.push_back('"');
			for (size_t i = 0; i < text.size(); ++i)
			{
				unsigned char c = static_cast<unsigned char>(text[i]);
				switch (c)
				{
					case '"': out += "\\\""; break;
					case '\\': out += "\\\\"; break;
					case '\b': out += "\\b"; break;
					case '\f': out += "\\f"; break;
					case '\n': out += "\\n"; break;
					case '\r': out += "\\r"; break;
					case '\t': out += "\\t"; break;
					default:
						if (c < 0x20)
						{
							static const char hex[] = "0123456789abcdef";
							out += "\\u00";
							out.push_back(hex[c >> 4]);
							out.push_back(hex[c & 0x0f]);
						}
						else
							out.push_back(static_cast<char>(c));
						break;
				}
			}
			out.push_back('"');
		}

		void Indent(string &out, int n)
		{
			out.append(static_cast<size_t>(n) * 2, ' ');
		}

	}

	JSON::JSON(Type type)
		: m_Type(type), m_Bool(false)
	{
	}

	JSON::~JSON()
	{
		for (size_t i = 0; i < m_Items.size(); ++i) delete m_Items[i];
		for (size_t i = 0; i < m_Members.size(); ++i) delete m_Members[i].Value;
	}

	JSON *JSON::MakeNull() { return new JSON(Null); }
	JSON *JSON::MakeBoolean(bool value)
	{
		JSON *json = new JSON(Boolean);
		json->m_Bool = value;
		return json;
	}
	JSON *JSON::MakeNumber(const string &lexeme)
	{
		JSON *json = new JSON(Number);
		json->m_Text = lexeme;
		return json;
	}
	JSON *JSON::MakeString(const string &text)
	{
		JSON *json = new JSON(String);
		json->m_Text = text;
		return json;
	}
	JSON *JSON::MakeArray() { return new JSON(Array); }
	JSON *JSON::MakeObject() { return new JSON(Object); }

	JSON *JSON::Duplicate() const
	{
		JSON *copy = new JSON(m_Type);
		copy->m_Bool = m_Bool;
		copy->m_Text = m_Text;
		for (size_t i = 0; i < m_Items.size(); ++i)
			copy->m_Items.push_back(m_Items[i]->Duplicate());
		for (size_t i = 0; i < m_Members.size(); ++i)
		{
			Member member;
			member.Key = m_Members[i].Key;
			member.Value = m_Members[i].Value->Duplicate();
			copy->m_Members.push_back(member);
		}
		return copy;
	}

	bool JSON::Integer(long &value) const
	{
		if (m_Type != Number || m_Text.empty()) return false;
		for (size_t i = (m_Text[0] == '-') ? 1 : 0; i < m_Text.size(); ++i)
			if (m_Text[i] < '0' || m_Text[i] > '9') return false;
		errno = 0;
		char *end = NULL;
		long v = strtol(m_Text.c_str(), &end, 10);
		if (errno == ERANGE || end != m_Text.c_str() + m_Text.size()) return false;
		value = v;
		return true;
	}

	size_t JSON::Size() const
	{
		if (m_Type == Array) return m_Items.size();
		if (m_Type == Object) return m_Members.size();
		return 0;
	}

	const JSON *JSON::At(size_t index) const
	{
		if (m_Type != Array || index >= m_Items.size()) return NULL;
		return m_Items[index];
	}

	JSON *JSON::At(size_t index)
	{
		if (m_Type != Array || index >= m_Items.size()) return NULL;
		return m_Items[index];
	}

	const JSON *JSON::Get(const char *key) const
	{
		return key ? Get(string(key)) : NULL;
	}

	JSON *JSON::Get(const char *key)
	{
		return key ? Get(string(key)) : NULL;
	}

	const JSON *JSON::Get(const string &key) const
	{
		if (m_Type != Object)
			return NULL;

		for (size_t i = 0; i < m_Members.size(); ++i)
			if (m_Members[i].Key == key)
				return m_Members[i].Value;

		return NULL;
	}

	JSON *JSON::Get(const string &key)
	{
		return const_cast<JSON *>(static_cast<const JSON *>(this)->Get(key));
	}

	vector<string> JSON::Keys() const
	{
		vector<string> keys;
		for (size_t i = 0; i < m_Members.size(); ++i) keys.push_back(m_Members[i].Key);
		sort(keys.begin(), keys.end());
		return keys;
	}

	void JSON::SetOwned(const string &key, JSON *value)
	{
		if (m_Type != Object)
		{
			delete value;
			return;
		}
		for (size_t i = 0; i < m_Members.size(); ++i)
		{
			if (m_Members[i].Key == key)
			{
				delete m_Members[i].Value;
				m_Members[i].Value = value;
				return;
			}
		}
		Member member;
		member.Key = key;
		member.Value = value;
		m_Members.push_back(member);
	}

	void JSON::AppendOwned(JSON *value)
	{
		if (m_Type != Array)
		{
			delete value;
			return;
		}
		m_Items.push_back(value);
	}

	void JSON::Write(string &out, bool pretty, int indent) const
	{
		switch (m_Type)
		{
			case Null: out += "null"; break;
			case Boolean: out += m_Bool ? "true" : "false"; break;
			case Number: out += m_Text; break;
			case String: AppendEscaped(out, m_Text); break;
			case Array:
				if (m_Items.empty()) { out += "[]"; break; }
				out.push_back('[');
				if (pretty) out.push_back('\n');
				for (size_t i = 0; i < m_Items.size(); ++i)
				{
					if (i)
					{
						out.push_back(',');
						if (pretty) out.push_back('\n');
					}
					if (pretty) Indent(out, indent + 1);
					m_Items[i]->Write(out, pretty, indent + 1);
				}
				if (pretty)
				{
					out.push_back('\n');
					Indent(out, indent);
				}
				out.push_back(']');
				break;
			case Object:
			{
				vector<KeyValue> members;
				for (size_t i = 0; i < m_Members.size(); ++i)
				{
					KeyValue item;
					item.Key = m_Members[i].Key;
					item.Value = m_Members[i].Value;
					members.push_back(item);
				}
				sort(members.begin(), members.end(), KeyLess);
				if (members.empty()) { out += "{}"; break; }
				out.push_back('{');
				if (pretty) out.push_back('\n');
				for (size_t i = 0; i < members.size(); ++i)
				{
					if (i)
					{
						out.push_back(',');
						if (pretty) out.push_back('\n');
					}
					if (pretty) Indent(out, indent + 1);
					AppendEscaped(out, members[i].Key);
					out += pretty ? ": " : ":";
					members[i].Value->Write(out, pretty, indent + 1);
				}
				if (pretty)
				{
					out.push_back('\n');
					Indent(out, indent);
				}
				out.push_back('}');
				break;
			}
		}
	}

	string JSON::Stringify(bool pretty) const
	{
		string out;
		Write(out, pretty, 0);
		return out;
	}

	namespace
	{
		struct JSONOwnership
		{
			static void Release(JSON *value) { delete value; }
		};
	}

	JSON *ParseJSONText(const string &text, string *error)
	{
		return SpiralJSONDetail::ReadText<JSON, JSONOwnership>(text, false, error);
	}

	JSON *ParseJSON(const char *fileName, string *error)
	{
		return SpiralJSONDetail::ReadFile<JSON, JSONOwnership>(fileName, false, error);
	}

}
