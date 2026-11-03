# Panel file object model (draft)

Two layers, deliberately separate:

1. **Reading** (`panel_model.py`): the panel file is loaded as a dictionary.
   `PanelFile` is a small dataclass holding it. Nodes, controls, pages,
   bindings and future extensions remain ordinary JSON objects and arrays.
   The reader has no registry of control classes and does not need to
   understand a node's `type` to load it, edit it, or save it again.
2. **Contracts** (`panel_contracts.py`, `contracts/*.json`): versioned,
   namespaced control descriptions, applied *after* reading. Resolution
   reports what each node is, without rewriting the file.

```python
from pathlib import Path
from panel_model import loads, dumps
from panel_contracts import Registry, resolve

file = loads(Path('Plugins/Devices/OscillatorPlugin/prebuilt-ui.json').read_text())
node = file.data['panel']['children'][0]
node['tooltip'] = 'Edited tooltip'          # data, not a Python class
text = dumps(file, indent=2)

registry = Registry.load('tools/panel_model/contracts')
report = resolve(file, registry, assume={'Slick': 1, 'Spiral': 1})
report.counts()      # Counter({'valid': 25})
report.assumed       # True: this file declares no control_contracts
```

## The middle ground, as mocked up

The first draft had one Python class per control (unknown type = failure).
The second had only dictionaries (nothing checked). This draft keeps the
dictionary as the model and moves the *class hierarchy* into data:

```json
{
  "namespace": "Slick",
  "version": 1,
  "controls": {
    "Control":      {"abstract": true, "required": ["bounds"],
                     "attributes": {"name": ["string"], "bounds": ["array"], "tooltip": ["object", "string"]}},
    "ValueControl": {"extends": "Slick::Control", "abstract": true,
                     "attributes": {"property": ["string"], "min": ["number", "object"]}},
    "Knob":         {"extends": "Slick::ValueControl", "attributes": {"scale_ticks": ["integer"], "style": ["string"]}},
    "Repeat":       {"extends": "Slick::Container"}
  }
}
```

(abridged; the real files are `contracts/slick-1.json` and `contracts/spiral-1.json`)

- A control is identified by `Namespace::Name`. Two namespaces are mocked:
  `Slick` (the shared control vocabulary, including the structural
  `Notebook`, `VirtualScrollArea` and `Repeat`) and `Spiral` (the scopes and
  Spiral's own controls: `LCD`, `LED`, `MatrixGrid`, `PortList`, ...).
  `Spiral::LCD` extends `Slick::TextEntry` across the namespace boundary.
- `extends` carries attributes and `required` down the chain; `abstract`
  bases cannot be used as a node type. Attribute values are checked against a
  small set of kinds (`string`, `integer`, `number`, `boolean`, `object`,
  `array`); `nodes` and `pages` mark where nested controls live, which is
  the only thing the walker needs to know to find them.
- A file selects contract versions per namespace, at the root:

  ```json
  {"control_contracts": {"Slick": 1, "Spiral": 1}, "panel": {}}
  ```

  A file without that key (every current file) resolves under versions the
  caller passes as `assume=`, and the report says `assumed`. Type names
  written without a namespace (`"Button"`, every current file) resolve if
  exactly one selected namespace defines them; `"Slick::Button"` also works.
  Nothing in the file is renamed either way.

Each node gets one of four statuses:

| status | meaning |
|---|---|
| `valid` | known contract, every field known and of the declared kind |
| `extended` | known contract plus fields it does not declare (kept, listed) |
| `invalid` | known contract, but a wrong kind, a missing required field, or an abstract type |
| `unresolved` | no usable contract: unknown name, undeclared or unavailable namespace/version, ambiguous short name, broken `extends` |

Unresolved nodes are preserved and not walked into (their contract would say
where their children are). Known containers are walked even when invalid.

```sh
PYTHONPATH=tools/panel_model python -m panel_contracts Plugins/Devices/MatrixPlugin/prebuilt-ui.json
# Plugins/Devices/MatrixPlugin/prebuilt-ui.json: {'valid': 117} under Slick 1 Spiral 1 (assumed)
```

## Where version 1 of the contracts comes from

`contracts_from_corpus.py` writes `contracts/slick-1.json` and
`contracts/spiral-1.json` (`--check` fails when they are stale; a test runs
it). The **hierarchy and namespace split are hand-written in that script and
are the proposal**. The **attribute names and kinds are an inventory**: what
the 48 checked-in `prebuilt-ui.json` panels and the hand-written LADSPA panel
contain, plus the shapes `LADSPAPanel.cpp` emits at run time (listed in the
script, with that citation). So "valid under Slick 1 + Spiral 1" currently
means "shaped like the panels the public branch writes today"; it is not yet
a reviewed statement of what each attribute means or which are optional.

Not checked by this mock: the shape inside `write`, `delta`, conditions,
localized text and the other nested objects (only their outer kind); array
items; binding targets against a device; the panel envelope (`layouts`,
`modes`, `properties`, `state`); whether either application can render a
node. Validity here is contract shape, not renderability.

## Running

Python 3.9+ and `simplejson` (so JSON decimals round-trip as
`decimal.Decimal` instead of being rounded through binary floats):

```sh
python3 -m venv .panel-model-venv
.panel-model-venv/bin/python -m pip install -r tools/panel_model/requirements.txt
```

Reading rejects malformed JSON, non-finite numbers and duplicate object keys
(a dictionary cannot keep both values); it accepts any version field without
inferring that it understands it. Round trips preserve JSON values and
missing fields, not whitespace, key order or number spelling. Preservation is
not behavioral compatibility: nothing here instantiates a control.

## What a converted panel looks like

`panel_convert.py` rewrites a panel the way the proposal would have it, using
the resolver's own answers: it declares `control_contracts`, writes each
resolved node's type as its qualified contract name, applies the Slick 2
fold, rewrites the action and condition spellings as bindings and event
handlers, drops every translation key the text's position implies, and folds
runs of choice buttons into one `Repeat` over items. Unresolved nodes and
everything else stay as written.

```sh
PYTHONPATH=tools/panel_model python -m panel_convert Plugins/Devices/DiskWriterPlugin/prebuilt-ui.json
```

### The file button is a Button with a `file` hook

DiskWriter's file button, before and after (the hook, its `then` writes and
`write_off` are untouched):

```json
{ "type": "FileChoosingButton", "name": "open", "bounds": [-2, 71, 75, 20],
  "text": { "default": "Open", "key": "plugin.41.panel.open.text" },
  "file": { "save": true, "extension": ".wav",
            "title": { "default": "Pick a Wav file to save to", "key": "plugin.41.panel.open.title" },
            "then": [ { "property": "COMMAND", "value": "OPENWAV", "with": ["text"] } ] },
  "write_off": [ { "property": "COMMAND", "value": "CLOSEWAV" } ] }
```

```json
{ "type": "Slick::Button", "name": "open", "bounds": [-2, 71, 75, 20],
  "text": "Open", "toggle": true,
  "file": { "save": true, "extension": ".wav", "title": "Pick a Wav file to save to" },
  "on": { "choose": [ { "write": "COMMAND", "value": "OPENWAV", "with": ["text"] } ],
          "off":    [ { "write": "COMMAND", "value": "CLOSEWAV" } ] } }
```

(`then` and `write_off` became handlers; see below.)

`contracts/slick-2.json` is Slick 1 with that fold: `FileChoosingButton` is
gone and its attributes (`file`, ...) join `Button`. The private loader
already treats a `Button` carrying `file` as the file-choosing control
(`Foundation/libSSMUI/Layout.cpp`, the `ControlRegistry::Instance().Create(
node->Get("file") ? "FileChoosingButton" : type)` line); the public loader
does the same for `list` (`PanelButton::Create`) but not yet for `file`, and
both trees' DiskWriter panels still write the node type. A converted file
declares `{"Slick": 2, "Spiral": 1}` and resolves valid; re-declared as Slick
1 its `Button` is `extended` by `file`, which is the version gate working.

### Translation keys are derived, not spelled

Today every text carries its catalog key, and the key repeats the plugin id
that the root already states:

```json
"text": { "default": "16bit", "key": "plugin.41.panel.bits-16.text" }
```

`panel_text.py` derives the key from where the text sits, so the file need
not say it: `plugin.<id>` from the root `plugin_id`, then

| text at | key |
|---|---|
| `panel.help` | `plugin.<id>.panel.help` |
| a control's `text`, `tooltip`, `file.title`, ... | `plugin.<id>.panel.<name>.<attribute>` |
| `pages[i].title` of a Notebook | `plugin.<id>.panel.<name>.page.<i>` |
| `items[i]` of a Choice | `plugin.<id>.panel.<name>.item.<i>` |
| `items[i].<attribute>` of a Repeat | `plugin.<id>.panel.<name>.item.<i>.<attribute>` |
| `properties[].display_name` / `.hint` | `plugin.<id>.property.<KEY>[.hint]` |
| `properties[].choices[i].display_name` | `plugin.<id>.property.<KEY>.choice.<i>` |

`<name>` is the control's `name` with any `-{index}` removed, so every copy
of a Repeat shares one key. A text that is then only a `default` collapses
to the plain string; one with `translations` keeps the object. A text is a
plain string or `{default, translations?, key?}` (the `text` kind in the
contracts), and a spelled `key` is an override that wins. The corpus needs
the override 49 times out of 1,175: three panels share
`ssm.default-panel.help`, one Repeat child and one `text_when.off` reuse
another text's key, and Matrix's and MixSwitch's properties blocks still
carry keys from an earlier property naming (`plugin.18.property.ECHOCUR` on
`CURRENT`, ...) that a key derived from today's property name would not
match. Those stay spelled; everything else goes.

The lookup side, `PluginUI::Translate(key, fallback)` in private, does not
change: the key is computed where the text is read instead of parsed from
the file. Public has no lookup yet.

### Attributes bind to state; events have handlers

Ten spellings say what a control does today: `write`, `write_off`, `delta`,
`sets`, `follows`, `clear_indicators`, `file.then`, `enabled_when`,
`visible_when`, `text_when`, and MatrixGrid's `on`/`off`/`set` command
names. The loaders show two ideas underneath (`GUI/Panels/PanelControl.cpp`,
`Controls/PanelButton.cpp`, `Spiral/PanelMatrixGrid.cpp`), and one design
smell: a button *is a toggle because it has `write_off`*
(`PanelControl::IsToggle`).

**A binding.** The attribute keeps its own name and takes a literal or a
binding object. "When" is one kind of binding; cases compose:

```json
"enabled": true
"enabled": { "property": "RECORDING", "equals": 0 }
"enabled": { "any": [ { "property": "RECORDING", "equals": 0 }, { "property": "ARMED" } ] }
"visible": { "property": "CURRENTSAMPLE", "equals": "{index}" }
"text":    { "property": "PLAYING", "on": "@||", "off": "@>" }
"value":   { "control": "meter", "attribute": "min" }
```

A binding's source is a property (its value or truth; compared with
`equals`, `not_equals` or `one_of`; or selecting an `on`/`off` text) or
another control's attribute (`control` + `attribute`, today's `follows`);
`any`, `all` and `not` compose them. The `binding` kind in the contracts
checks that shape, the one place the mock looks inside an object, because
bindings are the format's own grammar.

**An event with handlers.** One attribute, `on`, from event name to the
actions it runs; every action is one verb. The mode is stated, not inferred:

```json
{ "type": "Slick::Button", "name": "record", "property": "RECORDING", "text": "Record", "toggle": true,
  "on": { "on":  [ { "write": "COMMAND", "value": "RECORD" } ],
          "off": [ { "write": "COMMAND", "value": "STOP" } ] } }
```

```json
{ "type": "Slick::Button", "name": "open", "text": "Open", "toggle": true,
  "file": { "save": true, "extension": ".wav", "title": "Pick a Wav file to save to" },
  "on": { "choose": [ { "write": "COMMAND", "value": "OPENWAV", "with": ["text"] } ],
          "off":    [ { "write": "COMMAND", "value": "CLOSEWAV" } ] } }
```

```json
{ "type": "Slick::Button", "name": "remove", "text": "-",
  "on": { "press": [ { "delta": "CHANNELS", "amount": -1 }, { "clear": ["peak"] } ] } }
```

```json
{ "type": "Spiral::MatrixGrid", "name": "grid", "grid": "Grid", "step": "Step", "volume": "GridVolume",
  "on": { "activate":   [ { "write": "COMMAND", "value": "MAT_ACTIVATE",   "with": ["x", "y"] } ],
          "deactivate": [ { "write": "COMMAND", "value": "MAT_DEACTIVATE", "with": ["x", "y"] } ],
          "volume":     [ { "write": "COMMAND", "value": "MAT_VOLUME",     "with": ["x", "y", "volume"] } ] } }
```

Events, as the public loader fires them: `press` (a momentary button),
`on`/`off` (a toggle), `change` (a value control; also a button's own 1/0,
where `sets` and `clear_indicators` fire), `choose` (after the file dialog),
and the grid's `activate`/`deactivate`/`volume`. Each event names its
arguments and `with` picks them: `"value"`/`"text"` from a change, `x`/`y`/
`volume` from a cell, as today. Verbs: `write` (today's write entry with
the verb named; `delta` may bump a list of properties, as Jack's does),
`delta`, `set` (today's `sets`), `clear` (today's `clear_indicators`).
Handlers run in the order written; the converter keeps the loader's order
(delta, writes, set, clear).

`panel_events.py` does this rewrite and its inverse; on all 48 panels the
inverse gives the original back, so nothing is lost. 113 nodes change; 158
attributes become `on`, `enabled`, `visible`, `text` or `value`. In version 2
of both namespaces the old spellings are gone: a rewritten panel is valid
under `{"Slick": 2, "Spiral": 2}` and `extended` by `on`/`enabled` under
version 1 (a test), which is the gate working the other way round too.

### A run of choices is one definition repeated over items

Three LED buttons on DiskWriter select the bit depth. They are one
definition with a different `choice`, name, text and position each:

```json
{ "type": "LEDButton", "name": "bits-16", "bounds": [-2, 1, 23, 23], "choice": 16, "property": "BITS PER SAMPLE",
  "text": { "default": "16bit", "key": "plugin.41.panel.bits-16.text" }, "enabled_when": { "equals": 0, "property": "RECORDING" } },
{ "type": "LEDButton", "name": "bits-24", "bounds": [-2, 24, 23, 23], "choice": 24, "property": "BITS PER SAMPLE",
  "text": { "default": "24bit", "key": "plugin.41.panel.bits-24.text" }, "enabled_when": { "equals": 0, "property": "RECORDING" } },
{ "type": "LEDButton", "name": "bits-32", "bounds": [-2, 47, 23, 23], "choice": 32, "property": "BITS PER SAMPLE",
  "text": { "default": "32bit", "key": "plugin.41.panel.bits-32.text" }, "enabled_when": { "equals": 0, "property": "RECORDING" } }
```

Both loaders already have `Repeat`: a `children` template copied
`max_count` times (or as many as `count_property` says), `{index}` and
`{index0}` substituted, each copy offset by `step_x`/`step_y` from the
Repeat's own origin. `panel_repeat.py` writes that behaviour down (`expand`)
and extends it one step: `items`, an array of objects, instead of a count.
One copy per item; every key of the item is a variable the template may use:

```json
{ "type": "Slick::Repeat", "name": "bits", "bounds": [-2, 1, 23, 23], "step_y": 23,
  "items": [ { "choice": 16 }, { "choice": 24 }, { "choice": 32 } ],
  "children": [
    { "type": "Spiral::LEDButton", "name": "bits-{choice}", "bounds": [0, 0, 23, 23],
      "choice": "{choice}", "property": "BITS PER SAMPLE", "text": "{choice}bit",
      "enabled_when": { "equals": 0, "property": "RECORDING" } } ] }
```

A value that is exactly `"{choice}"` becomes the item's value with its own
kind (16, not `"16"`); inside a longer string it is spelled out. That is the
rule the public loader already applies to `"{index}"`. ComplexEnvelope's
mode buttons carry their own words, so the items do:

```json
{ "type": "Slick::Repeat", "name": "mode", "bounds": [125, 162, 70, 16], "step_y": 16,
  "items": [ { "choice": 0, "id": "straight", "text": "Straight", "tooltip": "Join the handles with straight lines" },
             { "choice": 1, "id": "smooth",   "text": "Smooth",   "tooltip": "A smooth curve through every handle" },
             { "choice": 2, "id": "bezier",   "text": "Bezier",   "tooltip": "The smooth curve with a handle pair on every point to pull it by" } ],
  "children": [
    { "type": "Slick::RadioButton", "name": "mode-{id}", "bounds": [0, 0, 70, 16],
      "choice": "{choice}", "property": "MODE", "text": "{text}", "tooltip": "{tooltip}" } ] }
```

Logic's seven operator buttons wrap into two columns, so there the items
carry `bounds` and the template says `"bounds": "{bounds}"`. Matrix's 64
step LEDs are purely arithmetic (`choice` is the index, 7 px apart) and
become a counted Repeat with `"choice": "{index}"`, which both loaders
could build today.

In the contracts a Repeat's `children` is a `template`, not `nodes`: the
resolver lets a placeholder stand for any kind there, and checks that every
placeholder names `index`, `index0` or a key that **every** item supplies
(`"{low}"` with one item lacking `low` is invalid). `items` is Slick 2.
Expanding a converted panel gives the original's controls back, for all 48
panels (a test); the fold is the converter's inverse of that expansion.

Across the corpus: 471 nodes become 390, ten Repeats replace 103 nodes, and
the files shrink to about three quarters of their size, most of it the keys. Two runs do not
fold because the names give no group: Meter's `vu`/`mm`, and Noise's
`noisetype-white-ledbutton`/`noisetype-pink-ledbutton`.

Every checked-in panel converts, resolves valid under the declared versions,
and, expanded, differs from its original only in the root key, the type
names and the spelled-out keys (a test checks exactly that). The converter
is a demonstration, not a migration tool: the checked-in panels are not
rewritten, and neither application reads `control_contracts`, qualified
names, derived keys, `items`, bindings or `on` yet.

## Open decisions this mock does not make

- The namespace separator and names (`Slick::`, `Spiral::`), and whether the
  scopes belong in `Spiral` (as mocked, after private's
  `Spiral::Controls::Scopes`) or in a third namespace.
- The envelope key (`control_contracts`) and whether versions are per
  namespace (as mocked), per panel, or per node.
- Extension policy: `extended` is reported, not rejected, here. Whether a
  renderer may ignore extensions, and whether an added optional attribute
  needs a new contract version, is undecided.
- `FileChoosingButton` folds into `Button` + `file` in Slick 2 (the decision
  recorded in the parity notes and already the private loader's behaviour).
  What the fold does not settle: private's schema requires `file.property`
  (the path lands in a string property, `then` writes follow), public passes
  the path as a command argument (`then[].with: ["text"]`, no
  `file.property`). That is the command-versus-property binding question, not
  the button.
- Items: substitution (as mocked; one mechanism, the one `{index}` already
  uses) versus overlaying each item onto a single template child, which
  would spare the `"text": "{text}"` lines but needs a rule for templates
  with several children. Whether the per-item key should be the position
  (`item.0`, as pages are) or the `choice` value. The name of the slug
  variable (`id`). Whether a run of RadioButtons is really a `Choice` with
  a presentation, which would make the group a control rather than a Repeat.
- Events: the vocabulary per control (which events a Knob or a grid has,
  and their arguments) is not yet in the contracts; `on` accepts any event
  name and checks only the action shape. The next step is a per-control
  `events: {change: ["value", "text"], ...}` declaration. `property` is
  itself a binding (`"value": {"property": "CUTOFF"}`, two-way) and
  `if_property` a build-time one; both keep their spelling here. Whether
  the public loader should keep inferring a toggle from an `off` handler
  while `toggle` is absent.
- Private-only node types (`PointPlot`, `Panel`, `TextArea`, ...) are
  unresolved under these contracts; a private namespace version has not been
  extracted. Neither application enforces any of this yet.
