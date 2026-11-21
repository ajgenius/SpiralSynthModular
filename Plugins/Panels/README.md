# Panels

Panels kept as files. A device SSM builds carries its panel inside its own
`info.json`, under `ui`, and it is compiled into the module or the host. A
panel file here is for what has no such panel, or is to be shown in place of
one: one folder per panel, installed to `<plugins>/panels/<Folder>/info.json`.

- A plugin loaded from outside, such as a LADSPA plugin, has no `info.json`
  of ours, so its panel is a file: `"for": { "ladspa": <unique id> }`.
- For a device, `"for": { "device": <its id> }`. The file's panel is shown
  in place of the one the device has built in, and is the panel of a device
  that has none.

## Where panel files are looked for

First to last, and the first folder with a file for something wins:

1. each folder of `SSM_PANEL_PATH` (separated by `:`),
2. `~/.spiral/panels`,
3. the installed `<plugins>/panels`.

A file is read when the panel is opened, so one can be edited and the panel
opened again without starting SSM again.

A device's panel file is taken whole or not at all. If it does not load, or
asks for a control or a property this build has not got, SSM says so on its
error output and shows the device's own panel.

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
