#!/bin/bash
# Build the reviewed native timing fixes without replacing system libraries.
set -euo pipefail

if [[ $# != 1 || $1 != /* || $(uname -s) != Darwin ]]; then
	printf 'Usage on macOS: %s /absolute/new/install-prefix\n' "$0" >&2
	exit 2
fi

prefix=$1
if [[ -e "$prefix" ]]; then
	printf 'Refusing to overwrite existing prefix: %s\n' "$prefix" >&2
	exit 2
fi

script_dir=$(cd "$(dirname "$0")" && pwd)
python=${WAF_PYTHON:-/usr/bin/python3}
jobs=${JOBS:-4}
deps=${NATIVE_DEPS_PREFIX:-/opt/local}
"$python" -c 'import sys; assert (3, 6) <= sys.version_info[:2] < (3, 12), "JACK 1.9.22 waf requires Python 3.6-3.11"'
for tool in git clang clang++ make; do command -v "$tool" >/dev/null; done

# JACK's macOS build needs Aften. These search paths are local to this process.
export CPATH="$deps/include${CPATH:+:$CPATH}"
export LIBRARY_PATH="$deps/lib${LIBRARY_PATH:+:$LIBRARY_PATH}"
mkdir -p "$prefix/src" "$prefix/logs"

fetch_source()
{
	local name=$1 url=$2 revision=$3 patch=$4
	git init -q "$prefix/src/$name"
	git -C "$prefix/src/$name" remote add origin "$url"
	git -C "$prefix/src/$name" fetch -q --depth 1 origin "$revision"
	git -C "$prefix/src/$name" checkout -q --detach FETCH_HEAD
	[[ $(git -C "$prefix/src/$name" rev-parse HEAD) == "$revision" ]]
	git -C "$prefix/src/$name" apply --check "$script_dir/patches/$patch"
	git -C "$prefix/src/$name" apply "$script_dir/patches/$patch"
}

fetch_source jack2 https://github.com/jackaudio/jack2.git \
	4f58969432339a250ce87fe855fb962c67d00ddb jack2-coreaudio-latency.patch
fetch_source portaudio https://github.com/PortAudio/portaudio.git \
	88ab584e7bf4358599744cd662cfbc978f41efbf portaudio-coreaudio-latency.patch

(
	cd "$prefix/src/jack2"
	"$python" ./waf configure --prefix="$prefix/jack" --tests=yes
	"$python" ./waf build -j"$jobs"
	./build/tests/jack_coreaudio_latency_test
	"$python" ./waf install
) >"$prefix/logs/jack.log" 2>&1
printf 'JACK build and latency regression passed.\n'

(
	cd "$prefix/src/portaudio"
	./configure --prefix="$prefix/portaudio" --disable-mac-universal
	make -j"$jobs"
	clang -std=c11 -Wno-deprecated-declarations -Iinclude -Isrc/common \
		-Isrc/os/unix -Isrc/hostapi/coreaudio qa/paqa_mac_core_latency.c \
		lib/.libs/libportaudio.a -framework CoreAudio -framework AudioUnit \
		-framework AudioToolbox -framework CoreFoundation -framework CoreServices \
		-lpthread -lm -o paqa_mac_core_latency
	./paqa_mac_core_latency
	make install
) >"$prefix/logs/portaudio.log" 2>&1
printf 'PortAudio build and latency regression passed.\n'

# Sourcing this file opts the invoking shell into these libraries and tools.
# It does not start a JACK server or change any running application's libraries.
{
	printf 'ssm_native_prefix=%q\n' "$prefix"
	cat <<'ENV'
export PATH="$ssm_native_prefix/jack/bin:$PATH"
export PKG_CONFIG_PATH="$ssm_native_prefix/jack/lib/pkgconfig:$ssm_native_prefix/portaudio/lib/pkgconfig${PKG_CONFIG_PATH:+:$PKG_CONFIG_PATH}"
export DYLD_LIBRARY_PATH="$ssm_native_prefix/jack/lib:$ssm_native_prefix/portaudio/lib${DYLD_LIBRARY_PATH:+:$DYLD_LIBRARY_PATH}"
export JACK_DRIVER_DIR="$ssm_native_prefix/jack/lib/jack"
unset ssm_native_prefix
ENV
} >"$prefix/env.sh"

printf 'Ready. Source %s/env.sh before building or running either SSM stack.\n' "$prefix"
