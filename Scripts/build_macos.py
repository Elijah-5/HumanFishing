#!/usr/bin/env python3
"""Rebuild project plugins in a disposable host; never changes installed Xcode/UE.

Close the editor before running. UE_ROOT may select a different UE 5.2 install.
The Xcode 15 shim changes the version query only, not the actual compiler.
"""
from pathlib import Path
import json
import os
import shutil
import subprocess
import time

PROJECT = Path(__file__).resolve().parents[1]
ENGINE = Path(os.environ.get('UE_ROOT', '/Users/Shared/Epic Games/UE_5.2')) / 'Engine'

def main():
    if not (ENGINE / 'Build/Build.version').exists():
        raise SystemExit('Set UE_ROOT to the Unreal Engine 5.2 installation.')
    version = json.loads((ENGINE / 'Build/Build.version').read_text())
    if (version['MajorVersion'], version['MinorVersion']) != (5, 2):
        raise SystemExit('This project requires UE 5.2.')
    result = subprocess.run(['pgrep', '-fl', 'UnrealEditor.app/Contents/MacOS/UnrealEditor'], capture_output=True, text=True)
    if result.returncode == 0 and str(PROJECT) in result.stdout:
        raise SystemExit('Close the project editor before replacing plugin binaries.')
    work = Path('/private/tmp/humanfishing-build-ue52')
    work.mkdir(parents=True, exist_ok=True)
    host = work / 'HostProject'
    plugin = host / 'Plugins/HumanFishingGameplay'
    shutil.copytree(PROJECT / 'Plugins/HumanFishingGameplay', plugin,
                    ignore=shutil.ignore_patterns('Binaries', 'Intermediate', '.DS_Store'), dirs_exist_ok=True)
    for cached in (plugin/'Source').rglob('*'):
        if cached.is_file() and not (PROJECT/'Plugins/HumanFishingGameplay'/cached.relative_to(plugin)).exists():
            cached.unlink()
    (host / 'HostProject.uproject').write_text(json.dumps({'FileVersion': 3, 'Plugins': [
        {'Name': 'HumanFishingGameplay', 'Enabled': True}]}))
    env = os.environ.copy()
    actual = subprocess.check_output(['xcodebuild', '-version'], text=True).splitlines()[0]
    major = int(actual.split()[1].split('.')[0])
    extra = []
    if major > 15:
        shim = work / 'tools'
        shim.mkdir(exist_ok=True)
        launcher = shim / 'xcodebuild'
        launcher.write_text('#!/bin/sh\nif [ "$#" -eq 1 ] && [ "$1" = "-version" ]; then\n'
                            "printf 'Xcode 15.4\\nBuild version 15F31d\\n'\nexit 0\nfi\n"
                            'exec /usr/bin/xcodebuild "$@"\n')
        launcher.chmod(0o755)
        env['PATH'] = str(shim) + os.pathsep + env.get('PATH', '')
        extra = ['-OverrideBuildEnvironment',
                 '-CompilerArguments=-Wno-deprecated-literal-operator -fno-access-control']
        print(actual + '; reporting 15.4 only to this build. Using the real compiler.', flush=True)
    log = work / 'build.log'
    command = [str(ENGINE / 'Build/BatchFiles/Mac/Build.sh'), 'UnrealEditor', 'Mac', 'Development',
               '-Project=' + str(host / 'HostProject.uproject'), '-architecture=arm64',
               '-noubtmakefiles', '-nohotreload', '-log=' + str(log)] + extra
    print('Build log:', log, flush=True)
    subprocess.run(command, env=env, cwd=ENGINE.parent, check=True)
    built = plugin / 'Binaries/Mac'
    destination = PROJECT / 'Plugins/HumanFishingGameplay/Binaries/Mac'
    backup = PROJECT / 'Saved/PluginBackups' / time.strftime('%Y%m%d-%H%M%S')
    if destination.exists():
        shutil.copytree(destination, backup)
    destination.mkdir(parents=True, exist_ok=True)
    modules = json.loads((built / 'UnrealEditor.modules').read_text())
    for name in list(modules['Modules'].values()) + ['UnrealEditor.modules']:
        temporary = destination / (name + '.new')
        shutil.copy2(built / name, temporary)
        temporary.replace(destination / name)
    print('Installed:', ', '.join(modules['Modules']), flush=True)
    print('Previous binaries:', backup, flush=True)

if __name__ == '__main__':
    main()
