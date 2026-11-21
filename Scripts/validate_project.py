#!/usr/bin/env python3
"""UE 5.2 automation and cook checks. Reports stay under Saved/Validation."""
import argparse, os, subprocess, shutil
from pathlib import Path

def main():
    parser=argparse.ArgumentParser()
    parser.add_argument('check',choices=['tests','blueprints','cook','standalone','smoke'])
    parser.add_argument('--filter',default='HumanFishing',help='Automation test name or prefix (tests only).')
    parser.add_argument('--packaged',action='store_true',help='Use the cooked Mac app for a short visual check.')
    parser.add_argument('--width',type=int,default=1280)
    parser.add_argument('--height',type=int,default=720)
    parser.add_argument('--readme-capture',action='store_true',help='Capture real gameplay frames during the short smoke fixture.')
    args=parser.parse_args()
    if args.packaged and args.check not in ('standalone','smoke'):parser.error('--packaged requires smoke or standalone')
    if args.readme_capture and args.check!='smoke':parser.error('--readme-capture requires smoke')
    project=Path(__file__).resolve().parents[1]
    engine=Path(os.environ.get('UE_ROOT','/Users/Shared/Epic Games/UE_5.2'))/'Engine'
    output=project/'Saved/Validation';output.mkdir(parents=True,exist_ok=True)
    log=output/((args.check if args.check!='tests' or args.filter=='HumanFishing' else 'tests-'+args.filter.replace('.','-'))+'.log')
    result_name='SmokeResult.txt' if args.check=='smoke' else 'StandaloneResult.txt'
    if args.check in ('standalone','smoke'):
        for old in [output/result_name,Path('/private/tmp/humanfishing-validation-profile/Saved/Validation')/result_name]:
            old.unlink(missing_ok=True)
    command=[str(engine/'Binaries/Mac/UnrealEditor.app/Contents/MacOS/UnrealEditor'),str(project/'HumanFishing.uproject'),
             '-unattended','-nop4','-nosplash','-nosound','-NullRHI','-abslog='+str(log)]
    if args.packaged:
        command[0]=str(project/'Build/Mac-Walkthrough/HumanFishing.app/Contents/MacOS/HumanFishing')
        command.remove(str(project/'HumanFishing.uproject'))
    if args.check in ('standalone','smoke'):
        command.remove('-NullRHI')
        command+=['/Game/HumanFishing/Maps/L_SaveMenu','-game','-RenderOffscreen','-Windowed','-ForceRes','-ResX='+str(args.width),'-ResY='+str(args.height),
                  '-HFValidation','-UserDir=/private/tmp/humanfishing-validation-profile','-ExecCmds=hf.validate']
        if args.check=='smoke':
            command.remove('/Game/HumanFishing/Maps/L_SaveMenu')
            command+=['/Game/HumanFishing/Maps/L_DeepOcean','-HFQuickSmoke']
        if args.readme_capture:
            frames=Path('/private/tmp/humanfishing-validation-profile/Saved/Validation/ReadmeFrames')
            if frames.exists():shutil.rmtree(frames)
            command+=['-HFReadmeCapture']
    elif args.check=='tests':
        command+=['-ExecCmds=Automation RunTests '+args.filter,'-TestExit=Automation Test Queue Empty',
                  '-ReportExportPath='+str(output/('Automation' if args.filter=='HumanFishing' else 'FocusedAutomation'))]
    elif args.check=='blueprints':
        command+=['-run=CompileAllBlueprints','-ProjectOnly']
    else:
        command+=['-run=Cook','-TargetPlatform=Mac','-unversioned','-UTF8Output',
                  '-Map=/Game/HumanFishing/Maps/L_SaveMenu+/Game/HumanFishing/Maps/L_ShorePrototype+/Game/HumanFishing/Maps/L_DeepOcean+/Game/HumanFishing/Maps/L_ScenicCove']
    print('Validation log:',log,flush=True)
    result=subprocess.run(command,stdout=subprocess.DEVNULL,stderr=subprocess.STDOUT)
    lines=log.read_text(errors='replace').splitlines()
    for line in lines:
        if any(s in line for s in ['Error:','Test Completed','Test Succeeded','Test Failed','Summary','Total Test','Warning/Error Summary','Success -','Failure -','Blueprints will be compiled','HF_STANDALONE','HF_CAPTURE']):
            print(line)
    if args.check in ('standalone','smoke'):
        profile=Path('/private/tmp/humanfishing-validation-profile/Saved/Validation')
        if args.readme_capture and (output/'ReadmeFrames').exists():
            shutil.rmtree(output/'ReadmeFrames')
        if profile.exists():shutil.copytree(profile,output,dirs_exist_ok=True)
        result_file=output/result_name
        if not result_file.exists() or result_file.read_text()!='PASS':raise SystemExit(1)
    raise SystemExit(result.returncode)

if __name__=='__main__':main()
