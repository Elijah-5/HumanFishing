#!/usr/bin/env python3
"""Open a disposable live shop for Retina pointer/layout verification."""
from pathlib import Path
import argparse,os,subprocess
parser=argparse.ArgumentParser()
parser.add_argument('--packaged',action='store_true')
args=parser.parse_args()
project=Path(__file__).resolve().parents[1]
engine=Path(os.environ.get('UE_ROOT','/Users/Shared/Epic Games/UE_5.2'))/'Engine'
app=project/'Build/Mac-Walkthrough/HumanFishing.app' if args.packaged else engine/'Binaries/Mac/UnrealEditor.app'
command=['/usr/bin/open','-n','-W','-a',str(app),'--args']
if not args.packaged:command.append(str(project/'HumanFishing.uproject'))
command += [
         '/Game/HumanFishing/Maps/L_DeepOcean','-game','-Windowed','-ResX=1280','-ResY=720','-nosplash','-nosound','-nop4',
         '-UserDir=/private/tmp/humanfishing-ui-retina-profile',
         '-ExecCmds=hf.dev on,hf.seaweed 200,hf.shop',
         '-abslog='+str(project/'Saved/Validation/retina-ui.log')]
raise SystemExit(subprocess.run(command,stdout=subprocess.DEVNULL,stderr=subprocess.STDOUT).returncode)
