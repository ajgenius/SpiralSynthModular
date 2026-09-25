#!/bin/sh
# Package a staged Unix installation using the macOS developer tools.
set -eu

fail() { echo "bundle: $*" >&2; exit 1; }
[ "$(uname -s)" = Darwin ] || fail "packaging requires macOS"
[ "$#" = 7 ] || fail "expected make, bindir, libdir, plugindir, srcdir, version, output"
make_command=$1
bindir=$2
libdir=$3
plugindir=$4
srcdir=$5
version=$6
destination=$7
for path in "$bindir" "$libdir" "$plugindir"; do
    case $path in /*) ;; *) fail "installation paths must be absolute: $path" ;; esac
done
case $destination in /*) ;; *) destination=$PWD/$destination ;; esac
identifier=org.pawfal.SpiralSynthModular
if [ -e "$destination" ] || [ -L "$destination" ]; then
    [ ! -L "$destination" ] || fail "refusing to replace a symlink: $destination"
    old_id=$(/usr/libexec/PlistBuddy -c 'Print :CFBundleIdentifier' "$destination/Contents/Info.plist" 2>/dev/null) || fail "unrecognized output: $destination"
    [ "$old_id" = "$identifier" ] || fail "refusing to replace unrelated output: $destination"
fi
mkdir -p "$(dirname "$destination")"
# Legacy libtool install recipes cannot reliably quote a DESTDIR with spaces.
# Stage in /tmp, then move the finished app to the requested destination.
work=$(mktemp -d /tmp/ssm-bundle-XXXXXX)
trap 'rm -rf "$work"' 0
trap 'exit 1' 1 2 3 15
stage=$work/stage
app=$work/SpiralSynthModular.app
contents=$app/Contents
frameworks=$contents/Frameworks
resources=$contents/Resources
original_executable=$stage$bindir/SpiralSynthModular
"$make_command" install "DESTDIR=$stage"
mkdir -p "$contents/MacOS" "$frameworks" "$resources" "$work/sources"
cp -p "$original_executable" "$contents/MacOS/SpiralSynthModular"
cp -R "$stage$plugindir" "$resources/SpiralPlugins"
cp -R "$srcdir/Examples" "$resources/Examples"
cp "$srcdir/COPYING" "$srcdir/GUI/SpiralSynthModular.icns" "$resources/"

canonical() (
    path=$1
    while [ -L "$path" ]; do
        link=$(readlink "$path")
        case $link in /*) path=$link ;; *) path=$(dirname "$path")/$link ;; esac
    done
    directory=$(cd "$(dirname "$path")" && pwd -P)
    printf '%s/%s\n' "$directory" "$(basename "$path")"
)
expand() {
    case $1 in
        @loader_path/*) printf '%s/%s\n' "$(dirname "$2")" "${1#@loader_path/}" ;;
        @executable_path/*) printf '%s/%s\n' "$(dirname "$original_executable")" "${1#@executable_path/}" ;;
        *) printf '%s\n' "$1" ;;
    esac
}
rpaths() {
    otool -l "$1" | awk '/cmd LC_RPATH/ { getline; getline; sub(/^ *path /, ""); sub(/ \(offset .*$/, ""); print }'
}
resolve() (
    dependency=$1
    source=$2
    # Prefer this staged installation over older installed project libraries.
    installed=$stage$libdir/$(basename "$dependency")
    if [ -f "$installed" ]; then canonical "$installed"; exit; fi
    case $dependency in
        @rpath/*)
            candidates=$({ rpaths "$source" | while IFS= read -r path; do expand "$path" "$source"; done
                           rpaths "$original_executable" | while IFS= read -r path; do expand "$path" "$original_executable"; done
                         } | while IFS= read -r path; do printf '%s/%s\n' "$path" "${dependency#@rpath/}"; done) ;;
        *) candidates=$(expand "$dependency" "$source") ;;
    esac
    printf '%s\n' "$candidates" | while IFS= read -r path; do
        case $path in
            /*)
                if [ -f "$stage$path" ]; then canonical "$stage$path"; exit 0; fi
                if [ -f "$path" ]; then canonical "$path"; exit 0; fi ;;
        esac
    done
)
relocate() (
    source=$1
    target=$2
    chmod u+w "$target"
    ids=$(otool -D "$source" | sed '1d')
    dependencies=$(otool -L "$source")
    printf '%s\n' "$dependencies" | sed '1d; s/^[[:space:]]*//; s/ (compatibility version .*//' |
    while IFS= read -r dependency; do
        [ "$dependency" != "$ids" ] || continue
        case $dependency in /System/Library/*|/usr/lib/*) continue ;; esac
        resolved=$(resolve "$dependency" "$source")
        [ -n "$resolved" ] || fail "cannot resolve $dependency from $source"
        case $resolved in *.framework/*) fail "non-system frameworks are not supported: $resolved" ;; esac
        name=$(basename "$resolved")
        library=$frameworks/$name
        if [ -f "$work/sources/$name" ]; then
            [ "$(cat "$work/sources/$name")" = "$resolved" ] || fail "conflicting library names: $name"
        else
            printf '%s\n' "$resolved" > "$work/sources/$name"
            cp -p "$resolved" "$library"
            relocate "$resolved" "$library"
        fi
        install_name_tool -change "$dependency" "@executable_path/../Frameworks/$name" "$target"
    done
    if [ -n "$ids" ]; then
        install_name_tool -id "@executable_path/../Frameworks/$(basename "$target")" "$target"
    fi
    codesign --force --sign - "$target"
)
relocate "$original_executable" "$contents/MacOS/SpiralSynthModular"
find "$resources/SpiralPlugins" -type f -name '*.so' -print |
while IFS= read -r target; do
    relative=${target#"$resources/SpiralPlugins/"}
    relocate "$stage$plugindir/$relative" "$target"
done
# Version is substituted into XML, so escape XML metacharacters.
xml_version=$(printf '%s' "$version" | sed 's/\&/\&amp;/g; s/</\&lt;/g; s/>/\&gt;/g')
cat > "$contents/Info.plist" <<PLIST
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0"><dict>
<key>CFBundleExecutable</key><string>SpiralSynthModular</string>
<key>CFBundleIdentifier</key><string>$identifier</string>
<key>CFBundleName</key><string>SpiralSynthModular</string>
<key>CFBundlePackageType</key><string>APPL</string>
<key>CFBundleVersion</key><string>$xml_version</string>
<key>CFBundleShortVersionString</key><string>$xml_version</string>
<key>CFBundleIconFile</key><string>SpiralSynthModular.icns</string>
<key>NSHighResolutionCapable</key><true/>
</dict></plist>
PLIST
codesign --force --sign - "$app"
if [ -e "$destination" ]; then rm -rf "$destination"; fi
mv "$app" "$destination"
printf 'Created %s\n' "$destination"
