// Manifest reading is independent of GUI libraries and never opens a module.
// GPL-2.0-or-later.
#include "PluginManifest.h"
#include "JSONParser.h"
#include <climits>
using SpiralJSON::JSONValue;

namespace {
bool text(const JSONValue *object, const char *key, std::string &out) {
	const JSONValue *v = object ? object->Get(key) : NULL;
	if (!v || v->GetType() != JSONValue::String || v->Text().empty() ||
	    v->Text().find('\0') != std::string::npos)
		return false;
	out = v->Text();
	return true;
}
bool basename(const std::string &name) {
	return !name.empty() && name != "." && name != ".." &&
	       name.find('/') == std::string::npos && name.find('\\') == std::string::npos;
}
}

bool PluginManifest::Read(const std::string &path, std::string &error)
{
	*this = PluginManifest();
	JSONValue *root = SpiralJSON::ParseJSON(path.c_str(), false, &error);
	if (!root)
		return false;
	const JSONValue *schema = root->Get("schema_version"), *identifier = root->Get("id");
	long s = 0, i = 0;
	bool reference = text(root, "registration", registration) && registration == "reference";
	bool validID = identifier && identifier->Integer(i) && i >= 0 && i <= INT_MAX;
	if (!validID && reference && identifier && identifier->GetType() == JSONValue::Null) {
		validID = true;
		i = -1;
	}
	bool ok = root->GetType() == JSONValue::Object && schema && schema->Integer(s) && s == 1 && validID &&
		  text(root, "name", name) && text(root, "type", type) &&
		  (type == "dsp" || type == "audio-driver" || type == "gui") &&
		  text(root, "category", category) && text(root, "version", version) &&
		  text(root, "registration", registration);
	id = static_cast<int>(i);
	const JSONValue *host = root->Get("host");
	std::string hostName;
	ok = ok && host && host->GetType() == JSONValue::Object && text(host, "name", hostName) &&
	     hostName == "SpiralSynthModular" && text(host, "version", hostVersion) && text(host, "abi", hostABI);
	const JSONValue *authorList = root->Get("authors");
	ok = ok && authorList && authorList->GetType() == JSONValue::Array && authorList->Size() > 0;
	if (ok)
		for (size_t a = 0; a < authorList->Size(); ++a) {
			const JSONValue *author = authorList->At(a);
			if (!author || author->GetType() != JSONValue::String || author->Text().empty() ||
			    author->Text().find('\0') != std::string::npos) {
				ok = false;
				break;
			}
			authors.push_back(author->Text());
		}
	if (type == "gui") {
		const JSONValue *gui = root->Get("gui_stack");
		ok = ok && gui && gui->GetType() == JSONValue::Object && text(gui, "name", guiName) &&
		     text(gui, "abi", guiABI) && text(gui, "version", guiVersion);
		ok = ok && (registration == "builtin" || registration == "module" || registration == "reference");
	} else
		ok = ok && (registration == "module" || reference) && !root->Get("gui_stack");
	if (registration == "module")
		ok = ok && text(root, "module", module) && basename(module);
	else
		ok = ok && !root->Get("module");
	delete root;
	if (!ok)
		error = "Invalid plugin manifest: required fields, types, or schema_version";
	else
		error.clear();
	return ok;
}

bool PluginManifest::Compatible(const std::string &host, const std::string &abi,
			       const std::string &gui, const std::string &guiAbi,
			       std::string &error) const
{
	if (hostVersion != host || hostABI != abi) {
		error = "Plugin requires a different SSM version or host ABI";
		return false;
	}
	if (type == "gui" && (guiName != gui || guiABI != guiAbi)) {
		error = "Plugin requires a different GUI stack";
		return false;
	}
	if (registration == "reference") {
		error = "Reference-only plugin has no compatible built implementation";
		return false;
	}
	error.clear();
	return true;
}
