// SPDX-License-Identifier: GPL-2.0-or-later
#include "PatchProject.h"
#include "License.h"

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

		// The full licence text beside the patch, when the rights ask for
		// it and this build has it. Nothing to load: the identifier in the
		// document is what is read back, the file is for whoever opens the
		// package by hand.
		class Project::LicensePart : public Spumoni::Part
		{
		public:
			explicit LicensePart(const Project &project) : m_Project(project) {}
			virtual std::string Name() const { return "licenses"; }
			virtual bool Required() const { return false; }
			virtual bool Load(Spumoni::Folder &, const std::string &, const std::string &, std::string &)
			{
				return true;
			}
			virtual void Commit() {}
			virtual void Discard() {}

			static std::string FileName() { return "licenses/LICENSE.txt"; }

			// What the save writes, or empty when there is nothing to.
			static std::string Text(const DocumentSection &document)
			{
				const RightsSection &rights = document.Rights;
				if (!rights.BundleText || rights.License.empty())
					return std::string();

				std::string text = LicenseFullText(rights.License);
				if (!text.empty() && !rights.Copyright.empty())
					text = "Copyright " + rights.Copyright + "\n\n" + text;
				return text;
			}

			virtual bool Store(Spumoni::Container::Writer &writer, const std::string &branchRoot,
				Spumoni::Store *, std::string &error)
			{
				const std::string text = Text(m_Project.GetDocument());
				return text.empty() || writer.AddMemory(branchRoot + FileName(), text, error);
			}

		private:
			const Project &m_Project;
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
			, m_License(new LicensePart(*this))
		{
			AddPart(*m_Source);
			AddPart(*m_Sidecars);
			AddPart(*m_License);
			m_SourcePath = path;
		}

		Project::~Project()
		{
			delete m_Source;
			delete m_Sidecars;
			delete m_License;
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

		namespace
		{
			// A string member, or nothing. A field stated as a number or an
			// object is not a string and is left unread rather than
			// stringified into something that was never written.
			std::string TextOf(const Spumoni::JSON *owner, const char *key)
			{
				const Spumoni::JSON *member = owner ? owner->Get(key) : NULL;
				return member && member->GetType() == Spumoni::JSON::String
					? member->Text() : std::string();
			}
		}

		DocumentSection Project::GetDocument() const
		{
			DocumentSection document;
			const std::string &text = GetIdentity().ApplicationMetadata;
			if (text.empty())
				return document;

			// The load keeps the metadata file's whole text, so this reads
			// the same keys the save path writes. A key the project does
			// not state stays empty, which Empty() reports as nothing to
			// show: an absent title is not an empty title.
			Spumoni::JSONOwner root(Spumoni::ParseJSONText(text));
			if (!root.get() || root->GetType() != Spumoni::JSON::Object)
				return document;

			document.Title = TextOf(root.get(), "Title");
			document.Description = TextOf(root.get(), "Description");
			document.SavedBy = TextOf(root.get(), "SavedBy");
			document.CreatedAt = TextOf(root.get(), "CreatedAt");
			document.SavedAt = TextOf(root.get(), "SavedAt");

			if (const Spumoni::JSON *rights = root->Get("Rights"))
			{
				document.Rights.Copyright = TextOf(rights, "Copyright");
				document.Rights.License = TextOf(rights, "License");
				const Spumoni::JSON *bundle = rights->Get("BundleText");
				document.Rights.BundleText = bundle
					&& bundle->GetType() == Spumoni::JSON::Boolean && bundle->AsBool();
				document.Rights.LicenseFile = TextOf(rights, "LicenseFile");
			}

			// Credits are a list of who did what. An entry naming nobody
			// says nothing, so it is dropped rather than shown as a blank
			// row with a role beside it.
			const Spumoni::JSON *credits = root->Get("Credits");
			if (credits && credits->GetType() == Spumoni::JSON::Array)
			{
				for (size_t i = 0; i < credits->Size(); ++i)
				{
					const Spumoni::JSON *entry = credits->At(i);
					if (!entry || entry->GetType() != Spumoni::JSON::Object)
						continue;

					Credit credit;
					credit.Name = TextOf(entry, "Name");
					credit.Role = TextOf(entry, "Role");
					if (!credit.Name.empty())
						document.Credits.push_back(credit);
				}
			}

			return document;
		}

		void Project::SetDocument(const DocumentSection &document)
		{
			typedef Spumoni::JSON J;
			std::string &text = GetIdentity().ApplicationMetadata;

			// Start from what is there, minus the document's own keys,
			// so a key this host does not know survives the round trip.
			// JSON has no erase: the object is rebuilt without them.
			J *root = J::MakeObject();
			if (!text.empty())
			{
				Spumoni::JSONOwner previous(Spumoni::ParseJSONText(text));
				if (previous.get() && previous->GetType() == J::Object)
				{
					const std::vector<std::string> keys = previous->Keys();
					for (size_t i = 0; i < keys.size(); ++i)
					{
						const std::string &key = keys[i];
						if (key == "Title" || key == "Description" || key == "SavedBy"
							|| key == "CreatedAt" || key == "SavedAt"
							|| key == "Credits" || key == "Rights")
							continue;
						if (const J *value = previous->Get(key))
							root->SetOwned(key, value->Duplicate());
					}
				}
			}

			// The same keys, in the same shape, as the private tree's
			// patch writer: a field left empty grows no key.
			if (!document.Title.empty())
				root->SetOwned("Title", J::MakeString(document.Title));
			if (!document.Description.empty())
				root->SetOwned("Description", J::MakeString(document.Description));
			if (!document.SavedBy.empty())
				root->SetOwned("SavedBy", J::MakeString(document.SavedBy));
			if (!document.CreatedAt.empty())
				root->SetOwned("CreatedAt", J::MakeString(document.CreatedAt));
			if (!document.SavedAt.empty())
				root->SetOwned("SavedAt", J::MakeString(document.SavedAt));

			if (!document.Credits.empty())
			{
				J *credits = J::MakeArray();
				for (size_t i = 0; i < document.Credits.size(); ++i)
				{
					if (document.Credits[i].Name.empty())
						continue;
					J *credit = J::MakeObject();
					credit->SetOwned("Name", J::MakeString(document.Credits[i].Name));
					if (!document.Credits[i].Role.empty())
						credit->SetOwned("Role", J::MakeString(document.Credits[i].Role));
					credits->AppendOwned(credit);
				}
				root->SetOwned("Credits", credits);
			}

			RightsSection rights = document.Rights;
			// Bundling is a promise about the save. It is only made when
			// there is a text to keep it with, and the path recorded is
			// the one LicensePart writes.
			rights.LicenseFile = LicensePart::Text(document).empty()
				? std::string() : LicensePart::FileName();
			rights.BundleText = !rights.LicenseFile.empty();
			if (!rights.Empty())
			{
				J *section = J::MakeObject();
				if (!rights.Copyright.empty())
					section->SetOwned("Copyright", J::MakeString(rights.Copyright));
				if (!rights.License.empty())
					section->SetOwned("License", J::MakeString(rights.License));
				if (rights.BundleText)
					section->SetOwned("BundleText", J::MakeBoolean(true));
				if (!rights.LicenseFile.empty())
					section->SetOwned("LicenseFile", J::MakeString(rights.LicenseFile));
				root->SetOwned("Rights", section);
			}

			// A project that claims nothing carries no metadata text at
			// all, which is what Empty() reads back as absent.
			text = root->Keys().empty() ? std::string() : root->Stringify(true);
			delete root;
		}
	}
}
