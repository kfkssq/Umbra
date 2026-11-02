param(
    [Parameter(Mandatory=$true)][string]$Manifest,
    [switch]$Apply,
    [string]$Ids = '',
    [string]$EngineRoot = '',
    [string]$Report = ''
)
$ErrorActionPreference = 'Stop'
$projectRoot = (Resolve-Path (Join-Path $PSScriptRoot '../..')).Path
$projectPath = Join-Path $projectRoot 'Umbra.uproject'
if ($Apply -and (Get-Process UnrealEditor,UnrealEditor-Cmd -ErrorAction SilentlyContinue)) {
    throw 'Close Unreal Editor first to prevent concurrent package writes.'
}
$association = (Get-Content -LiteralPath $projectPath -Raw | ConvertFrom-Json).EngineAssociation
if (!$EngineRoot) {
    $installations = Get-Content -LiteralPath "$env:ProgramData/Epic/UnrealEngineLauncher/LauncherInstalled.dat" -Raw | ConvertFrom-Json
    $EngineRoot = ($installations.InstallationList | Where-Object AppName -eq "UE_$association" | Select-Object -First 1).InstallLocation
}
if (!$EngineRoot) { throw 'Associated UE installation not found; supply -EngineRoot.' }
$version = Get-Content -LiteralPath (Join-Path $EngineRoot 'Engine/Build/Build.version') -Raw | ConvertFrom-Json
if ("$($version.MajorVersion).$($version.MinorVersion)" -ne $association) { throw 'Engine version does not match uproject.' }
$manifestPath = (Resolve-Path -LiteralPath $Manifest).Path
$reportFolder = Join-Path $projectRoot 'Saved/EquipmentImport'
New-Item -ItemType Directory -Force -Path $reportFolder | Out-Null
if (!$Report) { $Report = Join-Path $reportFolder (Get-Date -Format 'yyyyMMdd_HHmmss_fff') }
if (![IO.Path]::GetExtension($Report)) { $Report += '.json' }
$Report = [IO.Path]::GetFullPath($Report)
if (Test-Path -LiteralPath $Report) { throw 'Report already exists; choose a new report path to retain history.' }
$env:UMBRA_IMPORT_MANIFEST = $manifestPath
$env:UMBRA_IMPORT_REPORT = $Report
$env:UMBRA_IMPORT_APPLY = if ($Apply) {'1'} else {'0'}
$env:UMBRA_IMPORT_IDS = $Ids
try {
    & (Join-Path $EngineRoot 'Engine/Binaries/Win64/UnrealEditor-Cmd.exe') $projectPath -run=pythonscript "-script=$PSScriptRoot/entry.py" '-EnablePlugins=PythonScriptPlugin,EditorScriptingUtilities' '-ExecCmds=Interchange.FeatureFlags.Import.FBX 0' -unattended -nop4 -nullrhi -nosound -stdout "-abslog=$Report.log"
    $editorExitCode = $LASTEXITCODE
    if (!(Test-Path -LiteralPath $Report)) { throw "Editor did not produce a report; inspect $Report.log" }
    $result = Get-Content -LiteralPath $Report -Raw | ConvertFrom-Json
    $warnings = @(Select-String -LiteralPath "$Report.log" -Pattern 'FBXImport: Warning:|LogMaterial: Warning:|LogTexture: Warning:' | ForEach-Object { $_.Line } | Select-Object -Unique)
    $result | Add-Member -NotePropertyName engine_log -NotePropertyValue "$Report.log" -Force
    $result | Add-Member -NotePropertyName batch_engine_warnings -NotePropertyValue $warnings -Force
    $result | ConvertTo-Json -Depth 100 | Set-Content -LiteralPath $Report -Encoding utf8
    Write-Host "Report: $Report; saved: $($result.saved.Count); failed: $($result.failed_ids -join ', ')"
    if ($editorExitCode -ne 0 -or !$result.ok) { throw 'Import incomplete; see report and engine_log.' }
} finally {
    Remove-Item Env:UMBRA_IMPORT_MANIFEST,Env:UMBRA_IMPORT_REPORT,Env:UMBRA_IMPORT_APPLY,Env:UMBRA_IMPORT_IDS -ErrorAction SilentlyContinue
}
