#!/bin/sh
set -eu
module_root=$(mktemp -d "${TMPDIR:-/tmp}/ssm-audio-modules.XXXXXX")
trap 'rm -rf "$module_root"' EXIT HUP INT TERM
mkdir "$module_root/audio"
set -- "$module_root"
for backend in $AUDIO_MODULE_DIRS; do
    mkdir "$module_root/audio/$backend"
    cp "$backend/.libs/${backend}_Audio$AUDIO_MODULE_SUFFIX" "$module_root/audio/$backend/"
    set -- "$@" "$backend"
done
./audio-modules-test "$@"
