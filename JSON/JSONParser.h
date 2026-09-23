// YAJL JSON reader. GPL-2.0-or-later.
#ifndef SPIRAL_JSON_PARSER_H
#define SPIRAL_JSON_PARSER_H
#include <cstddef>
#include <map>
#include <string>
#include <vector>

namespace SpiralJSON {
// Delete the returned root once. Get and At are borrowed.
class JSONValue {
public:
	enum Type { Null, Boolean, Number, String, Array, Object };
	~JSONValue();
	Type GetType() const { return mType; }
	const std::string &Text() const { return mText; }
	bool AsBool() const { return mBool; } // X11's Bool macro breaks Bool() under FLTK
	bool Integer(long &value) const;
	const JSONValue *Get(const std::string &key) const;
	const JSONValue *At(size_t index) const;
	size_t Size() const;
	std::vector<std::string> Keys() const;
private:
	explicit JSONValue(Type type);
	JSONValue(const JSONValue &);
	JSONValue &operator=(const JSONValue &);
	Type mType;
	bool mBool;
	std::string mText;
	std::map<std::string, JSONValue *> mMembers;
	std::vector<JSONValue *> mElements;
	friend class JSONParser;
};

JSONValue *ParseJSON(const char *fileName, bool caseInsensitive = false,
		     std::string *error = NULL);
}
#endif
