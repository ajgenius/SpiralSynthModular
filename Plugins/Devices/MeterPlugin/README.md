# Meter

Meter stays a module for now. In the private tree it is a data-only device:
an `info.json` with `"registration": "builtin"` and `"builtin": "passthrough"`,
no module, its one port pair declared with `"shadow": "ring"` so the host
keeps a rolling buffer of the input for the meter to draw. This repo's
passthrough devices only support `"shadow": "snapshot"`, one latest value, so
Meter can't move until the passthrough reader and `PassthroughPlugin` learn
the ring shadow. When they do, this folder keeps `info.json` and
`SpiralIcon.xpm` and loses `MeterPlugin.C`, `MeterPlugin.h` and the module
rules in `Makefile.am`.
