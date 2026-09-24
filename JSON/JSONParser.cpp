// YAJL JSON reader. Ordinary C++ ownership. GPL-2.0-or-later.
#include "JSONParser.h"
#include <yajl/yajl_parse.h>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace SpiralJSON
{
	JSONValue::JSONValue(Type type):
	    mType(type),
	    mBool(false)
	{
	}

	JSONValue::~JSONValue()
	{
		for (std::map<std::string, JSONValue *>::iterator i = mMembers.begin(); i != mMembers.end(); ++i)
			delete i->second;

		for (size_t i = 0; i < mElements.size(); ++i)
			delete mElements[i];
	}

	bool JSONValue::Integer(long &value) const
	{
		if (mType != Number || mText.empty())
			return false;

		char *end = NULL;

		errno = 0;

		long result = strtol(mText.c_str(), &end, 10);

		if (errno == ERANGE || end != mText.c_str() + mText.size())
			return false;

		value = result;

		return true;
	}

	const JSONValue *JSONValue::Get(const std::string &key) const
	{
		std::map<std::string, JSONValue *>::const_iterator i = mMembers.find(key);

		return i == mMembers.end() ? NULL : i->second;
	}

	const JSONValue *JSONValue::At(size_t index) const
	{
		return index < mElements.size() ? mElements[index] : NULL;
	}

	size_t JSONValue::Size() const
	{
		return mType == Object ? mMembers.size() : mElements.size();
	}

	std::vector<std::string> JSONValue::Keys() const
	{
		std::vector<std::string> keys;

		if (mType != Object)
			return keys;

		keys.reserve(mMembers.size());

		for (std::map<std::string, JSONValue *>::const_iterator i = mMembers.begin(); i != mMembers.end(); ++i)
			keys.push_back(i->first);

		return keys;
	}

	class JSONParser
	{
		struct Frame
		{
			JSONValue *container;
			std::string key;
			bool hasKey;

			explicit Frame(JSONValue *value):
			    container(value),
			    hasKey(false)
			{
			}
		};

		JSONValue *root;
		std::vector<Frame> stack;
		bool folding;
		std::string failure;
		yajl_handle handle;

		int Append(JSONValue *value)
		{
			if (stack.empty())
			{
				if (root)
				{
					delete value;
					failure = "Multiple JSON roots";

					return 0;
				}

				root = value;
			}
			else
			{
				Frame &frame = stack.back();

				if (frame.container->GetType() == JSONValue::Object)
				{
					if (!frame.hasKey)
					{
						delete value;
						failure = "Missing JSON object key";

						return 0;
					}

					std::map<std::string, JSONValue *>::iterator existing =
					    frame.container->mMembers.find(frame.key);

					if (existing != frame.container->mMembers.end())
						delete existing->second;

					frame.container->mMembers[frame.key] = value;
					frame.hasKey = false;
				}
				else
					frame.container->mElements.push_back(value);
			}

			return 1;
		}

		int Push(JSONValue::Type type)
		{
			JSONValue *value = new JSONValue(type);

			if (!Append(value))
				return 0;

			stack.push_back(Frame(value));

			return 1;
		}

		static int Null(void *c)
		{
			return static_cast<JSONParser *>(c)->Append(new JSONValue(JSONValue::Null));
		}

		static int Boolean(void *c, int b)
		{
			JSONValue *v = new JSONValue(JSONValue::Boolean);

			v->mBool = b != 0;

			return static_cast<JSONParser *>(c)->Append(v);
		}

		static int Number(void *c, const char *s, size_t n)
		{
			JSONValue *v = new JSONValue(JSONValue::Number);

			v->mText.assign(s, n);

			return static_cast<JSONParser *>(c)->Append(v);
		}

		static int String(void *c, const unsigned char *s, size_t n)
		{
			JSONValue *v = new JSONValue(JSONValue::String);

			v->mText.assign(reinterpret_cast<const char *>(s), n);

			return static_cast<JSONParser *>(c)->Append(v);
		}

		static int Map(void *c)
		{
			return static_cast<JSONParser *>(c)->Push(JSONValue::Object);
		}

		static int Array(void *c)
		{
			return static_cast<JSONParser *>(c)->Push(JSONValue::Array);
		}

		static int Key(void *c, const unsigned char *s, size_t n)
		{
			JSONParser *p = static_cast<JSONParser *>(c);

			if (p->stack.empty())
				return 0;

			Frame &frame = p->stack.back();

			frame.key.assign(reinterpret_cast<const char *>(s), n);

			if (p->folding)
				for (size_t i = 0; i < n; ++i)
					if (frame.key[i] >= 'A' && frame.key[i] <= 'Z')
						frame.key[i] += 'a' - 'A';

			frame.hasKey = true;

			return 1;
		}

		static int Pop(void *c)
		{
			JSONParser *p = static_cast<JSONParser *>(c);

			if (p->stack.empty())
				return 0;

			p->stack.pop_back();

			return 1;
		}

	public:
		explicit JSONParser(bool fold):
		    root(NULL),
		    folding(fold)
		{
			static const yajl_callbacks callbacks = {Null, Boolean, NULL, NULL, Number, String,
								 Map, Key, Pop, Array, Pop};

			handle = yajl_alloc(&callbacks, NULL, this);
		}

		~JSONParser()
		{
			if (handle)
				yajl_free(handle);

			delete root;
		}

		JSONValue *Finish(yajl_status status, const unsigned char *buffer, size_t length, std::string *error)
		{
			if (status == yajl_status_ok && failure.empty())
				status = yajl_complete_parse(handle);

			if (status != yajl_status_ok || !failure.empty() || !root || !stack.empty())
			{
				if (error)
				{
					if (!failure.empty())
						*error = failure;
					else
					{
						unsigned char *message = yajl_get_error(handle, 0, buffer, length);

						*error = message ? reinterpret_cast<char *>(message) : "Invalid JSON";

						if (message)
							yajl_free_error(handle, message);
					}
				}

				return NULL;
			}

			JSONValue *result = root;

			root = NULL;

			return result;
		}

		JSONValue *Parse(const char *name, std::string *error)
		{
			if (error)
				error->clear();

			FILE *file = name ? fopen(name, "rb") : NULL;

			if (!file || !handle)
			{
				if (file)
					fclose(file);

				if (error)
					*error = "Cannot open JSON file or allocate parser";

				return NULL;
			}

			unsigned char buffer[4096];
			size_t length = 0;
			yajl_status status = yajl_status_ok;

			while ((length = fread(buffer, 1, sizeof(buffer), file)) != 0)
			{
				status = yajl_parse(handle, buffer, length);

				if (status != yajl_status_ok)
					break;
			}

			if (ferror(file))
				failure = "Error reading JSON file";

			fclose(file);

			return Finish(status, buffer, length, error);
		}
	};

	JSONValue *ParseJSON(const char *fileName, bool caseInsensitive, std::string *error)
	{
		JSONParser parser(caseInsensitive);

		return parser.Parse(fileName, error);
	}
}
