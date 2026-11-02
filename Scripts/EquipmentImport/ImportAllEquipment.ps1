param(
 [string]$Queue='Equipment_All_001',
 [int]$BatchSize=30,
 [int]$ChunkSize=6,
 [string]$Config="$PSScriptRoot/item_catalog.local.example.json",
 [string]$Rules="$PSScriptRoot/dcx_rules.json",
 [switch]$Apply
)
$ErrorActionPreference='Stop'
$root=(Resolve-Path "$PSScriptRoot/../..").Path
$tools=Join-Path $root 'Saved/EquipmentImport/dcx_tools.json'
$settings=Get-Content -LiteralPath $tools -Raw | ConvertFrom-Json
$python=Join-Path $settings.ue_root 'Engine/Binaries/ThirdParty/Python3/Win64/python.exe'
$cli=@('-B',"$PSScriptRoot/import_all.py",'--queue',$Queue,'--config',$Config,
       '--rules',$Rules,'--tools',$tools,'--batch-size',"$BatchSize",'--chunk-size',"$ChunkSize")
if($Apply){$cli+='--apply'}
Push-Location $root
try {
 & $python @cli
 $result=$LASTEXITCODE
 if($result -eq 1){Write-Warning 'Queue completed with failed resources. See summary.md and failures.json.'}
 elseif($result -ne 0){Write-Warning 'Queue stopped. Inspect the error and rerun the same command to resume.'}
 exit $result
} finally {Pop-Location}
