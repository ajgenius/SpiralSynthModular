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
				root.SetOwned("format", Spumoni::JSON::MakeString("SpiralSynthModular File Ver 9"));
				Spumoni::JSON *main = Spumoni::JSON::MakeObject();
				main->SetOwned("name", Spumoni::JSON::MakeString("patch.spiral.legacy.ssm"));
				main->SetOwned("format", Spumoni::JSON::MakeString("SpiralSynthModular File Ver 4"));
				root.SetOwned("main", main);

				Spumoni::JSON *application = Spumoni::JSON::MakeObject();
				application->SetOwned("name", Spumoni::JSON::MakeString("SpiralSynthModular"));
				root.SetOwned("application", application);
			}

			virtual bool Accept(const Spumoni::JSON &root, Spumoni::Identity &, std::string &error) const
			{
				const Spumoni::JSON *application = root.Get("application");
				const Spumoni::JSON *name = application ? application->Get("name") : NULL;
				if (!name || name->GetType() != Spumoni::JSON::String || name->Text() != "SpiralSynthModular")
				{
					error = "project.spiral.json has invalid application information";
					return false;
				}

				// Earlier public drafts had application.name alone. A stamped
				// project must explicitly identify the payload this host reads.
				const Spumoni::JSON *format = root.Get("format");
				const Spumoni::JSON *main = root.Get("main");
				if (!format && !main)
					return true;

				const Spumoni::JSON *member = main ? main->Get("name") : NULL;
				const Spumoni::JSON *header = main ? main->Get("format") : NULL;
				if (!format || format->GetType() != Spumoni::JSON::String
					|| format->Text() != "SpiralSynthModular File Ver 9"
					|| !member || member->GetType() != Spumoni::JSON::String
					|| member->Text() != "patch.spiral.legacy.ssm"
					|| !header || header->GetType() != Spumoni::JSON::String
					|| header->Text() != "SpiralSynthModular File Ver 4")
				{
					error = "Unsupported project version or main payload; this host reads positional Ver 4 packages";
					return false;
				}

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
