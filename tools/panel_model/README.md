# Panel file object model (draft)

The panel file is loaded as a dictionary. `PanelFile` is a small dataclass
containing that dictionary; nodes, controls, pages, bindings and future
extensions remain ordinary JSON objects and arrays. The reader has no registry
of control classes and does not need to understand a node's `type` to load it.

For example, a current `"Button"`, a possible `"Slick::Button"`, and an
unknown `"Other::NewControl"` all load the same way. The latter two spellings
are examples, not names or a namespace convention established by this draft.
Unknown fields at any depth are retained on read and write. An object inside an
unknown attribute is left alone, even if it has a `type` field: the reader
must not guess whether it represents a control.

```python
from pathlib import Path
from panel_model import loads, dumps

path = Path('Plugins/Devices/OscillatorPlugin/prebuilt-ui.json')
file = loads(path.read_text())
node = file.data['panel']['children'][0]
print(node['type'], node.get('name'))
node['tooltip'] = 'Edited tooltip'
text = dumps(file, indent=2)
```

## Running

Python 3.9+ and `simplejson` are needed. The package is used so JSON decimal
numbers can be read into `decimal.Decimal` and written without rounding;
the standard library's default float parsing loses precision in real panel
files. For example, from the repository root:

```sh
python3 -m venv .panel-model-venv
.panel-model-venv/bin/python -m pip install -r tools/panel_model/requirements.txt
```

Use a venv outside the checkout if preferred. Fractions in the dictionary are
`Decimal`, not `float`; code adding numbers can use `Decimal` too.

## What this does and does not promise

- Round trips preserve JSON **values** and missing fields, including unknown
  fields and unknown node types. They do not preserve whitespace, key spelling
  escapes, or the exact lexical spelling of numbers. No current file is
  rewritten by the tests.
- Reading accepts old, current or future version fields without inferring that
  their semantics are understood. It only requires a JSON object at the root.
- Duplicate JSON object keys are rejected: a dictionary cannot preserve both
  values. Malformed JSON and non-finite numbers are also rejected rather than
  written as invalid JSON. This is syntax safety, not panel validation.
- **Preservation is not behavioral compatibility.** No control is instantiated
  here, and no claims are made that either application can render an unknown
  control, apply an unknown field, or interpret a future version correctly.
- A separate, optional, versioned contract/registry can later distinguish a
  validated type from an unresolved one and verify known fields. Namespacing,
  contract versions, validation and error policy remain design decisions; the
  reader does not infer or enforce them. Neither public nor private application
  uses this draft yet.

Tests round-trip the public prebuilt panels and the handwritten LADSPA panel.
Another test uses a synthetic example based on the LADSPA generator; it is not
a live generated panel. The private corpus has not yet been checked.
