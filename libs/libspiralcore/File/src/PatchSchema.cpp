// SPDX-License-Identifier: GPL-2.0-or-later
#include "PatchFile.h"
#include "FileVersion.h"
#include "JSON.h"
#include <sstream>

namespace Spiral
{
	namespace File
	{
		namespace
		{
			typedef Spumoni::JSON J;
			using namespace Core;

			J *PortList(const std::vector<PortName> &ports)
			{
				J *list = J::MakeArray();
				for (size_t i = 0; i < ports.size(); ++i)
				{
					J *port = J::MakeObject();
					port->SetOwned("Name", J::MakeString(ports[i].Name));
					if (!ports[i].Type.empty())
						port->SetOwned("Type", J::MakeString(ports[i].Type));
					list->AppendOwned(port);
				}
				return list;
			}

			void Gather(const PatchDescription &patch, J &definitions)
			{
				for (size_t o = 0; o < patch.Objects.size(); ++o)
					for (size_t d = 0; d < patch.Objects[o].Devices.size(); ++d)
					{
						const DeviceDescription &device = patch.Objects[o].Devices[d];
						const std::string name = device.Value("name");
						J *entry = definitions.Get(name);
						if (!entry)
						{
							entry = J::MakeObject();
							J *native = J::MakeObject();
							native->SetOwned("Name", J::MakeString(name));
							native->SetOwned("ID", J::MakeString(device.IsSubPatch() ? "-1" : device.Value("id")));
							if (!device.Value("version").empty() && device.Value("version") != "0")
								native->SetOwned("Version", J::MakeString(device.Value("version")));
							entry->SetOwned("NativePlugin", native);
							entry->SetOwned("Fields", J::MakeObject());
							definitions.SetOwned(name, entry);
						}
						for (size_t i = 0; i < device.Properties.size(); ++i)
						{
							const PropertyDescription &prop = device.Properties[i];
							J *field = J::MakeObject();
							field->SetOwned("Type", J::MakeString(prop.Type));
							entry->Get("Fields")->SetOwned(prop.Name, field);
						}
						if (!entry->Get("Ports") && (!device.Inputs.empty() || !device.Outputs.empty()))
						{
							J *ports = J::MakeObject();
							ports->SetOwned("Inputs", PortList(device.Inputs));
							ports->SetOwned("Outputs", PortList(device.Outputs));
							entry->SetOwned("Ports", ports);
						}
						if (device.Inner())
							Gather(*device.Inner(), definitions);
					}
			}

			void SupplyPorts(const J *list, std::vector<PortName> &ports)
			{
				if (!list || list->GetType() != J::Array)
					return;
				for (size_t i = 0; i < list->Size(); ++i)
				{
					const J *spec = list->At(i);
					if (!spec || spec->GetType() != J::Object)
						continue;
					if (ports.size() <= i)
						ports.resize(i + 1);
					const J *name = spec->Get("Name"), *type = spec->Get("Type");
					if (ports[i].Name.empty() && name && name->GetType() == J::String)
						ports[i].Name = name->Text();
					if (ports[i].Type.empty() && type && type->GetType() == J::String)
						ports[i].Type = type->Text();
				}
			}

			void Supply(const J &defs, PatchDescription &patch)
			{
				for (size_t o = 0; o < patch.Objects.size(); ++o)
					for (size_t d = 0; d < patch.Objects[o].Devices.size(); ++d)
					{
						DeviceDescription &device = patch.Objects[o].Devices[d];
						const J *entry = defs.Get(device.Value("name"));
						const J *ports = entry ? entry->Get("Ports") : NULL;
						if (ports)
						{
							SupplyPorts(ports->Get("Inputs"), device.Inputs);
							SupplyPorts(ports->Get("Outputs"), device.Outputs);
						}
						if (device.Inner())
							Supply(defs, *device.Inner());
					}
			}
		}

		void PatchFile::ApplySchema(const std::string &text, Core::PatchDescription &definition)
		{
			if (text.empty())
				return;
			Spumoni::JSONOwner root(Spumoni::ParseJSONText(text));
			const J *format = root.get() ? root->Get("format") : NULL;
			const J *defs = root.get() ? root->Get("DeviceDefinitions") : NULL;
			if (format && format->Text() == Version::SchemaHeader() && defs && defs->GetType() == J::Object)
				Supply(*defs, definition);
		}

		std::string PatchFile::Schema(const Core::PatchDescription &definition)
		{
			Spumoni::JSONOwner root(J::MakeObject());
			root->SetOwned("format", J::MakeString(Version::SchemaHeader()));
			root->SetOwned("Type", J::MakeString("Contract"));
			std::ostringstream version;
			version << Version::Patch;
			root->SetOwned("PatchFileVersion", J::MakeNumber(version.str()));
			root->SetOwned("Coverage", J::MakeString("Device property shapes and ports as the saved patch knew them."));
			J *policy = J::MakeObject();
			policy->SetOwned("File", J::MakeString("regenerate-on-save"));
			policy->SetOwned("Operation", J::MakeString("replace"));
			root->SetOwned("UpdatePolicy", policy);

			J *propertyTypes = J::MakeObject();
			static const char *builtin[] = {
				"string", "note", "complex", "ToggleProperty",
				"SignedProperty", "FloatProperty", "UnsignedProperty", NULL
			};
			for (size_t i = 0; builtin[i]; ++i)
				propertyTypes->SetOwned(builtin[i], J::MakeObject());
			root->SetOwned("PropertyTypes", propertyTypes);

			J *portTypes = J::MakeObject();
			portTypes->SetOwned("AudioPort", J::MakeObject());
			portTypes->SetOwned("ControlPort", J::MakeObject());
			portTypes->SetOwned("MidiEventPort", J::MakeObject());
			portTypes->SetOwned("MusicalEventPort", J::MakeObject());
			root->SetOwned("PortTypes", portTypes);

			J *structure = J::MakeObject();
			structure->SetOwned("Document", J::MakeObject());
			structure->SetOwned("Patch", J::MakeObject());
			structure->SetOwned("Device", J::MakeObject());
			structure->SetOwned("Wire", J::MakeObject());
			root->SetOwned("Structure", structure);

			J *defs = J::MakeObject();
			Gather(definition, *defs);
			root->SetOwned("DeviceDefinitions", defs);

			return root->Stringify(true);
		}
	}
}
