// Manifest reading is independent of GUI libraries and never opens a module.
// GPL-2.0-or-later.
#include "PluginManifest.h"
#include "JSON.h"
#include <climits>
#include <cstdlib>
typedef Spumoni::JSON JSONValue;

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
bool number(const JSONValue *v, double &out) {
	if (!v || v->GetType() != JSONValue::Number)
		return false;
	out = std::strtod(v->Text().c_str(), NULL);
	return true;
}
bool number(const JSONValue *object, const char *key, double &out) {
	const JSONValue *v = object ? object->Get(key) : NULL;
	if (!v)
		return true;
	if (v->GetType() == JSONValue::Boolean) {
		out = v->AsBool() ? 1 : 0;
		return true;
	}
	return number(v, out);
}
}

static bool passthrough(const JSONValue *root, PluginManifest &m)
{
	const JSONValue *ports = root->Get("ports"), *properties = root->Get("properties");
	if (!ports || ports->GetType() != JSONValue::Array || ports->Size() == 0)
		return false;
	for (size_t i = 0; i < ports->Size(); ++i) {
		PluginManifest::Port port;
		const JSONValue *p = ports->At(i);
		if (!text(p, "input", port.input) || !text(p, "output", port.output))
			return false;
		// How the testing tree's host shadows the port for its panels: the
		// latest block, every block in turn, both or neither. A panel here
		// asks the device for a feed whatever this says, so each is read
		// and the one manifest serves both trees.
		if (p->Get("shadow") && (!text(p, "shadow", port.shadow) ||
		    (port.shadow != "snapshot" && port.shadow != "ring" && port.shadow != "both" && port.shadow != "none")))
			return false;
		m.ports.push_back(port);
	}
	if (properties && properties->GetType() != JSONValue::Array)
		return false;
	for (size_t i = 0; properties && i < properties->Size(); ++i) {
		PluginManifest::Property property;
		const JSONValue *p = properties->At(i);
		if (!text(p, "name", property.name) || !text(p, "type", property.type) ||
		    (property.type != "toggle" && property.type != "float" && property.type != "unsigned"))
			return false;
		text(p, "hint", property.hint);
		if (property.type == "toggle")
			property.max = 1;
		if (!number(p, "default", property.value) || !number(p, "min", property.min) ||
		    !number(p, "max", property.max) || !number(p, "step", property.step))
			return false;
		m.properties.push_back(property);
	}
	const JSONValue *size = root->Get("size");
	double w = 0, h = 0;
	if (size && (size->GetType() != JSONValue::Array || size->Size() != 2 ||
		     !number(size->At(0), w) || !number(size->At(1), h)))
		return false;
	double version = 0;
	if (!number(root, "state_version", version) || version < 0 || version != static_cast<int>(version))
		return false;
	m.stateVersion = static_cast<int>(version);
	m.width = static_cast<int>(w);
	m.height = static_cast<int>(h);
	return true;
}

bool PluginManifest::Read(const std::string &path, std::string &error)
{
	*this = PluginManifest();
	JSONValue *root = Spumoni::ParseJSON(path.c_str(), &error);
	if (!root)
		return false;
	return Take(root, error);
}

bool PluginManifest::Parse(const std::string &text, std::string &error)
{
	*this = PluginManifest();
	JSONValue *root = Spumoni::ParseJSONText(text, &error);
	if (!root)
		return false;
	return Take(root, error);
}

bool PluginManifest::Take(Spumoni::JSON *root, std::string &error)
{
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
	} else {
		ok = ok && (registration == "module" || registration == "builtin" || reference) && !root->Get("gui_stack");
		if (ok && registration == "builtin")
			ok = text(root, "builtin", builtin) &&
			     (builtin == "core" || (builtin == "passthrough" && passthrough(root, *this)));
	}
	if (registration == "module")
		ok = ok && text(root, "module", module) && basename(module);
	else
		ok = ok && !root->Get("module");
	const JSONValue *panel = root->Get("ui");
	if (panel && panel->GetType() == JSONValue::Object)
		ui = panel->Stringify(false);
	else if (panel)
		ok = false;
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
