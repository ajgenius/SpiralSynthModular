# Spumoni

A layered package format, and the library that reads and writes it.

Spumoni is for applications whose document is "a few JSON files plus the
things they point at", and whose users want save points they can go back to
without keeping twenty copies of the file. One package holds every save
point of one document; each save point is a complete, self-contained layer.
The name is the Italian layered ice cream: distinct layers, one thing you can
carry, and it sits on the shelf beside Spicy and Gumbo.

Spumoni knows nothing about any particular application. SpiralSynthModular's
`.ssmp` is one application of it; the library, its test and this README do
not mention synthesisers.

## The shape of a package

```
<container>                     a ZIP, a tarball (.tar / .tgz), or a bare folder
├── <manifest>                  the package's identity and the application's section
└── branches/
    └── <uuid>/                 one folder per save point ("branch")
        ├── <part>              the application's files: data, an optional schema, …
        ├── <part>
        ├── assets/             files the parts reference, bundled beside them
        └── <anything else>     carried forward untouched from the branch it came from
```

Three things are versioned independently, and a reader refuses a newer stamp
at its own level and says which level:

| level | how it is identified | who owns it |
|---|---|---|
| container | by what it is (ZIP signature, gzip stream, ustar header, a directory) | the container format |
| package | `"package": "Spumoni Package Ver 1"` in the manifest | Spumoni |
| payloads | the application's own headers on the manifest and on every part | the application |

**Package Ver 1** is the layout above. Ver 2 will add package-wide
content-addressed assets shared across branches; Ver 3 checkpoints and a
replayable journal. None of that moves an application's payload versions, and
an application bumping its payload never moves the package version. A
manifest without the stamp predates it and is read as Ver 1.

The manifest, as Spumoni writes it, plus whatever the application adds:

```json
{
  "package": "Spumoni Package Ver 1",
  "package_id": "…uuid…",
  "current_branch": "…uuid…",
  "saved_at": "2026-09-29T20:00:00Z",
  "branches": [ { "id": "…", "kind": "named", "path": "branches/…/",
                  "parent_id": null, "fork_save_id": null } ],
  "names": { "project": "My Document", "branches": { "…uuid…": "My Document" } }
}
```

## The layers of the library

Each layer is an interface with at least one implementation, and each knows
nothing about the layers above it.

**`Container`** — the box: one file holding a tree. A `Writer` builds a new
archive entry by entry and replaces the target atomically on `Finish`;
`Extract` unpacks into a `Folder`. Entry names are relative, normalised,
forward-slash paths (`SafeArchivePath`); `SafeName` folds anything else.
Kinds: `Zip` (ZIP32, stored or deflated, the portable default), `Tar` (ustar,
gzip-compressed when the path ends `.tgz`/`.tar.gz`, streamed), `Directory`
(a bare folder, swapped into place on `Finish`; a package under version
control). `Container::Sniff(path)` tells them apart by content,
`Container::ForPath(path)` by content then extension, falling back to ZIP.

**`Folder`** — the working tree a package is unpacked into, edited in and
packed back from, addressed by relative path: `IsFile`, `IsDirectory`,
`List`, `Read`, `Write`, `MakeDirectory`, `RemoveEntry`, streaming through
`OpenWrite`/`CloseWrite`/`OpenRead`, `ImportTree`/`ExportTree`, and `AddTo`
to put one entry into a container the cheapest way the kind knows.
`DiskFolder` is a private temporary directory (or, through `Adopt`, an
existing one, not owned). `MemoryFolder` holds the tree entirely in memory:
no path, no mount, no file record, nothing outside the process can open it.
`PathFor(relative)` is the one door out, for code that can only take a path:
a disk folder answers with the file, a memory folder materialises that one
entry into a private scratch it owns, keeps in step, and removes with itself.
`Bytes()` says how much a memory folder holds. `Path` is the handful of plain
absolute-path helpers the implementations share; it is not the interface.

**`Identity`** — what a package says about itself: its UUID, its branches
with UUIDs, names and lineage (`parent_id`, `fork_save_id`), which one is
active, who saved it; and the bookkeeping an application does on them
(`Begin`, `Fork`, `ActivateNamed`, `EnsureActiveListed`, `AdoptUnlisted`,
`SuggestedName`). `SaveRequest` is what a write is asked: replace the active
branch or add a new one, and where to preserve the other branches from (the
previous package file, or the live workspace `Folder`).

**`Package`** — the arrangement: unpacks a container into a `Folder`, reads
and writes the manifest (its own stamp and identity first, then the
application's section), lists branch folders, carries the other branches and
the source branch's extras forward on a write, writes the active branch
through a `Payload`, rewrites a manifest in place. The application supplies a
`Layout` (manifest name, the names of its parts, its work-folder prefix, the
kind of folder to make), a `Package::Application` (`Describe` writes its
manifest section, `Accept` checks one being read), and a `Payload`.

**`Project`** — an open package with a save surface. It owns the identity,
the source path and the workspace, and knows what every layered package
does: `OpenPackage`/`OpenSavePoint` (unpack, load every part into its
staging, commit all or nothing), `SaveAs` (a different file: a package of its
own; the same file: replace the branch named after it), `CreateSavePoint`
(this state as a named branch, new or replaced), `UpdateManifest`, and the
identity resolution behind them. The workspace is memory unless the package
is larger than the format's `MemoryLimit`; a new project has one from the
start (laid out as an empty package, identity begun), and after every save
the file is unpacked again so the workspace mirrors it. An application
derives from `Project`, supplies a `Format` (manifest name, application
section, extensions, work prefix, memory limit) and adds its **`Part`**s: one
per file in a branch, each with a `Name`, whether it is `Required`, `Load`
into its own staging, `Commit`/`Discard`, and `Store`.

## Using it

Three things to write, and no file handling:

```cpp
struct MyApplication : Spumoni::Package::Application
{
    void Describe(Slick::JSONValue &root, const Spumoni::Identity &) const
    { root.Set("format", Slick::JSONValue::MakeString("MyApp File Ver 1")); }
    bool Accept(const Slick::JSONValue &root, Spumoni::Identity &, std::string &error) const
    { /* check "format"; return false with a reason if it is not yours */ }
};

struct DataPart : Spumoni::Part
{
    std::string Name() const { return "data.json"; }
    bool Load(Spumoni::Folder &folder, const std::string &branchRoot,
              const std::string &packagePath, std::string &error)
    { std::string text; return folder.Read(branchRoot + Name(), text, error) && Parse(text, m_Staged, error); }
    void Commit() { m_Live.swap(m_Staged); }
    void Discard() { m_Staged.clear(); }
    bool Store(Spumoni::Container::Writer &writer, const std::string &branchRoot, std::string &error)
    { return writer.AddMemory(branchRoot + Name(), Serialise(m_Live), error); }
    // …
};

class MyProject : public Spumoni::Project
{
public:
    MyProject() : Spumoni::Project(MyFormat()) { AddPart(m_Data); }
    // HasContent / OnReset tell Project whether there is anything to save.
private:
    DataPart m_Data;
};
```

Then `project.SaveAs("song.mypkg", error)`, `project.CreateSavePoint("before
the bridge", false, error)`, `project.OpenSavePoint(id, error)`. The
container follows the path: `song.mypkg` is a ZIP, `song.tgz` a tarball,
`song/` a folder. A file referenced by a part goes into the branch's
`assets/` through `Container::AddTree` on the way out and comes back through
`Folder::PathFor` on the way in.

## Building

libSpumoni depends on zlib and on libSlick++ (its JSON values). It is C++03.
Inside SpiralSynthModular it is a convenience library linked statically into
libSSMCore; `make check` here runs `spumoni-package-test`, which tells the
whole story (two save points, extras carried, open by id, a newer package
format refused at the package level, a manifest rewrite, the memory folder's
one door out) over zip, directory, tar and tgz, with a toy application.

## Status and direction

Ver 1 is what is described here. Coming: shared content-addressed assets
(Ver 2), checkpoints and a journal (Ver 3), a compressed-tar family beyond
gzip, and lazy materialisation of assets once parts can hand out streams
instead of paths.
