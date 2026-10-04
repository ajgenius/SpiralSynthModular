# libSpumoni (v1)

Layered packages: a container (ZIP, tar, or a directory), a manifest, and
one folder per save point. This tree is Spumoni v1 only. It was brought
over from SpiralTesting `Foundation/libSpumoni` at `061828df`. Shared
content-addressed stores (v2) and journals, JSON deltas, and checkpoints
(v3) are not included. The external JSON library that Package used for
manifests was removed; manifests use `Spumoni::JSON` in this directory.


This library is an AI-generated draft. It was written to work out packages
and the file pieces around them, not as a stable interface. Expect the
structure to be reworked, on the public tree and the private one, before
anything here is ABI stable.

Nothing in SpiralSynthModular calls this library yet. It is built as
`libSpumoni.a` and linked into `libspiralcore`.

`Package::FormatVersion` is 1. A newer stamp is refused. An optional
application metadata file is still written when `Layout::MetadataName`
is set; that file is not a journal.

## JSON

`Spumoni::JSON` is a C++03 DOM:

- `ParseJSON(text, &error)`
- `MakeNull`, `MakeBool`, `MakeNumber` (lexeme kept as text), `MakeString`,
  `MakeArray`, `MakeObject`
- `Get(key)`, `At(index)`, `Keys()` (sorted), `Size`, `GetType`, `Text`, `Bool`
- `SetOwned` / `AppendOwned` (take ownership)
- `Duplicate`
- `Stringify(pretty)` with object keys sorted
- `JSONOwner` holds a value in `std::auto_ptr`

`Package::Application::Describe` / `Accept` take `Spumoni::JSON`.
