// SPDX-License-Identifier: GPL-2.0-or-later
#include "PatchProject.h"

#include "JSON.h"

namespace Spiral
{
	namespace File
	{
		class SSMApplication : public Spumoni::Package::Application
		{
		public:
			virtual void Describe(Spumoni::JSON &root, const Spumoni::Identity &) const
			{
				Spumoni::JSON *application = Spumoni::JSON::MakeObject();
				application->SetOwned("name", Spumoni::JSON::MakeString("SpiralSynthModular"));
				root.SetOwned("application", application);
			}

			virtual bool Accept(const Spumoni::JSON &, Spumoni::Identity &, std::string &) const
			{
				return true;
			}
		};

		const Spumoni::Format &Format()
		{
			static const SSMApplication application;
			static Spumoni::Format format;
			if (format.Application == NULL)
			{
				format.ManifestName = "spumoni.json";
				format.MetadataName = "project.spiral.json";
				format.MetadataKey = "spiral";
				format.LegacyManifestName = "SSM.json";
				format.Application = &application;
				format.Extensions.push_back(".ssmp");
				format.WorkPrefix = "spiralsynthmodular-ssmp-";
			}
			return format;
		}

		Project::Project(const std::string &path)
			: Spumoni::Project(Format())
			, m_Source(new Spumoni::SourcePart("patch.spiral.legacy.ssm"))
		{
			AddPart(*m_Source);
			m_SourcePath = path;
		}

		Project::~Project()
		{
			delete m_Source;
		}

		bool Project::PathLooksLikePackage(const std::string &path)
		{
			Project judge(path);
			return judge.LooksLikePackage(path) || judge.IsPackage(path);
		}
	}
}
