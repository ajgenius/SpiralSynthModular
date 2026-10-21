# panel-gen

Written by GROK (Grok Bot). Generates the `prebuilt-ui.json` panel documents
for the devices listed in `specs.py` from each device's registered
properties, so the property entries match the device exactly.

Generated here: PoshSampler, Distributor, Splitter, EnvFollower,
Transpose, Output.

**Hand-maintained, not generated:** NoteSnap (its keys sit inside a
VirtualScrollArea since d9657c2d; its `make` line is disabled), Midi,
SpiralLoop, Stream, DiskWriter, Jack and Controller (translations of
their editors), and every other device's panel. Edit those documents
directly.

Commands are named: a spec passes `commands={'NAME': number | {'command':
number, 'args': [channels...]}}` (the device header's `GUICommands`), the
helpers write `{"property": "COMMAND", "value": "NAME"}` and `finish`
adds the COMMAND property entry carrying the map.

## Usage

1. Configure and build the tree (default build directory `<root>/.build`).
2. Build the property dumper against that build:

       tools/panel-gen/build-dump.sh [build-dir]

3. Regenerate (writes into `Plugins/Devices/<Name>Plugin/prebuilt-ui.json`):

       PANELGEN_BUILD=<build-dir> python3 tools/panel-gen/specs.py

   `PANELGEN_BUILD` defaults to `<root>/.build`; `PANELGEN_DUMP` overrides
   the dumper's path.
4. `git diff` should be empty unless you changed a spec or a device's
   properties. Run `make check` (device-panels) after any change.

## Names

Control names are stable (`open`, `load`, `operator-and`, ...) and carry
no generated numbers: names feed translation keys and test lookups. A
duplicate name stops the generator; give repeated controls a prefix with
`pl.prefix = 'sample1-'` (as PoshSampler does) and reset it afterwards.

## Files

- `specs.py`: one `make(...)` per device: layout, size, help text.
- `panels.py`: control helpers (knob, spin, led, button, file), property
  entries and the document writer.
- `dump.cpp`, `build-dump.sh`: prints a device's properties as JSON, or
  its state version with a second argument.
