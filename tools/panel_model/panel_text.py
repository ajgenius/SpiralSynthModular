"""Translation keys derived from where a text sits, instead of spelled out.

A text is a plain string or `{"default": ..., "translations": {...}}`. Its
catalog key is implied by the plugin id at the root and the text's place:

    panel.help                              plugin.<id>.panel.help
    <control>.text / .tooltip / .file.title plugin.<id>.panel.<name>.<attribute>
    <Notebook>.pages[i].title               plugin.<id>.panel.<name>.page.<i>
    <Choice>.items[i]                       plugin.<id>.panel.<name>.item.<i>
    properties[].display_name / .hint       plugin.<id>.property.<KEY>[.hint]
    properties[].choices[i].display_name    plugin.<id>.property.<KEY>.choice.<i>

`<name>` is the control's `name` with any `-{index}` suffix removed, so every
copy of a Repeat shares one key. An explicit `key` on a text is an override
and wins; it is what a text shared between controls or panels needs.
"""
from __future__ import annotations

import re

INDEX = re.compile(r'-?\{index0?\}')
TEXT_ATTRIBUTES = ('text', 'tooltip', 'title', 'on', 'off', 'placeholder', 'help', 'display_name', 'hint')


TEXT_KEYS = {'default', 'key', 'translations'}


def texts(data, path=None):
    """Every text object with the path it sits at. A text object is a dict of
    `default` (a string) plus optionally `key` and `translations`; a property
    description whose default VALUE is a string is not one."""
    path = path or []
    if isinstance(data, dict):
        if isinstance(data.get('default'), str) and set(data) <= TEXT_KEYS:
            yield path, data
        for key, value in data.items():
            yield from texts(value, path + [key])
    elif isinstance(data, list):
        for index, value in enumerate(data):
            yield from texts(value, path + [index])


def key_for(data, path):
    """The derived key for the text at `path`, or None when nothing implies one."""
    plugin_id = data.get('plugin_id')
    if not isinstance(plugin_id, int) or isinstance(plugin_id, bool) or not path:
        return None
    head = path[0]
    if head in ('properties', 'state') and len(path) >= 3:
        entry = data[head][path[1]]
        key = entry.get('property') if isinstance(entry, dict) else None
        rest = path[2:]
        if not isinstance(key, str):
            return None
        if rest == ['display_name']:
            return f'plugin.{plugin_id}.property.{key}'
        if rest == ['hint']:
            return f'plugin.{plugin_id}.property.{key}.hint'
        if len(rest) == 3 and rest[0] == 'choices' and rest[2] == 'display_name':
            return f'plugin.{plugin_id}.property.{key}.choice.{rest[1]}'
        return None
    if path == ['panel', 'help']:
        return f'plugin.{plugin_id}.panel.help'
    node, name, at = None, None, 0
    walk = data
    for index, step in enumerate(path):
        walk = walk[step]
        if isinstance(walk, dict) and 'type' in walk:
            node, name, at = walk, walk.get('name'), index
    if node is None or not isinstance(name, str) or not name:
        return None
    base = INDEX.sub('', name)
    rest = path[at + 1:]
    if len(rest) == 3 and rest[0] == 'pages' and rest[2] == 'title':
        return f'plugin.{plugin_id}.panel.{base}.page.{rest[1]}'
    if len(rest) == 2 and rest[0] == 'items':
        return f'plugin.{plugin_id}.panel.{base}.item.{rest[1]}'
    if len(rest) == 3 and rest[0] == 'items' and rest[2] in TEXT_ATTRIBUTES:
        return f'plugin.{plugin_id}.panel.{base}.item.{rest[1]}.{rest[2]}'
    if len(rest) == 2 and rest[0] in TEXT_ATTRIBUTES and rest[1] in ('on', 'off'):
        return f'plugin.{plugin_id}.panel.{base}.{rest[0]}.{rest[1]}'      # a text binding's cases
    if rest and rest[-1] in TEXT_ATTRIBUTES:
        return f'plugin.{plugin_id}.panel.{base}.{rest[-1]}'
    return None


def strip_keys(data):
    """Remove every `key` the rule would derive, collapsing a text that is
    then only a `default` into the plain string. Returns (stripped, kept)."""
    stripped = kept = 0
    for path, text in list(texts(data)):
        if 'key' not in text:
            continue
        if text['key'] != key_for(data, path):
            kept += 1
            continue
        del text['key']
        stripped += 1
        if list(text) == ['default']:
            parent = data
            for step in path[:-1]:
                parent = parent[step]
            parent[path[-1]] = text['default']
    return stripped, kept
