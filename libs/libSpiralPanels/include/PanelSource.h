// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef SPIRAL_PANELS_PANEL_SOURCE_H
#define SPIRAL_PANELS_PANEL_SOURCE_H

#include "DataSource.h"
#include <map>
#include <string>
#include <vector>

namespace Spiral
{
	namespace Panels
	{
		class PanelProperty;

		class Feed
		{
		public:
			virtual ~Feed() {}
			virtual bool Take(std::vector<float> &out) = 0;
		};

		class PanelSource
		{
		public:
			virtual ~PanelSource() {}

			virtual Spiral::Data::DataSource &Data() = 0;
			virtual void Collect() = 0;
			virtual void Refresh() = 0;
			virtual bool AddElement(const PanelProperty &property) = 0;
			virtual void Describe(std::map<std::string, PanelProperty *> &properties) const = 0;
			virtual bool Write(const std::string &key, double value) = 0;
			virtual bool WriteText(const std::string &key, const std::string &text) = 0;
			virtual bool ReadNow(const PanelProperty &property, double &value) const = 0;
			virtual bool Command(const std::string &name, const std::vector<std::string> &args) = 0;
			virtual bool ReadBytes(const std::string &channel, std::vector<unsigned char> &bytes) const = 0;
			virtual bool ReadFloats(const std::string &channel, std::vector<float> &values) const = 0;
			// A table the device hands over whole: its cells in use, as numbers.
			virtual bool ReadTable(const std::string &channel, std::vector<double> &cells) const = 0;
			virtual bool FetchFloats(const std::string &command, const std::string &sizeChannel,
					const std::string &bufferChannel, std::vector<float> &values) = 0;
			virtual void Wait() = 0;
			virtual Feed *RequestFeed(bool output, int port) = 0;
			virtual void ReleaseFeed(Feed *feed) = 0;
			virtual unsigned Colour() const = 0;
			virtual bool ReadTableSince(const std::string &channel, unsigned long &, std::vector<double> &cells) const
			{
				return ReadTable(channel, cells);
			}
		};
	}
}

#endif
