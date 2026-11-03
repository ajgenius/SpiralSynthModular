"""Versioned control contracts, resolved against a panel after it is read.

Reading a panel (`panel_model.loads`) never needs these. A contract names a
control type inside a namespace, its attributes and value kinds, its base
contract, and which attributes hold nested controls. Resolution reports, per
node: valid, extended (known type carrying unknown fields), invalid, or
unresolved (no usable contract). Nothing in the file is rewritten.
"""
from __future__ import annotations

from collections import Counter
from dataclasses import dataclass, field
from decimal import Decimal
import json
from pathlib import Path
import re
import sys
from typing import Any, Optional


def is_text(value):
    """A localizable text: a plain string, or an object of `default` (string)
    plus optionally `key` and `translations`."""
    return isinstance(value, str) or (
        isinstance(value, dict) and isinstance(value.get('default'), str)
        and set(value) <= {'default', 'key', 'translations'})


COMPARISONS = {'equals', 'not_equals', 'one_of'}


def is_binding(value):
    """A value taken from state. Sources: a property (optionally compared, or
    selecting an `on`/`off` text by its truth) or another control's
    attribute. Cases compose with `any`, `all` and `not`."""
    if not isinstance(value, dict) or not value:
        return False
    keys = set(value)
    if keys in ({'any'}, {'all'}):
        (cases,) = value.values()
        return isinstance(cases, list) and len(cases) > 0 and all(is_binding(case) for case in cases)
    if keys == {'not'}:
        return is_binding(value['not'])
    if 'property' in keys and isinstance(value['property'], str):
        extra = keys - {'property'}
        if not extra:
            return True                                   # the property's truth or value itself
        if extra == {'on', 'off'}:
            return is_text(value['on']) and is_text(value['off'])
        if len(extra) == 1 and extra <= COMPARISONS:
            (comparison,) = extra
            return isinstance(value[comparison], list) if comparison == 'one_of' else True
        return False
    if keys == {'control', 'attribute'}:
        return isinstance(value['control'], str) and isinstance(value['attribute'], str)
    return False


VERBS = {'write': (str,), 'delta': (str, list), 'set': (str,), 'clear': (list,)}   # delta may bump several properties


def is_handlers(value):
    """`{event: [action, ...]}`; every action has exactly one verb."""
    if not isinstance(value, dict) or not value:
        return False
    for event, actions in value.items():
        if not isinstance(event, str) or not isinstance(actions, list) or not actions:
            return False
        for action in actions:
            if not isinstance(action, dict):
                return False
            verbs = [verb for verb in action if verb in VERBS]
            if len(verbs) != 1 or not isinstance(action[verbs[0]], VERBS[verbs[0]]):
                return False
    return True


KINDS = {
    'string': lambda v: isinstance(v, str),
    'integer': lambda v: type(v) is int,
    'number': lambda v: type(v) in (int, float, Decimal),
    'boolean': lambda v: isinstance(v, bool),
    'object': lambda v: isinstance(v, dict),
    'array': lambda v: isinstance(v, list),
    'null': lambda v: v is None,
    'text': is_text,                           # a string or a localized text object
    'binding': is_binding,                     # a value taken from state (see is_binding)
    'handlers': is_handlers,                   # {event: [action, ...]}
    'nodes': lambda v: isinstance(v, list),   # an array of nested controls
    'template': lambda v: isinstance(v, list),   # nested controls in which "{name}" stands for an item's value
    'pages': lambda v: isinstance(v, list),   # an array of {title, children: nodes}
}


class ContractError(ValueError):
    """A contract document is malformed (a bug in the contracts, not in a panel)."""


@dataclass(frozen=True)
class Contract:
    qualified: str
    extends: Optional[str]
    abstract: bool
    attributes: dict[str, tuple[str, ...]]
    required: tuple[str, ...]


class Registry:
    """Contract tables keyed by (namespace, version)."""

    def __init__(self):
        self._tables: dict[tuple[str, int], dict[str, Contract]] = {}

    @classmethod
    def from_documents(cls, documents) -> 'Registry':
        registry = cls()
        for document in documents:
            registry.add(document)
        return registry

    @classmethod
    def load(cls, directory) -> 'Registry':
        paths = sorted(Path(directory).glob('*.json'))
        if not paths:
            raise ContractError(f'no contract documents in {directory}')
        return cls.from_documents(json.loads(path.read_text()) for path in paths)

    def add(self, document):
        try:
            namespace, version = document['namespace'], document['version']
            controls = document['controls']
        except (KeyError, TypeError) as error:
            raise ContractError(f'contract document needs namespace, version, controls: {error}')
        if not isinstance(version, int) or isinstance(version, bool):
            raise ContractError(f'{namespace}: version must be an integer')
        table = {}
        for short, spec in controls.items():
            attributes = {}
            for name, kinds in spec.get('attributes', {}).items():
                unknown = [kind for kind in kinds if kind not in KINDS]
                if unknown or not kinds:
                    raise ContractError(f'{namespace}::{short}.{name}: unknown kinds {unknown or kinds}')
                attributes[name] = tuple(kinds)
            table[short] = Contract(f'{namespace}::{short}', spec.get('extends'), bool(spec.get('abstract')),
                                    attributes, tuple(spec.get('required', ())))
        self._tables[(namespace, version)] = table

    def table(self, namespace: str, version: int) -> Optional[dict[str, Contract]]:
        return self._tables.get((namespace, version))

    def versions(self, namespace: str) -> list[int]:
        return sorted(v for n, v in self._tables if n == namespace)


@dataclass
class NodeReport:
    path: str
    node: Any
    type: Any
    contract: Optional[str]
    status: str
    problems: list[str] = field(default_factory=list)
    extensions: list[str] = field(default_factory=list)


@dataclass
class Report:
    contracts: dict[str, int]
    assumed: bool
    problems: list[str] = field(default_factory=list)
    nodes: list[NodeReport] = field(default_factory=list)

    def counts(self) -> Counter:
        return Counter(node.status for node in self.nodes)

    def ok(self) -> bool:
        return not self.problems and all(node.status == 'valid' for node in self.nodes)


class _Resolver:
    def __init__(self, registry: Registry, selected: dict[str, int], report: Report):
        self.tables: dict[str, dict[str, Contract]] = {}
        self.report = report
        for namespace, version in selected.items():
            table = registry.table(namespace, version)
            if table is None:
                report.problems.append(f'{namespace} {version} is not available (have {registry.versions(namespace)})')
            else:
                self.tables[namespace] = table

    def find(self, type_name: str):
        """Return (contract, problem) for a type name as written in the file."""
        if '::' in type_name:
            namespace, short = type_name.split('::', 1)
            table = self.tables.get(namespace)
            if table is None:
                return None, f'{type_name}: namespace {namespace} is not declared or not available'
            if short not in table:
                return None, f'{type_name}: not in the selected {namespace} contracts'
            return table[short], None
        matches = [table[type_name] for table in self.tables.values() if type_name in table]
        if len(matches) == 1:
            return matches[0], None
        if not matches:
            return None, f'{type_name}: not in any declared namespace ({", ".join(self.tables) or "none"})'
        return None, f'{type_name}: ambiguous, defined by {", ".join(m.qualified for m in matches)}'

    def flatten(self, contract: Contract):
        """Merge attributes and required names along the extends chain."""
        chain, seen = [], set()
        while contract is not None:
            if contract.qualified in seen:
                return None, f'{contract.qualified}: extends cycle'
            seen.add(contract.qualified)
            chain.append(contract)
            if contract.extends is None:
                break
            base, problem = self.find(contract.extends)
            if base is None:
                return None, f'{contract.qualified}: base {problem}'
            contract = base
        attributes, required = {}, set()
        for member in reversed(chain):
            attributes.update(member.attributes)
            required.update(member.required)
        return (attributes, required), None

    def walk(self, node, path: str, variables: Optional[frozenset] = None):
        """`variables` is the set of placeholder names in scope: None outside
        any template, otherwise index/index0 plus every key that each item of
        the nearest enclosing Repeat supplies."""
        if not isinstance(node, dict):
            self.report.nodes.append(NodeReport(path, node, None, None, 'unresolved', ['not an object']))
            return
        type_name = node.get('type')
        if not isinstance(type_name, str):
            self.report.nodes.append(NodeReport(path, node, type_name, None, 'unresolved',
                                                ['type is missing or not a string']))
            return
        contract, problem = self.find(type_name)
        if contract is None:
            self.report.nodes.append(NodeReport(path, node, type_name, None, 'unresolved', [problem]))
            return
        if contract.abstract:
            self.report.nodes.append(NodeReport(path, node, type_name, contract.qualified, 'invalid',
                                                [f'{contract.qualified} is abstract']))
            return
        flattened, problem = self.flatten(contract)
        if flattened is None:
            self.report.nodes.append(NodeReport(path, node, type_name, None, 'unresolved', [problem]))
            return
        attributes, required = flattened
        problems, extensions = [], []
        for key, value in node.items():
            if key == 'type':
                continue
            kinds = attributes.get(key)
            if kinds is None:
                extensions.append(key)
            elif any(KINDS[kind](value) for kind in kinds):
                continue
            elif variables is not None and placeholder(value) is not None:
                if placeholder(value) not in variables:
                    problems.append(f'{key}: placeholder {value} is not supplied by every item')
            else:
                problems.append(f'{key}: expected {"/".join(kinds)}, got {type(value).__name__}')
        for key in sorted(required):
            if key not in node:
                problems.append(f'{key}: required')
        status = 'invalid' if problems else 'extended' if extensions else 'valid'
        self.report.nodes.append(NodeReport(path, node, type_name, contract.qualified, status, problems, extensions))
        for key, kinds in attributes.items():
            value = node.get(key)
            if not isinstance(value, list):
                continue
            if 'nodes' in kinds or 'template' in kinds:
                scope = template_variables(node) if 'template' in kinds else variables
                for index, child in enumerate(value):
                    self.walk(child, f'{path}/{key}/{index}', scope)
            elif 'pages' in kinds:
                for index, page in enumerate(value):
                    children = page.get('children') if isinstance(page, dict) else None
                    if isinstance(children, list):
                        for child_index, child in enumerate(children):
                            self.walk(child, f'{path}/{key}/{index}/children/{child_index}', variables)


PLACEHOLDER = re.compile(r'^\{([A-Za-z_][A-Za-z0-9_]*)\}$')


def placeholder(value) -> Optional[str]:
    """The variable name when `value` is exactly one placeholder, else None."""
    match = PLACEHOLDER.match(value) if isinstance(value, str) else None
    return match.group(1) if match else None


def template_variables(node) -> frozenset:
    """What a Repeat's template may refer to: the index, and every key that
    each of its items supplies (none when it repeats by count)."""
    names = {'index', 'index0'}
    items = node.get('items')
    if isinstance(items, list) and items and all(isinstance(item, dict) for item in items):
        names |= set.intersection(*(set(item) for item in items))
    return frozenset(names)


def resolve(panel_file, registry: Registry, assume: Optional[dict[str, int]] = None) -> Report:
    """Resolve every control in `panel_file` against the contract versions the
    file declares in `control_contracts`, or the `assume`d versions when it
    declares none. The file's data is not modified."""
    declared = panel_file.data.get('control_contracts')
    if declared is None:
        report = Report(dict(assume or {}), assumed=True)
    elif isinstance(declared, dict) and all(isinstance(v, int) and not isinstance(v, bool) for v in declared.values()):
        report = Report(dict(declared), assumed=False)
    else:
        report = Report({}, assumed=False, problems=['control_contracts must map namespace names to integer versions'])
    resolver = _Resolver(registry, report.contracts, report)
    panel = panel_file.data.get('panel')
    children = panel.get('children') if isinstance(panel, dict) else None
    if not isinstance(children, list):
        report.problems.append('panel/children is not an array')
        return report
    for index, child in enumerate(children):
        resolver.walk(child, f'panel/children/{index}')
    return report


def main(argv):
    from panel_model import loads
    contracts_dir = Path(__file__).resolve().parent / 'contracts'
    registry = Registry.load(contracts_dir)
    # A file that declares nothing predates the declaration: assume the
    # oldest contract version of each namespace, not the newest.
    assume = {namespace: registry.versions(namespace)[0] for namespace, _ in registry._tables}
    worst = 0
    for name in argv:
        report = resolve(loads(Path(name).read_text()), registry, assume=assume)
        versions = ' '.join(f'{k} {v}' for k, v in report.contracts.items())
        print(f'{name}: {dict(report.counts())} under {versions}{" (assumed)" if report.assumed else ""}')
        for problem in report.problems:
            print(f'  {problem}')
        for node in report.nodes:
            if node.status != 'valid':
                detail = node.problems or [f'extensions {node.extensions}']
                print(f'  {node.status:<10} {node.path} ({node.type}): {"; ".join(detail)}')
        worst = max(worst, 0 if report.ok() else 1)
    return worst


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
