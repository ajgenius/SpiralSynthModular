# CMT (Richard Furse's Computer Music Toolkit), vendored

Source: CMT 1.18, <https://www.ladspa.org/cmt/> (`cmt_1.18.tgz`,
sha256 `a82f8636de1f4ada386a199a017a9cd775a49b49e716b11e8dd3f723c93df6ca`),
GPL version 2: `doc/COPYING` and `doc/license.html` are upstream's. Freeverb in
`src/freeverb` is Jezar at Dreampoint's public domain reverb.

## Why it is in the tree

Old SSM patches use it, and a patch whose LADSPA device names a plugin nobody
has loses that device's sound. Across the patches we have (the song
collection, every `Examples` folder, the 0.2.2 release's examples and their
re-saves), CMT's Freeverb (ID 1123) is in 160, far more than any plugin but
GVerb and the Dyson compressor; its one-pole high-pass (1052), one-second
feedback delay (1060) and stereo amplifier (1068) are in a few more.

It is built and installed with the swh-plugins, under the application's own
plugin directory (`<plugins>/ladspa`), and like them it is skipped by
`--disable-bundled-ladspa`. A distribution's own CMT is untouched: the host
searches its own directory first and keeps the system ones behind it, and
LADSPAInfo keeps the first copy of a duplicated plugin ID.

## What was changed

Nothing in the sources. Upstream's `src/Makefile` and `gcc_exports.map` are not
vendored: `Makefile.am` builds the same objects into one module, `cmt.so`, and
exports only `ladspa_descriptor` as upstream's version script does. It is
compiled without upstream's `-Werror`, so a newer compiler's warnings cannot
stop the build.
