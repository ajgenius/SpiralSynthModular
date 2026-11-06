#!/usr/bin/env python3
import json, os, sys
from uifile import write_ui

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
OUT = os.path.join(ROOT, "Plugins/Devices/MatrixPlugin/info.json")
PLUGIN = 18
NUM_PATSEQ = 16
MATX, MATY = 64, 32

COMMANDS = {
    "MAT_LENGTH": {"command": 1, "args": ["LENGTH"]},
    "MAT_SPEED": {"command": 2, "args": ["SPEED"]},
    "MAT_ACTIVATE": {"command": 3, "args": ["X", "Y"]},
    "MAT_DEACTIVATE": {"command": 4, "args": ["X", "Y"]},
    "MAT_OCTAVE": {"command": 5, "args": ["OCTAVE"]},
    "COPY": 6, "PASTE": 7, "CLEAR": 8, "TUP": 9, "TDOWN": 10,
    "MAT_VOLUME": {"command": 11, "args": ["X", "Y", "VOLUME"]},
    "SET_CURRENT": {"command": 12, "args": ["NUM"]},
    "SET_PATSEQ": {"command": 13, "args": ["Y", "NUM"]},
}


def i18n(name, text):
    return {"default": text, "key": "plugin.%d.%s" % (PLUGIN, name)}


def prop(name, cls, kind, text, hint=None, read_only=False, **extra):
    p = {"class": cls, "type": kind, "property": name, "read_only": read_only, "internal": False,
         "display_name": i18n("property." + name, text), "hint": i18n("property.%s.hint" % name, hint or text)}
    p.update(extra)
    return p


def number(name, text, lo, hi, step, default, read_only=False, cls="FloatProperty"):
    return prop(name, cls, "number", text, read_only=read_only, min=lo, max=hi, step=step, page=step, default=default, logarithmic=False)


def integer(name, text, lo, hi, default, read_only=False):
    return prop(name, "IntProperty", "integer", text, read_only=read_only, min=lo, max=hi, step=1, page=1, default=default, logarithmic=False)


def toggle(name, text, default):
    return prop(name, "ToggleProperty", "boolean", text, default=default)


PROPERTIES = [
    prop("COMMAND", "StringProperty", "string", "COMMAND", default="", internal=True, omitted="internal", commands=COMMANDS, hint="Command"),
    integer("CURRENT", "View pattern", 0, 15, 0),
    prop("DEVICETYPE", "StringProperty", "string", "DeviceType", read_only=True, default="Matrix", omitted="identity"),
    prop("DEVICEVERSION", "UnsignedProperty", "integer", "DeviceVersion", read_only=True, default=1, min=0, max=0, step=0, page=0, logarithmic=False, omitted="identity"),
    integer("ECHOCUR", "Play pattern", 0, 15, 0, read_only=True),
    integer("ECHOLENGTH", "Length", 1, 64, 64, read_only=True),
    integer("ECHOOCTAVE", "Octave", 0, 6, 0, read_only=True),
    number("ECHOSPEED", "Speed", 0, 50, 0.125, 1, read_only=True),
    integer("LENGTH", "Length", 1, 64, 64),
    toggle("NOTECUT", "NoteCut", False),
    toggle("TRIGGERSELECT", "Trigger Select", False),
    integer("NUM", "Pattern", -1, 15, 0),
    integer("OCTAVE", "Octave", 0, 6, 0),
    integer("PATSEQSTEP", "Sequence step", 0, NUM_PATSEQ - 1, 0, read_only=True),
    integer("STEP", "Step", 0, MATX - 1, 0, read_only=True),
    number("SPEED", "Speed", 0, 50, 0.125, 1),
    number("VOLUME", "Volume", 0, 1, 0.001, 1),
    integer("X", "Step", 0, MATX - 1, 0),
    integer("Y", "Note", 0, MATY - 1, 0),
] + [integer("ECHOPATSEQ%d" % n, "Sequence %d" % n, -1, 15, 0 if n == 0 else -1, read_only=True) for n in range(NUM_PATSEQ)]


FRAME_X, FRAME_Y = 2, 14
EDITOR_W, EDITOR_H = 600, 270


def label(name, text, x, y, w, h, size=10):
    return {"type": "Label", "name": name, "bounds": [x, y, w, h], "size": size, "text": i18n("panel.%s.text" % name, text)}


def command(name, *with_):
    w = {"property": "COMMAND", "value": name}
    if with_:
        w["with"] = list(with_)
    return w


def counter(name, x, y, w, h, shown, sends=None, tip=None, **extra):
    c = {"type": "Spinner", "name": name, "bounds": [x, y, w, h], "property": shown, "simple": True, "digits": 0, "step": 1}
    if sends:
        c["write"] = [command(sends, "value")]
    if tip:
        c["tooltip"] = i18n("panel.%s.tooltip" % name, tip)
    c.update(extra)
    return c


def button(name, text, x, y, w, h, sends=None, tip=None, **extra):
    b = {"type": "Button", "name": name, "bounds": [x, y, w, h], "text": i18n("panel.%s.text" % name, text)}
    if sends:
        b["write"] = [command(sends)]
    if tip:
        b["tooltip"] = i18n("panel.%s.tooltip" % name, tip)
    b.update(extra)
    return b


CHILDREN = [
    counter("pattern", 5, 20, 40, 20, "CURRENT", tip="The pattern shown and edited"),
    label("pattern-label", "View", 5, 40, 40, 10),
    counter("play", 50, 20, 40, 20, "ECHOCUR", "SET_CURRENT", tip="The pattern that plays"),
    label("play-label", "Play", 50, 40, 40, 10),
    counter("length", 5, 55, 40, 17, "ECHOLENGTH", "MAT_LENGTH", tip="Steps in the pattern"),
    label("length-label", "Length", 5, 72, 40, 10),
    {"type": "Knob", "name": "speed", "bounds": [50, 59, 40, 40], "property": "ECHOSPEED", "mapping": {"scale": 8}, "step": 1,
     "write": [command("MAT_SPEED", "value")],
     "tooltip": i18n("panel.speed.tooltip", "Steps a beat, times eight")},
    label("speed-label", "Speed", 50, 99, 40, 11),
    counter("octave", 5, 87, 40, 17, "ECHOOCTAVE", "MAT_OCTAVE", tip="Octave of the lowest row"),
    label("octave-label", "Octave", 5, 104, 40, 10),
    counter("speed-value", 5, 119, 85, 17, "ECHOSPEED", "MAT_SPEED", mapping={"scale": 8}),
    button("copy", "Copy", 5, 141, 40, 17, "COPY", "Copy the pattern shown"),
    button("paste", "Paste", 50, 141, 40, 17, "PASTE", "Paste over the pattern shown"),
    button("clear", "Clear", 5, 163, 85, 17, "CLEAR", "Clear the pattern shown"),
    button("up", "Up", 5, 185, 40, 17, "TUP", "Transpose the pattern up a note"),
    button("down", "Down", 50, 185, 40, 17, "TDOWN", "Transpose the pattern down a note"),
    label("transpose-label", "Transpose", 5, 203, 85, 10),
    button("trig", "Trig Select", 5, 218, 85, 17, property="TRIGGERSELECT",
           tip="Under the external clock, select the pattern by the trigger's pitch rather than resetting on its level"),
    button("notecut", "NoteCut", 5, 240, 85, 20, property="NOTECUT", tip="Cut the note on a new step"),
    {"type": "MatrixGrid", "name": "grid", "bounds": [90, 40, 463, 224], "columns": MATX, "rows": MATY,
     "note_names": 15,
     "channel": "EchoGrid", "step_channel": "Step", "on": "MAT_ACTIVATE", "off": "MAT_DEACTIVATE",
     "volume_channel": "EchoVolume", "volume_command": "MAT_VOLUME",
     "tooltip": i18n("panel.grid.tooltip", "Click a step on or off; right-drag or scroll an on step to set its volume")},
    label("patseq-label", "Pat Seq", 560, 25, 30, 10),
]

for n in range(MATX):
    CHILDREN.append({"type": "LED", "name": "step-%d" % n, "bounds": [103 + n * 7, 20, 15, 15], "property": "STEP", "choice": n, "color_role": "matrix.status"})

for n in range(NUM_PATSEQ):
    y = 40 + n * 14
    CHILDREN.append({"type": "Spinner", "name": "patseq-%d" % n, "bounds": [560, y, 25, 12], "property": "ECHOPATSEQ%d" % n,
                     "simple": True, "digits": 0, "step": 1, "min": 0 if n == 0 else -1, "max": 15,
                     "write": [command("SET_PATSEQ", n, "value")]})
    CHILDREN.append({"type": "LED", "name": "patseq-%d-led" % n, "bounds": [585, y, 15, 15], "property": "PATSEQSTEP", "choice": n, "color_role": "matrix.status"})



def framed(bounds, rx=1.0, ry=1.0):
    x, y, w, h = bounds
    return [int(round(x * rx)) - FRAME_X, int(round(y * ry)) - FRAME_Y, int(round(w * rx)), int(round(h * ry))]


def layout(name, width, height):
    rx, ry = width / float(EDITOR_W), height / float(EDITOR_H)
    return {"name": name, "width": width - 2 * FRAME_X, "height": height - FRAME_Y - FRAME_X,
            "controls": {c["name"]: framed(c["bounds"], rx, ry) for c in CHILDREN}}


PRIVATE_W, PRIVATE_H = int(round(618 * 1.25)), int(round(253 * 1.25))
LAYOUTS = [
    layout("classic", EDITOR_W, EDITOR_H),
    layout("middle", (EDITOR_W + PRIVATE_W) // 2, (EDITOR_H + PRIVATE_H) // 2),
    layout("large", PRIVATE_W, PRIVATE_H),
]

DOCUMENT = {
    "schema_version": 1,
    "host_version": "0.3.43",
    "plugin_id": PLUGIN,
    "panel": {
        "schema_version": 1,
        "width": EDITOR_W - 2 * FRAME_X,
        "height": EDITOR_H - FRAME_Y - FRAME_X,
        "help": i18n("panel.help", "A step sequencer: each column is a step, each row a note. View picks the pattern you edit, "
                     "Play the one that plays. Right-drag or scroll an on step to set its volume. Pat Seq lists the patterns "
                     "played in turn; -1 ends it. The square button steps through the layouts."),
        "children": [dict(c, bounds=framed(c["bounds"])) for c in CHILDREN],
        "layouts": LAYOUTS,
    },
    "properties": PROPERTIES,
}

if __name__ == "__main__":
    out = sys.argv[1] if len(sys.argv) > 1 else OUT
    write_ui(out, DOCUMENT)
    print("wrote", out, len(CHILDREN), "controls")
