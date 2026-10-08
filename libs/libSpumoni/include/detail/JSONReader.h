// SPDX-License-Identifier: GPL-2.0-or-later
// Shared C++03 reader extracted from public libSpumoni/src/JSON.cpp.
// Origin: ajgenius/spiral-synth-modular, feature/patch-legacy-name at a98039c.
// JSON supplies Make*, Get, Text, SetOwned and AppendOwned; Ownership::Release
// disposes a root. Each tree keeps its native ownership without an intermediate DOM.
#ifndef SPIRAL_JSON_READER_H
#define SPIRAL_JSON_READER_H

#include <cstddef>
#include <cstring>
#include <fstream>
#include <string>

namespace SpiralJSONDetail
{
	using std::string;
	inline void AppendUtf8(string &out, unsigned long code)
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
	inline size_t Utf8Length(const unsigned char *p, size_t remaining)
	{
		if (p[0] < 0x80) return 1;
		size_t n;
		unsigned long code;
		if ((p[0] & 0xE0) == 0xC0) { n = 2; code = p[0] & 0x1F; }
		else if ((p[0] & 0xF0) == 0xE0) { n = 3; code = p[0] & 0x0F; }
		else if ((p[0] & 0xF8) == 0xF0) { n = 4; code = p[0] & 0x07; }
		else return 0;
		if (n > remaining) return 0;
		for (size_t i = 1; i < n; ++i)
		{
			if ((p[i] & 0xC0) != 0x80) return 0;
			code = (code << 6) | (p[i] & 0x3F);
		}
		if ((n == 2 && code < 0x80) || (n == 3 && code < 0x800) || (n == 4 && code < 0x10000)) return 0;
		if ((code >= 0xD800 && code <= 0xDFFF) || code > 0x10FFFF) return 0;
		return n;
	}

	template<class JSON, class Ownership> class Reader
	{
	public:
		Reader(const char *text, size_t length, bool fold, string *error)
			: m_P(text), m_End(text + length), m_Error(error), m_Depth(0), m_Fold(fold)
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
				Ownership::Release(value);
				Fail("Trailing data after JSON value");
				return NULL;
			}
			return value;
		}

	private:
		const char *m_P;
		const char *m_End;
		string *m_Error;
		size_t m_Depth;
		bool m_Fold;

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
					size_t n = Utf8Length(reinterpret_cast<const unsigned char *>(m_P), m_End - m_P);
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
							else
							{
								Fail("Missing low surrogate");
								return NULL;
							}
						}
						else if (code >= 0xDC00 && code <= 0xDFFF)
						{
							Fail("Unpaired low surrogate");
							return NULL;
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
				if (!item) { Ownership::Release(array); return NULL; }
				array->AppendOwned(item);
				Skip();
				if (*m_P == ',') { ++m_P; Skip(); continue; }
				if (*m_P == ']') { ++m_P; --m_Depth; return array; }
				break;
			}
			Ownership::Release(array);
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
				if (!key) { Ownership::Release(object); return NULL; }
				string name = key->Text();
				if (m_Fold)
					for (size_t i = 0; i < name.size(); ++i)
						if (name[i] >= 'A' && name[i] <= 'Z')
							name[i] += 'a' - 'A';

				Ownership::Release(key);
				Skip();
				if (*m_P != ':') { Ownership::Release(object); Fail("Expected ':'"); return NULL; }
				++m_P;
				if (object->Get(name)) { Ownership::Release(object); Fail(("Duplicate key \"" + name + "\"").c_str()); return NULL; }
				JSON *item = Value();
				if (!item) { Ownership::Release(object); return NULL; }
				object->SetOwned(name, item);
				Skip();
				if (*m_P == ',') { ++m_P; continue; }
				if (*m_P == '}') { ++m_P; --m_Depth; return object; }
				break;
			}
			Ownership::Release(object);
			Fail("Unterminated object");
			return NULL;
		}
	};

	template<class JSON, class Ownership>
	JSON *ReadText(const string &text, bool fold, string *error)
	{
		if (error) error->clear();
		if (text.size() > SizeLimit)
		{
			if (error) *error = "JSON text exceeds 16 MiB";
			return NULL;
		}
		string fail;
		Reader<JSON, Ownership> parser(text.c_str(), text.size(), fold, &fail);
		JSON *value = parser.Parse();
		if (value && parser.End() != text.c_str() + text.size())
		{
			Ownership::Release(value);
			value = NULL;
			fail = "NUL byte in JSON text";
		}
		if (!value && error) *error = fail.empty() ? "Invalid JSON" : fail;
		return value;
	}

	template<class JSON, class Ownership>
	JSON *ReadFile(const char *fileName, bool fold, string *error)
	{
		if (error) error->clear();
		if (!fileName)
		{
			if (error) *error = "No JSON file name";
			return NULL;
		}
		std::ifstream in(fileName, std::ios::in | std::ios::binary);
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
		if (in.bad())
		{
			if (error) *error = string(fileName) + ": Error reading JSON file";
			return NULL;
		}

		JSON *value = ReadText<JSON, Ownership>(text, fold, error);
		if (!value && error) *error = string(fileName) + ": " + *error;
		return value;
	}

}
#endif
