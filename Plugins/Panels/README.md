# Panels

Panels for devices whose code SSM doesn't ship. A device SSM builds carries
its panel inside its own `info.json`, under `ui`, and it is compiled into the
module or the host. A plugin loaded from outside, such as a LADSPA plugin,
has no `info.json` of ours, so its panel lives here instead, one folder per
panel, installed to `<plugins>/panels/<Folder>/info.json`.

Each `info.json` names what it is for and holds the panel document under
`ui`:

```json
{
  "schema_version": 1,
  "name": "GVerb",
  "type": "panel",
  "for": { "ladspa": 1216 },
  "ui": { "control_contracts": { "Controls": 1 }, "panel": { } }
}
```

## LADSPA

The LADSPA device writes its own Knobs, Sliders and Setup pages round
whatever plugin it has loaded (`libs/libspiralcore/Plugin/src/LADSPAPanel.cpp`).
A panel here whose `for.ladspa` is the loaded plugin's unique ID shows as the
Panel page beside those three. Its nodes bind the device's per-input
properties (`DEFAULT3`, `MIN3`, `MAX3`, `CLAMP3`, `VALUE3`, `CONNECTED3`) and
`UNIQUEID`, `PAGE` and `UPDATEINPUTS`.
