param(
 [string]$SourceRoot = "$env:USERPROFILE/Desktop/EldenRingAsset/parts",
 [Parameter(Mandatory=$true)][string]$Batch,
 [string]$Names = '',
 [switch]$Apply,
 [switch]$ConvertOnly,
 [string]$Tools = '',
 [string]$Rules = "$PSScriptRoot/dcx_rules.json"
)
$ErrorActionPreference='Stop'
$projectRoot=(Resolve-Path "$PSScriptRoot/../..").Path
if (!$Tools) {
 $Tools=Join-Path $projectRoot 'Saved/EquipmentImport/dcx_tools.json'
 if (!(Test-Path -LiteralPath $Tools)) { throw 'Create Saved/EquipmentImport/dcx_tools.json from dcx_tools.example.json first.' }
}
$settings=Get-Content -LiteralPath $Tools -Raw | ConvertFrom-Json
$python=Join-Path $settings.ue_root 'Engine/Binaries/ThirdParty/Python3/Win64/python.exe'
$cli=@('-B',"$PSScriptRoot/dcx_run.py",'--source',$SourceRoot,'--batch',$Batch,'--tools',$Tools,'--rules',$Rules)
if($Names) { $cli+=@('--names',$Names) }
if($Apply) { $cli+='--apply' }
if($ConvertOnly) { $cli+='--convert-only' }
& $python @cli
if($LASTEXITCODE -ne 0) { throw 'DCX pipeline incomplete; inspect the printed REPORT path.' }
