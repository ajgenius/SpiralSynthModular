# swh-plugins (Steve Harris' LADSPA plugins), vendored

Source: <https://github.com/swh/ladspa>, GPL-2.0-or-later. `COPYING` and `AUTHORS`
are upstream's. The reverb this project's LADSPA tutorial needs, GVerb, is
Juhana Sadeharju's DSP with Steve Harris' LADSPA wrapper.

## Why these are in the tree

LADSPA is a header and a `dlopen` convention, so the LADSPA host builds anywhere,
but a host is useless with no plugins to host. Linux and the BSDs package
swh-plugins, so there they are the user's to install and ours to leave alone.
macOS packages nothing of the sort, which left `Examples/Tutorial7-LADSPA.ssm`
unopenable on a Mac — it asks for GVerb by name.

So these are built and bundled **on macOS only**. Building them on a platform
whose package manager also ships them would put two copies of the same unique IDs
on the LADSPA search path, and the host would load whichever it found first.

## What was changed

Nothing in the sources. Upstream's `configure.ac`, `Makefile.am` and `autogen.sh`
are not vendored, because SSM builds these with its own build system; `config.h`
here replaces the one upstream's configure would have generated, and is described
in its own comment.

The C is not stored in the tree. Upstream generates it from the `*.xml` plugin
descriptions with `makestub.pl`, and so do we, at build time — Perl is the only
build-time tool this needs.

## What is not built

93 of the 98 build with no dependency beyond libm. The rest are skipped rather
than pulling libraries into a bundle for four plugins:

| plugin | needs |
| --- | --- |
| `gsm_1215` | libgsm |
| `imp_1199`, `mbeq_1197`, `pitch_scale_1193`, `pitch_scale_1194` | FFTW (the sources want FFTW 2's `rfftw.h`) |

`util/pitchscale.c` belongs to the FFTW set and is skipped with it.
