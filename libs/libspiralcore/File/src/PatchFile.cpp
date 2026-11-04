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
		using Core::Description;

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

			// The classes and names the keys cannot carry, on nodes the
			// values built: a shape for a node no value reached is dropped.
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
	}
}
