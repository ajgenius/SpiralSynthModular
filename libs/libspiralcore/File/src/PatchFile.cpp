// SPDX-License-Identifier: GPL-2.0-or-later
/*  SpiralSynthModular
 *  Copyright (C) 2026 ajgenius
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include "PatchFile.h"
#include "FileVersion.h"
#include "JSON.h"
#include "Compatibility.h"

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <vector>

namespace Spiral
{
	namespace File
	{
		using Core::PropertyDescription;
		using Core::DeviceDescription;
		using Core::SubPatchConfig;
		using Core::WireDescription;
		using Core::ObjectDescription;
		using Core::StateValue;
		using Core::PortBind;
		using Core::PortName;
		using Core::PatchDescription;
		using Core::MidiDeviceDescription;
		using Core::MidiBindingDescription;
		using Core::Description;

		namespace
		{
			typedef Spumoni::JSON J;

			std::string ScalarText(const J *v)
			{
				if (!v)
					return std::string();
				switch (v->GetType())
				{
				case J::String:
				case J::Number:
					return v->Text();
				case J::Boolean:
					return v->AsBool() ? "true" : "false";
				default:
					return std::string();
				}
			}

			bool IsScalar(const J *v)
			{
				return v && v->GetType() != J::Object && v->GetType() != J::Array;
			}

			bool LooksNumeric(const std::string &text)
			{
				if (text.empty())
					return false;
				char *end = NULL;
				errno = 0;
				const double value = std::strtod(text.c_str(), &end);
				return errno == 0 && end && *end == 0 && text[0] != ' ' && SSMCompat::IsFinite(value);
			}

			long LongOr(const J *v, long fallback)
			{
				long value = 0;
				if (v && v->Integer(value))
					return value;
				const std::string text = ScalarText(v);
				char *end = NULL;
				value = std::strtol(text.c_str(), &end, 10);
				return (!text.empty() && end && *end == 0) ? value : fallback;
			}

			std::string Decimal(long n)
			{
				char text[32];
				std::snprintf(text, sizeof(text), "%ld", n);
				return text;
			}

			J *MakeScalar(const std::string &text, bool asString)
			{
				if (!asString)
				{
					if (text == "true" || text == "false")
						return J::MakeBoolean(text == "true");
					if (LooksNumeric(text))
						return J::MakeNumber(text);
				}
				return J::MakeString(text);
			}

			void ReadProperty(const J &node, PropertyDescription &out)
			{
				out.Name = ScalarText(node.Get("name"));
				out.Type = ScalarText(node.Get("type"));
				out.PropertyClassName = ScalarText(node.Get("class"));
				std::vector<std::string> keys = node.Keys();
				for (size_t i = 0; i < keys.size(); ++i)
				{
					const std::string &key = keys[i];
					if (key == "name" || key == "type" || key == "class" || key == "values")
						continue;
					const J *v = node.Get(key);
					if (IsScalar(v) && v->GetType() != J::Null)
						out.SetValue(key, ScalarText(v));
				}
				const J *children = node.Get("values");
				if (children && children->GetType() == J::Array)
				{
					for (size_t i = 0; i < children->Size(); ++i)
					{
						const J *child = children->At(i);
						if (child && child->GetType() == J::Object)
							ReadProperty(*child, out.AddChild());
					}
				}
			}

			void ReadPlacement(const J *gui, ObjectDescription &object)
			{
				if (!gui || gui->GetType() != J::Object)
					return;
				std::vector<std::string> keys = gui->Keys();
				for (size_t i = 0; i < keys.size(); ++i)
				{
					const J *v = gui->Get(keys[i]);
					if (IsScalar(v) && v->GetType() != J::Null)
						object.Values[keys[i]] = ScalarText(v);
				}
				const J *state = gui->Get("state");
				if (!state || state->GetType() != J::Object)
					return;
				keys = state->Keys();
				for (size_t i = 0; i < keys.size(); ++i)
				{
					const J *v = state->Get(keys[i]);
					if (v->GetType() == J::Number)
						object.State[keys[i]] = StateValue(StateValue::Number, v->Text());
					else if (v->GetType() == J::Boolean)
						object.State[keys[i]] = StateValue(StateValue::Boolean, ScalarText(v));
					else if (v->GetType() == J::String)
						object.State[keys[i]] = StateValue(StateValue::String, v->Text());
				}
			}

			bool ReadBodyInto(const J &body, PatchDescription &out, std::string &error);

			void ReadPorts(const J *list, std::vector<PortName> &out)
			{
				if (!list || list->GetType() != J::Array)
					return;
				for (size_t i = 0; i < list->Size(); ++i)
				{
					const J *port = list->At(i);
					if (port && port->GetType() == J::Object)
						out.push_back(PortName(ScalarText(port->Get("name")), ScalarText(port->Get("type"))));
					else
						out.push_back(PortName());
				}
			}

			bool ReadSubPatch(const J *sub, DeviceDescription &device, std::string &error)
			{
				PatchDescription &inner = device.MakeSubPatch();
				device.Config.Voices = 0;
				if (!sub || sub->GetType() != J::Object)
					return true;
				const J *voices = sub->Get("Voices");
				const J *poly = sub->Get("PolyOutput");
				long count = 0;
				if ((voices && (!voices->Integer(count) || count < 0 || count > 256))
				    || (poly && poly->GetType() != J::Boolean))
				{
					error = "Invalid subpatch voice configuration";
					return false;
				}
				device.Config.Mode = SubPatchConfig::ModeSpelling(ScalarText(sub->Get("Mode")));
				device.Config.Voices = count;
				device.Config.PolyOutput = poly && poly->AsBool();
				device.Config.Link = SubPatchConfig::LinkSpelling(ScalarText(sub->Get("Link")));
				device.Config.BufferFrames = LongOr(sub->Get("BufferFrames"), 0);
				device.Config.LinkName = ScalarText(sub->Get("LinkName"));
				const J *body = sub->Get("Patch");
				if (body && body->GetType() == J::Object && !ReadBodyInto(*body, inner, error))
					return false;
				const J *binds = sub->Get("Binds");
				for (size_t b = 0; binds && binds->GetType() == J::Array && b < binds->Size(); ++b)
				{
					const J *bind = binds->At(b);
					if (!bind || bind->GetType() != J::Object)
						continue;
					const std::string outer = ScalarText(bind->Get("outer"));
					const std::string::size_type slash = outer.find('/');
					if (slash == std::string::npos)
						continue;
					const long index = std::strtol(outer.c_str() + slash + 1, NULL, 10);
					if (index < 0)
						continue;
					device.Binds.push_back(PortBind(outer.compare(0, slash, "Input") == 0, (unsigned)index,
						ScalarText(bind->Get("inner"))));
				}
				return true;
			}

			bool ReadDevice(const J &node, const J *gui, PatchDescription &out, std::string &error)
			{
				std::string id = ScalarText(node.Get("patch_id"));
				if (id.empty())
					id = ScalarText(node.Get("id"));
				if (!id.empty() && out.FindDevice(id))
				{
					error = "Duplicate device id '" + id + "' in patch file";
					return false;
				}
				std::string name = ScalarText(node.Get("name"));
				const J *sub = node.Get("SubPatch");
				const bool isSubPatch = (sub && sub->GetType() == J::Object) || name == "SubPatch";
				if (name.empty())
					name = isSubPatch ? "SubPatch" : "Device";

				ObjectDescription &object = out.AddObject();
				DeviceDescription &device = object.AddDevice();
				device.SetValue("patch_id", id);
				device.SetValue("id", Decimal(LongOr(node.Get("id"), -1)));
				device.SetValue("name", name);
				if (node.Get("version"))
					device.SetValue("version", Decimal(LongOr(node.Get("version"), 0)));
				ReadPorts(node.Get("inputs"), device.Inputs);
				ReadPorts(node.Get("outputs"), device.Outputs);
				if (isSubPatch && !ReadSubPatch(sub, device, error))
					return false;

				const J *props = node.Get("properties");
				if (props && props->GetType() == J::Array)
				{
					for (size_t i = 0; i < props->Size(); ++i)
					{
						const J *p = props->At(i);
						if (p && p->GetType() == J::Object)
							ReadProperty(*p, device.AddProperty("", "", ""));
					}
				}

				ReadPlacement(gui, object);
				if (object.Value("text").empty())
					object.SetValue("text", name);
				return true;
			}

			bool ReadMidi(const J *midi, PatchDescription &out, std::string &error)
			{
				if (!midi)
					return true;
				const J *devices = midi->GetType() == J::Object ? midi->Get("devices") : NULL;
				if (!devices || devices->GetType() != J::Array)
				{
					error = "JSON patch midi must be an object with a devices array";
					return false;
				}
				for (size_t d = 0; d < devices->Size(); ++d)
				{
					const J *device = devices->At(d);
					const J *id = device && device->GetType() == J::Object ? device->Get("device") : NULL;
					if (!IsScalar(id) || id->GetType() == J::Null)
						continue;
					MidiDeviceDescription listed(ScalarText(id));
					const J *bindings = device->Get("bindings");
					for (size_t b = 0; bindings && bindings->GetType() == J::Array && b < bindings->Size(); ++b)
					{
						const J *binding = bindings->At(b);
						if (!binding || binding->GetType() != J::Object)
							continue;
						const J *control = binding->Get("control");
						const J *channel = binding->Get("channel");
						const J *controller = binding->Get("controller");
						if (!control || control->GetType() != J::String || !controller || controller->GetType() != J::Number)
							continue;
						const long number = std::strtol(controller->Text().c_str(), NULL, 10);
						const long on = channel && channel->GetType() == J::Number ? std::strtol(channel->Text().c_str(), NULL, 10) : 0;
						if (number < 0 || number > 127 || on < 0 || on > 16)
							continue;
						listed.Bindings.push_back(MidiBindingDescription(control->Text(), on, number));
					}
					out.Midi.push_back(listed);
				}
				return true;
			}

			J *MidiJSON(const std::vector<MidiDeviceDescription> &midi)
			{
				J *devices = J::MakeArray();
				for (size_t d = 0; d < midi.size(); ++d)
				{
					J *device = J::MakeObject();
					device->SetOwned("device", J::MakeString(midi[d].Device));
					J *bindings = J::MakeArray();
					for (size_t b = 0; b < midi[d].Bindings.size(); ++b)
					{
						const MidiBindingDescription &bound = midi[d].Bindings[b];
						J *binding = J::MakeObject();
						binding->SetOwned("control", J::MakeString(bound.Control));
						if (bound.Channel)
							binding->SetOwned("channel", J::MakeNumber(Decimal(bound.Channel)));
						binding->SetOwned("controller", J::MakeNumber(Decimal(bound.Controller)));
						bindings->AppendOwned(binding);
					}
					device->SetOwned("bindings", bindings);
					devices->AppendOwned(device);
				}
				J *node = J::MakeObject();
				node->SetOwned("devices", devices);
				return node;
			}

			bool ReadBodyInto(const J &body, PatchDescription &out, std::string &error)
			{
				const J *voices = body.Get("Voices");
				long count = 0;
				if (voices && (!voices->Integer(count) || count < 0 || count > 256))
				{
					error = "Invalid patch voice count";
					return false;
				}
				out.Voices = unsigned(count);

				const J *objects = body.Get("objects");
				if (!objects || objects->GetType() != J::Array)
				{
					error = "JSON patch is missing an objects array";
					return false;
				}
				const J *wires = body.Get("wires");
				if (wires && wires->GetType() != J::Array)
				{
					error = "JSON patch wires must be an array";
					return false;
				}

				const J *host = body.Get("host");
				if (host && host->GetType() != J::Object)
				{
					error = "JSON patch host must be an object";
					return false;
				}
				if (host)
				{
					std::vector<std::string> names = host->Keys();
					for (size_t i = 0; i < names.size(); ++i)
					{
						const J *v = host->Get(names[i]);
						if (IsScalar(v) && v->GetType() != J::Null)
							out.Values[names[i]] = ScalarText(v);
					}
				}

				if (!ReadMidi(body.Get("midi"), out, error))
					return false;

				for (size_t oi = 0; oi < objects->Size(); ++oi)
				{
					const J *object = objects->At(oi);
					if (!object || object->GetType() != J::Object)
						continue;
					const J *gui = object->Get("gui");
					const J *devices = object->Get("devices");
					if (!devices || devices->GetType() != J::Array || devices->Size() == 0)
					{
						ObjectDescription &comment = out.AddObject();
						ReadPlacement(gui, comment);
						comment.Values["type"] = "comment";
						continue;
					}
					for (size_t di = 0; di < devices->Size(); ++di)
					{
						const J *device = devices->At(di);
						if (!device || device->GetType() != J::Object)
							continue;
						if (!ReadDevice(*device, gui, out, error))
							return false;
					}
				}

				for (size_t wi = 0; wires && wi < wires->Size(); ++wi)
				{
					const J *wire = wires->At(wi);
					if (!wire || wire->GetType() != J::Object)
						continue;
					const std::string from = ScalarText(wire->Get("from"));
					const std::string to = ScalarText(wire->Get("to"));
					const long output = LongOr(wire->Get("output"), 0);
					const long input = LongOr(wire->Get("input"), 0);
					if (output < 0 || input < 0)
					{
						error = "Wire with a negative port index";
						return false;
					}
					if (!out.FindDevice(from) || !out.FindDevice(to))
					{
						error = "Wire references unknown device '"
							+ (out.FindDevice(from) ? to : from) + "'";
						return false;
					}
					WireDescription &added = out.AddWire();
					added.FromDevice = from;
					added.FromPort = Decimal(output);
					added.ToDevice = to;
					added.ToPort = Decimal(input);
				}
				return true;
			}

			J *PropertyJSON(const PropertyDescription &property)
			{
				J *node = J::MakeObject();
				node->SetOwned("name", J::MakeString(property.Name));
				node->SetOwned("type", J::MakeString(property.Type));
				if (!property.PropertyClassName.empty())
					node->SetOwned("class", J::MakeString(property.PropertyClassName));
				const bool stringValue = property.Type == "string";
				for (std::map<std::string, std::string>::const_iterator it = property.Values.begin();
				     it != property.Values.end(); ++it)
					node->SetOwned(it->first, MakeScalar(it->second,
						stringValue && it->first == "value"));
				if (!property.Children.empty() || property.Type == "complex")
				{
					J *children = J::MakeArray();
					for (size_t i = 0; i < property.Children.size(); ++i)
						children->AppendOwned(PropertyJSON(property.Children[i]));
					node->SetOwned("values", children);
				}
				return node;
			}

			J *BodyJSON(const PatchDescription &definition);

			J *PortsJSON(const std::vector<PortName> &ports)
			{
				J *list = J::MakeArray();
				for (size_t i = 0; i < ports.size(); ++i)
				{
					J *port = J::MakeObject();
					port->SetOwned("name", J::MakeString(ports[i].Name));
					if (!ports[i].Type.empty())
						port->SetOwned("type", J::MakeString(ports[i].Type));
					list->AppendOwned(port);
				}
				return list;
			}

			J *SubPatchJSON(const DeviceDescription &device)
			{
				J *wrap = J::MakeObject();
				wrap->SetOwned("Mode", J::MakeString(SubPatchConfig::ModeSpelling(device.Config.Mode)));
				wrap->SetOwned("Voices", J::MakeNumber(Decimal(device.Config.Voices)));
				wrap->SetOwned("PolyOutput", J::MakeBoolean(device.Config.PolyOutput));
				if (SubPatchConfig::ModeSpelling(device.Config.Mode) == "Isolated")
				{
					wrap->SetOwned("Link", J::MakeString(SubPatchConfig::LinkSpelling(device.Config.Link)));
					if (device.Config.BufferFrames)
						wrap->SetOwned("BufferFrames", J::MakeNumber(Decimal(device.Config.BufferFrames)));
					if (!device.Config.LinkName.empty())
						wrap->SetOwned("LinkName", J::MakeString(device.Config.LinkName));
				}
				wrap->SetOwned("Patch", BodyJSON(*device.Inner()));
				if (!device.Binds.empty())
				{
					J *binds = J::MakeArray();
					for (size_t b = 0; b < device.Binds.size(); ++b)
					{
						J *bind = J::MakeObject();
						bind->SetOwned("outer", J::MakeString(std::string(device.Binds[b].OuterIsInput ? "Input/" : "Output/")
							+ Decimal(device.Binds[b].OuterIndex)));
						bind->SetOwned("inner", J::MakeString(device.Binds[b].InnerPath));
						binds->AppendOwned(bind);
					}
					wrap->SetOwned("Binds", binds);
				}
				return wrap;
			}

			J *DeviceJSON(const DeviceDescription &device)
			{
				J *node = J::MakeObject();
				node->SetOwned("patch_id", J::MakeString(device.Value("patch_id")));
				if (device.IsSubPatch())
				{
					node->SetOwned("name", J::MakeString("SubPatch"));
					node->SetOwned("SubPatch", SubPatchJSON(device));
				}
				else
				{
					node->SetOwned("id", J::MakeNumber(Decimal(std::strtol(device.Value("id").c_str(), NULL, 10))));
					node->SetOwned("name", J::MakeString(device.Value("name")));
				}
				const long version = std::strtol(device.Value("version").c_str(), NULL, 10);
				if (version != 0)
					node->SetOwned("version", J::MakeNumber(Decimal(version)));
				if (!device.Inputs.empty())
					node->SetOwned("inputs", PortsJSON(device.Inputs));
				if (!device.Outputs.empty())
					node->SetOwned("outputs", PortsJSON(device.Outputs));
				J *props = J::MakeArray();
				for (size_t i = 0; i < device.Properties.size(); ++i)
					props->AppendOwned(PropertyJSON(device.Properties[i]));
				node->SetOwned("properties", props);
				return node;
			}

			J *BodyJSON(const PatchDescription &definition)
			{
				J *body = J::MakeObject();
				J *objects = J::MakeArray();
				for (size_t oi = 0; oi < definition.Objects.size(); ++oi)
				{
					const ObjectDescription &placed = definition.Objects[oi];
					J *object = J::MakeObject();
					J *gui = J::MakeObject();
					gui->SetOwned("x", J::MakeNumber(Decimal(std::strtol(placed.Value("x").c_str(), NULL, 10))));
					gui->SetOwned("y", J::MakeNumber(Decimal(std::strtol(placed.Value("y").c_str(), NULL, 10))));
					gui->SetOwned("collapsed", J::MakeBoolean(placed.Value("collapsed") == "true"));
					gui->SetOwned("text", J::MakeString(placed.Value("text")));
					if (placed.IsComment())
						gui->SetOwned("type", J::MakeString("comment"));
					if (!placed.State.empty())
					{
						J *state = J::MakeObject();
						for (std::map<std::string, StateValue>::const_iterator s = placed.State.begin(); s != placed.State.end(); ++s)
							state->SetOwned(s->first, s->second.Type == StateValue::Number ? J::MakeNumber(s->second.Text)
							    : s->second.Type == StateValue::Boolean ? J::MakeBoolean(s->second.Text == "true")
							    : J::MakeString(s->second.Text));
						gui->SetOwned("state", state);
					}
					object->SetOwned("gui", gui);
					J *list = J::MakeArray();
					for (size_t di = 0; di < placed.Devices.size(); ++di)
						list->AppendOwned(DeviceJSON(placed.Devices[di]));
					object->SetOwned("devices", list);
					objects->AppendOwned(object);
				}
				body->SetOwned("objects", objects);

				J *wires = J::MakeArray();
				for (size_t i = 0; i < definition.Wires.size(); ++i)
				{
					const WireDescription &wire = definition.Wires[i];
					J *node = J::MakeObject();
					node->SetOwned("from", J::MakeString(wire.FromDevice));
					node->SetOwned("output", J::MakeString(wire.FromPort));
					node->SetOwned("to", J::MakeString(wire.ToDevice));
					node->SetOwned("input", J::MakeString(wire.ToPort));
					wires->AppendOwned(node);
				}
				body->SetOwned("wires", wires);
				body->SetOwned("Voices", J::MakeNumber(Decimal((long)definition.Voices)));

				if (!definition.Values.empty())
				{
					J *host = J::MakeObject();
					for (std::map<std::string, std::string>::const_iterator it
						     = definition.Values.begin();
					     it != definition.Values.end(); ++it)
						host->SetOwned(it->first, J::MakeString(it->second));
					body->SetOwned("host", host);
				}

				if (!definition.Midi.empty())
					body->SetOwned("midi", MidiJSON(definition.Midi));
				return body;
			}
		}

		namespace
		{
			void SplitPath(const std::string &key, std::string &name, std::vector<size_t> &indices,
			               std::string &field)
			{
				const size_t dot = key.rfind('.');
				const std::string path = dot == std::string::npos ? key : key.substr(0, dot);
				field = dot == std::string::npos ? std::string() : key.substr(dot + 1);
				const size_t open = path.find('[');
				name = path.substr(0, open);
				indices.clear();
				for (size_t at = open; at != std::string::npos && at < path.size(); at = path.find('[', at + 1))
				{
					const size_t close = path.find(']', at);
					if (close == std::string::npos) break;
					indices.push_back((size_t)std::strtoul(path.c_str() + at + 1, NULL, 10));
				}
			}

			PropertyDescription &ChildAt(PropertyDescription &parent, size_t index)
			{
				parent.Type = "complex";
				if (parent.PropertyClassName.empty()) parent.PropertyClassName = "VectorProperty";
				while (parent.Children.size() <= index)
				{
					PropertyDescription &blank = parent.AddChild();
					blank.Type = "numeric";
				}
				return parent.Children[index];
			}

			std::string OnOff(const std::string &text)
			{
				return text == "1" ? "on" : text == "0" ? "off" : text;
			}

			std::string ValueOf(const PropertyDescription &property)
			{
				const std::string &text = property.Value();
				if (property.Type != "string" && (text == "on" || text == "off"))
					return text == "on" ? "1" : "0";
				return text;
			}

			std::string TypeOf(const std::string &propertyClass)
			{
				if (propertyClass == "StringProperty") return "string";
				if (propertyClass == "NoteProperty") return "note";
				if (propertyClass == "VectorProperty" || propertyClass == "ComplexProperty"
				    || propertyClass == "ListProperty" || propertyClass == "MatrixPattern") return "complex";
				return "numeric";
			}

			void Flatten(const PropertyDescription &property, const std::string &path, Description &out)
			{
				if (property.Type == "complex" || !property.Children.empty())
				{
					for (size_t i = 0; i < property.Children.size(); ++i)
					{
						char index[32];
						std::snprintf(index, sizeof index, "[%u]", (unsigned)i);
						const PropertyDescription &child = property.Children[i];
						const bool holds = child.Type == "complex" || !child.Children.empty();
						if (!path.empty())
							out.Shaped(path + index, holds ? child.PropertyClassName : std::string(), child.Name);
						Flatten(child, path + index, out);
					}
					if (!path.empty() && path.find('[') == std::string::npos)
						out.Shaped(path, property.PropertyClassName, std::string());
					return;
				}
				if (path.empty())
				{
					out.Value(ValueOf(property));
					return;
				}
				out.Value(ValueOf(property)).Key(path, property.PropertyClassName);
				if (property.Type == "note")
				{
					static const char *const fields[] = { "octave", "velocity", "start", "end" };
					for (size_t f = 0; f < sizeof fields / sizeof fields[0]; ++f)
					{
						std::map<std::string, std::string>::const_iterator at = property.Values.find(fields[f]);
						if (at == property.Values.end())
							continue;
						out.Separator(" ").Value(at->second).Key(path + "." + fields[f], property.PropertyClassName);
					}
				}
			}
		}

		void PatchFile::Properties(const Description &described, DeviceDescription &out)
		{
			const std::vector<std::string> &values = described.Values();
			const std::vector<std::string> &keys = described.Keys();
			const std::vector<std::string> &classes = described.Classes();

			const bool bare = !described.Named();
			bool versionSeen = false;
			for (size_t i = 0; i < values.size(); ++i)
			{
				if (keys[i].empty())
				{
					if (!versionSeen) { out.SetValue("version", values[i]); versionSeen = true; }
					else if (bare) out.AddProperty("", "string", "").SetValue(values[i]);
					continue;
				}
				std::string name, field; std::vector<size_t> indices;
				SplitPath(keys[i], name, indices, field);
				PropertyDescription *node = out.FindProperty(name);
				if (!node) node = &out.AddProperty(name, "numeric", "");
				for (size_t d = 0; d < indices.size(); ++d) node = &ChildAt(*node, indices[d]);
				node->PropertyClassName = classes[i];
				node->Type = TypeOf(classes[i]);
				if (field.empty()) node->SetValue(classes[i] == "ToggleProperty" ? OnOff(values[i]) : values[i]);
				else node->SetValue(field, values[i]);
			}

			const std::vector<Description::Shape> &shapes = described.Shapes();
			for (size_t s = 0; s < shapes.size(); ++s)
			{
				std::string name, field; std::vector<size_t> indices;
				SplitPath(shapes[s].Key, name, indices, field);
				PropertyDescription *node = out.FindProperty(name);
				for (size_t d = 0; node && d < indices.size(); ++d)
					node = indices[d] < node->Children.size() ? &node->Children[indices[d]] : NULL;
				if (!node || !field.empty())
					continue;
				if (!shapes[s].Class.empty())
				{
					node->PropertyClassName = shapes[s].Class;
					if (TypeOf(shapes[s].Class) == "complex")
						node->Type = "complex";
				}
				if (!indices.empty())
					node->Name = shapes[s].Name;
			}
		}

		void PatchFile::Describe(const DeviceDescription &device, Description &out)
		{
			out = Description();
			if (device.HasValue("version"))
				out.Value(device.Value("version"));
			for (size_t i = 0; i < device.Properties.size(); ++i)
				Flatten(device.Properties[i], device.Properties[i].Name, out);
		}

		bool PatchFile::ReadBody(const J &body, PatchDescription &out, std::string &error)
		{
			error.clear();
			if (body.GetType() != J::Object)
			{
				error = "JSON patch root must be an object";
				return false;
			}
			PatchDescription built;
			if (!ReadBodyInto(body, built, error))
				return false;
			out.Swap(built);
			return true;
		}

		bool PatchFile::Read(const J &document, PatchDescription &out, std::string &error)
		{
			error.clear();
			if (document.GetType() != J::Object)
			{
				error = "JSON patch root must be an object";
				return false;
			}
			const J *format = document.Get("format");
			long found = 0;
			Version::Status status = Version::Check(ScalarText(format), Version::PatchKind,
				Version::Patch, found);
			if (status != Version::Current && status != Version::Older)
			{
				error = "patch: " + Version::Reason(status, Version::PatchKind, found,
					Version::Patch);
				return false;
			}
			return ReadBody(document, out, error);
		}

		bool PatchFile::ReadText(const std::string &text, PatchDescription &out,
			std::string &error)
		{
			error.clear();
			if (text.find('\0') != std::string::npos)
			{
				error = "Unescaped NUL in JSON input";
				return false;
			}
			const size_t start = text.compare(0, 3, "\xef\xbb\xbf") == 0 ? 3 : 0;
			Spumoni::JSONOwner root(Spumoni::ParseJSONText(text.substr(start), &error));
			if (!root.get())
				return false;
			return Read(*root.get(), out, error);
		}

		bool PatchFile::Text(const PatchDescription &definition, std::string &json,
			std::string &error)
		{
			error.clear();
			for (size_t w = 0; w < definition.Wires.size(); ++w)
			{
				const WireDescription &wire = definition.Wires[w];
				if (!definition.FindDevice(wire.FromDevice) || !definition.FindDevice(wire.ToDevice))
				{
					error = "Wire references unknown device '"
						+ (definition.FindDevice(wire.FromDevice) ? wire.ToDevice : wire.FromDevice) + "'";
					return false;
				}
			}
			Spumoni::JSONOwner root(BodyJSON(definition));
			root->SetOwned("format", J::MakeString(Version::PatchHeader()));
			root->SetOwned("version", J::MakeNumber(Decimal(Version::Patch)));
			json = root->Stringify(true);
			return true;
		}

		bool PatchFile::WriteFile(const PatchDescription &definition, const char *path,
			std::string &error)
		{
			error.clear();
			if (!path || !*path)
			{
				error = "No patch file path specified";
				return false;
			}
			std::string text;
			if (!Text(definition, text, error))
				return false;
			FILE *file = std::fopen(path, "wb");
			if (!file)
			{
				error = "Cannot write JSON patch";
				return false;
			}
			bool ok = std::fwrite(text.data(), 1, text.size(), file) == text.size();
			if (std::fclose(file) != 0)
				ok = false;
			if (!ok)
				error = "Error writing JSON patch";
			return ok;
		}

	}
}
