// SPDX-License-Identifier: GPL-2.0-or-later
#include "FltkTheme.h"
#include "Gzip.h"
#include "JSON.h"
#include "Panel.h"
#include "PanelControl.h"
#include <FL/Fl_Widget.H>
#include <cstddef>
#include <cstdlib>
#include <fnmatch.h>
#include <iostream>
#include <map>
#include <vector>
#include <sstream>

const unsigned char *PlasticTheme(size_t *size);

namespace Spiral
{
	namespace Panels
	{
		namespace
		{
			struct Name
			{
				const char *Key;
				int Value;
			};

			const Name Boxes[] = {
				{"none", FL_NO_BOX}, {"flat", FL_FLAT_BOX}, {"up", FL_UP_BOX}, {"down", FL_DOWN_BOX},
				{"up_frame", FL_UP_FRAME}, {"down_frame", FL_DOWN_FRAME}, {"thin_up", FL_THIN_UP_BOX},
				{"thin_down", FL_THIN_DOWN_BOX}, {"engraved", FL_ENGRAVED_BOX}, {"embossed", FL_EMBOSSED_BOX},
				{"border", FL_BORDER_BOX}, {"round_up", FL_ROUND_UP_BOX}, {"round_down", FL_ROUND_DOWN_BOX},
				{"plastic_up", FL_PLASTIC_UP_BOX}, {"plastic_down", FL_PLASTIC_DOWN_BOX},
				{"plastic_thin_up", FL_PLASTIC_THIN_UP_BOX}, {"plastic_thin_down", FL_PLASTIC_THIN_DOWN_BOX}};

			const Name Colours[] = {
				{"foreground", FL_FOREGROUND_COLOR}, {"background", FL_BACKGROUND_COLOR},
				{"background2", FL_BACKGROUND2_COLOR}, {"inactive", FL_INACTIVE_COLOR}, {"selection", FL_SELECTION_COLOR},
				{"gray", FL_GRAY}, {"dark1", FL_DARK1}, {"dark2", FL_DARK2}, {"dark3", FL_DARK3}, {"light1", FL_LIGHT1},
				{"light2", FL_LIGHT2}, {"light3", FL_LIGHT3}, {"black", FL_BLACK}, {"white", FL_WHITE}, {"red", FL_RED},
				{"green", FL_GREEN}, {"yellow", FL_YELLOW}, {"blue", FL_BLUE}, {"magenta", FL_MAGENTA},
				{"cyan", FL_CYAN}};

			const Name Fonts[] = {
				{"helvetica", FL_HELVETICA}, {"helvetica_bold", FL_HELVETICA_BOLD}, {"courier", FL_COURIER},
				{"courier_bold", FL_COURIER_BOLD}, {"times", FL_TIMES}, {"symbol", FL_SYMBOL}};

			bool Find(const Name *names, size_t count, const std::string &key, int &value)
			{
				for (size_t i = 0; i < count; ++i)
				{
					if (key == names[i].Key)
					{
						value = names[i].Value;

						return true;
					}
				}

				return false;
			}

			class Theme
			{
			public:
				static Theme &Get()
				{
					static Theme theme;

					return theme;
				}

				~Theme()
				{
					Reset();
				}

				bool Load(const std::string &text, std::string &error)
				{
					Spumoni::JSON *root = Spumoni::ParseJSONText(text, &error);

					if (!root)
						return false;

					if (root->GetType() != Spumoni::JSON::Object || !root->Get("controls"))
					{
						delete root;
						error = "a theme needs \"controls\"";

						return false;
					}

					Reset();
					m_Root = root;

					return true;
				}

				const Spumoni::JSON *Root()
				{
					if (!m_Root)
						LoadBuiltIn();

					return m_Root;
				}

				const Spumoni::JSON *Merged(const std::string &key, const std::vector<const Spumoni::JSON *> &layers)
				{
					std::map<std::string, Spumoni::JSON *>::iterator found = m_Looks.find(key);

					if (found != m_Looks.end())
						return found->second;

					Spumoni::JSON *look = Spumoni::JSON::MakeObject();

					for (size_t i = 0; i < layers.size(); ++i)
					{
						const std::vector<std::string> keys = layers[i]->Keys();

						for (size_t k = 0; k < keys.size(); ++k)
						{
							if (keys[k] != "variants" && keys[k] != "roles")
								look->SetOwned(keys[k], layers[i]->Get(keys[k])->Duplicate());
						}
					}

					m_Looks[key] = look;

					return look;
				}

				const Spumoni::JSON *Palette(const std::string &name)
				{
					const Spumoni::JSON *palette = Root() ? Root()->Get("palette") : NULL;

					return palette ? palette->Get(name) : NULL;
				}

			private:
				Theme(): m_Root(NULL) {}

				void Reset()
				{
					for (std::map<std::string, Spumoni::JSON *>::iterator i = m_Looks.begin(); i != m_Looks.end(); ++i)
						delete i->second;

					m_Looks.clear();
					delete m_Root;
					m_Root = NULL;
				}

				void LoadBuiltIn()
				{
					size_t size = 0;
					const unsigned char *data = PlasticTheme(&size);
					std::string text, error;

					if (!Spumoni::Gzip::Decompress(data, size, text, error) || !Load(text, error))
						std::cerr << "plastic.fltk.json: " << error << std::endl;
				}

				Spumoni::JSON *m_Root;
				std::map<std::string, Spumoni::JSON *> m_Looks;
			};

			bool Matches(const Spumoni::JSON *patterns, const std::string &name)
			{
				if (!patterns)
					return true;

				for (size_t i = 0; i < patterns->Size(); ++i)
				{
					if (fnmatch(patterns->At(i)->Text().c_str(), name.c_str(), 0) == 0)
						return true;
				}

				return false;
			}

			void Layer(const Spumoni::JSON *entry, const std::string &variant, const std::string &role,
					std::vector<const Spumoni::JSON *> &layers)
			{
				if (!entry)
					return;

				layers.push_back(entry);
				const Spumoni::JSON *variants = entry->Get("variants");

				if (!variant.empty() && variants && variants->Get(variant))
					layers.push_back(variants->Get(variant));

				const Spumoni::JSON *roles = entry->Get("roles");

				if (!role.empty() && roles && roles->Get(role))
					layers.push_back(roles->Get(role));
			}

			Fl_Color ColourOf(const Spumoni::JSON *value, unsigned device, int depth)
			{
				if (!value || depth > 8)
					return FL_FOREGROUND_COLOR;

				if (value->GetType() == Spumoni::JSON::Array && value->Size() == 3)
					return fl_rgb_color((uchar)std::atoi(value->At(0)->Text().c_str()),
						(uchar)std::atoi(value->At(1)->Text().c_str()), (uchar)std::atoi(value->At(2)->Text().c_str()));

				if (value->GetType() == Spumoni::JSON::Number)
					return (Fl_Color)std::strtoul(value->Text().c_str(), NULL, 10);

				const std::string text = value->Text();

				if (text == "device")
					return (Fl_Color)device;

				if (!text.empty() && text[0] == '$')
					return ColourOf(Theme::Get().Palette(text.substr(1)), device, depth + 1);

				int colour = 0;

				if (Find(Colours, sizeof Colours / sizeof Colours[0], text, colour))
					return (Fl_Color)colour;

				return FL_FOREGROUND_COLOR;
			}
		}

		bool FltkLook::Has(const char *key) const
		{
			const Spumoni::JSON *value = m_Values ? m_Values->Get(key) : NULL;

			return value && !(value->GetType() == Spumoni::JSON::String && value->Text() == "default");
		}

		Fl_Boxtype FltkLook::Box(const char *key) const
		{
			int box = FL_NO_BOX;
			Find(Boxes, sizeof Boxes / sizeof Boxes[0], Text(key), box);

			return (Fl_Boxtype)box;
		}

		Fl_Color FltkLook::Colour(const char *key) const
		{
			return ColourOf(m_Values ? m_Values->Get(key) : NULL, m_Device, 0);
		}

		Fl_Font FltkLook::Font(const char *key) const
		{
			int font = FL_HELVETICA;
			Find(Fonts, sizeof Fonts / sizeof Fonts[0], Text(key), font);

			return (Fl_Font)font;
		}

		int FltkLook::Size(const char *key) const
		{
			const Spumoni::JSON *value = m_Values ? m_Values->Get(key) : NULL;

			return value ? std::atoi(value->Text().c_str()) : 0;
		}

		std::string FltkLook::Text(const char *key) const
		{
			const Spumoni::JSON *value = m_Values ? m_Values->Get(key) : NULL;

			return value && value->GetType() == Spumoni::JSON::String ? value->Text() : std::string();
		}

		void FltkLook::Apply(Fl_Widget *widget) const
		{
			if (Has("box"))
				widget->box(Box("box"));

			if (Has("color"))
				widget->color(Colour("color"));

			if (Has("selection_color"))
				widget->selection_color(Colour("selection_color"));

			if (Has("label_color"))
				widget->labelcolor(Colour("label_color"));

			if (Has("label_size"))
				widget->labelsize(Size("label_size"));

			if (Has("label_font"))
				widget->labelfont(Font("label_font"));
		}

		FltkLook FltkTheme::For(const PanelControl &control, const std::string &type, const Spumoni::JSON &node,
				const std::string &variant, int sized)
		{
			Theme &theme = Theme::Get();
			const Spumoni::JSON *root = theme.Root();

			if (!root)
				return FltkLook();

			const std::string role = PanelControl::TextOf(node.Get("color_role"));
			const long device = control.Owner().PluginId();
			std::vector<const Spumoni::JSON *> layers;
			std::ostringstream key;
			key << type << '|' << variant << '|' << role << '|' << device << '|' << control.Name();
			Layer(root->Get("controls")->Get(type), variant, role, layers);
			const Spumoni::JSON *overrides = root->Get("overrides");

			for (size_t i = 0; overrides && i < overrides->Size(); ++i)
			{
				const Spumoni::JSON *entry = overrides->At(i);
				long id = -1;
				const Spumoni::JSON *types = entry->Get("types");

				if (entry->Get("device") && entry->Get("device")->Integer(id) && id == device &&
						Matches(entry->Get("controls"), control.Name()) && Matches(types, type))
					Layer(entry->Get("look"), variant, role, layers);
			}

			if (node.Get("size") && sized != SizeNone)
			{
				Spumoni::JSON *size = Spumoni::JSON::MakeObject();

				if (sized & SizeLabel)
					size->SetOwned("label_size", node.Get("size")->Duplicate());

				if (sized & SizeText)
					size->SetOwned("text_size", node.Get("size")->Duplicate());

				std::ostringstream sizedKey;
				sizedKey << key.str() << "|size" << sized << '=' << node.Get("size")->Text();
				std::vector<const Spumoni::JSON *> withSize = layers;
				withSize.push_back(size);
				FltkLook look(theme.Merged(sizedKey.str(), withSize), control.Colour());
				delete size;

				return look;
			}

			return FltkLook(theme.Merged(key.str(), layers), control.Colour());
		}

		FltkLook FltkTheme::Base(const std::string &type)
		{
			Theme &theme = Theme::Get();
			const Spumoni::JSON *root = theme.Root();

			if (!root)
				return FltkLook();

			std::vector<const Spumoni::JSON *> layers;
			Layer(root->Get("controls")->Get(type), std::string(), std::string(), layers);

			return FltkLook(theme.Merged(type + "|base", layers), 0);
		}

		bool FltkTheme::Load(const std::string &text, std::string &error)
		{
			return Theme::Get().Load(text, error);
		}
	}
}
