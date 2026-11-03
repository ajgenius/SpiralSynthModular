"""Rewrite a panel into the proposed form, using the resolver's answers.

`convert` declares `control_contracts`, writes every resolved node's type as
its qualified contract name, applies the version-2 fold (a
`FileChoosingButton` is a `Button` with a `file` hook), and drops every
translation key the text's position implies (`panel_text`). Unresolved
nodes, overriding keys and everything else in the file are left exactly as
written.
"""
from __future__ import annotations

import sys
from pathlib import Path

from panel_contracts import Registry, resolve
from panel_model import PanelFile, dumps, loads
from panel_events import rewrite
from panel_repeat import fold_runs
from panel_text import strip_keys

INVENTORY = {'Slick': 1, 'Spiral': 1}   # what today's files are shaped like
DECLARE = {'Slick': 2, 'Spiral': 2}     # what a converted file declares
FOLDS = {'Slick::FileChoosingButton': 'Slick::Button'}


def convert(panel_file: PanelFile, registry: Registry) -> PanelFile:
    data = loads(dumps(panel_file)).data            # a deep copy, Decimals intact
    copy = PanelFile(data)
    report = resolve(copy, registry, assume=INVENTORY)
    for node in report.nodes:
        if node.contract and node.status != 'unresolved':
            node.node['type'] = FOLDS.get(node.contract, node.contract)
    rewrite(data)         # bindings and events first: the key rule sees the final positions
    strip_keys(data)
    fold_runs(data)
    # The Repeats the fold made are bare "Repeat"; a second pass names them.
    for node in resolve(copy, registry, assume=DECLARE).nodes:
        if node.contract and node.status != 'unresolved':
            node.node['type'] = node.contract
    data.pop('control_contracts', None)
    return PanelFile({'control_contracts': dict(DECLARE), **data})


def main(argv):
    registry = Registry.load(Path(__file__).resolve().parent / 'contracts')
    for name in argv:
        converted = convert(loads(Path(name).read_text()), registry)
        print(dumps(converted, indent=2))
    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
