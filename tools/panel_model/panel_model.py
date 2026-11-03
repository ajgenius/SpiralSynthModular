"""A panel file is JSON data; reading it does not resolve control types.

Control contracts and renderers can interpret the data later. In particular,
unknown fields and unknown control names are not discarded during a round trip.
"""
from dataclasses import dataclass
from typing import Any

import simplejson as json


class PanelFormatError(ValueError):
    """The JSON cannot be represented safely as a panel-file dictionary."""


@dataclass
class PanelFile:
    # The dictionary is the model: nested controls and their fields are data,
    # not Python classes that the file reader needs to know about.
    data: dict[str, Any]


def _object_without_duplicates(pairs):
    result = {}
    for key, value in pairs:
        if key in result:
            raise PanelFormatError(f'duplicate JSON key {key!r}')
        result[key] = value
    return result


def loads(text: str) -> PanelFile:
    """Read an object without interpreting versions, controls, or attributes."""
    data = json.loads(text, use_decimal=True, object_pairs_hook=_object_without_duplicates)
    if not isinstance(data, dict):
        raise PanelFormatError('panel file root must be a JSON object')
    return PanelFile(data)


def dumps(panel_file: PanelFile, **options) -> str:
    """Serialize the current dictionary, preserving decimal values exactly."""
    # simplejson's allow_nan=False does not reject non-finite Decimal values.
    # Check the emitted JSON too, so a later edit cannot save NaN/Infinity or
    # create colliding keys through the encoder's key coercion.
    text = json.dumps(panel_file.data, use_decimal=True, allow_nan=False, **options)
    loads(text)
    return text
