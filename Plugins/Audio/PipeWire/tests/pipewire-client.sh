#!/bin/sh
set -eu
command -v pipewire >/dev/null 2>&1 || exit 77
command -v timeout >/dev/null 2>&1 || exit 77
test_root=$(mktemp -d "${TMPDIR:-/tmp}/ssm-pipewire.XXXXXX")
server_pid=
cleanup()
{
    if [ -n "$server_pid" ]; then
        kill "$server_pid" 2>/dev/null || true
        wait "$server_pid" 2>/dev/null || true
    fi
    rm -rf "$test_root"
}
trap cleanup EXIT HUP INT TERM
cp "${srcdir:-.}/tests/pipewire-client.conf" "$test_root/client.conf"
export PIPEWIRE_RUNTIME_DIR="$test_root"
export PIPEWIRE_REMOTE=ssm-audio-test
export PIPEWIRE_CONFIG_DIR="$test_root"
pipewire -c "$(cd "${srcdir:-.}/tests" && pwd)/pipewire-server.conf" >"$test_root/server.log" 2>&1 &
server_pid=$!
n=0
while [ ! -S "$test_root/ssm-audio-test" ]; do
    n=$((n + 1))
    if [ "$n" -ge 50 ] || ! kill -0 "$server_pid" 2>/dev/null; then
        cat "$test_root/server.log"
        exit 1
    fi
    sleep 0.1
done
timeout 20 ./pipewire-client-test
