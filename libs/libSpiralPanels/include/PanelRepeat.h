// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef SPIRAL_PANELS_REPEAT_H
#define SPIRAL_PANELS_REPEAT_H

#include <map>
#include <string>
#include <vector>

namespace Spumoni
{
	class JSON;
}

namespace Spiral
{
	namespace Panels
	{
		class Document;
		class Node;

		class Repeat
		{
		public:
			explicit Repeat(const Node &node);

			static bool Is(const Node &node);

			bool ByItems() const { return m_Items != NULL; }
			size_t DefaultCount() const;
			long IndexBase() const { return m_Base; }
			long StepX() const { return m_StepX; }
			long StepY() const { return m_StepY; }
			std::string CountProperty() const;

			void Origin(size_t copy, long &x, long &y) const;
			std::vector<Node *> Copy(size_t copy) const;

		private:
			const Node &m_Node;
			const Spumoni::JSON *m_Items;
			long m_Base, m_StepX, m_StepY, m_X, m_Y;
		};

		typedef std::map<std::string, const Spumoni::JSON *> Variables;

		Spumoni::JSON *Substitute(const Spumoni::JSON &value, const Variables &scope);
		std::string Spell(const Spumoni::JSON &value);
		size_t Expand(Document &document);
	}
}

#endif
