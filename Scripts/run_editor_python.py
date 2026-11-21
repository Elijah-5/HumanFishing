#!/usr/bin/env python3
"""Run an explicit editor task, with an isolated log and reliable Python quoting."""
from pathlib import Path
import argparse
import base64
import os
import subprocess

def main():
    parser=argparse.ArgumentParser()
    parser.add_argument('script', type=Path)
    parser.add_argument('--log', default='/private/tmp/humanfishing-editor-task.log')
    parser.add_argument('--render', action='store_true')
    parser.add_argument('--async-task', action='store_true', help='Task owns its tick callbacks and calls quit_editor when done')
    args=parser.parse_args()
    project=Path(__file__).resolve().parents[1]
    engine=Path(os.environ.get('UE_ROOT','/Users/Shared/Epic Games/UE_5.2'))/'Engine'
    script=args.script.resolve()
    code="import unreal\ntry:\n exec(compile(open("+repr(str(script))+", encoding='utf-8').read(), "+repr(str(script))+", 'exec'))\nexcept Exception:\n import traceback; unreal.log_error(traceback.format_exc())\nfinally:\n unreal.SystemLibrary.quit_editor()\n"
    if args.async_task:
        code=code.replace("finally:\n unreal.SystemLibrary.quit_editor()\n", "")
        code=code.replace("import traceback; unreal.log_error(traceback.format_exc())", "import traceback; unreal.log_error(traceback.format_exc()); unreal.SystemLibrary.quit_editor()")
    payload=base64.b64encode(code.encode()).decode()
    command=[str(engine/'Binaries/Mac/UnrealEditor.app/Contents/MacOS/UnrealEditor'), str(project/'HumanFishing.uproject'),
             '-unattended','-nop4','-nosplash','-nosound',
             "-ExecCmds=py exec(__import__('base64').b64decode('"+payload+"'))", '-abslog='+args.log]
    if not args.render: command.append('-NullRHI')
    else: command+=['-RenderOffscreen','-Windowed','-ResX=1280','-ResY=720','-NoPause']
    print('Editor log:',args.log,flush=True)
    result=subprocess.run(command, stdout=subprocess.DEVNULL, stderr=subprocess.STDOUT)
    log=Path(args.log).read_text(errors='replace')
    lines=log.splitlines()
    show=set()
    for i,line in enumerate(lines):
        if any(x in line for x in ['LogPython:','LogBlueprint: Error:','Fatal error:']):
            show.update(range(i,min(i+(12 if 'Error:' in line else 1),len(lines))))
    for i in sorted(show): print(lines[i])
    if 'LogPython: Error:' in log or 'LogBlueprint: Error:' in log: raise SystemExit(1)
    raise SystemExit(result.returncode)

if __name__=='__main__': main()
