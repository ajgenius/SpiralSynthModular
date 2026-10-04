// SPDX-License-Identifier: GPL-2.0-or-later
// A small JSON DOM for Spumoni manifests. C++03. Numbers keep the lexeme
// they were parsed with; object keys are sorted when stringified.
#ifndef SPUMONI_JSON_H
#define SPUMONI_JSON_H

#include <memory>
#include <string>
#include <vector>

namespace Spumoni
{

	class JSON
	{
	public:
		enum Type
		{
			Null,
			Boolean,
			Number,
			String,
			Array,
			Object
		};

		static JSON *MakeNull();
		static JSON *MakeBool(bool value);
		static JSON *MakeNumber(const std::string &lexeme);
		static JSON *MakeString(const std::string &text);
		static JSON *MakeArray();
		static JSON *MakeObject();

		~JSON();
		JSON *Duplicate() const;

		Type GetType() const { return m_Type; }
		bool Bool() const { return m_Bool; }
		const std::string &Text() const { return m_Text; }
		size_t Size() const;
		const JSON *At(size_t index) const;
		JSON *At(size_t index);
		const JSON *Get(const char *key) const;
		JSON *Get(const char *key);
		std::vector<std::string> Keys() const;

		// Takes ownership of value, including when this is not an object
		// or array (the value is then deleted).
		void SetOwned(const std::string &key, JSON *value);
		void AppendOwned(JSON *value);

		// Keys are always sorted. pretty adds newlines and two-space indents.
		std::string Stringify(bool pretty) const;

	private:
		struct Member
		{
			std::string Key;
			JSON *Value;
			Member() : Value(NULL) {}
		};

		explicit JSON(Type type);
		JSON(const JSON &);
		JSON &operator=(const JSON &);

		void Write(std::string &out, bool pretty, int indent) const;

		Type m_Type;
		bool m_Bool;
		std::string m_Text;
		std::vector<JSON *> m_Items;
		std::vector<Member> m_Members;
	};

	class JSONOwner
	{
	public:
		explicit JSONOwner(JSON *value) : m_Value(value) {}
		JSON *get() const { return m_Value.get(); }
		JSON &operator*() const { return *m_Value; }
		JSON *operator->() const { return m_Value.get(); }

	private:
		JSONOwner(const JSONOwner &);
		JSONOwner &operator=(const JSONOwner &);
		std::auto_ptr<JSON> m_Value;
	};

	// Parse one JSON value. Trailing whitespace is allowed; anything else
	// fails. On failure returns NULL and, when error is not NULL, sets it.
	JSON *ParseJSON(const char *text, std::string *error);

}

#endif
