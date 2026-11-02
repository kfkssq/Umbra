param([string]$Config = "$PSScriptRoot/item_catalog.local.example.json")
$ErrorActionPreference = 'Stop'
$projectRoot = (Resolve-Path "$PSScriptRoot/../..").Path
$settings = Get-Content -LiteralPath "$projectRoot/Saved/EquipmentImport/dcx_tools.json" -Raw | ConvertFrom-Json
$python = Join-Path $settings.ue_root 'Engine/Binaries/ThirdParty/Python3/Win64/python.exe'
Push-Location $projectRoot
try {
    & $python -B "$PSScriptRoot/item_catalog.py" --config $Config
    if ($LASTEXITCODE -ne 0) { throw 'Catalog build failed; check the error above.' }
} finally { Pop-Location }
