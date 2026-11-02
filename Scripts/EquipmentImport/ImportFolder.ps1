param(
    [Parameter(Mandatory=$true)][string]$SourceRoot,
    [Parameter(Mandatory=$true)][string]$Batch,
    [switch]$Apply,
    [string]$EngineRoot = ''
)
$ErrorActionPreference = 'Stop'
$projectRoot = (Resolve-Path (Join-Path $PSScriptRoot '../..')).Path
if (!$EngineRoot) {
    $association = (Get-Content (Join-Path $projectRoot 'Umbra.uproject') -Raw | ConvertFrom-Json).EngineAssociation
    $installed = Get-Content "$env:ProgramData/Epic/UnrealEngineLauncher/LauncherInstalled.dat" -Raw | ConvertFrom-Json
    $EngineRoot = ($installed.InstallationList | Where-Object AppName -eq "UE_$association" | Select-Object -First 1).InstallLocation
}
if (!$EngineRoot) { throw 'Associated engine not found; supply -EngineRoot.' }
$reportFolder = Join-Path $projectRoot 'Saved/EquipmentImport'
New-Item -ItemType Directory -Force -Path $reportFolder | Out-Null
$manifest = Join-Path $reportFolder ((Get-Date -Format 'yyyyMMdd_HHmmss_fff') + '_manifest.json')
& (Join-Path $EngineRoot 'Engine/Binaries/ThirdParty/Python3/Win64/python.exe') -B "$PSScriptRoot/scan.py" $SourceRoot $manifest --batch $Batch
if ($LASTEXITCODE -ne 0) { throw 'Folder scan failed.' }
& "$PSScriptRoot/Run.ps1" -Manifest $manifest -EngineRoot $EngineRoot -Apply:$Apply
