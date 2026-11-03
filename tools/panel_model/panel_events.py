"""Bindings and events, in place of ten attribute spellings.

Today a panel says what a control does with `write`, `write_off`, `delta`,
`sets`, `follows`, `clear_indicators`, `file.then`, `enabled_when`,
`visible_when` and `text_when`. The loaders show these are two ideas:

**A binding**: an attribute whose value comes from state. The attribute keeps
its own name and takes a literal or a binding object; "when" is one kind of
binding and cases compose:

    "enabled": true
    "enabled": {"property": "RECORDING", "equals": 0}
    "enabled": {"any": [{"property": "RECORDING", "equals": 0}, {"property": "ARMED"}]}
    "text":    {"property": "PLAYING", "on": "@||", "off": "@>"}
    "value":   {"control": "meter", "attribute": "min"}

**An event with handlers**: one attribute, `on`, from event name to the list
of actions, each action one verb:

    "toggle": true,
    "on": {"on":  [{"write": "COMMAND", "value": "RECORD"}],
           "off": [{"write": "COMMAND", "value": "STOP"}]}

Events as the public loader fires them (GUI/Panels/PanelControl.cpp,
Controls/PanelButton.cpp, Controls/PanelFileChoosingButton.cpp): `press` for
a momentary button, `on`/`off` for a toggle, `change` for a value control
(and for a button's own 1/0 where `sets` and `clear_indicators` fire),
`choose` after the file dialog. Actions: `write` (today's write entry,
`{property, value?, with?}` with the verb named), `delta`, `set`, `clear`.

The loader infers that a button is a toggle from `write_off` being present
(`PanelControl::IsToggle`). Here the mode is stated: `toggle: true`.

`rewrite` is deterministic and `unrewrite` is its inverse on every current
panel; the converter uses the first and the tests prove the second.
"""
from __future__ import annotations

import copy

BUTTONS = {'Button', 'LEDButton', 'FileChoosingButton'}
BINDINGS = {'enabled_when': 'enabled', 'visible_when': 'visible'}
LEGACY = ('write', 'write_off', 'delta', 'sets', 'follows', 'clear_indicators', 'text_when') + tuple(BINDINGS)
# MatrixGrid spells three handlers as command names (GUI/Panels/Spiral/
# PanelMatrixGrid.cpp: Act() sends `on`/`off` with [x, y], Volume() sends
# `set` with [x, y, volume]). Each event names its arguments; `with` picks
# them, as it picks "value"/"text" from a change today.
GRID_EVENTS = {'on': ('activate', ['x', 'y']), 'off': ('deactivate', ['x', 'y']), 'set': ('volume', ['x', 'y', 'volume'])}


def rewrite(data) -> int:
    changed = 0
    for node in every_node(data):
        if rewrite_node(node):
            changed += 1
    return changed


def unrewrite(data) -> int:
    changed = 0
    for node in every_node(data):
        if unrewrite_node(node):
            changed += 1
    return changed


def every_node(data):
    def walk(items):
        for node in items:
            if not isinstance(node, dict):
                continue
            yield node
            if isinstance(node.get('children'), list):
                yield from walk(node['children'])
            for page in node.get('pages', []) or []:
                if isinstance(page, dict) and isinstance(page.get('children'), list):
                    yield from walk(page['children'])
    yield from walk(data['panel']['children'])


def short(type_name):
    return type_name.split('::')[-1] if isinstance(type_name, str) else ''


def rewrite_node(node) -> bool:
    kind = short(node.get('type'))
    if kind == 'MatrixGrid' and any(key in node for key in GRID_EVENTS):
        handlers = {}
        for key, (event, arguments) in GRID_EVENTS.items():
            if isinstance(node.get(key), str):
                handlers[event] = [{'write': 'COMMAND', 'value': node.pop(key), 'with': list(arguments)}]
        node['on'] = handlers
        return True
    if not any(key in node for key in LEGACY) and not (isinstance(node.get('file'), dict) and 'then' in node['file']):
        return False
    is_button = kind in BUTTONS
    is_file = kind == 'FileChoosingButton' or (kind == 'Button' and isinstance(node.get('file'), dict))
    toggle = 'write_off' in node or node.get('toggle') is True
    # Bindings.
    for old, new in BINDINGS.items():
        if old in node:
            node[new] = node.pop(old)
    if isinstance(node.get('text_when'), dict):
        when = node.pop('text_when')
        node['text'] = {'property': when['property'], 'on': when['on'], 'off': when['off']}
    if 'follows' in node:
        node['value'] = node.pop('follows')
    # Events.
    handlers = {}
    if is_file:
        then = node['file'].pop('then', None) if isinstance(node.get('file'), dict) else None
        if then:
            handlers['choose'] = [write_action(w) for w in then]
        node.pop('write', None)
    main = 'press' if is_button and not toggle else 'on' if is_button else 'change'
    if 'delta' in node:
        delta = node.pop('delta')
        action = {'delta': delta['property']}
        action.update({k: v for k, v in delta.items() if k != 'property'})
        handlers.setdefault('press', []).append(action)
    if 'write' in node:
        handlers.setdefault(main, []).extend(write_action(w) for w in node.pop('write'))
    if 'write_off' in node:
        handlers.setdefault('off', []).extend(write_action(w) for w in node.pop('write_off'))
    if 'sets' in node:
        sets = node.pop('sets')
        handlers.setdefault('change', []).append({'set': sets['control'], 'attribute': sets['attribute']})
    if 'clear_indicators' in node:
        event = 'press' if is_button and not toggle else 'change'
        handlers.setdefault(event, []).append({'clear': node.pop('clear_indicators')})
    if handlers:
        node['on'] = handlers
    if toggle:
        node['toggle'] = True
    return True


def write_action(entry):
    action = {'write': entry['property']}
    action.update({k: v for k, v in entry.items() if k != 'property'})
    return action


def unrewrite_node(node) -> bool:
    touched = False
    if short(node.get('type')) == 'MatrixGrid' and isinstance(node.get('on'), dict):
        handlers = node.pop('on')
        for key, (event, _) in GRID_EVENTS.items():
            if event in handlers:
                node[key] = handlers[event][0]['value']
        return True
    for old, new in BINDINGS.items():
        if isinstance(node.get(new), dict):
            node[old] = node.pop(new)
            touched = True
    if isinstance(node.get('text'), dict) and 'property' in node['text']:
        binding = node.pop('text')
        node['text'] = copy.deepcopy(binding['off'])
        node['text_when'] = {'off': binding['off'], 'on': binding['on'], 'property': binding['property']}
        touched = True
    if isinstance(node.get('value'), dict):
        node['follows'] = node.pop('value')
        touched = True
    handlers = node.get('on')
    if isinstance(handlers, dict):
        node.pop('on')
        touched = True
        for event, actions in handlers.items():
            for action in actions:
                if 'write' in action:
                    entry = {'property': action['write']}
                    entry.update({k: v for k, v in action.items() if k != 'write'})
                    if event == 'choose':
                        node.setdefault('file', {}).setdefault('then', []).append(entry)
                    elif event == 'off':
                        node.setdefault('write_off', []).append(entry)
                    else:
                        node.setdefault('write', []).append(entry)
                elif 'delta' in action:
                    delta = {'property': action['delta']}
                    delta.update({k: v for k, v in action.items() if k != 'delta'})
                    node['delta'] = delta
                elif 'set' in action:
                    node['sets'] = {'control': action['set'], 'attribute': action['attribute']}
                elif 'clear' in action:
                    node['clear_indicators'] = action['clear']
    if node.get('toggle') is True and 'write_off' in node:
        del node['toggle']       # the loader infers it from write_off; the file never said it
        touched = True
    return touched
