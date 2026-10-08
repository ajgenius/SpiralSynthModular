// SPDX-License-Identifier: GPL-2.0-or-later
#include "JSON.h"

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

		void AppendUtf8(string &out, unsigned long code)
		{
			if (code <= 0x7f)
				out.push_back(static_cast<char>(code));
			else if (code <= 0x7ff)
			{
				out.push_back(static_cast<char>(0xc0 | (code >> 6)));
				out.push_back(static_cast<char>(0x80 | (code & 0x3f)));
			}
			else if (code <= 0xffff)
			{
				out.push_back(static_cast<char>(0xe0 | (code >> 12)));
				out.push_back(static_cast<char>(0x80 | ((code >> 6) & 0x3f)));
				out.push_back(static_cast<char>(0x80 | (code & 0x3f)));
			}
			else
			{
				out.push_back(static_cast<char>(0xf0 | (code >> 18)));
				out.push_back(static_cast<char>(0x80 | ((code >> 12) & 0x3f)));
				out.push_back(static_cast<char>(0x80 | ((code >> 6) & 0x3f)));
				out.push_back(static_cast<char>(0x80 | (code & 0x3f)));
			}
		}

		const size_t DepthLimit = 64;
		const size_t SizeLimit = 16 * 1024 * 1024;

		// The bytes of one UTF-8 encoded character at p, or 0 when they are
		// not one (overlong forms, surrogates and anything past U+10FFFF
		// included).
		size_t Utf8Length(const unsigned char *p)
		{
			if (p[0] < 0x80) return 1;
			size_t n;
			unsigned long code;
			if ((p[0] & 0xE0) == 0xC0) { n = 2; code = p[0] & 0x1F; }
			else if ((p[0] & 0xF0) == 0xE0) { n = 3; code = p[0] & 0x0F; }
			else if ((p[0] & 0xF8) == 0xF0) { n = 4; code = p[0] & 0x07; }
			else return 0;
			for (size_t i = 1; i < n; ++i)
			{
				if ((p[i] & 0xC0) != 0x80) return 0;
				code = (code << 6) | (p[i] & 0x3F);
			}
			if ((n == 2 && code < 0x80) || (n == 3 && code < 0x800) || (n == 4 && code < 0x10000)) return 0;
			if ((code >= 0xD800 && code <= 0xDFFF) || code > 0x10FFFF) return 0;
			return n;
		}

		class Parser
		{
		public:
			Parser(const char *text, string *error)
				: m_P(text ? text : ""), m_Error(error), m_Depth(0)
			{
			}

			// The first byte not consumed, for the caller to check against the
			// text's length: a NUL byte inside the text ends parsing early.
			const char *End() const { return m_P; }

			JSON *Parse()
			{
				Skip();
				JSON *value = Value();
				if (!value) return NULL;
				Skip();
				if (*m_P)
				{
					delete value;
					Fail("Trailing data after JSON value");
					return NULL;
				}
				return value;
			}

		private:
			const char *m_P;
			string *m_Error;
			size_t m_Depth;

			bool Enter()
			{
				if (++m_Depth > DepthLimit) { Fail("JSON nested too deeply"); return false; }
				return true;
			}

			bool Fail(const char *message)
			{
				if (m_Error && m_Error->empty()) *m_Error = message;
				return false;
			}

			void Skip()
			{
				while (*m_P == ' ' || *m_P == '\t' || *m_P == '\n' || *m_P == '\r')
					++m_P;
			}

			JSON *Value()
			{
				Skip();
				if (!*m_P) { Fail("Unexpected end of JSON"); return NULL; }
				if (*m_P == '{') return Object();
				if (*m_P == '[') return Array();
				if (*m_P == '"') return String();
				if (*m_P == 't' || *m_P == 'f') return Bool();
				if (*m_P == 'n') return Null();
				if (*m_P == '-' || (*m_P >= '0' && *m_P <= '9')) return Number();
				Fail("Invalid JSON value");
				return NULL;
			}

			bool Literal(const char *word)
			{
				for (size_t i = 0; word[i]; ++i)
				{
					if (m_P[i] != word[i]) return Fail("Invalid JSON literal");
				}
				m_P += strlen(word);
				return true;
			}

			JSON *Null()
			{
				if (!Literal("null")) return NULL;
				return JSON::MakeNull();
			}

			JSON *Bool()
			{
				if (*m_P == 't')
				{
					if (!Literal("true")) return NULL;
					return JSON::MakeBoolean(true);
				}
				if (!Literal("false")) return NULL;
				return JSON::MakeBoolean(false);
			}

			JSON *Number()
			{
				const char *start = m_P;
				if (*m_P == '-') ++m_P;
				if (*m_P == '0') ++m_P;
				else if (*m_P >= '1' && *m_P <= '9')
				{
					while (*m_P >= '0' && *m_P <= '9') ++m_P;
				}
				else
				{
					Fail("Invalid JSON number");
					return NULL;
				}
				if (*m_P == '.')
				{
					++m_P;
					if (*m_P < '0' || *m_P > '9') { Fail("Invalid JSON number"); return NULL; }
					while (*m_P >= '0' && *m_P <= '9') ++m_P;
				}
				if (*m_P == 'e' || *m_P == 'E')
				{
					++m_P;
					if (*m_P == '+' || *m_P == '-') ++m_P;
					if (*m_P < '0' || *m_P > '9') { Fail("Invalid JSON number"); return NULL; }
					while (*m_P >= '0' && *m_P <= '9') ++m_P;
				}
				return JSON::MakeNumber(string(start, m_P));
			}

			int Hex()
			{
				char c = *m_P;
				if (c >= '0' && c <= '9') { ++m_P; return c - '0'; }
				if (c >= 'a' && c <= 'f') { ++m_P; return c - 'a' + 10; }
				if (c >= 'A' && c <= 'F') { ++m_P; return c - 'A' + 10; }
				return -1;
			}

			JSON *String()
			{
				if (*m_P != '"') { Fail("Expected string"); return NULL; }
				++m_P;
				string text;
				while (*m_P && *m_P != '"')
				{
					if (static_cast<unsigned char>(*m_P) < 0x20)
					{
						Fail("Unescaped control character in string");
						return NULL;
					}
					if (*m_P != '\\')
					{
						size_t n = Utf8Length(reinterpret_cast<const unsigned char *>(m_P));
						if (!n) { Fail("Invalid UTF-8 in string"); return NULL; }
						text.append(m_P, n);
						m_P += n;
						continue;
					}
					++m_P;
					if (!*m_P) { Fail("Unterminated string escape"); return NULL; }
					char e = *m_P++;
					switch (e)
					{
						case '"': case '\\': case '/': text.push_back(e); break;
						case 'b': text.push_back('\b'); break;
						case 'f': text.push_back('\f'); break;
						case 'n': text.push_back('\n'); break;
						case 'r': text.push_back('\r'); break;
						case 't': text.push_back('\t'); break;
						case 'u':
						{
							unsigned long code = 0;
							for (int i = 0; i < 4; ++i)
							{
								int h = Hex();
								if (h < 0) { Fail("Invalid \\u escape"); return NULL; }
								code = (code << 4) | static_cast<unsigned long>(h);
							}
							if (code >= 0xD800 && code <= 0xDBFF)
							{
								if (m_P[0] == '\\' && m_P[1] == 'u')
								{
									m_P += 2;
									unsigned long low = 0;
									for (int i = 0; i < 4; ++i)
									{
										int h = Hex();
										if (h < 0) { Fail("Invalid \\u escape"); return NULL; }
										low = (low << 4) | static_cast<unsigned long>(h);
									}
									if (low < 0xDC00 || low > 0xDFFF)
									{
										Fail("Invalid surrogate pair");
										return NULL;
									}
									code = 0x10000 + (((code - 0xD800) << 10) | (low - 0xDC00));
								}
							}
							AppendUtf8(text, code);
							break;
						}
						default:
							Fail("Invalid string escape");
							return NULL;
					}
				}
				if (*m_P != '"') { Fail("Unterminated string"); return NULL; }
				++m_P;
				return JSON::MakeString(text);
			}

			JSON *Array()
			{
				if (*m_P != '[') { Fail("Expected array"); return NULL; }
				if (!Enter()) return NULL;
				++m_P;
				JSON *array = JSON::MakeArray();
				Skip();
				if (*m_P == ']') { ++m_P; --m_Depth; return array; }
				while (*m_P)
				{
					JSON *item = Value();
					if (!item) { delete array; return NULL; }
					array->AppendOwned(item);
					Skip();
					if (*m_P == ',') { ++m_P; Skip(); continue; }
					if (*m_P == ']') { ++m_P; --m_Depth; return array; }
					break;
				}
				delete array;
				Fail("Unterminated array");
				return NULL;
			}

			JSON *Object()
			{
				if (*m_P != '{') { Fail("Expected object"); return NULL; }
				if (!Enter()) return NULL;
				++m_P;
				JSON *object = JSON::MakeObject();
				Skip();
				if (*m_P == '}') { ++m_P; --m_Depth; return object; }
				while (*m_P)
				{
					Skip();
					JSON *key = String();
					if (!key) { delete object; return NULL; }
					string name = key->Text();
					delete key;
					Skip();
					if (*m_P != ':') { delete object; Fail("Expected ':'"); return NULL; }
					++m_P;
					if (object->Get(name)) { delete object; Fail(("Duplicate key \"" + name + "\"").c_str()); return NULL; }
					JSON *item = Value();
					if (!item) { delete object; return NULL; }
					object->SetOwned(name, item);
					Skip();
					if (*m_P == ',') { ++m_P; continue; }
					if (*m_P == '}') { ++m_P; --m_Depth; return object; }
					break;
				}
				delete object;
				Fail("Unterminated object");
				return NULL;
			}
		};
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
		if (m_Type != Object || !key) return NULL;
		for (size_t i = 0; i < m_Members.size(); ++i)
			if (m_Members[i].Key == key) return m_Members[i].Value;
		return NULL;
	}

	JSON *JSON::Get(const char *key)
	{
		if (m_Type != Object || !key) return NULL;
		for (size_t i = 0; i < m_Members.size(); ++i)
			if (m_Members[i].Key == key) return m_Members[i].Value;
		return NULL;
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

	JSON *ParseJSONText(const string &text, string *error)
	{
		if (error) error->clear();
		if (text.size() > SizeLimit)
		{
			if (error) *error = "JSON text exceeds 16 MiB";
			return NULL;
		}
		string fail;
		Parser parser(text.c_str(), &fail);
		JSON *value = parser.Parse();
		if (value && parser.End() != text.c_str() + text.size())
		{
			delete value;
			value = NULL;
			fail = "NUL byte in JSON text";
		}
		if (!value && error) *error = fail.empty() ? "Invalid JSON" : fail;
		return value;
	}

	JSON *ParseJSON(const char *fileName, string *error)
	{
		if (error) error->clear();
		if (!fileName)
		{
			if (error) *error = "No JSON file name";
			return NULL;
		}
		ifstream in(fileName, ios::in | ios::binary);
		if (!in)
		{
			if (error) *error = string("Cannot open ") + fileName;
			return NULL;
		}
		string text;
		char buffer[65536];
		while (in.read(buffer, sizeof buffer) || in.gcount())
		{
			text.append(buffer, static_cast<size_t>(in.gcount()));
			if (text.size() > SizeLimit)
			{
				if (error) *error = string(fileName) + ": JSON file exceeds 16 MiB";
				return NULL;
			}
		}
		JSON *value = ParseJSONText(text, error);
		if (!value && error) *error = string(fileName) + ": " + *error;
		return value;
	}

}
