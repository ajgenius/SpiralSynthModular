// SPDX-License-Identifier: GPL-2.0-or-later
// A reader refuses a newer package stamp and says which level. Shared stores
// and journals are not in this build.
#ifndef SPUMONI_PACKAGE_H
#define SPUMONI_PACKAGE_H

#include "Container.h"
#include "Identity.h"

#include <string>
#include <vector>

#include "Folder.h"
#include "JSON.h"

namespace Spumoni
{

	class Package
	{
	public:
		static const long FormatVersion = 3;

		static std::string FormatName();
		static std::string Stamp();
		static std::string Stamp(long version);

		enum VersionStatus
		{
			Current,
			Older,
			Newer,
			Foreign
		};

		static VersionStatus Check(const std::string &stamp, long &found);

		static std::string Reason(VersionStatus status, long found);

		struct Layout
		{
			std::string ManifestName;
			std::string MetadataName;
			std::string MetadataKey;
			std::string LegacyManifestName;
			std::vector<std::string> Payload;
			std::string WorkPrefix;
			Folder *(*MakeFolder)();
			Layout() : MakeFolder(NULL) {}
		};

		class Application
		{
		public:
			virtual ~Application() {}
			virtual void Describe(JSON &root, const Identity &identity) const = 0;
			virtual bool Accept(const JSON &root, Identity &identity, std::string &error) const = 0;
		};

		class Payload
		{
		public:
			virtual ~Payload() {}
			virtual bool Write(Container::Writer &writer, const std::string &branchRoot, std::string &error) = 0;
		};

		Package(const Container &container, const Layout &layout, const Application &application);

		const Layout &Files() const { return m_Layout; }

		bool Open(const std::string &path, const std::string &branchId,
			Folder &folder, Identity &identity, std::string &branchRoot, std::string &error) const;

		bool Write(const std::string &path, const Identity &identity, const SaveRequest &request,
			Payload &payload, std::string &error) const;

		std::vector<std::string> BranchesIn(const SaveRequest &request, std::string &error) const;

		bool WriteManifest(const std::string &path, const Identity &identity, std::string &error) const;

		std::string ManifestJSON(const Identity &identity) const;
		std::string MetadataJSON(const Identity &identity) const;

		bool ReadManifest(const Folder &folder, Identity &identity, std::string &branchRoot, std::string &error) const;
		bool ReadManifestText(const std::string &text, Identity &identity, std::string &branchRoot, std::string &error) const;

		static std::vector<std::string> ListBranches(const Folder &source);

		Folder *NewFolder() const;

		static std::string BranchRoot(const std::string &id) { return "branches/" + id + "/"; }

		static std::string AssetsDir() { return "assets"; }

		// Now, as a manifest stamps it: "YYYY-MM-DDTHH:MM:SSZ".
		static std::string SaveTimeUTC();

	private:
		class View;

		Package(const Package &);
		Package &operator=(const Package &);

		bool OpenPreserveSource(const SaveRequest &request, Folder *&source, std::string &error) const;
		bool CopyPreservedBranches(Container::Writer &writer, const Folder &source, const std::string &rewriteID, std::string &error) const;
		bool CopyActiveExtras(Container::Writer &writer, const Folder &source, const std::string &sourceID, const std::string &activeID, std::string &error) const;
		static bool ReadActiveBranch(const JSON &root, const std::string &activeBranch, std::string &activePath, std::string &error);

		const Container &m_Container;
		Layout m_Layout;
		const Application &m_Application;
	};

} // namespace Spumoni

#endif
