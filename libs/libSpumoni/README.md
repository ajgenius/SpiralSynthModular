# libSpumoni

Layered packages: a container (ZIP, tar, or a directory), a manifest, one
folder per save point, and a content-addressed store of assets shared by
every save point (Package Ver 2). It was brought over from SpiralTesting
`Foundation/libSpumoni` at `061828df`, the Store with its own commits.
Journals, JSON deltas and checkpoints from the private tree's history
are not included. The external JSON library that Package used for
manifests was removed; `Spumoni::JSON` in this directory is the JSON
reader of the whole tree (libspiralcore and the host read the file
contract, plugin manifests and the JSON form of a description through it).


This library is an AI-generated draft. It was written to work out packages
and the file pieces around them, not as a stable interface. Expect the
structure to be reworked, on the public tree and the private one, before
anything here is ABI stable.

It is built as `libSpumoni.a` and linked into `libspiralcore`, which
depends on it and never the reverse.

`Package::FormatVersion` is 3, as the private tree's: Ver 3 lets the
application's metadata live in a file of its own beside the manifest,
written when `Layout::MetadataName` is set; that file is not a journal.
A newer stamp is refused.

## JSON

`Spumoni::JSON` is a C++03 DOM:

- `ParseJSONText(text, &error)`, `ParseJSON(fileName, &error)`: strict
  (duplicate keys, invalid UTF-8, a NUL byte, more than 64 nested
  containers or 16 MiB are refused)
- `MakeNull`, `MakeBoolean`, `MakeNumber` (lexeme kept as text), `MakeString`,
  `MakeArray`, `MakeObject`
- `Get(key)`, `At(index)`, `Keys()` (sorted), `Size`, `GetType`, `Text`,
  `AsBool`, `Integer(long&)`
- `SetOwned` / `AppendOwned` (take ownership)
- `Duplicate`
- `Stringify(pretty)` with object keys sorted
- `JSONOwner` holds a value in `std::auto_ptr`

`Package::Application::Describe` / `Accept` take `Spumoni::JSON`.
