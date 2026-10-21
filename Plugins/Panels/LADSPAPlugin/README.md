# LADSPA panels

The LADSPA device writes its own panel round whatever plugin it has
loaded (`libs/libspiralcore/Plugin/src/LADSPAPanel.cpp`): the Knobs,
Sliders and Setup pages its editor module had. A hand-written page for one
plugin goes in this directory as `<unique id>.json`, a panel document
whose nodes bind the device's per-input properties (`DEFAULT3`, `MIN3`,
`MAX3`, `CLAMP3`, `VALUE3`, `CONNECTED3`, and `UNIQUEID`, `PAGE`,
`UPDATEINPUTS`); it shows as the Panel page, beside the three, when the
plugin is loaded. Installed as `<plugins>/panels/LADSPAPlugin/<id>.json`.
