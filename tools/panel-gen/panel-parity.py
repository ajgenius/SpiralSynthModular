#!/usr/bin/env python3
import difflib
from uifile import read_ui, write_ui
import json
import os
import re
import sys

FRAME_X, FRAME_Y = 2, 14
FONT_HEIGHT = {8: 9, 10: 11, 12: 14, 14: 16}
HERE = os.path.dirname(os.path.abspath(__file__))
LINE = re.compile(r'^(\d+) (.+?) (-?\d+) (-?\d+) (-?\d+) (-?\d+) "(.*)" (\d+)$')

CHROME = {("Fl_Button", "X"), ("Fl_Button", "?")}


def read_tree(path):
    out = []
    for line in open(path, encoding="latin-1"):
        m = LINE.match(line.rstrip("\n"))
        if m:
            depth, cls, x, y, w, h, label, size = m.groups()
            out.append(dict(depth=int(depth), cls=cls, x=int(x), y=int(y), w=int(w), h=int(h), label=label, size=int(size)))
    return out


def read_controls(path):
    out = []
    for line in open(path):
        parts = line.split()
        if len(parts) >= 7:
            name, parent = parts[:2]
            cls = " ".join(parts[2:-4])
            x, y, w, h = parts[-4:]
            out.append(dict(name=name, parent=parent, cls=cls, x=int(x), y=int(y), w=int(w), h=int(h)))
    return out


ALIASES = {}


def alias(control):
    if control["cls"] == "Fl_Box" and control["name"].endswith("-text"):
        return "Fl_Output"
    return ALIASES.get(control["cls"], control["cls"])


def editor_widgets(tree):
    out = []
    for i, n in enumerate(tree):
        if n["depth"] == 0 or (n["depth"] == 1 and (n["cls"], n["label"]) in CHROME):
            continue
        if n["cls"] == "Fl_Group" and i > 0 and any(p["cls"] == "Fl_Tabs" and p["depth"] == n["depth"] - 1 for p in tree[:i][::-1][:40]):
            continue
        out.append(n)
    return out


def ordered_controls(controls, tree):
    def key(c):
        for i, n in enumerate(tree):
            if (n["cls"], n["x"], n["y"], n["w"], n["h"]) == (c["cls"], c["x"], c["y"], c["w"], c["h"]):
                return i
        return len(tree)
    return sorted(controls, key=key)


def find(node, name):
    for c in node.get("children", []):
        if c.get("name") == name:
            return c
        r = find(c, name)
        if r:
            return r
    for page in node.get("pages", []):
        r = find(page, name)
        if r:
            return r
    return None


def parity(device, shots, tree_dir, apply):
    editor_path = os.path.join(shots, device + "-editor.txt")
    if not os.path.exists(editor_path):
        print("%s: no editor" % device)
        return
    editor = read_tree(editor_path)
    panel = read_tree(os.path.join(shots, device + "-panel.txt"))
    controls = ordered_controls(read_controls(os.path.join(shots, device + "-controls.txt")), panel)
    widgets = editor_widgets(editor)

    def norm(s):
        return "".join(ch for ch in s.lower() if ch.isalnum())

    def control_label(i):
        c = controls[i]
        for n in panel:
            if (n["cls"], n["x"], n["y"], n["w"], n["h"]) == (c["cls"], c["x"], c["y"], c["w"], c["h"]) and n["label"]:
                return n["label"]
        if i + 1 < len(controls) and controls[i + 1]["cls"] == "Fl_Box":
            nxt = controls[i + 1]
            for n in panel:
                if (n["cls"], n["x"], n["y"], n["w"], n["h"]) == ("Fl_Box", nxt["x"], nxt["y"], nxt["w"], nxt["h"]):
                    return n["label"]
        return ""

    pairs = {}
    taken = set()
    for i, c in enumerate(controls):
        if alias(c) == "Fl_Box":
            continue
        mine = norm(control_label(i))
        if not mine:
            continue
        for k, w in enumerate(widgets):
            if k in taken or w["cls"] != alias(c) or not w["label"]:
                continue
            theirs = norm(w["label"])
            if theirs == mine or theirs.startswith(mine) or mine.startswith(theirs):
                pairs[c["name"]] = w
                taken.add(k)
                break
    rest_w = [k for k in range(len(widgets)) if k not in taken]
    rest_c = [i for i, c in enumerate(controls) if c["name"] not in pairs]
    matcher = difflib.SequenceMatcher(None, [widgets[k]["cls"] for k in rest_w], [alias(controls[i]) for i in rest_c], autojunk=False)
    for tag, i1, i2, j1, j2 in matcher.get_opcodes():
        if tag == "equal":
            for k in range(i2 - i1):
                pairs[controls[rest_c[j1 + k]]["name"]] = widgets[rest_w[i1 + k]]
    unmatched_widgets = [w for w in widgets if not any(p is w for p in pairs.values())]
    unmatched_controls = [c for c in controls if c["name"] not in pairs and alias(c) != "Fl_Box"]
    clean = not unmatched_widgets and not unmatched_controls

    ew, eh = editor[0]["w"], editor[0]["h"]
    pw, ph = panel[0]["w"], panel[0]["h"]
    print("%s: editor %dx%d panel %dx%d; %d of %d controls matched, %d editor widgets unmatched%s"
          % (device, ew, eh, pw, ph, len(pairs), len(controls), len(unmatched_widgets), "; clean" if clean else ""))
    for c in unmatched_controls:
        print("   panel only: %s %s" % (c["name"], c["cls"]))
    for w in unmatched_widgets:
        print("   editor only: %s %d %d %d %d \"%s\"" % (w["cls"], w["x"], w["y"], w["w"], w["h"], w["label"]))

    deltas = {}
    sizes = {}
    last = (0, 0)
    last_size = None
    last_w = None
    for i, c in enumerate(controls):
        w = pairs.get(c["name"])
        if w:
            last = (w["x"] - c["x"], w["y"] - c["y"])
            last_size = w["size"] if w["label"] else None
            last_w = w
            keep = c["cls"] in ALIASES
            deltas[c["name"]] = last + ((c["w"], c["h"]) if keep else (w["w"], w["h"]))
            if w["label"] and c["cls"] in ("Fl_Button", "Fl_LED_Button", "Fl_Check_Button", "Fl_Light_Button"):
                sizes[c["name"]] = w["size"]
            if last != (0, 0) or (w["w"], w["h"]) != (c["w"], c["h"]):
                print("   %-28s %-12s at %4d,%-4d editor %4d,%-4d  %dx%d -> %dx%d" % (c["name"], c["cls"], c["x"], c["y"], w["x"], w["y"], c["w"], c["h"], w["w"], w["h"]))
        else:
            deltas[c["name"]] = last + (c["w"], c["h"])
            if last_size is not None and c["cls"] == "Fl_Box":
                sizes[c["name"]] = last_size
            if (c["cls"] == "Fl_Box" and last_w and last_w["label"]
                    and norm(control_label(i)) == norm(last_w["label"]) and c["y"] + last[1] >= last_w["y"] + last_w["h"] - 1):
                box = (last_w["x"] - c["x"], last_w["y"] + last_w["h"] - c["y"], last_w["w"], FONT_HEIGHT.get(last_w["size"], c["h"]))
                if box != deltas[c["name"]]:
                    print("   %-28s %-12s bottom label of %s: %d,%d %dx%d -> %d,%d %dx%d" % (c["name"], c["cls"], last_w["cls"], c["x"], c["y"], c["w"], c["h"], last_w["x"], last_w["y"] + last_w["h"], box[2], box[3]))
                deltas[c["name"]] = box
            elif last != (0, 0):
                print("   %-28s %-12s follows by %d,%d" % (c["name"], c["cls"], last[0], last[1]))
    if not apply or (apply == "clean" and not clean):
        return

    doc_path = os.path.join(tree_dir, "Plugins", "Devices", device + "Plugin", "info.json")
    doc = read_ui(doc_path)
    root = doc["panel"]
    by_name = {c["name"]: c for c in controls}
    for c in controls:
        node = find(root, c["name"])
        if not node or "bounds" not in node:
            continue
        dx, dy, w, h = deltas[c["name"]]
        if c["parent"] != "-" and c["parent"] in deltas:
            dx -= deltas[c["parent"]][0]
            dy -= deltas[c["parent"]][1]
        node["bounds"] = [node["bounds"][0] + dx, node["bounds"][1] + dy, w, h]
        if c["name"] in sizes:
            if sizes[c["name"]] == 10:
                node.pop("size", None)
            else:
                node["size"] = sizes[c["name"]]
    root["width"] = ew - 2 * FRAME_X
    root["height"] = eh - FRAME_Y - FRAME_X
    write_ui(doc_path, doc)
    print("   wrote %s" % doc_path)


def main():
    args = sys.argv[1:]
    apply = "clean" if "--apply-clean" in args else "--apply" in args
    args = [a for a in args if a not in ("--apply", "--apply-clean")]
    shots = os.path.expanduser("~/.hermes/cache/scratch/shots")
    tree = os.path.join(HERE, "panels")
    if "--shots" in args:
        i = args.index("--shots"); shots = args[i + 1]; del args[i:i + 2]
    if "--tree" in args:
        i = args.index("--tree"); tree = args[i + 1]; del args[i:i + 2]
    for device in args:
        parity(device, shots, tree, apply)


if __name__ == "__main__":
    main()
