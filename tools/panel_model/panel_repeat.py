"""Repeat: one definition, many copies.

The node both loaders already have repeats its `children` template by a
count (`count_property`/`max_count`), substituting `{index}` and `{index0}`
and offsetting each copy by `step_x`/`step_y` from the Repeat's own origin.

This module writes that behaviour down (`expand`) and extends it one step: a
Repeat may carry `items`, an array of objects, instead of a count. Then there
is one copy per item and every key of the item is a variable the template
may use as `{key}`:

    {"type": "Repeat", "name": "bits", "bounds": [-2, 1, 23, 23], "step_y": 23,
     "items": [{"choice": 16}, {"choice": 24}, {"choice": 32}],
     "children": [{"type": "LEDButton", "name": "bits-{choice}", "bounds": [0, 0, 23, 23],
                   "property": "BITS PER SAMPLE", "choice": "{choice}", "text": "{choice}bit"}]}

A value that is exactly `"{key}"` becomes the item's value with its own JSON
kind (16, not "16"; a localized text object stays an object); a placeholder
inside a longer string is spelled out. That is the rule the public loader
already applies to `"{index}"`.

`fold_runs` is the inverse used by the converter: runs of sibling nodes
that share a type and a property and differ only in `choice` (and in what
hangs off it) become one Repeat over items, or a counted Repeat when the run
is purely arithmetic.
"""
from __future__ import annotations

import re
from decimal import Decimal

PLACEHOLDER = re.compile(r'\{([A-Za-z_][A-Za-z0-9_]*)\}')


def expand(data) -> int:
    """Replace every Repeat in `data['panel']` by its copies, in place.
    Returns how many were expanded. Copies carry absolute bounds."""
    count = 0

    def walk(nodes, origin):
        nonlocal count
        out = []
        for node in nodes:
            if isinstance(node, dict) and node.get('type', '').split('::')[-1] == 'Repeat':
                count += 1
                out.extend(copies(node, origin))
                continue
            if isinstance(node, dict):
                if isinstance(node.get('children'), list):
                    node['children'] = walk(node['children'], origin)
                for page in node.get('pages', []) or []:
                    if isinstance(page, dict) and isinstance(page.get('children'), list):
                        page['children'] = walk(page['children'], origin)
            out.append(node)
        return out

    def copies(repeat, origin):
        bounds = repeat.get('bounds') or [0, 0]
        base = int(repeat.get('index_base', 0))
        step_x, step_y = int(repeat.get('step_x', 0)), int(repeat.get('step_y', 0))
        items = repeat.get('items')
        if isinstance(items, list):
            variables = [dict(item) for item in items]
        else:
            variables = [{} for _ in range(int(repeat.get('max_count', 1)))]
        result = []
        for i, item in enumerate(variables):
            scope = dict(item, index=i + base, index0=i)
            at = (origin[0] + int(bounds[0]) + step_x * i, origin[1] + int(bounds[1]) + step_y * i)
            for child in repeat.get('children', []):
                copy = substitute(child, scope)
                if isinstance(copy, dict) and isinstance(copy.get('bounds'), list) and len(copy['bounds']) >= 2:
                    copy['bounds'] = [copy['bounds'][0] + at[0], copy['bounds'][1] + at[1]] + list(copy['bounds'][2:])
                result.extend(walk([copy], at) if isinstance(copy, dict) else [copy])
        return result

    data['panel']['children'] = walk(data['panel']['children'], (0, 0))
    return count


def substitute(value, scope):
    if isinstance(value, dict):
        return {k: substitute(v, scope) for k, v in value.items()}
    if isinstance(value, list):
        return [substitute(v, scope) for v in value]
    if isinstance(value, str):
        whole = PLACEHOLDER.fullmatch(value)
        if whole and whole.group(1) in scope:
            return substitute(scope[whole.group(1)], {})   # the item's value, its own kind
        return PLACEHOLDER.sub(lambda m: spell(scope[m.group(1)]) if m.group(1) in scope else m.group(0), value)
    return value


def spell(value) -> str:
    if isinstance(value, bool):
        return 'true' if value else 'false'
    if isinstance(value, dict) and isinstance(value.get('default'), str):
        return value['default']
    return str(value)


# --- folding -----------------------------------------------------------------

SLUG = re.compile(r'^(?P<base>.+)-(?P<slug>[^-]+)$')


def split_name(name: str) -> tuple[str, str]:
    """`mode-straight` -> ('mode', 'straight'); callers check `foldable` first."""
    match = SLUG.match(name)
    assert match is not None, name
    return match.group('base'), match.group('slug')


def fold_runs(data) -> int:
    """Fold every run of sibling choice nodes into a Repeat, in place.
    Returns how many Repeats were made."""
    made = 0

    def walk(nodes):
        nonlocal made
        out, i = [], 0
        while i < len(nodes):
            run = run_at(nodes, i)
            if run:
                out.append(fold(run))
                made += 1
                i += len(run)
                continue
            node = nodes[i]
            if isinstance(node, dict):
                if isinstance(node.get('children'), list):
                    node['children'] = walk(node['children'])
                for page in node.get('pages', []) or []:
                    if isinstance(page, dict) and isinstance(page.get('children'), list):
                        page['children'] = walk(page['children'])
            out.append(node)
            i += 1
        return out

    data['panel']['children'] = walk(data['panel']['children'])
    return made


def run_at(nodes, start):
    """The longest run from `start` that `fold` can represent, or None."""
    first = nodes[start]
    if not foldable(first):
        return None
    run = [first]
    for node in nodes[start + 1:]:
        if not foldable(node) or (node['type'], node['property']) != (first['type'], first['property']):
            break
        if split_name(node['name'])[0] != split_name(first['name'])[0]:
            break
        if node['bounds'][2:] != first['bounds'][2:]:
            break
        run.append(node)
    if len(run) < 2 or len({n['choice'] for n in run}) != len(run):
        return None
    constant = {k for k in first if k not in ('name', 'bounds', 'choice') and all(n.get(k) == first[k] for n in run)}
    if any(set(n) - constant - {'name', 'bounds', 'choice'} - VARIABLE for n in run):
        return None
    return run


VARIABLE = {'text', 'tooltip', 'glyph'}   # what may differ per item besides choice, name and bounds


def foldable(node):
    return (isinstance(node, dict) and isinstance(node.get('property'), str) and 'choice' in node
            and isinstance(node.get('name'), str) and SLUG.match(node['name']) is not None
            and isinstance(node.get('bounds'), list) and len(node['bounds']) == 4
            and not isinstance(node.get('children'), list))


def fold(run):
    first = run[0]
    base = split_name(first['name'])[0]
    slugs = [split_name(n['name'])[1] for n in run]
    keys = [k for k in first if k not in ('name', 'bounds')]
    varying = [k for k in keys if any(n.get(k) != first[k] for n in run)]
    template = {'type': first['type'], 'name': None, 'bounds': None}
    template.update({k: ('{%s}' % k if k in varying else first[k]) for k in keys})
    items = [{k: n[k] for k in varying} for n in run]
    by_choice = all(s == str(n['choice']) for s, n in zip(slugs, run))
    if by_choice:
        template['name'] = f'{base}-{{choice}}'
    else:
        template['name'] = f'{base}-{{id}}'
        for item, slug in zip(items, slugs):
            item['id'] = slug
    for key in varying:
        if key == 'text' and all(isinstance(n['text'], str) and n['text'] == f"{n['choice']}bit" for n in run):
            template['text'] = '{choice}bit'
            for item in items:
                del item['text']
    x0, y0, w, h = first['bounds']
    dx = {n['bounds'][0] - m['bounds'][0] for m, n in zip(run, run[1:])}
    dy = {n['bounds'][1] - m['bounds'][1] for m, n in zip(run, run[1:])}
    repeat = {'type': 'Repeat', 'name': base, 'bounds': [x0, y0, w, h]}
    if len(dx) == 1 and len(dy) == 1:
        template['bounds'] = [0, 0, w, h]
        if dx != {0}:
            repeat['step_x'] = dx.pop()
        if dy != {0}:
            repeat['step_y'] = dy.pop()
    else:
        template['bounds'] = '{bounds}'
        for item, n in zip(items, run):
            item['bounds'] = [n['bounds'][0] - x0, n['bounds'][1] - y0, w, h]
    arithmetic = (by_choice and varying == ['choice'] and template['bounds'] != '{bounds}'
                  and all(n['choice'] == i for i, n in enumerate(run)))
    if arithmetic:
        repeat['max_count'] = len(run)
        template['choice'] = '{index}'
        template['name'] = f'{base}-{{index}}'
    else:
        repeat['items'] = items
    ordered = {k: template[k] for k in ('type', 'name', 'bounds')}
    ordered.update({k: v for k, v in template.items() if k not in ordered})
    repeat['children'] = [ordered]
    return repeat
