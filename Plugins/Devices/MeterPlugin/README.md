# Meter

The device registry reads this directory's `info.json` and creates a built-in
passthrough. The audio input feeds the output for each voice. The device writes
the selected voice, or the sum for All, into one ring buffer. Closing the panel
leaves the audio path connected.

The panel reads samples from the ring to drive the bar, digits and range
readouts. Scope uses the same passthrough with a snapshot shadow.
