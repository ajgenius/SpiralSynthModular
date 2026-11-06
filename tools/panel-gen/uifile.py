import json


def _sorted(value):
    if isinstance(value, dict):
        return {k: _sorted(value[k]) for k in sorted(value)}
    if isinstance(value, list):
        return [_sorted(v) for v in value]
    return value


def read_ui(info_path):
    return json.load(open(info_path))["ui"]


def write_ui(info_path, doc):
    info = json.load(open(info_path))
    info["ui"] = _sorted(doc)
    open(info_path, "w").write(json.dumps(info, indent=2) + "\n")
