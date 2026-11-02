param(
 [string]$Selection = "$PSScriptRoot/item_smoke.json",
 [string]$Config = "$PSScriptRoot/item_catalog.local.example.json",
 [string]$Rules = "$PSScriptRoot/dcx_rules.json",
 [switch]$Apply
)
$ErrorActionPreference='Stop'
$root=(Resolve-Path "$PSScriptRoot/../..").Path
$settings=Get-Content -LiteralPath "$root/Saved/EquipmentImport/dcx_tools.json" -Raw | ConvertFrom-Json
$python=Join-Path $settings.ue_root 'Engine/Binaries/ThirdParty/Python3/Win64/python.exe'
$cli=@('-B',"$PSScriptRoot/import_items.py",'--selection',$Selection,'--config',$Config,'--rules',$Rules)
if($Apply) { $cli+='--apply' }
Push-Location $root
try {
 & $python @cli
 if($LASTEXITCODE -ne 0) { throw 'Item import incomplete; inspect ITEM REPORT.' }
} finally { Pop-Location }
