#!/usr/bin/env python3
"""Build/cook/archive the current UE 5.2 Mac game with a scoped Xcode shim."""
import os
from pathlib import Path
import subprocess
import shutil

project = Path(__file__).resolve().parents[1]
engine = Path(os.environ.get('UE_ROOT', '/Users/Shared/Epic Games/UE_5.2')) / 'Engine'
work = Path('/private/tmp/humanfishing-package-ue52')
work.mkdir(parents=True, exist_ok=True)
env = os.environ.copy()
major = int(subprocess.check_output(['xcodebuild', '-version'], text=True).split()[1].split('.')[0])
ubt = '-architecture=arm64'
if major > 15:
    shim = work / 'tools'
    shim.mkdir(exist_ok=True)
    launcher = shim / 'xcodebuild'
    launcher.write_text('#!/bin/sh\nif [ "$#" -eq 1 ] && [ "$1" = "-version" ]; then\n'
                        "printf 'Xcode 15.4\\nBuild version 15F31d\\n'\nexit 0\nfi\n"
                        'exec /usr/bin/xcodebuild "$@"\n')
    launcher.chmod(0o755)
    env['PATH'] = str(shim) + os.pathsep + env.get('PATH', '')
    ubt += ' -OverrideBuildEnvironment -CompilerArguments="-Wno-deprecated-literal-operator -fno-access-control"'
ubt += ' -NoHotReload'
archive = project / 'Build/Mac-Walkthrough-Staging'
current = project / 'Build/Mac-Walkthrough'
log = work / 'package.log'
command = [str(engine / 'Build/BatchFiles/RunUAT.sh'), 'BuildCookRun',
           '-project=' + str(project / 'HumanFishing.uproject'), '-noP4', '-unattended',
           '-platform=Mac', '-clientconfig=Development', '-build', '-cook', '-stage',
           '-pak', '-archive', '-archivedirectory=' + str(archive), '-nodebuginfo',
           '-nocompileeditor', '-utf8output', '-UbtArgs=' + ubt]
print('Package log:', log, '\nOutput:', archive, flush=True)
with log.open('w') as stream:
    result = subprocess.run(command, cwd=project, env=env, stdout=stream, stderr=subprocess.STDOUT)
print('\n'.join(log.read_text(errors='replace').splitlines()[-30:]), flush=True)
if result.returncode == 0:
    app = archive / 'HumanFishing.app'
    startup_log = work / 'startup.log'
    subprocess.run([str(app / 'Contents/MacOS/HumanFishing'), '-NullRHI', '-unattended',
                    '-nosound', '-nosplash', '-UserDir=/private/tmp/humanfishing-package-smoke-profile',
                    '-ExecCmds=quit', '-abslog=' + str(startup_log)],
                   stdout=subprocess.DEVNULL, stderr=subprocess.STDOUT, check=True, timeout=90)
    previous = project / 'Build/.Mac-Walkthrough-previous'
    if previous.exists(): shutil.rmtree(previous)
    if current.exists(): current.rename(previous)
    try: archive.rename(current)
    except Exception:
        if previous.exists(): previous.rename(current)
        raise
    if previous.exists(): shutil.rmtree(previous)
    print('Ready:', current / 'HumanFishing.app', flush=True)
raise SystemExit(result.returncode)
