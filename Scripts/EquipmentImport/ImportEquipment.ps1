param(
 [string]$Selection,
 [string]$Batch,
 [ValidateSet('Staging','Library')][string]$Destination,
 [string]$ExcludeArchives,
 [int]$Limit=30,
 [int]$ChunkSize=10,
 [string]$Config="$PSScriptRoot/item_catalog.local.example.json",
 [string]$Rules="$PSScriptRoot/dcx_rules.json",
 [switch]$Apply
)
$ErrorActionPreference='Stop'
$root=(Resolve-Path "$PSScriptRoot/../..").Path
$settings=Get-Content -LiteralPath "$root/Saved/EquipmentImport/dcx_tools.json" -Raw | ConvertFrom-Json
$python=Join-Path $settings.ue_root 'Engine/Binaries/ThirdParty/Python3/Win64/python.exe'
$cli=@('-B',"$PSScriptRoot/bulk_import.py",'--config',$Config,'--rules',$Rules,'--limit',"$Limit",'--chunk-size',"$ChunkSize")
if($Selection){$cli+=@('--selection',$Selection)}
if($Batch){$cli+=@('--batch',$Batch)}
if($Destination){$cli+=@('--destination',$Destination)}
if($ExcludeArchives){$cli+=@('--exclude-archives',$ExcludeArchives)}
if($Apply){$cli+='--apply'}
Push-Location $root
try {
 & $python @cli
 if($LASTEXITCODE -ne 0){Write-Warning 'Batch incomplete. Successful assets retained; inspect BULK REPORT and retry.json.'}
 exit $LASTEXITCODE
} finally {Pop-Location}
