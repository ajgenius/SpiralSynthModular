// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef SPIRAL_PANELS_FLTK_THEME_H
#define SPIRAL_PANELS_FLTK_THEME_H

#include <FL/Enumerations.H>
#include <string>

class Fl_Widget;

namespace Spumoni
{
	class JSON;
}

namespace Spiral
{
	namespace Panels
	{
		class PanelControl;

		class FltkLook
		{
		public:
			FltkLook(): m_Values(NULL), m_Device(0) {}
			FltkLook(const Spumoni::JSON *values, unsigned device): m_Values(values), m_Device(device) {}

			bool Has(const char *key) const;
			Fl_Boxtype Box(const char *key) const;
			Fl_Color Colour(const char *key) const;
			Fl_Font Font(const char *key) const;
			int Size(const char *key) const;
			std::string Text(const char *key) const;
			void Apply(Fl_Widget *widget) const;

			template <class W> void ApplyText(W *widget) const
			{
				if (Has("text_size"))
					widget->textsize(Size("text_size"));

				if (Has("text_font"))
					widget->textfont(Font("text_font"));

				if (Has("text_color"))
					widget->textcolor(Colour("text_color"));
			}

		private:
			const Spumoni::JSON *m_Values;
			unsigned m_Device;
		};

		class FltkTheme
		{
		public:
			enum Sized
			{
				SizeNone = 0,
				SizeLabel = 1,
				SizeText = 2
			};

			static FltkLook For(const PanelControl &control, const std::string &type, const Spumoni::JSON &node,
					const std::string &variant = std::string(), int sized = SizeNone);
			static FltkLook Base(const std::string &type);
			static bool Load(const std::string &text, std::string &error);
		};
	}
}

#endif
