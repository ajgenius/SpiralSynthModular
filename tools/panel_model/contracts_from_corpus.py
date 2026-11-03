#!/usr/bin/env python3
"""Write contracts/slick-1.json and contracts/spiral-1.json.

Version 1 of each namespace is an INVENTORY of what the public panel branch
writes today: the attribute names and value kinds come from the checked-in
panels plus the shapes the LADSPA generator emits. The class hierarchy and the
namespace split below are the proposal; the attribute lists are observations.
Run from the repository root; `--check` exits 1 when the files are stale.
"""
import json
import sys
from collections import defaultdict
from decimal import Decimal
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from panel_model import loads  # noqa: E402
from panel_contracts import is_text  # noqa: E402

ROOT = Path.cwd()
CONTRACTS = Path(__file__).resolve().parent / 'contracts'

# The proposal. `attributes` on an abstract base lists the names hoisted to
# it; a concrete control's own attributes are whatever was observed on it and
# is not already inherited. Scopes follow private's Spiral::Controls::Scopes.
HIERARCHY = {
    'Slick': {
        'Control': dict(abstract=True, required=['bounds'],
                        attributes=['name', 'bounds', 'tooltip', 'if_property', 'visible_when', 'enabled_when']),
        'Container': dict(abstract=True, extends='Slick::Control', attributes=['children']),
        'ValueControl': dict(abstract=True, extends='Slick::Control',
                             attributes=['property', 'min', 'max', 'step', 'page', 'mapping', 'sets', 'write']),
        'Knob': dict(extends='Slick::ValueControl'),
        'Slider': dict(extends='Slick::ValueControl'),
        'Spinner': dict(extends='Slick::ValueControl'),
        'Button': dict(extends='Slick::Control'),
        'FileChoosingButton': dict(extends='Slick::Button'),
        'RadioButton': dict(extends='Slick::Control'),
        'CheckBox': dict(extends='Slick::Control'),
        'Choice': dict(extends='Slick::Control'),
        'Label': dict(extends='Slick::Control'),
        'TextEntry': dict(extends='Slick::Control'),
        'Notebook': dict(extends='Slick::Control'),
        'VirtualScrollArea': dict(extends='Slick::Container'),
        'Repeat': dict(extends='Slick::Container'),
    },
    'Spiral': {
        'LCD': dict(extends='Slick::TextEntry'),
        'LED': dict(extends='Slick::Control'),
        'LEDButton': dict(extends='Slick::Control'),
        'SevenSegment': dict(extends='Slick::Control'),
        'Scope': dict(extends='Slick::Control'),
        'FunctionPlot': dict(extends='Slick::Control'),
        'MeterBar': dict(extends='Slick::Control'),
        'EnvelopeCurve': dict(extends='Slick::Control'),
        'PianoKeys': dict(extends='Slick::Control'),
        'SampleWaveform': dict(extends='Slick::Control'),
        'MatrixGrid': dict(extends='Slick::Control'),
        'LoopWaveform': dict(extends='Slick::Control'),
        'ValueTable': dict(extends='Slick::Control'),
        'PortList': dict(extends='Slick::Control'),
    },
}

# Shapes written at run time by libs/libspiralcore/Plugin/src/LADSPAPanel.cpp
# (no checked-in file carries them).
GENERATED = [
    ('Label', 'align', 'string'),
    ('TextEntry', 'box', 'string'), ('TextEntry', 'write', 'array'),
    ('VirtualScrollArea', 'show_horizontal', 'boolean'), ('VirtualScrollArea', 'content_size', 'array'),
    ('VirtualScrollArea', 'box', 'string'),
    ('CheckBox', 'write', 'array'),
    ('Choice', 'size', 'integer'), ('Choice', 'write', 'array'),
    ('LEDButton', 'size', 'integer'), ('LEDButton', 'text', 'string'),
    ('Knob', 'min', 'object'), ('Knob', 'max', 'object'), ('Knob', 'enabled_when', 'object'),
    ('Slider', 'enabled_when', 'object'),
    ('TextEntry', 'enabled_when', 'object'), ('CheckBox', 'enabled_when', 'object'),
    ('Button', 'enabled_when', 'object'),
]

CHILD_KINDS = {'children': 'nodes', 'pages': 'pages'}
# A Repeat's children are a template: "{index}" already stands for a number
# in them (Panel::BuildRepeat substitutes the quoted form), so they are
# declared `template`, not `nodes`, from version 1.
TEMPLATE_OWNERS = {'Repeat'}

# Version 2 of Slick applies the "X plus one setting is a key on X" rule the
# private loader already follows (Layout.cpp creates a FileChoosingButton
# control for any Button node carrying `file`): the node type goes, its
# attributes join Button's. Nothing else changes between the two versions.
FOLDS = {2: {'FileChoosingButton': 'Button'}}
# ... and lets a Repeat carry `items` (one copy per item, each key a template
# variable) instead of a count. See panel_repeat.py.
#
# ... and replaces ten attribute spellings by two ideas (see panel_events.py):
# a binding, where an attribute keeps its name and takes a literal or a
# binding object (`enabled`, `visible`, `text`, `value`), and an event with
# handlers (`on`). A button states that it is a `toggle` instead of the
# loader inferring it from `write_off`. The old spellings leave version 2.
# `*` applies to every control that has the attribute, adding to its kinds.
ADDITIONS = {2: {
    'Repeat': {'items': ['array']},
    'Control': {'on': ['handlers'], 'enabled': ['boolean', 'binding'], 'visible': ['boolean', 'binding'],
                'value': ['binding']},
    'Button': {'toggle': ['boolean']}, 'LEDButton': {'toggle': ['boolean']},
    '*': {'text': ['binding']},
}}
REMOVALS = {2: {
    '*': {'enabled_when', 'visible_when', 'text_when', 'write', 'write_off', 'delta', 'sets', 'follows',
          'clear_indicators'},
    # The grid's cell commands become handlers (activate/deactivate/volume).
    'MatrixGrid': {'on', 'off', 'set'},
}}


def kind_of(value):
    if isinstance(value, bool):
        return 'boolean'
    if isinstance(value, int):
        return 'integer'
    if isinstance(value, (float, Decimal)):
        return 'number'
    if is_text(value) and isinstance(value, dict):
        return 'text'
    return {str: 'string', dict: 'object', list: 'array', type(None): 'null'}[type(value)]


def observe():
    seen = defaultdict(lambda: defaultdict(set))

    def walk(nodes):
        for node in nodes:
            if not isinstance(node, dict) or not isinstance(node.get('type'), str):
                continue
            for key, value in node.items():
                if key != 'type':
                    seen[node['type']][key].add(kind_of(value))
            if isinstance(node.get('children'), list):
                walk(node['children'])
            if isinstance(node.get('pages'), list):
                for page in node['pages']:
                    if isinstance(page, dict) and isinstance(page.get('children'), list):
                        walk(page['children'])

    paths = sorted(ROOT.glob('Plugins/Devices/*/prebuilt-ui.json'))
    paths.append(ROOT / 'Plugins/Panels/LADSPAPlugin/1216.json')
    if len(paths) < 2:
        sys.exit('run from the repository root: no panels found')
    for path in paths:
        walk(loads(path.read_text()).data['panel']['children'])
    for type_name, key, kind in GENERATED:
        seen[type_name][key].add(kind)
    return seen


def kinds(observed):
    names = set(observed)
    if 'number' in names:
        names.discard('integer')
    if 'text' in names:
        # A localizable text is written as a plain string or as the localized
        # object; one kind covers both spellings.
        names.discard('string')
    return sorted(names)


def build(seen):
    specs = {f'{ns}::{short}': dict(spec, namespace=ns, short=short)
             for ns, controls in HIERARCHY.items() for short, spec in controls.items()}
    unplaced = sorted(set(seen) - {s['short'] for s in specs.values()})
    if unplaced:
        sys.exit(f'observed types with no place in HIERARCHY: {unplaced}')

    def descendants(qualified):
        return [q for q, s in specs.items() if s.get('extends') == qualified]

    def subtree(qualified):
        found = [qualified]
        for child in descendants(qualified):
            found.extend(subtree(child))
        return found

    def ancestors(qualified):
        chain = []
        while specs[qualified].get('extends'):
            qualified = specs[qualified]['extends']
            chain.append(qualified)
        return chain

    attributes = {}
    for qualified, spec in specs.items():
        if spec.get('abstract'):
            hoisted = {}
            for name in spec['attributes']:
                observed = set()
                for member in subtree(qualified):
                    observed |= seen.get(specs[member]['short'], {}).get(name, set())
                if not observed:
                    sys.exit(f'{qualified}.{name}: hoisted attribute never observed')
                hoisted[name] = observed
            attributes[qualified] = hoisted
        else:
            attributes[qualified] = {k: set(v) for k, v in seen.get(spec['short'], {}).items()}
    for qualified, spec in specs.items():
        if not spec.get('abstract'):
            inherited = set()
            for base in ancestors(qualified):
                inherited |= set(attributes[base])
            attributes[qualified] = {k: v for k, v in attributes[qualified].items() if k not in inherited}

    documents = {}
    for ns, controls in HIERARCHY.items():
        document = {'namespace': ns, 'version': 1, 'controls': {}}
        for short, spec in controls.items():
            qualified = f'{ns}::{short}'
            entry = {}
            if spec.get('extends'):
                entry['extends'] = spec['extends']
            if spec.get('abstract'):
                entry['abstract'] = True
            if spec.get('required'):
                entry['required'] = list(spec['required'])
            entry['attributes'] = {name: [child_kind(short, name)] if name in CHILD_KINDS else kinds(observed)
                                   for name, observed in sorted(attributes[qualified].items())}
            if short in TEMPLATE_OWNERS:
                # Overrides the `nodes` kind inherited from the container base.
                entry['attributes'] = dict(sorted({**entry['attributes'], 'children': ['template']}.items()))
            document['controls'][short] = entry
        documents[f'{ns.lower()}-1.json'] = compact(json.dumps(document, indent=2)) + '\n'
        for version in sorted(set(FOLDS) | set(ADDITIONS) | set(REMOVALS)):
            later = fold(document, {k: v for k, v in FOLDS.get(version, {}).items() if k in document['controls']})
            if later is None:
                later = dict(document, controls={k: dict(v, attributes=dict(v['attributes']))
                                                 for k, v in document['controls'].items()})
            for short, gone in REMOVALS.get(version, {}).items():
                for target, spec in later['controls'].items():
                    if short in ('*', target):
                        for name in gone:
                            spec['attributes'].pop(name, None)
            for short, added in ADDITIONS.get(version, {}).items():
                targets = [s for s, spec in later['controls'].items()
                           if (short == '*' and set(added) & set(spec['attributes'])) or s == short]
                for target in targets:
                    attrs = later['controls'][target]['attributes']
                    for name, extra in added.items():
                        if short == '*' and name not in attrs:
                            continue
                        attrs[name] = kinds(set(attrs.get(name, [])) | set(extra))
                    later['controls'][target]['attributes'] = dict(sorted(attrs.items()))
            later = dict(later, version=version)
            documents[f'{ns.lower()}-{version}.json'] = compact(json.dumps(later, indent=2)) + '\n'
    return documents


def child_kind(control, attribute):
    if attribute == 'children' and control in TEMPLATE_OWNERS:
        return 'template'
    return CHILD_KINDS[attribute]


def fold(document, folds):
    """A later version in which each folded node type's own attributes join
    the target's and the node type itself is gone."""
    if not folds:
        return None
    controls = {short: dict(spec, attributes=dict(spec['attributes']))
                for short, spec in document['controls'].items()}
    for gone, into in folds.items():
        if controls[gone].get('extends') != f"{document['namespace']}::{into}":
            sys.exit(f'{gone} can only fold into its base, not {into}')
        for name, kinds_ in controls[gone]['attributes'].items():
            merged = set(controls[into]['attributes'].get(name, [])) | set(kinds_)
            controls[into]['attributes'][name] = kinds(merged)
        controls[into]['attributes'] = dict(sorted(controls[into]['attributes'].items()))
        del controls[gone]
    return dict(document, controls=controls)


def compact(text):
    """Keep each kind list on one line so a contract reads as a table."""
    import re
    return re.sub(r'\[\s+("[^"]*"(?:,\s+"[^"]*")*)\s+\]', lambda m: '[' + re.sub(r',\s+', ', ', m.group(1)) + ']', text)


def main(argv):
    documents = build(observe())
    stale = []
    for name, text in documents.items():
        path = CONTRACTS / name
        if '--check' in argv:
            if not path.exists() or path.read_text() != text:
                stale.append(name)
        else:
            CONTRACTS.mkdir(exist_ok=True)
            path.write_text(text)
            print(f'wrote {path.relative_to(ROOT) if path.is_relative_to(ROOT) else path}')
    if stale:
        print(f'stale: {", ".join(stale)} (run {Path(__file__).name} to regenerate)')
        return 1
    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
