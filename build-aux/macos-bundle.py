#!/usr/bin/env python
# Optional macOS packaging helper; compatible with Python 2.7 and Python 3.
from __future__ import print_function
import argparse
import os
import plistlib
import shutil
import shlex
import subprocess
import sys
import tempfile


def output(args):
    return subprocess.check_output(args).decode('utf-8')


def read_plist(path):
    if hasattr(plistlib, 'load'):
        with open(path, 'rb') as stream:
            return plistlib.load(stream)
    return plistlib.readPlist(path)


def write_plist(value, path):
    if hasattr(plistlib, 'dump'):
        with open(path, 'wb') as stream:
            plistlib.dump(value, stream)
    else:
        plistlib.writePlist(value, path)


def build(args):
    if sys.platform != 'darwin':
        raise RuntimeError('bundle packaging requires macOS')
    destination = os.path.abspath(args.output)
    identifier = 'org.pawfal.SpiralSynthModular'
    if os.path.lexists(destination):
        info = os.path.join(destination, 'Contents', 'Info.plist')
        if os.path.islink(destination) or not os.path.isfile(info) or read_plist(info).get('CFBundleIdentifier') != identifier:
            raise RuntimeError('refusing to replace unrelated output: ' + destination)
    parent = os.path.dirname(destination)
    if not os.path.isdir(parent):
        os.makedirs(parent)
    work = tempfile.mkdtemp(prefix='.ssm-bundle-', dir=parent)
    try:
        stage = os.path.join(work, 'stage')
        subprocess.check_call(shlex.split(args.make) + ['install', 'DESTDIR=' + stage])
        def staged(path):
            if not os.path.isabs(path):
                raise RuntimeError('bundle installation paths must be absolute: ' + path)
            return stage + path
        app = os.path.join(work, 'SpiralSynthModular.app')
        contents = os.path.join(app, 'Contents')
        macos = os.path.join(contents, 'MacOS')
        frameworks = os.path.join(contents, 'Frameworks')
        resources = os.path.join(contents, 'Resources')
        for path in (macos, frameworks, resources):
            os.makedirs(path)
        executable = os.path.join(macos, 'SpiralSynthModular')
        original_executable = staged(os.path.join(args.bindir, 'SpiralSynthModular'))
        shutil.copy2(original_executable, executable)
        plugins = os.path.join(resources, 'SpiralPlugins')
        shutil.copytree(staged(args.plugindir), plugins)
        shutil.copytree(os.path.join(args.srcdir, 'Examples'), os.path.join(resources, 'Examples'))
        shutil.copy2(os.path.join(args.srcdir, 'COPYING'), resources)
        queue = [(original_executable, executable)]
        for root, dirs, files in os.walk(plugins):
            for name in files:
                if name.endswith('.so'):
                    target = os.path.join(root, name)
                    source = os.path.join(staged(args.plugindir), os.path.relpath(target, plugins))
                    queue.append((source, target))
        copied = {}
        names = {}
        binaries = []
        def expand(path, source):
            if path.startswith('@loader_path/'):
                return os.path.normpath(os.path.join(os.path.dirname(source), path[len('@loader_path/'):]))
            if path.startswith('@executable_path/'):
                return os.path.normpath(os.path.join(os.path.dirname(original_executable), path[len('@executable_path/'):]))
            return path
        def rpaths(source):
            lines = output(['otool', '-l', source]).splitlines()
            result = []
            for index, line in enumerate(lines):
                if line.strip() == 'cmd LC_RPATH':
                    path = lines[index + 2].strip()[len('path '):].rsplit(' (offset ', 1)[0]
                    result.append(expand(path, source))
            return result
        executable_rpaths = rpaths(original_executable)
        def resolve(dependency, source):
            # Prefer this staged installation over any older installed copy.
            installed = os.path.join(staged(args.libdir), os.path.basename(dependency))
            if os.path.isfile(installed):
                return os.path.realpath(installed)
            candidates = [expand(dependency, source)]
            if dependency.startswith('@rpath/'):
                candidates = [os.path.join(p, dependency[len('@rpath/'):]) for p in rpaths(source) + executable_rpaths]
            for candidate in candidates:
                for path in ([stage + candidate, candidate] if candidate.startswith('/') else []):
                    if os.path.isfile(path):
                        return os.path.realpath(path)
            raise RuntimeError('cannot resolve %s from %s' % (dependency, source))
        while queue:
            source, target = queue.pop(0)
            binaries.append(target)
            os.chmod(target, os.stat(target).st_mode | 0o200)
            ids = output(['otool', '-D', source]).splitlines()[1:]
            for line in output(['otool', '-L', source]).splitlines()[1:]:
                dependency = line.strip().rsplit(' (compatibility version ', 1)[0]
                if dependency in ids or dependency.startswith(('/System/Library/', '/usr/lib/')):
                    continue
                resolved = resolve(dependency, source)
                if '.framework/' in resolved:
                    raise RuntimeError('non-system frameworks are not supported: ' + resolved)
                if resolved not in copied:
                    name = os.path.basename(resolved)
                    if name in names and names[name] != resolved:
                        raise RuntimeError('conflicting library names: ' + name)
                    names[name] = resolved
                    library = os.path.join(frameworks, name)
                    copied[resolved] = library
                    shutil.copy2(resolved, library)
                    queue.append((resolved, library))
                replacement = '@loader_path/' + os.path.relpath(copied[resolved], os.path.dirname(target))
                subprocess.check_call(['install_name_tool', '-change', dependency, replacement, target])
            if ids:
                subprocess.check_call(['install_name_tool', '-id', '@loader_path/' + os.path.basename(target), target])
        write_plist({'CFBundleExecutable': 'SpiralSynthModular',
                     'CFBundleIdentifier': identifier,
                     'CFBundleName': 'SpiralSynthModular',
                     'CFBundlePackageType': 'APPL',
                     'CFBundleVersion': args.version,
                     'CFBundleShortVersionString': args.version,
                     'NSHighResolutionCapable': True}, os.path.join(contents, 'Info.plist'))
        # Ad-hoc signing permits local execution after relocation, including ARM Macs.
        for binary in reversed(binaries):
            subprocess.check_call(['codesign', '--force', '--sign', '-', binary])
        subprocess.check_call(['codesign', '--force', '--sign', '-', app])
        if os.path.exists(destination):
            shutil.rmtree(destination)
        shutil.move(app, destination)
        print('Created ' + destination)
    finally:
        shutil.rmtree(work)


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    for name in ('make', 'bindir', 'libdir', 'plugindir', 'srcdir', 'version', 'output'):
        parser.add_argument('--' + name, required=True)
    try:
        build(parser.parse_args())
    except (RuntimeError, OSError, subprocess.CalledProcessError) as error:
        sys.exit('bundle: ' + str(error))
