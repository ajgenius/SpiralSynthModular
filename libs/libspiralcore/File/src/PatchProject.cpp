// SPDX-License-Identifier: GPL-2.0-or-later
#include "PatchProject.h"

#include "JSON.h"
#include "Directory.h"
#include <memory>

namespace Spiral
{
	namespace File
	{
		class SSMApplication : public Spumoni::Package::Application
		{
		public:
			virtual void Describe(Spumoni::JSON &root, const Spumoni::Identity &identity) const
			{
				root.SetOwned("format", Spumoni::JSON::MakeString("SpiralSynthModular File Ver 9"));
				Spumoni::JSON *main = Spumoni::JSON::MakeObject();
				main->SetOwned("name", Spumoni::JSON::MakeString("patch.spiral.legacy.ssm"));
				main->SetOwned("format", Spumoni::JSON::MakeString("SpiralSynthModular File Ver 4"));
				root.SetOwned("main", main);

				Spumoni::JSON *application = Spumoni::JSON::MakeObject();
				application->SetOwned("name", Spumoni::JSON::MakeString("SpiralSynthModular"));
				root.SetOwned("application", application);

				// The project's own title, credits and rights. Spumoni holds
				// them as opaque text because only this handler knows their
				// shape, and they are written beside the format rather than
				// under a key of their own: that is where the newer envelope
				// spells them. A project that claims none grows no keys.
				if (!identity.ApplicationMetadata.empty())
				{
					Spumoni::JSON *metadata =
						Spumoni::ParseJSONText(identity.ApplicationMetadata);
					if (metadata && metadata->GetType() == Spumoni::JSON::Object)
					{
						const std::vector<std::string> keys = metadata->Keys();
						for (size_t i = 0; i < keys.size(); ++i)
						{
							// This host owns the stamp. Carrying it over from
							// the stored text would write an older project's
							// version back out as if it were ours.
							if (keys[i] == "format" || keys[i] == "main"
								|| keys[i] == "application")
								continue;
							if (const Spumoni::JSON *value = metadata->Get(keys[i]))
								root.SetOwned(keys[i], value->Duplicate());
						}
					}
					delete metadata;
				}
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

		// Plugins still take filesystem paths. Keep their sidecars in owned
		// scratch directories while the package stores them per branch.
		class Project::SidecarPart : public Spumoni::Part
		{
		public:
			SidecarPart() : m_Live(NULL), m_Staged(NULL) {}
			virtual ~SidecarPart() { Clear(); }
			virtual std::string Name() const { return "patch.spiral.legacy.ssm_files"; }
			virtual bool Required() const { return false; }

			void Clear()
			{
				delete m_Live;
				m_Live = NULL;
				Discard();
			}

			bool Begin(std::string &error)
			{
				std::auto_ptr<Spumoni::DiskFolder> fresh(new Spumoni::DiskFolder);
				if (!fresh->Create("ssm-sidecars-", error))
					return false;

				delete m_Live;
				m_Live = fresh.release();
				return true;
			}

			std::string Directory() const { return m_Live ? m_Live->Root() + "/" : std::string(); }

			virtual bool Load(Spumoni::Folder &folder, const std::string &branchRoot,
				const std::string &, std::string &error)
			{
				Discard();
				m_Staged = new Spumoni::DiskFolder;
				return m_Staged->Create("ssm-sidecars-", error)
					&& Copy(folder, branchRoot + Name(), "", *m_Staged, error);
			}

			virtual void Commit()
			{
				delete m_Live;
				m_Live = m_Staged;
				m_Staged = NULL;
			}

			virtual void Discard()
			{
				delete m_Staged;
				m_Staged = NULL;
			}

			virtual bool Store(Spumoni::Container::Writer &writer, const std::string &branchRoot,
				Spumoni::Store *, std::string &error)
			{
				return !m_Live || (m_Live->MakeDirectory("", error)
					&& m_Live->ExportTree(writer, "", branchRoot + Name(), error));
			}

		private:
			static bool Copy(Spumoni::Folder &source, const std::string &from, const std::string &to,
				Spumoni::DiskFolder &target, std::string &error)
			{
				if (source.IsFile(from))
				{
					std::string path;
					return source.PathFor(from, path, error)
						&& Spumoni::Directory::CopyFile(path, target.Root() + "/" + to, error);
				}

				std::vector<std::string> names;
				if (!source.List(from, names) || !target.MakeDirectory(to, error))
				{
					if (error.empty())
						error = "Cannot read package sidecars: " + from;

					return false;
				}

				for (size_t i = 0; i < names.size(); ++i)
					if (!Copy(source, from + "/" + names[i], Spumoni::Path::Join(to, names[i]), target, error))
						return false;

				return true;
			}

			Spumoni::DiskFolder *m_Live;
			Spumoni::DiskFolder *m_Staged;
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
			, m_Sidecars(new SidecarPart)
		{
			AddPart(*m_Source);
			AddPart(*m_Sidecars);
			m_SourcePath = path;
		}

		Project::~Project()
		{
			delete m_Source;
			delete m_Sidecars;
		}

		void Project::OnReset()
		{
			m_Source->Clear();
			m_Sidecars->Clear();
		}

		bool Project::BeginSidecars(std::string &error)
		{
			return m_Sidecars->Begin(error);
		}

		std::string Project::SidecarDirectory() const
		{
			const std::string bundled = m_Sidecars->Directory();
			return bundled.empty() ? SourcePath() + "_files/" : bundled;
		}

		bool Project::PathLooksLikePackage(const std::string &path)
		{
			Project judge(path);
			return judge.LooksLikePackage(path) || judge.IsPackage(path);
		}
	}
}
