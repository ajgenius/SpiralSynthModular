#!/bin/sh
# SPDX-License-Identifier: GPL-2.0-or-later
set -e
here=$(cd "$(dirname "$0")" && pwd)
root=$(cd "$here/../.." && pwd)
build=$(cd "${1:-$root/.build}" && pwd)
inc="-I$root/libs/libspiralcore/include"
for d in "$root"/libs/libspiralcore/*/include; do inc="$inc -I$d"; done
mkdir -p "$build/tools/panel-gen"
${CXX:-c++} -std=c++11 -O0 -I"$build" -I"$root" $inc "$here/dump.cpp" \
	-L"$build/libs/libspiralcore/.libs" -Wl,-rpath,"$build/libs/libspiralcore/.libs" \
	-lspiralcore -ldl -o "$build/tools/panel-gen/dump"
echo "$build/tools/panel-gen/dump"
