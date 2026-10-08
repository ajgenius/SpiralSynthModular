// SPDX-License-Identifier: GPL-2.0-or-later
// The JSON form of a description read back: an object whose "values" and
// "between" are arrays of strings, one more gap than values. Anything else
// is refused with a reason; a value is whatever string the writer put there.
#include "Description.h"
#include "JSON.h"
#include <memory>
#include <sstream>

namespace spiralcore
{
	static bool Strings(const Spumoni::JSON *list, const char *name,
	                    std::vector<std::string> &out, std::string &error)
	{
		if (!list || list->GetType() != Spumoni::JSON::Array)
		{
			error = std::string("\"") + name + "\" is not an array";
			return false;
		}
		for (size_t i = 0; i < list->Size(); ++i)
		{
			const Spumoni::JSON *item = list->At(i);
			if (!item || item->GetType() != Spumoni::JSON::String)
			{
				std::ostringstream where;
				where << "\"" << name << "\"[" << i << "] is not a string";
				error = where.str();
				return false;
			}
			out.push_back(item->Text());
		}
		return true;
	}

	bool Description::FromJSON(const std::string &text, Description &out, std::string &error)
	{
		std::auto_ptr<Spumoni::JSON> root(Spumoni::ParseJSONText(text, &error));
		if (!root.get())
			return false;
		if (root->GetType() != Spumoni::JSON::Object)
		{
			error = "a description is an object";
			return false;
		}
		Description d;
		d.m_Between.clear();
		if (!Strings(root->Get("values"), "values", d.m_Values, error) ||
		    !Strings(root->Get("between"), "between", d.m_Between, error))
			return false;
		if (d.m_Between.size() != d.m_Values.size() + 1)
		{
			error = "\"between\" needs one entry more than \"values\"";
			return false;
		}
		out = d;
		return true;
	}
}
